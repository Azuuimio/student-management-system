// student.c —— 学生数据模型与动态数组容器

// 项目头文件
#include "student.h"

// 标准库头文件
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// 常量定义
#define INITIAL_CAPACITY 8  // 初始容量
#define COL_ID 12  // 学号列宽
#define COL_NAME 12  // 姓名列宽
#define COL_AGE 12  // 年龄列宽
#define SEPARATOR_WIDTH 41  // 分隔线宽度

// 内部函数声明
static int list_grow(StudentList *list);
static int compare_score_desc(const void *a, const void *b);
static int compare_score_asc(const void *a, const void *b);
static int compare_id_asc(const void *a, const void *b);
static int display_width(const char *s);
static void print_padded(const char *s, int width);

// 函数：初始化容器。
void list_init(StudentList *list) {
    // 检查 list 是否为 NULL，避免解引用空指针
    if (list == NULL) {
        return;
    }
    list->data = NULL;
    list->size = 0;
    list->capacity = 0;
}

// 函数：释放容器内存并重置容器。
void list_free(StudentList *list) {
    // 检查 list 是否为 NULL，避免解引用空指针
    if (list == NULL) {
        return;
    }
    free(list->data);  // 释放后将指针置为 NULL，避免保留悬空指针
    list->data = NULL;
    list->size = 0;
    list->capacity = 0;
}

// 函数：在容量不足时扩容。
// 返回值：0 表示成功或无需扩容；-1 表示内存分配失败。
static int list_grow(StudentList *list) {
    size_t new_capacity;
    Student *new_data;
    if (list->size < list->capacity) {
        return 0;  // 不需要扩容
    }
    new_capacity = (list->capacity == 0) ? INITIAL_CAPACITY : list->capacity * 2;
    // 先用临时变量保存 realloc 的返回值，避免扩容失败时丢失原内存地址
    new_data = realloc(list->data, new_capacity * sizeof(Student));
    if (new_data == NULL) {
        return -1;  // 扩容失败
    }
    list->data = new_data;
    list->capacity = new_capacity;
    return 0;  // 扩容成功
}

// 函数：根据学号查找学生。
// 返回值：找到时返回容器内记录的地址；未找到或参数错误时返回 NULL。
// 注意：增删或排序后应重新查询，不宜长期保存返回的指针。
Student *list_find_by_id(const StudentList *list, const char *id) {
    if (list == NULL || id == NULL) {
        return NULL;  // 参数错误
    }
    size_t i;
    for (i = 0; i < list->size; i++) {
        if (strcmp(list->data[i].id, id) == 0) {
            return &list->data[i];  // 找到，返回记录地址
        }
    }
    return NULL;  // 找不到
}

// 函数：根据姓名查找学生（子串匹配）。
// 返回值：成功时返回指针数组，并将匹配数量写入 *out_count；无匹配时数量为 0。
//         参数错误或内存不足时返回 NULL。
// 注意：调用方负责 free 返回的数组；数组内的学生指针指向容器中的记录，不能单独释放。
//       增删或排序后应重新查询，不宜长期保存数组内的学生指针。
Student **student_list_find_by_name(const StudentList *list, const char *name, size_t *out_count) {
    if (list == NULL || name == NULL || name[0] == '\0' || out_count == NULL) {
        return NULL;  // 参数错误
    }
    *out_count = 0;
    size_t cap = (list->size > 0) ? list->size : 1;  // 避免调用 malloc(0)
    Student **results = malloc(cap * sizeof(*results));
    if (results == NULL) {
        return NULL;  // 内存不足
    }
    size_t count = 0;
    for (size_t i = 0; i < list->size; i++) {
        if (strstr(list->data[i].name, name) != NULL) {
            results[count] = &list->data[i];
            count++;
        }
    }
    *out_count = count;
    return results;
}

// 函数：添加学生。
// 返回值：0 表示成功；-1 表示学号已存在；-2 表示内存不足；1 表示参数错误。
int list_add(StudentList *list, const Student *stu) {
    if (list == NULL || stu == NULL) {
        return 1;  // 参数错误
    }
    if (list_find_by_id(list, stu->id) != NULL) {
        return -1;  // 学号已存在
    }
    if (list_grow(list) == -1) {
        return -2;  // 内存不足
    }
    list->data[list->size] = *stu;
    list->size++;
    return 0;  // 成功
}

// 函数：根据学号删除学生。
// 返回值：0 表示成功；-1 表示未找到指定学号的学生；1 表示参数错误。
int list_remove_by_id(StudentList *list, const char *id) {
    if (list == NULL || id == NULL) {
        return 1;  // 参数错误
    }
    for (size_t i = 0; i < list->size; i++) {
        if (strcmp(list->data[i].id, id) == 0) {
            if (i + 1 < list->size) {
                // 使用 memmove 将被删元素之后的所有元素整体前移一位
                memmove(&list->data[i], &list->data[i + 1], (list->size - i - 1) * sizeof(Student));
            }
            list->size--;
            return 0;  // 删除成功
        }
    }
    return -1;  // 未找到指定学号的学生
}

/*
 * 关于 qsort 比较函数的返回值规则：
 * 返回负数：a 排在 b 前。
 * 返回零：a 与 b 在当前比较规则下相等。
 * 返回正数：a 排在 b 后。
 */
// 函数：按成绩降序排序的比较函数（传入 qsort）。
static int compare_score_desc(const void *a, const void *b) {
    // void * 可保存指向对象的指针，但不能直接解引用、访问成员或进行标准 C 指针运算
    // 因此，先转换为指向实际对象类型的指针
    const Student *sa = (const Student *)a;
    const Student *sb = (const Student *)b;
    // 浮点数比较直接使用 < 和 >，并按三种结果返回整数
    // 避免将浮点差值转换为 int 时丢失小数部分，误将不同成绩判为相等
    if (sa->score < sb->score)
        return 1;
    if (sa->score > sb->score)
        return -1;
    return 0;
}

// 函数：按成绩升序排序的比较函数（传入 qsort）。
static int compare_score_asc(const void *a, const void *b) {
    return compare_score_desc(b, a);  // 交换比较参数，将降序比较转换为升序比较
}

// 函数：按成绩排序。
// 参数：descending 非零表示降序，0 表示升序。
void list_sort_by_score(StudentList *list, int descending) {
    if (list == NULL || list->size < 2) {
        return;  // 参数错误或无需排序
    }
    if (descending) {
        qsort(list->data, list->size, sizeof(Student), compare_score_desc);
    } else {
        qsort(list->data, list->size, sizeof(Student), compare_score_asc);
    }
}

// 函数：按学号排序的比较函数（传入 qsort）。
static int compare_id_asc(const void *a, const void *b) {
    const Student *sa = (const Student *)a;
    const Student *sb = (const Student *)b;
    // strcmp 的返回值符合 qsort 比较函数的约定，可直接返回
    return strcmp(sa->id, sb->id);
}

// 函数：按学号升序排序。
void list_sort_by_id(StudentList *list) {
    if (list == NULL || list->size < 2) {
        return;  // 参数错误或无需排序
    }
    qsort(list->data, list->size, sizeof(Student), compare_id_asc);
}

/*
 * 关于中文显示宽度：
 * 打印表格时常用 printf("%-16s%-24s", id, name)，这在纯英文下工作良好，但姓名
 * 包含中文时可能错位。原因在于显示宽度和字节数不同：printf 的 %-24s 按字节数
 * 补齐，不足 24 字节就补空格；常用汉字在 UTF-8 中占 3 字节，在控制台通常占 2 列。
 * 解决办法是自行计算显示宽度并补空格，代替 %-Ns 的自动填充。本程序简化处理如下：
 *     ASCII 字符（< 0x80）：占 1 列。
 *     非 ASCII 的 UTF-8 首字节（非 10xx xxxx）：视为全角字符，占 2 列。
 *     UTF-8 后续字节（10xx xxxx）：不另占列，属于前一个字符。
 * 此规则适用于本项目常见的中英文输入，并未覆盖所有 Unicode 字符的显示宽度。
 */
// 函数：计算字符串在终端显示的列宽。
// 返回值：字符串在终端显示的列宽。
static int display_width(const char *s) {
    int width = 0;
    while (*s != '\0') {
        // 以无符号方式安全地读取当前字节存入 c，同时将指针推进到下一个字节
        // 使用 unsigned char，避免 char 为有符号类型时将高位为 1 的字节解释为负数
        unsigned char c = (unsigned char)*s++;
        if (c < 0x80) {
            width += 1;  // ASCII 字符占 1 列
        } else if ((c & 0xC0) != 0x80) {
            // 通过按位与将 c 的低 6 位清零，保留最高 2 位，以区分 UTF-8 首字节和后续字节
            width += 2;  // 非 ASCII 的 UTF-8 首字节按 2 列计
        }
        // UTF-8 后续字节不另占列
    }
    return width;
}

// 函数：打印指定字符串并补齐空格。
static void print_padded(const char *s, int width) {
    if (s == NULL || width <= 0) {
        return;
    }
    int padding = width - display_width(s);
    printf("%s", s);
    for (int i = 0; i < padding; i++) {
        putchar(' ');
    }
}

// 函数：打印学生列表表头。
void student_print_header(void) {
    print_padded("学号", COL_ID);
    print_padded("姓名", COL_NAME);
    print_padded("年龄", COL_AGE);
    printf("成绩\n");
}

// 函数：打印学生列表分隔线。
void student_print_separator(void) {
    for (int i = 0; i < SEPARATOR_WIDTH; i++) {
        putchar('-');
    }
    putchar('\n');
}

// 函数：打印单个学生信息。
void student_print(const Student *stu) {
    if (stu == NULL) {
        return;
    }
    // age 为 int 类型，先转换为字符串再按列宽输出
    char age_str[32];
    snprintf(age_str, sizeof(age_str), "%d", stu->age);
    print_padded(stu->id, COL_ID);
    print_padded(stu->name, COL_NAME);
    print_padded(age_str, COL_AGE);
    printf("%.1f\n", stu->score);
}

// 函数：打印学生列表。
void list_print(const StudentList *list) {
    if (list == NULL || list->size == 0) {
        printf("暂无学生记录\n");
        return;
    }
    student_print_separator();
    student_print_header();
    for (size_t i = 0; i < list->size; i++) {
        student_print(&list->data[i]);
    }
    student_print_separator();
    printf("共 %llu 条记录\n", (unsigned long long)list->size);
}
