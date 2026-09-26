// ui.c —— 菜单交互与安全输入

// 项目头文件
#include "ui.h"
#include "student.h"
#include "storage.h"

// 标准库头文件
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

// Windows API 头文件
#include <windows.h>

// 常量定义
#define DATA_FILE "students.dat"  // 数据文件名
#define INPUT_BUF_LEN 64  // 输入缓冲区长度

// 内部函数声明
static char *trim(char *s);
static char *read_line(const char *prompt, char *buf, size_t size);
static int read_int(const char *prompt, int min, int max, int *out);
static int read_double(const char *prompt, double min, double max, double *out);
static void copy_str(char *dst, size_t dst_size, const char *src);
static void clear_screen(void);
static void ui_add(StudentList *list, int *dirty);
static void ui_remove(StudentList *list, int *dirty);
static void ui_modify(StudentList *list, int *dirty);
static void ui_find_by_id(StudentList *list);
static void ui_find_by_name(StudentList *list);
static void ui_sort_show(StudentList *list);
static void ui_save(StudentList *list, int *dirty);
static void ui_load(StudentList *list, int *dirty);
static void press_enter_to_continue(void);
static void print_menu(void);

/*
 * 关于字符串字面量：
 * 字符串字面量不能被修改，传给会修改其内容的函数会产生未定义行为，可能导致
 * 程序异常退出。测试 trim 函数时，应使用字符数组（写法二）。
 * 写法一：仅输出字符串字面量。
 *     printf("%s\n", "测试字符串");
 * 写法二：使用可修改的字符数组。
 *     char s[] = "测试字符串";
 *     printf("%s\n", trim(s));
 */
// 函数：去除字符串首尾的空白字符。
// 返回值：去除首尾空白字符后的字符串。
static char *trim(char *s) {
    // 去除首部空白字符
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') {
        s++;
    }
    // 定位字符串末尾
    char *end;
    end = s + strlen(s);
    // 去除尾部空白字符
    while (end > s &&
           (*(end - 1) == ' ' || *(end - 1) == '\t' || *(end - 1) == '\n' || *(end - 1) == '\r')) {
        end--;
        *end = '\0';  // 在末尾添加字符串结束符
    }
    return s;  // 返回去除首尾空白字符后的字符串
}

// 函数：安全输入字符串。
// 返回值：去除首尾空白字符后的缓冲区内指针；遇到 EOF 或读取错误时返回 NULL。
static char *read_line(const char *prompt, char *buf, size_t size) {
    // 限制 size 不超过 INT_MAX，避免转换为 fgets 的 int 参数时超出范围
    size_t safe_size = (size > INT_MAX) ? INT_MAX : size;
    if (prompt != NULL) {
        printf("%s", prompt);
        fflush(stdout);  // 刷新输出缓冲区，确保提示信息立即输出
    }
    if (fgets(buf, (int)safe_size, stdin) == NULL) {
        return NULL;  // 遇到 EOF 或读取错误时返回 NULL
    }
    // 下方 if 语句用于丢弃本次读取未容纳的行尾内容
    // strchr 查找换行符；未找到时，当前行可能尚未读取完整
    // feof 检查流的文件结束标志，避免在已知到达文件末尾时继续清理
    // feof 返回非零值表示已设置文件结束标志，返回 0 表示尚未设置
    if (strchr(buf, '\n') == NULL && !feof(stdin)) {
        int ch;
        // 丢弃当前行剩余字符，直到遇到换行符或文件末尾
        while ((ch = getchar()) != '\n' && ch != EOF)
            ;
    }
    // 使用 strcspn 定位首个回车或换行符，并将其替换为字符串结束符
    buf[strcspn(buf, "\r\n")] = '\0';
    // 去除首尾空白字符后返回
    return trim(buf);
}

// 函数：读取 [min, max] 范围内的整数（输入无效时重试）。
// 返回值：1 表示成功并将整数写入 *out；0 表示遇到 EOF 或读取错误。
static int read_int(const char *prompt, int min, int max, int *out) {
    char buf[INPUT_BUF_LEN];
    char *line;
    char *end;
    long value;
    // 循环读取，直到获得有效输入、遇到 EOF 或发生读取错误
    for (;;) {
        line = read_line(prompt, buf, INPUT_BUF_LEN);
        if (line == NULL) {
            return 0;  // 读取失败
        }
        // 调用 strtol 前将 errno 清零，以便检查本次转换是否出错
        errno = 0;
        // 按十进制解析为 long；strtol 通过 &end 写回首个未解析字符的位置
        value = strtol(line, &end, 10);
        // 检查转换错误、有效字符、剩余字符和数值范围
        if (errno == 0 &&  // 检查转换未报告范围错误
            end != line &&  // 检查至少成功解析了一个字符（排除纯空白输入）
            *end == '\0' &&  // 检查数字后没有多余字符（排除类似“123abc”的输入）
            value >= min && value <= max) {  // 检查数值范围
            *out = (int)value;
            return 1;  // 读取成功
        }
        printf("输入无效，请输入%d-%d之间的整数。\n", min, max);
    }
}

// 函数：读取 [min, max] 范围内的浮点数（输入无效时重试）。
// 返回值：1 表示成功并将浮点数写入 *out；0 表示遇到 EOF 或读取错误。
static int read_double(const char *prompt, double min, double max, double *out) {
    char buf[INPUT_BUF_LEN];
    char *line;
    char *end;
    double value;
    // 循环读取，直到获得有效输入、遇到 EOF 或发生读取错误
    for (;;) {
        line = read_line(prompt, buf, INPUT_BUF_LEN);
        if (line == NULL) {
            return 0;  // 读取失败
        }
        // 调用 strtod 前将 errno 清零，以便检查本次转换是否出错
        errno = 0;
        // 解析为 double；strtod 通过 &end 写回首个未解析字符的位置
        value = strtod(line, &end);
        // 检查转换错误、有效字符、剩余字符和数值范围
        if (errno == 0 &&  // 检查转换未报告范围错误
            end != line &&  // 检查至少成功解析了一个字符
            *end == '\0' &&  // 检查数字后没有多余字符
            value >= min && value <= max) {  // 检查数值范围
            *out = value;
            return 1;  // 读取成功
        }
        printf("输入无效，请输入%.0f-%.0f之间的数字。\n", min, max);
    }
}

/*
 * 关于 snprintf 和 strcpy：
 * strcpy(dst, src) 只有两个参数，不接收目标缓冲区的容量。它以源字符串的 '\0'
 * 作为停止条件，即使超过目标缓冲区容量，也会继续写入，造成缓冲区溢出。
 * 因此，这里使用 snprintf 并传入目标缓冲区容量；空间不足时，结果会被截断。
 */
// 函数：按目标缓冲区容量复制字符串。
static void copy_str(char *dst, size_t dst_size, const char *src) {
    snprintf(dst, dst_size, "%s", src);
}

// 函数：清空屏幕（通过 Win32 API 实现）。
static void clear_screen(void) {
    HANDLE hOut;  // 控制台输出句柄
    // 控制台屏幕缓冲区信息，包含缓冲区尺寸、可见窗口范围、当前颜色属性等
    CONSOLE_SCREEN_BUFFER_INFO info;
    COORD origin;  // 填充操作的起始坐标
    DWORD cells;  // 要操作的单元格总数
    DWORD written;  // 实际写入的单元格数量（API 要求传入该输出参数）
    // 获取控制台输出句柄
    /*
     * 关于标准输入输出：
     * 标准输入、标准输出和标准错误是进程通常使用的三个标准数据通道。
     * GetStdHandle(STD_OUTPUT_HANDLE) 获取标准输出句柄；本程序在正常的控制台
     * 环境下运行时，该句柄指向控制台。
     */
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    // 检测是否为真正的控制台环境
    if (!GetConsoleScreenBufferInfo(hOut, &info)) {
        return;
    }
    origin.X = 0;
    origin.Y = 0;
    cells = (DWORD)info.dwSize.X * (DWORD)info.dwSize.Y;
    // 用空格字符填满缓冲区
    FillConsoleOutputCharacterW(hOut, L' ', cells, origin, &written);
    // 将光标移回缓冲区原点
    SetConsoleCursorPosition(hOut, origin);
}

// 函数：交互式添加学生。
static void ui_add(StudentList *list, int *dirty) {
    char buf[INPUT_BUF_LEN];
    char *line;
    Student stu;
    int ret;
    printf("—— 添加学生 ——\n");
    // 输入学号
    for (;;) {
        line = read_line("学号：", buf, sizeof(buf));
        if (line == NULL)
            return;
        if (line[0] == '\0') {
            printf("添加失败：学号不能为空。\n");
            return;
        }
        if (list_find_by_id(list, line) != NULL) {
            printf("添加失败：学号 %s 已存在。\n", line);
            return;
        }
        break;
    }
    copy_str(stu.id, sizeof(stu.id), line);
    // 输入姓名
    for (;;) {
        line = read_line("姓名：", buf, sizeof(buf));
        if (line == NULL)
            return;
        if (line[0] != '\0')
            break;
        printf("姓名不能为空，请重新输入。\n");
    }
    copy_str(stu.name, sizeof(stu.name), line);
    // 输入年龄
    if (!read_int("年龄：", 1, 150, &stu.age))
        return;
    // 输入成绩
    if (!read_double("成绩：", 0.0, 100.0, &stu.score))
        return;
    // 加入学生列表
    ret = list_add(list, &stu);
    if (ret == 0) {
        printf("添加成功。\n");
        *dirty = 1;  // 标记存在未保存的修改
    } else if (ret == -1) {
        printf("添加失败：学号 %s 已存在。\n", stu.id);
    } else if (ret == -2) {
        printf("添加失败：内存不足。\n");
    } else {
        printf("添加失败：参数错误。\n");
    }
}

// 函数：交互式删除学生。
static void ui_remove(StudentList *list, int *dirty) {
    char buf[INPUT_BUF_LEN];
    char *line;
    int ret;
    printf("—— 删除学生 ——\n");
    line = read_line("请输入要删除的学号：", buf, sizeof(buf));
    if (line == NULL)
        return;
    ret = list_remove_by_id(list, line);
    if (ret == 0) {
        printf("删除成功。\n");
        *dirty = 1;  // 标记存在未保存的修改
    } else if (ret == -1) {
        printf("未找到学号为 %s 的学生。\n", line);
    } else {
        printf("删除失败：参数错误。\n");
    }
}

// 函数：交互式修改学生信息。
static void ui_modify(StudentList *list, int *dirty) {
    char buf[INPUT_BUF_LEN];
    char *line;
    Student *stu;
    int choice;
    printf("—— 修改学生信息 ——\n");
    // 查找学生
    line = read_line("请输入要修改的学号：", buf, sizeof(buf));
    if (line == NULL)
        return;
    stu = list_find_by_id(list, line);
    if (stu == NULL) {
        printf("未找到该学生。\n");
        return;
    }
    printf("当前信息：\n");
    student_print_separator();
    student_print_header();
    student_print(stu);
    student_print_separator();
    // 修改信息
    printf("需要修改哪一项？\n");
    printf("1.姓名  2.年龄  3.成绩  0.取消\n");
    if (!read_int("请选择：", 0, 3, &choice) || choice == 0) {
        return;
    }
    switch (choice) {
        case 1:
            for (;;) {
                line = read_line("新姓名：", buf, sizeof(buf));
                if (line == NULL)
                    return;
                if (line[0] != '\0')
                    break;
                printf("姓名不能为空，请重新输入。\n");
            }
            copy_str(stu->name, sizeof(stu->name), line);
            break;
        case 2:
            if (!read_int("新年龄：", 1, 150, &stu->age))
                return;
            break;
        case 3:
            if (!read_double("新成绩：", 0.0, 100.0, &stu->score))
                return;
            break;
    }
    printf("修改成功。\n");
    *dirty = 1;  // 标记存在未保存的修改
}

// 函数：交互式按学号查询。
static void ui_find_by_id(StudentList *list) {
    char buf[INPUT_BUF_LEN];
    char *line;
    Student *stu;
    printf("—— 按学号查询 ——\n");
    line = read_line("请输入学号：", buf, sizeof(buf));
    if (line == NULL)
        return;
    if (line[0] == '\0') {
        printf("学号不能为空。\n");
        return;
    }
    stu = list_find_by_id(list, line);
    if (stu != NULL) {
        student_print_separator();
        student_print_header();
        student_print(stu);
        student_print_separator();
    } else {
        printf("未找到该学生。\n");
    }
}

// 函数：交互式按姓名查询。
static void ui_find_by_name(StudentList *list) {
    char buf[INPUT_BUF_LEN];
    char *line;
    printf("—— 按姓名查询 ——\n");
    line = read_line("请输入姓名(支持模糊匹配)：", buf, sizeof(buf));
    if (line == NULL)
        return;
    if (line[0] == '\0') {
        printf("姓名不能为空。\n");
        return;
    }
    size_t count = 0;
    Student **results = student_list_find_by_name(list, line, &count);
    if (results == NULL) {
        printf("查询失败。\n");
        return;
    }
    if (count > 0) {
        student_print_separator();
        student_print_header();
        for (size_t i = 0; i < count; i++)
            student_print(results[i]);
        student_print_separator();
        printf("共 %zu 位匹配的学生。\n", count);
    } else {
        printf("未找到匹配的学生。\n");
    }
    free(results);
}

// 函数：排序显示。
static void ui_sort_show(StudentList *list) {
    int choice;
    printf("—— 排序显示 ——\n");
    printf("排序方式：\n");
    printf("1.成绩从高到低\n");
    printf("2.成绩从低到高\n");
    printf("3.按学号排序\n");
    printf("0.取消\n");
    if (!read_int("请选择：", 0, 3, &choice) || choice == 0) {
        return;
    }
    if (choice == 3) {
        list_sort_by_id(list);
    } else {
        list_sort_by_score(list, choice == 1);
    }
    list_print(list);
}

// 函数：交互式写入文件。
static void ui_save(StudentList *list, int *dirty) {
    int ret = storage_save(DATA_FILE, list);
    if (ret == 0) {
        printf("已保存 %llu 条记录到 %s\n", (unsigned long long)list->size, DATA_FILE);
        *dirty = 0;
    } else {
        printf("保存失败：无法写入文件 %s，错误码 %d\n", DATA_FILE, ret);
    }
}

// 函数：交互式读取文件。
static void ui_load(StudentList *list, int *dirty) {
    int ret = storage_load(DATA_FILE, list);
    if (ret == 0) {
        printf("已从 %s 加载 %llu 条记录\n", DATA_FILE, (unsigned long long)list->size);
        *dirty = 0;
    } else if (ret == -3) {
        printf("加载失败：%s 不是有效的数据文件(格式不符)。\n", DATA_FILE);
    } else {
        printf("加载失败：无法读取文件 %s。\n", DATA_FILE);
    }
}

// 函数：打印主菜单。
static void print_menu(void) {
    printf("=========== 学生信息管理系统 ===========\n");
    printf("  1. 添加学生\n");
    printf("  2. 删除学生\n");
    printf("  3. 修改学生信息\n");
    printf("  4. 按学号查询\n");
    printf("  5. 按姓名查询\n");
    printf("  6. 显示全部学生\n");
    printf("  7. 排序显示\n");
    printf("  8. 保存到文件\n");
    printf("  9. 从文件加载\n");
    printf("  0. 退出\n");
    printf("========================================\n");
}

// 函数：按回车继续。
static void press_enter_to_continue(void) {
    char buf[INPUT_BUF_LEN];
    read_line("按回车键继续……", buf, sizeof(buf));
}

// 函数：交互界面主入口。
void ui_run(void) {
    StudentList list;
    int dirty = 0;
    int choice;
    int loaded;
    list_init(&list);
    // 启动时尝试自动加载上次的数据（文件不存在不算错误）
    loaded = (storage_load(DATA_FILE, &list) == 0);
    // 主循环
    for (;;) {
        clear_screen();
        print_menu();
        if (loaded) {  // 自动加载结果仅在首次显示菜单时提示
            printf("已自动加载 %s（%llu 条记录）\n", DATA_FILE, (unsigned long long)list.size);
            loaded = 0;
        }
        if (!read_int("请选择功能：", 0, 9, &choice)) {
            list_free(&list);
            break;
        }
        switch (choice) {
            case 1:
                ui_add(&list, &dirty);
                break;
            case 2:
                ui_remove(&list, &dirty);
                break;
            case 3:
                ui_modify(&list, &dirty);
                break;
            case 4:
                ui_find_by_id(&list);
                break;
            case 5:
                ui_find_by_name(&list);
                break;
            case 6:
                list_print(&list);
                break;
            case 7:
                ui_sort_show(&list);
                break;
            case 8:
                ui_save(&list, &dirty);
                break;
            case 9:
                ui_load(&list, &dirty);
                break;
            case 0:
                if (dirty) {
                    char buf[INPUT_BUF_LEN];
                    char *line =
                        read_line("存在未保存的修改，退出前是否保存？(y/n)：", buf, sizeof(buf));
                    if (line != NULL && (line[0] == 'y' || line[0] == 'Y')) {
                        ui_save(&list, &dirty);
                    }
                }
                list_free(&list);
                printf("再见！\n");
                return;
        }
        press_enter_to_continue();
    }
}
