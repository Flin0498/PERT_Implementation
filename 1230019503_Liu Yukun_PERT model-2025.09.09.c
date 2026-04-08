#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#define MAX_ACTIVITIES 50
#define MAX_PREDECESSORS 10
#define FILENAME_LENGTH 100

// 定义数学常量（如果系统没有定义）
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    char code;
    char description[100];
    int normal_cost;
    char predecessors[MAX_PREDECESSORS];
    int pred_count;
    
    double optimistic;
    double most_likely;
    double pessimistic;
    
    double mean_time;
    double variance;
    
    double ES;
    double EF;
    double LS;
    double LF;
    double slack;
    int is_critical;
} Activity;

Activity activities[MAX_ACTIVITIES];
int activity_count = 0;

// Function declarations
void initialize_sample_data();
int read_data_from_file(const char* filename);
void calculate_mean_and_variance();
void calculate_forward_pass();
void calculate_backward_pass();
void calculate_slack_and_critical();
void find_critical_path();
void print_results();
void calculate_standard_deviation_and_zscore();
void calculate_custom_zscore();
int find_activity_index(char code);
double get_max_ef_of_predecessors(char* predecessors, int pred_count);
void print_network_diagram();
double get_min_ls_of_successors(char code);
void clear_input_buffer();
void display_menu();
void print_activity_data();
void print_time_estimates();
double calculate_normal_probability(double z);
void parse_predecessors(char* pred_str, Activity* activity);
int parse_table_line(char* line, Activity* activity);
void remove_quotes_and_trim(char* str);
int smart_field_parser(char* line, char fields[][100], int max_fields);

int main() {
    char choice;
    char filename[FILENAME_LENGTH];
    int file_loaded = 0;
    
    printf("=== PERT ANALYSIS PROGRAM ===\n\n");
    
    // 询问用户是否从文件加载数据
    printf("Do you want to load data from file? (y/n): ");
    scanf(" %c", &choice);
    clear_input_buffer();
    
    if (choice == 'y' || choice == 'Y') {
        printf("Enter data file path: ");
        fgets(filename, FILENAME_LENGTH, stdin);
        filename[strcspn(filename, "\n")] = 0; // Remove newline
        
        file_loaded = read_data_from_file(filename);
        
        if (!file_loaded) {
            printf("Failed to load data from file. Using sample data instead.\n\n");
            initialize_sample_data();
        } else {
            printf("Data loaded successfully from: %s\n\n", filename);
        }
    } else {
        // 使用样本数据
        initialize_sample_data();
        printf("Using sample data for PERT analysis.\n\n");
    }
    
    if (activity_count == 0) {
        printf("Error: No activities initialized\n");
        printf("Press Enter to exit...");
        getchar();
        return 1;
    }
    
    // Perform PERT calculations
    printf("Performing PERT calculations...\n");
    calculate_mean_and_variance();
    calculate_forward_pass();
    calculate_backward_pass();
    calculate_slack_and_critical();
    
    printf("Calculations completed successfully!\n\n");
    
    // Interactive menu
    do {
        display_menu();
        scanf(" %c", &choice);
        clear_input_buffer();
        
        switch(choice) {
            case '1':
                print_activity_data();
                break;
            case '2':
                print_time_estimates();
                break;
            case '3':
                print_results();
                break;
            case '4':
                find_critical_path();
                break;
            case '5':
                calculate_standard_deviation_and_zscore();
                break;
            case '6':
                calculate_custom_zscore();
                break;
            case '7':
                print_network_diagram();
                break;
            case '8':
                printf("\n=== COMPLETE PERT ANALYSIS ===\n");
                print_activity_data();
                print_time_estimates();
                print_results();
                find_critical_path();
                calculate_standard_deviation_and_zscore();
                print_network_diagram();
                break;
            case '9':
                printf("Exiting program...\n");
                break;
            default:
                printf("Invalid choice. Please try again.\n");
        }
        
        if (choice != '9') {
            printf("\nPress Enter to continue...");
            getchar();
        }
        
    } while (choice != '9');
    
    printf("Thank you for using PERT Analysis Program!\n");
    printf("Press Enter to exit completely...");
    getchar();
    
    return 0;
}

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void display_menu() {
    printf("\n=== PERT ANALYSIS MENU ===\n");
    printf("1. Show Activity Data\n");
    printf("2. Show Time Estimates\n");
    printf("3. Show Schedule Analysis\n");
    printf("4. Show Critical Path\n");
    printf("5. Show Statistical Analysis\n");
    printf("6. Calculate Completion Probability for Target Date\n");
    printf("7. Show Network Diagram\n");
    printf("8. Show All Results\n");
    printf("9. Exit\n");
    printf("Choose an option (1-9): ");
}

// 去除引号和修剪字符串
void remove_quotes_and_trim(char* str) {
    if (str == NULL || strlen(str) == 0) return;
    
    // 去除开头空格和制表符
    char *start = str;
    while (*start == ' ' || *start == '\t') start++;
    
    // 去除结尾空格和制表符
    char *end = start + strlen(start) - 1;
    while (end > start && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) end--;
    *(end + 1) = '\0';
    
    // 去除引号
    if (start[0] == '"' && start[strlen(start)-1] == '"') {
        start[strlen(start)-1] = '\0';
        start++;
    } else if (start[0] == '\'' && start[strlen(start)-1] == '\'') {
        start[strlen(start)-1] = '\0';
        start++;
    }
    
    // 移动内容到原字符串开始
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

// 智能字段解析器 - 处理各种分隔符
int smart_field_parser(char* line, char fields[][100], int max_fields) {
    int field_count = 0;
    char temp[512];
    strcpy(temp, line);
    
    // 首先尝试制表符分割
    char* token = strtok(temp, "\t");
    while (token != NULL && field_count < max_fields) {
        remove_quotes_and_trim(token);
        if (strlen(token) > 0) {
            strcpy(fields[field_count], token);
            field_count++;
        }
        token = strtok(NULL, "\t");
    }
    
    // 如果制表符分割得到的字段数合适(>=7)，直接返回
    if (field_count >= 7) {
        return field_count;
    }
    
    // 如果字段数不够，尝试用多个连续空格分割
    if (field_count < 7) {
        field_count = 0;
        strcpy(temp, line);
        
        // 替换多个连续空格为单个制表符
        char cleaned[512];
        char *src = temp, *dst = cleaned;
        int space_count = 0;
        
        while (*src) {
            if (*src == ' ') {
                space_count++;
                if (space_count == 1) {
                    *dst++ = '\t';
                }
            } else {
                space_count = 0;
                *dst++ = *src;
            }
            src++;
        }
        *dst = '\0';
        
        // 用制表符分割
        token = strtok(cleaned, "\t");
        while (token != NULL && field_count < max_fields) {
            remove_quotes_and_trim(token);
            if (strlen(token) > 0) {
                strcpy(fields[field_count], token);
                field_count++;
            }
            token = strtok(NULL, "\t");
        }
    }
    
    return field_count;
}

// 解析前驱活动字符串
void parse_predecessors(char* pred_str, Activity* activity) {
    activity->pred_count = 0;
    
    if (strcmp(pred_str, "-") == 0 || strlen(pred_str) == 0 || 
        strcmp(pred_str, "None") == 0 || strcmp(pred_str, "none") == 0) {
        return; // 没有前驱活动
    }
    
    char temp_str[100];
    strcpy(temp_str, pred_str);
    
    char* token = strtok(temp_str, ",;");
    while (token != NULL && activity->pred_count < MAX_PREDECESSORS) {
        // 去除空格和引号
        remove_quotes_and_trim(token);
        
        if (strlen(token) > 0 && isalpha(token[0])) {
            activity->predecessors[activity->pred_count++] = token[0];
        }
        token = strtok(NULL, ",;");
    }
}

// 清理成本字符串 - 只保留数字
void clean_cost_string(char* cost_str, char* clean_cost) {
    clean_cost[0] = '\0';
    for (int i = 0; cost_str[i] != '\0'; i++) {
        if (isdigit(cost_str[i])) {
            char temp[2] = {cost_str[i], '\0'};
            strcat(clean_cost, temp);
        }
    }
    // 如果清理后为空，设置为0
    if (strlen(clean_cost) == 0) {
        strcpy(clean_cost, "0");
    }
}

// 解析表格行数据 - 通用版本
int parse_table_line(char* line, Activity* activity) {
    // 跳过表头行
    if (strstr(line, "Activity Code") != NULL || 
        strstr(line, "Activity Description") != NULL ||
        strstr(line, "Normal Cost") != NULL ||
        strstr(line, "Immediate Predecessors") != NULL ||
        strstr(line, "Optimistic Estimate") != NULL ||
        strstr(line, "Most Likely Estimate") != NULL ||
        strstr(line, "Pessimistic Estimate") != NULL) {
        return 0;
    }
    
    // 跳过空行
    if (strlen(line) == 0 || line[0] == '\n' || line[0] == '\r') {
        return 0;
    }
    
    char fields[20][100]; // 支持最多20个字段
    int field_count = smart_field_parser(line, fields, 20);
    
    if (field_count < 7) {
        return 0; // 字段数不够
    }
    
    // 查找活动代码字段（应该是第一个字段）
    int code_index = 0;
    if (strlen(fields[code_index]) == 1 && isalpha(fields[code_index][0])) {
        activity->code = fields[code_index][0];
    } else {
        return 0; // 无效的活动代码
    }
    
    // 查找描述字段（应该是第二个字段）
    int desc_index = 1;
    strcpy(activity->description, fields[desc_index]);
    
    // 查找成本字段 - 尝试不同的位置
    int cost_index = -1;
    for (int i = 2; i < field_count - 3; i++) {
        // 成本字段应该包含数字
        int has_digit = 0;
        for (int j = 0; fields[i][j] != '\0'; j++) {
            if (isdigit(fields[i][j])) {
                has_digit = 1;
                break;
            }
        }
        if (has_digit) {
            cost_index = i;
            break;
        }
    }
    
    if (cost_index == -1) {
        cost_index = 2; // 默认位置
    }
    
    char clean_cost[50];
    clean_cost_string(fields[cost_index], clean_cost);
    activity->normal_cost = atoi(clean_cost);
    
    // 前驱字段在成本字段之后
    int pred_index = cost_index + 1;
    if (pred_index >= field_count) pred_index = field_count - 4;
    
    // 时间估计字段应该是最后三个字段
    int opt_index = field_count - 3;
    int most_index = field_count - 2;
    int pess_index = field_count - 1;
    
    // 解析前驱活动
    parse_predecessors(fields[pred_index], activity);
    
    // 设置时间估计
    activity->optimistic = atof(fields[opt_index]);
    activity->most_likely = atof(fields[most_index]);
    activity->pessimistic = atof(fields[pess_index]);
    
    // 验证时间估计的有效性
    if (activity->optimistic <= 0 || activity->most_likely <= 0 || activity->pessimistic <= 0) {
        return 0;
    }
    
    return 1;
}

// 从文件读取数据 - 通用版本
int read_data_from_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Cannot open file %s\n", filename);
        return 0;
    }
    
    char line[512];
    activity_count = 0;
    int line_number = 0;
    
    printf("Reading data from file: %s\n", filename);
    
    while (fgets(line, sizeof(line), file) && activity_count < MAX_ACTIVITIES) {
        line_number++;
        // 去除换行符
        line[strcspn(line, "\n")] = 0;
        line[strcspn(line, "\r")] = 0;
        
        // 跳过空行
        if (strlen(line) == 0) continue;
        
        // 尝试解析表格行
        if (parse_table_line(line, &activities[activity_count])) {
            printf("  Loaded activity %c: %s\n", activities[activity_count].code, 
                   activities[activity_count].description);
            activity_count++;
        }
    }
    
    fclose(file);
    
    if (activity_count == 0) {
        printf("No valid data found in file.\n");
        printf("Please ensure the file has the correct format with at least 7 columns:\n");
        printf("ActivityCode, Description, Cost, Predecessors, Optimistic, MostLikely, Pessimistic\n");
        return 0;
    }
    
    printf("Successfully loaded %d activities from file.\n", activity_count);
    return 1;
}

// 计算标准正态分布累积概率的近似函数
double calculate_normal_probability(double z) {
    // 使用标准正态分布累积分布函数的近似计算
    if (z < -8.0) return 0.0;
    if (z > 8.0) return 1.0;
    
    // 使用Hastings有理逼近方法
    double t = 1.0 / (1.0 + 0.2316419 * fabs(z));
    double d = 1.0 / sqrt(2.0 * M_PI) * exp(-z * z / 2.0);
    double p = d * t * (0.319381530 + t * (-0.356563782 + t * (1.781477937 + t * (-1.821255978 + t * 1.330274429))));
    
    if (z > 0) {
        return 1.0 - p;
    } else {
        return p;
    }
}

void calculate_custom_zscore() {
    double target_days;
    char response;
    
    printf("\n=== COMPLETION PROBABILITY CALCULATOR ===\n");
    
    // 获取项目完成时间和标准差
    double project_duration = 0;
    double total_variance = 0;
    
    for (int i = 0; i < activity_count; i++) {
        if (activities[i].EF > project_duration) {
            project_duration = activities[i].EF;
        }
        if (activities[i].is_critical) {
            total_variance += activities[i].variance;
        }
    }
    
    double standard_deviation = sqrt(total_variance);
    
    printf("Project Expected Completion Time: %.2f days\n", project_duration);
    printf("Critical Path Standard Deviation: %.2f days\n\n", standard_deviation);
    
    do {
        printf("Enter target completion days: ");
        if (scanf("%lf", &target_days) != 1) {
            printf("Invalid input. Please enter a number.\n");
            clear_input_buffer();
            continue;
        }
        clear_input_buffer();
        
        // 计算Z-score
        double z_score = (target_days - project_duration) / standard_deviation;
        
        // 计算完成概率
        double probability = calculate_normal_probability(z_score) * 100;
        
        printf("\n--- RESULTS ---\n");
        printf("Target Completion Time: %.2f days\n", target_days);
        printf("Expected Completion Time: %.2f days\n", project_duration);
        printf("Z-score: %.4f\n", z_score);
        printf("Probability of completing within target: %.2f%%\n", probability);
        
        // 提供解释
        printf("\n--- INTERPRETATION ---\n");
        if (probability > 90) {
            printf("HIGH probability - Project is very likely to be completed on time\n");
        } else if (probability > 70) {
            printf("MODERATE probability - Project has good chance of completion\n");
        } else if (probability > 50) {
            printf("FAIR probability - Some risk of delay exists\n");
        } else if (probability > 30) {
            printf("LOW probability - Significant risk of delay\n");
        } else {
            printf("VERY LOW probability - High risk of missing deadline\n");
        }
        
        printf("\nCalculate another probability? (y/n): ");
        scanf(" %c", &response);
        clear_input_buffer();
        
    } while (response == 'y' || response == 'Y');
    
    // 显示一些常见Z-score值的参考
    printf("\n--- Z-SCORE REFERENCE ---\n");
    printf("| Z-score | Probability | Interpretation          |\n");
    printf("|---------|-------------|-------------------------|\n");
    printf("|  -3.0   |    0.13%%    | Very unlikely           |\n");
    printf("|  -2.0   |    2.28%%    | Unlikely                |\n");
    printf("|  -1.0   |   15.87%%    | Somewhat unlikely       |\n");
    printf("|   0.0   |   50.00%%    | Even chance             |\n");
    printf("|   1.0   |   84.13%%    | Somewhat likely         |\n");
    printf("|   2.0   |   97.72%%    | Likely                  |\n");
    printf("|   3.0   |   99.87%%    | Very likely             |\n");
}

void calculate_standard_deviation_and_zscore() {
    double total_variance = 0;
    
    // Calculate total variance of critical path activities
    for (int i = 0; i < activity_count; i++) {
        if (activities[i].is_critical) {
            total_variance += activities[i].variance;
        }
    }
    
    double standard_deviation = sqrt(total_variance);
    double project_duration = 0;
    
    // Find project total duration
    for (int i = 0; i < activity_count; i++) {
        if (activities[i].EF > project_duration) {
            project_duration = activities[i].EF;
        }
    }
    
    printf("\n=== STATISTICAL ANALYSIS ===\n");
    printf("Project Expected Completion Time: %.2f days\n", project_duration);
    printf("Critical Path Standard Deviation: %.2f days\n", standard_deviation);
    printf("Total Variance of Critical Path: %.3f\n", total_variance);
    
    // 使用英文表示，避免符号显示问题
    printf("\nStandard Z-score Reference:\n");
    printf("| Target Time | Z-score | Completion Probability |\n");
    printf("|-------------|---------|------------------------|\n");
    
    double target_times[] = {
        project_duration - 2 * standard_deviation,
        project_duration - standard_deviation,
        project_duration,
        project_duration + standard_deviation,
        project_duration + 2 * standard_deviation
    };
    
    const char* time_labels[] = {
        "Mean - 2SD", "Mean - SD", "Mean", "Mean + SD", "Mean + 2SD"
    };
    
    for (int i = 0; i < 5; i++) {
        double z_score = (target_times[i] - project_duration) / standard_deviation;
        double probability = calculate_normal_probability(z_score) * 100;
        printf("| %11s | %7.2f | %21.1f%% |\n", time_labels[i], z_score, probability);
    }
    
    printf("\nNote: Use option 6 to calculate probability for any target date.\n");
}

// 样本数据初始化（作为备用）
void initialize_sample_data() {
    activity_count = 14;
    
    // Activity A
    activities[0].code = 'A';
    strcpy(activities[0].description, "Site clearing");
    activities[0].normal_cost = 18415;
    activities[0].pred_count = 0;
    activities[0].optimistic = 1;
    activities[0].most_likely = 2;
    activities[0].pessimistic = 3;
    
    // Activity B
    activities[1].code = 'B';
    strcpy(activities[1].description, "Foundation");
    activities[1].normal_cost = 12643;
    activities[1].predecessors[0] = 'A';
    activities[1].pred_count = 1;
    activities[1].optimistic = 2;
    activities[1].most_likely = 3.5;
    activities[1].pessimistic = 8;
    
    // Activity C
    activities[2].code = 'C';
    strcpy(activities[2].description, "Block Laying");
    activities[2].normal_cost = 52811;
    activities[2].predecessors[0] = 'B';
    activities[2].pred_count = 1;
    activities[2].optimistic = 6;
    activities[2].most_likely = 9;
    activities[2].pessimistic = 18;
    
    // Activity D
    activities[3].code = 'D';
    strcpy(activities[3].description, "Roofing");
    activities[3].normal_cost = 41658;
    activities[3].predecessors[0] = 'C';
    activities[3].pred_count = 1;
    activities[3].optimistic = 4;
    activities[3].most_likely = 5.5;
    activities[3].pessimistic = 10;
    
    // Activity E
    activities[4].code = 'E';
    strcpy(activities[4].description, "Plumbing");
    activities[4].normal_cost = 50179;
    activities[4].predecessors[0] = 'C';
    activities[4].pred_count = 1;
    activities[4].optimistic = 1;
    activities[4].most_likely = 4.5;
    activities[4].pessimistic = 5;
    
    // Activity F
    activities[5].code = 'F';
    strcpy(activities[5].description, "Electrical work");
    activities[5].normal_cost = 52681;
    activities[5].predecessors[0] = 'E';
    activities[5].pred_count = 1;
    activities[5].optimistic = 4;
    activities[5].most_likely = 4;
    activities[5].pessimistic = 10;
    
    // Activity G
    activities[6].code = 'G';
    strcpy(activities[6].description, "Plastering");
    activities[6].normal_cost = 63894;
    activities[6].predecessors[0] = 'D';
    activities[6].pred_count = 1;
    activities[6].optimistic = 5;
    activities[6].most_likely = 6.5;
    activities[6].pessimistic = 11;
    
    // Activity H
    activities[7].code = 'H';
    strcpy(activities[7].description, "Fixing up of doors and windows");
    activities[7].normal_cost = 35905;
    activities[7].predecessors[0] = 'E';
    activities[7].predecessors[1] = 'G';
    activities[7].pred_count = 2;
    activities[7].optimistic = 5;
    activities[7].most_likely = 8;
    activities[7].pessimistic = 17;
    
    // Activity I
    activities[8].code = 'I';
    strcpy(activities[8].description, "Ceiling");
    activities[8].normal_cost = 40997;
    activities[8].predecessors[0] = 'C';
    activities[8].pred_count = 1;
    activities[8].optimistic = 3;
    activities[8].most_likely = 7.5;
    activities[8].pessimistic = 9;
    
    // Activity J
    activities[9].code = 'J';
    strcpy(activities[9].description, "Flooring");
    activities[9].normal_cost = 42394;
    activities[9].predecessors[0] = 'F';
    activities[9].predecessors[1] = 'I';
    activities[9].pred_count = 2;
    activities[9].optimistic = 3;
    activities[9].most_likely = 9;
    activities[9].pessimistic = 9;
    
    // Activity K
    activities[10].code = 'K';
    strcpy(activities[10].description, "Interior Fixtures");
    activities[10].normal_cost = 47643;
    activities[10].predecessors[0] = 'J';
    activities[10].pred_count = 1;
    activities[10].optimistic = 4;
    activities[10].most_likely = 4;
    activities[10].pessimistic = 4;
    
    // Activity L
    activities[11].code = 'L';
    strcpy(activities[11].description, "Exterior fixtures");
    activities[11].normal_cost = 33887;
    activities[11].predecessors[0] = 'J';
    activities[11].pred_count = 1;
    activities[11].optimistic = 1;
    activities[11].most_likely = 5.5;
    activities[11].pessimistic = 7;
    
    // Activity M
    activities[12].code = 'M';
    strcpy(activities[12].description, "Painting");
    activities[12].normal_cost = 41539;
    activities[12].predecessors[0] = 'H';
    activities[12].pred_count = 1;
    activities[12].optimistic = 1;
    activities[12].most_likely = 2;
    activities[12].pessimistic = 3;
    
    // Activity N
    activities[13].code = 'N';
    strcpy(activities[13].description, "Landscaping");
    activities[13].normal_cost = 36913;
    activities[13].predecessors[0] = 'K';
    activities[13].predecessors[1] = 'L';
    activities[13].pred_count = 2;
    activities[13].optimistic = 5;
    activities[13].most_likely = 5.5;
    activities[13].pessimistic = 9;
}

void calculate_mean_and_variance() {
    for (int i = 0; i < activity_count; i++) {
        Activity *act = &activities[i];
        act->mean_time = (act->optimistic + 4 * act->most_likely + act->pessimistic) / 6.0;
        act->variance = pow((act->pessimistic - act->optimistic), 2) / 36.0;
    }
}

void calculate_forward_pass() {
    int processed[MAX_ACTIVITIES] = {0};
    int order_count = 0;
    
    // Initialize ES for all activities to -1 (unprocessed)
    for (int i = 0; i < activity_count; i++) {
        activities[i].ES = -1;
    }
    
    // Process activities in topological order
    while (order_count < activity_count) {
        for (int i = 0; i < activity_count; i++) {
            if (!processed[i]) {
                Activity *act = &activities[i];
                int can_process = 1;
                
                // Check if all predecessors are processed
                for (int j = 0; j < act->pred_count; j++) {
                    char pred_code = act->predecessors[j];
                    int pred_index = find_activity_index(pred_code);
                    if (pred_index == -1 || activities[pred_index].ES == -1) {
                        can_process = 0;
                        break;
                    }
                }
                
                if (can_process) {
                    if (act->pred_count == 0) {
                        // Activities with no predecessors
                        act->ES = 0;
                    } else {
                        // ES = max(EF of all immediate predecessors)
                        act->ES = get_max_ef_of_predecessors(act->predecessors, act->pred_count);
                    }
                    
                    act->EF = act->ES + act->mean_time;
                    processed[i] = 1;
                    order_count++;
                }
            }
        }
    }
}

void calculate_backward_pass() {
    // Find project end time (maximum EF)
    double project_end_time = 0;
    for (int i = 0; i < activity_count; i++) {
        if (activities[i].EF > project_end_time) {
            project_end_time = activities[i].EF;
        }
    }
    
    // Initialize LF for all activities
    for (int i = 0; i < activity_count; i++) {
        activities[i].LF = project_end_time;
        activities[i].LS = activities[i].LF - activities[i].mean_time;
    }
    
    int processed[MAX_ACTIVITIES] = {0};
    int order_count = 0;
    
    // Process activities in reverse topological order
    while (order_count < activity_count) {
        for (int i = 0; i < activity_count; i++) {
            if (!processed[i]) {
                Activity *act = &activities[i];
                int can_process = 1;
                
                // Check if this activity has successors that are not processed
                for (int j = 0; j < activity_count; j++) {
                    for (int k = 0; k < activities[j].pred_count; k++) {
                        if (activities[j].predecessors[k] == act->code && !processed[j]) {
                            can_process = 0;
                            break;
                        }
                    }
                    if (!can_process) break;
                }
                
                if (can_process) {
                    // If activity has successors, LF = min(LS of all immediate successors)
                    double min_ls = get_min_ls_of_successors(act->code);
                    if (min_ls != -1) {
                        act->LF = min_ls;
                    }
                    act->LS = act->LF - act->mean_time;
                    processed[i] = 1;
                    order_count++;
                }
            }
        }
    }
}

void calculate_slack_and_critical() {
    for (int i = 0; i < activity_count; i++) {
        Activity *act = &activities[i];
        act->slack = act->LS - act->ES;
        // Consider activity critical if slack is very close to 0
        act->is_critical = (fabs(act->slack) < 0.001) ? 1 : 0;
    }
}

double get_max_ef_of_predecessors(char* predecessors, int pred_count) {
    double max_ef = -1;
    
    for (int i = 0; i < pred_count; i++) {
        char pred_code = predecessors[i];
        int pred_index = find_activity_index(pred_code);
        if (pred_index != -1) {
            if (activities[pred_index].EF > max_ef) {
                max_ef = activities[pred_index].EF;
            }
        }
    }
    
    return max_ef;
}

double get_min_ls_of_successors(char code) {
    double min_ls = -1;
    int found = 0;
    
    for (int i = 0; i < activity_count; i++) {
        for (int j = 0; j < activities[i].pred_count; j++) {
            if (activities[i].predecessors[j] == code) {
                if (!found || activities[i].LS < min_ls) {
                    min_ls = activities[i].LS;
                    found = 1;
                }
            }
        }
    }
    
    return min_ls;
}

int find_activity_index(char code) {
    for (int i = 0; i < activity_count; i++) {
        if (activities[i].code == code) {
            return i;
        }
    }
    return -1;
}

void print_activity_data() {
    printf("\n=== ACTIVITY BASIC INFORMATION ===\n\n");
    printf("| Code | Description                       | Normal Cost | Predecessors |\n");
    printf("|------|-----------------------------------|-------------|--------------|\n");
    
    for (int i = 0; i < activity_count; i++) {
        Activity act = activities[i];
        char pred_str[20] = "-";
        
        if (act.pred_count > 0) {
            strcpy(pred_str, "");
            for (int j = 0; j < act.pred_count; j++) {
                char temp[3];
                sprintf(temp, "%c", act.predecessors[j]);
                strcat(pred_str, temp);
                if (j < act.pred_count - 1) {
                    strcat(pred_str, ",");
                }
            }
        }
        
        printf("|  %c   | %-33s | %11d | %12s |\n",
               act.code, act.description, act.normal_cost, pred_str);
    }
}

void print_time_estimates() {
    printf("\n=== ACTIVITY TIME ESTIMATES ===\n\n");
    printf("| Code | Optimistic | Most Likely | Pessimistic | Mean Time | Variance |\n");
    printf("|------|------------|-------------|-------------|-----------|----------|\n");
    
    for (int i = 0; i < activity_count; i++) {
        Activity act = activities[i];
        printf("|  %c   |    %5.1f    |    %5.1f     |    %5.1f    |   %5.2f   |  %5.3f  |\n",
               act.code, act.optimistic, act.most_likely, act.pessimistic,
               act.mean_time, act.variance);
    }
}

void print_results() {
    printf("\n=== SCHEDULE ANALYSIS ===\n\n");
    printf("| Code |   ES   |   EF   |   LS   |   LF   |  Slack  | Critical |\n");
    printf("|------|--------|--------|--------|--------|---------|----------|\n");
    
    for (int i = 0; i < activity_count; i++) {
        Activity act = activities[i];
        printf("|  %c   | %6.2f | %6.2f | %6.2f | %6.2f | %7.2f |    %s    |\n",
               act.code, act.ES, act.EF, act.LS, act.LF,
               act.slack, act.is_critical ? "Yes" : "No");
    }
}

void find_critical_path() {
    printf("\n=== CRITICAL PATH ANALYSIS ===\n");
    
    // Find all critical activities
    int critical_activities[MAX_ACTIVITIES];
    int critical_count = 0;
    
    for (int i = 0; i < activity_count; i++) {
        if (activities[i].is_critical) {
            critical_activities[critical_count++] = i;
        }
    }
    
    // Sort critical activities by ES
    for (int i = 0; i < critical_count - 1; i++) {
        for (int j = i + 1; j < critical_count; j++) {
            if (activities[critical_activities[i]].ES > activities[critical_activities[j]].ES) {
                int temp = critical_activities[i];
                critical_activities[i] = critical_activities[j];
                critical_activities[j] = temp;
            }
        }
    }
    
    printf("Critical Path: ");
    double path_duration = 0;
    for (int i = 0; i < critical_count; i++) {
        printf("%c", activities[critical_activities[i]].code);
        path_duration += activities[critical_activities[i]].mean_time;
        if (i < critical_count - 1) {
            printf(" -> ");
        }
    }
    printf("\nCritical Path Duration: %.2f days\n", path_duration);
    
    printf("Critical Activities: ");
    for (int i = 0; i < critical_count; i++) {
        printf("%c ", activities[critical_activities[i]].code);
    }
    printf("\n");
    
    printf("Total Project Duration: %.2f days\n", activities[critical_activities[critical_count-1]].EF);
}

void print_network_diagram() {
    printf("\n=== NETWORK DIAGRAM ===\n");
    printf("Activity Dependencies:\n\n");
    
    for (int i = 0; i < activity_count; i++) {
        Activity act = activities[i];
        printf("Activity %c (%s)\n", act.code, act.description);
        printf("  Predecessors: ");
        
        if (act.pred_count == 0) {
            printf("Start");
        } else {
            for (int j = 0; j < act.pred_count; j++) {
                printf("%c", act.predecessors[j]);
                if (j < act.pred_count - 1) {
                    printf(" -> ");
                }
            }
        }
        
        printf(" -> %c", act.code);
        
        if (act.is_critical) {
            printf(" [CRITICAL]");
        }
        printf("\n\n");
    }
    
    printf("Network Calculation Order (Topological): ");
    // Determine and display calculation order
    int processed[MAX_ACTIVITIES] = {0};
    int order_count = 0;
    
    while (order_count < activity_count) {
        for (int i = 0; i < activity_count; i++) {
            if (!processed[i]) {
                Activity *act = &activities[i];
                int can_process = 1;
                
                for (int j = 0; j < act->pred_count; j++) {
                    char pred_code = act->predecessors[j];
                    int pred_index = find_activity_index(pred_code);
                    if (pred_index == -1 || !processed[pred_index]) {
                        can_process = 0;
                        break;
                    }
                }
                
                if (can_process) {
                    printf("%c ", act->code);
                    processed[i] = 1;
                    order_count++;
                }
            }
        }
    }
    printf("\n");
}