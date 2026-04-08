/* PERT engine */
(function () {
  "use strict";

  const MAX_ACTIVITIES = 50;

  function normalize(raw) {
    const code = String(raw.code || "").trim().toUpperCase().charAt(0);
    const predecessors = (raw.predecessors || [])
      .map((x) => String(x || "").trim().toUpperCase().charAt(0))
      .filter(Boolean);
    return {
      code,
      description: String(raw.description || "").trim(),
      normalCost: Number(raw.normalCost) || 0,
      predecessors,
      optimistic: Number(raw.o ?? raw.optimistic),
      mostLikely: Number(raw.m ?? raw.mostLikely),
      pessimistic: Number(raw.p ?? raw.pessimistic),
      meanTime: 0,
      variance: 0,
      ES: -1,
      EF: 0,
      LS: 0,
      LF: 0,
      slack: 0,
      isCritical: false,
    };
  }

  function idx(activities, code) {
    const c = String(code).charAt(0);
    return activities.findIndex((a) => a.code === c);
  }

  function meanVar(activities) {
    for (const a of activities) {
      a.meanTime = (a.optimistic + 4 * a.mostLikely + a.pessimistic) / 6;
      a.variance = Math.pow(a.pessimistic - a.optimistic, 2) / 36;
    }
  }

  function maxEfPred(activities, preds) {
    let m = -1;
    for (const p of preds) {
      const i = idx(activities, p);
      if (i >= 0 && activities[i].EF > m) m = activities[i].EF;
    }
    return m;
  }

  function minLsSucc(activities, code) {
    const c = String(code).charAt(0);
    let min = -1;
    let found = false;
    for (const a of activities) {
      if (a.predecessors.includes(c)) {
        if (!found || a.LS < min) {
          min = a.LS;
          found = true;
        }
      }
    }
    return min;
  }

  function forward(activities) {
    const n = activities.length;
    const done = new Array(n).fill(false);
    let count = 0;
    for (const a of activities) a.ES = -1;
    while (count < n) {
      for (let i = 0; i < n; i++) {
        if (done[i]) continue;
        const act = activities[i];
        let ok = true;
        for (const p of act.predecessors) {
          const pi = idx(activities, p);
          if (pi < 0 || activities[pi].ES === -1) {
            ok = false;
            break;
          }
        }
        if (ok) {
          act.ES = act.predecessors.length === 0 ? 0 : maxEfPred(activities, act.predecessors);
          act.EF = act.ES + act.meanTime;
          done[i] = true;
          count++;
        }
      }
    }
  }

  function backward(activities) {
    const n = activities.length;
    let end = 0;
    for (const a of activities) if (a.EF > end) end = a.EF;
    for (const a of activities) {
      a.LF = end;
      a.LS = a.LF - a.meanTime;
    }
    const done = new Array(n).fill(false);
    let count = 0;
    while (count < n) {
      for (let i = 0; i < n; i++) {
        if (done[i]) continue;
        const act = activities[i];
        let ok = true;
        for (let j = 0; j < n; j++) {
          if (!done[j] && activities[j].predecessors.includes(act.code)) {
            ok = false;
            break;
          }
        }
        if (ok) {
          const m = minLsSucc(activities, act.code);
          if (m >= 0) act.LF = m;
          act.LS = act.LF - act.meanTime;
          done[i] = true;
          count++;
        }
      }
    }
  }

  function slackCrit(activities) {
    for (const a of activities) {
      a.slack = a.LS - a.ES;
      a.isCritical = Math.abs(a.slack) < 0.001;
    }
  }

  function runPert(list) {
    if (!list || !list.length) return [];
    const activities = list.map((r) => normalize(r));
    meanVar(activities);
    forward(activities);
    backward(activities);
    slackCrit(activities);
    return activities;
  }

  function topoOrder(activities) {
    if (!activities || !activities.length) return [];
    const n = activities.length;
    const done = new Array(n).fill(false);
    const out = [];
    let count = 0;
    while (count < n) {
      for (let i = 0; i < n; i++) {
        if (done[i]) continue;
        const act = activities[i];
        let ok = true;
        for (const p of act.predecessors) {
          const pi = idx(activities, p);
          if (pi < 0 || !done[pi]) {
            ok = false;
            break;
          }
        }
        if (ok) {
          out.push(act.code);
          done[i] = true;
          count++;
        }
      }
    }
    return out;
  }

  function critPath(activities) {
    if (!activities || !activities.length) return { sorted: [], pathStr: "—", dur: 0, proj: 0 };
    const sorted = activities.filter((a) => a.isCritical).sort((a, b) => a.ES - b.ES);
    const pathStr = sorted.length ? sorted.map((a) => a.code).join(" → ") : "—";
    const dur = sorted.reduce((s, a) => s + a.meanTime, 0);
    const proj = Math.max(...activities.map((a) => a.EF));
    return { sorted, pathStr, dur, proj };
  }

  function projStats(activities) {
    if (!activities || !activities.length) return { projectDuration: 0, totalVar: 0, sd: 0 };
    let pd = 0;
    let tv = 0;
    for (const a of activities) {
      if (a.EF > pd) pd = a.EF;
      if (a.isCritical) tv += a.variance;
    }
    return { projectDuration: pd, totalVar: tv, sd: Math.sqrt(tv) };
  }

  function normalCdf(z) {
    if (z < -8) return 0;
    if (z > 8) return 1;
    const t = 1 / (1 + 0.2316419 * Math.abs(z));
    const d = (1 / Math.sqrt(2 * Math.PI)) * Math.exp(-(z * z) / 2);
    const p =
      d *
      t *
      (0.31938153 + t * (-0.356563782 + t * (1.781477937 + t * (-1.821255978 + t * 1.330274429))));
    return z > 0 ? 1 - p : p;
  }

  function interpretation(p) {
    if (p > 90) return "High — very likely on time.（较高：按期完成的可能性很大）";
    if (p > 70) return "Moderate — good chance.（中等偏上：有较大概率按期完成）";
    if (p > 50) return "Fair — some delay risk.（一般：存在一定延期风险）";
    if (p > 30) return "Low — significant risk.（偏低：延期风险较大）";
    return "Very low — high miss risk.（很低：难以按期完成的概率高）";
  }

  function parsePredecessorString(ps) {
    const s = String(ps || "").replace(/[",]/g, " ").trim();
    if (!s || /^[-–—]$|^none$/i.test(s)) return [];
    return s
      .split(/[;,]|\s+/)
      .map((x) => x.trim())
      .filter(Boolean)
      .map((x) => x.charAt(0).toUpperCase())
      .filter((c) => /[A-Za-z]/.test(c));
  }

  function activityFromSeven(code, desc, cost, preds, o, m, p) {
    const codeStr = String(code == null ? "" : code).trim();
    if (!/^[A-Za-z]$/.test(codeStr)) return null;
    const opt = parseFloat(String(o == null ? "" : o).replace(/,/g, ""));
    const most = parseFloat(String(m == null ? "" : m).replace(/,/g, ""));
    const pess = parseFloat(String(p == null ? "" : p).replace(/,/g, ""));
    if (!(opt > 0 && most > 0 && pess > 0)) return null;
    const costStr = String(cost == null ? "" : cost).replace(/\D/g, "") || "0";
    return normalize({
      code: codeStr,
      description: String(desc == null ? "" : desc).trim(),
      normalCost: parseInt(costStr, 10),
      predecessors: parsePredecessorString(preds),
      o: opt,
      m: most,
      p: pess,
    });
  }

  function mapHeaderRow(headerCells) {
    const H = headerCells.map((h, i) => ({
      i,
      t: String(h == null ? "" : h)
        .toLowerCase()
        .trim(),
    }));
    const pick = (pred) => {
      const x = H.find(({ t }) => pred(t));
      return x ? x.i : -1;
    };
    const m = {
      code: pick((t) => t.includes("activity code") || /^code$/i.test(t)),
      desc: pick((t) => t.includes("description")),
      cost: pick((t) => t.includes("normal cost") || t.includes("cost (bdt)") || (t.includes("cost") && t.includes("normal"))),
      pred: pick((t) => t.includes("predecessor")),
      o: pick((t) => t.includes("optimistic")),
      m: pick((t) => t.includes("most likely")),
      p: pick((t) => t.includes("pessimistic")),
    };
    const ncol = headerCells.length;
    const fb = [0, 1, 2, 3, 4, 5, 6];
    const keys = ["code", "desc", "cost", "pred", "o", "m", "p"];
    keys.forEach((k, j) => {
      if (m[k] < 0 || m[k] >= ncol) m[k] = ncol > j ? fb[j] : fb[Math.min(j, ncol - 1)];
    });
    return m;
  }

  function parseMatrix(data) {
    if (!data || !data.length) return [];
    const rows = data.map((r) => (Array.isArray(r) ? r : []).map((c) => (c == null ? "" : c)));
    let headerRow = -1;
    for (let r = 0; r < Math.min(25, rows.length); r++) {
      const line = rows[r].join(" ").toLowerCase();
      if (/activity\s*code/.test(line) && /description/.test(line)) {
        headerRow = r;
        break;
      }
    }
    let colMap;
    let start;
    if (headerRow >= 0) {
      colMap = mapHeaderRow(rows[headerRow]);
      start = headerRow + 1;
    } else {
      colMap = { code: 0, desc: 1, cost: 2, pred: 3, o: 4, m: 5, p: 6 };
      start = 0;
    }
    const out = [];
    for (let r = start; r < rows.length; r++) {
      const row = rows[r];
      const g = (i) => (i >= 0 && i < row.length ? String(row[i]).trim() : "");
      const act = activityFromSeven(g(colMap.code), g(colMap.desc), g(colMap.cost), g(colMap.pred), g(colMap.o), g(colMap.m), g(colMap.p));
      if (act) {
        out.push(act);
        if (out.length >= MAX_ACTIVITIES) break;
      }
    }
    return out;
  }

  window.PERT = {
    runPert,
    critPath,
    projStats,
    normalCdf,
    topoOrder,
    parseMatrix,
    activityFromSeven,
    interpretation,
  };
})();

