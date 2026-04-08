# PERT_Implementation

[![Language](https://img.shields.io/badge/Language-C%20%7C%20JavaScript-555?style=flat-square)](#)
[![UI](https://img.shields.io/badge/UI-Offline%20HTML%2FCSS%2FJS-2a4a6b?style=flat-square)](#)
[![Spreadsheet](https://img.shields.io/badge/Import-XLSX%20%7C%20XLS%20%7C%20CSV-0b6?style=flat-square)](#)

[English](#english) | [简体中文](#简体中文)

---

<h2 id="english">English</h2>

### 1. Abstract
This project implements **PERT (Program Evaluation and Review Technique)** analysis:

- Compute **mean time** and **variance** from three-point estimates (O/M/P)
- Run **forward pass** and **backward pass** to get **ES/EF/LS/LF**
- Compute **slack** and identify the **critical path**
- Estimate **completion probability** for a target duration (normal approximation)

It contains:
- A **C** console program (interactive menu + optional file input)
- An **offline web UI** (no server required) for spreadsheet upload or form input

### 2. Project Structure
```
├─ PERT model-2025.09.09.c
├─ PERT model-2025.09.09.xlsx   (example input) 
├─ index.html
├─ styles.css
├─ pert.js
└─ xlsx.full.min.js
```

### 3. Offline Web UI (recommended)
Open:
- `index.html`

Input methods:
- **Upload spreadsheet**: `.xlsx` / `.xls` / `.csv`
- **Manual form**: add/remove rows and click **Calculate**

Spreadsheet header requirements:
- `Activity Code`
- `Activity Description`
- `Normal Cost (BDT)`
- `Immediate Predecessors`
- `Optimistic Estimate`
- `Most Likely Estimate`
- `Pessimistic Estimate`

Notes:
- Inputs should use **English keyboard layout** (single-letter activity codes like `A`, `B`, …).
- `Immediate Predecessors` accepts multiple formats, e.g. `E,G`, `E G`, `E, G`.

### 4. C Program (CLI)
Compile (MinGW-w64 example):
```bash
gcc "PERT model-2025.09.09.c" -o pert.exe
```

Run:
```bash
./pert.exe
```

---

<h2 id="简体中文">简体中文</h2>

### 1. 项目简介
本项目实现 **PERT（计划评审技术）** 分析：

- 由三点估计（乐观/最可能/悲观）计算 **期望工期** 与 **方差**
- 前向/后向计算得到 **ES/EF/LS/LF**
- 计算 **时差** 并识别 **关键路径**
- 针对目标工期计算 **完工概率**（正态近似）

项目包含：
- **C 语言**控制台程序（交互菜单 + 可选文件输入）
- **全离线**网页前端（无需服务器）支持上传表格或手动填表

### 2. 目录结构
```
├─ PERT model-2025.09.09.c
├─ PERT model-2025.09.09.xlsx   (example input)
├─ index.html
├─ styles.css
├─ pert.js
└─ xlsx.full.min.js
```

### 3. 离线网页端（推荐）
直接打开：
- `index.html`

输入方式：
- **上传表格**：`.xlsx` / `.xls` / `.csv`
- **手动填表**：增减行后点击 **Calculate（计算）**

表格表头要求（必须为英文列名）：
- `Activity Code`
- `Activity Description`
- `Normal Cost (BDT)`
- `Immediate Predecessors`
- `Optimistic Estimate`
- `Most Likely Estimate`
- `Pessimistic Estimate`

注意：
- 录入建议使用 **英文输入法**（活动代码为单字母，如 `A`、`B`…）
- 紧前工序支持多种写法：`E,G`、`E G`、`E, G` 等价

### 4. C 程序（命令行）
编译（以 MinGW-w64 为例）：
```bash
gcc "PERT model-2025.09.09.c" -o pert.exe
```

运行：
```bash
./pert.exe
```
