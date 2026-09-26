// student.h —— 学生数据模型与链表容器

// 头文件保护
#ifndef STUDENT_H
#define STUDENT_H

// 标准库头文件
#include <stddef.h>

// 常量定义
#define STUDENTS_ID_LEN 10  // 学号缓冲区容量（字节，含字符串结束符）
#define STUDENTS_NAME_LEN 16  // 姓名缓冲区容量（字节，含字符串结束符）

// 学生信息结构体
typedef struct {
    char id[STUDENTS_ID_LEN];  // 学号
    char name[STUDENTS_NAME_LEN];  // 姓名
    int age;  // 年龄
    double score;  // 成绩
} Student;

// 学生链表节点
typedef struct StudentNode StudentNode;
struct StudentNode {
    Student data;  // 学生信息
    StudentNode *next;  // 下一个节点的地址
};

// 链表容器
typedef struct {
    StudentNode *head;  // 第一个节点的地址
    StudentNode *tail;  // 最后一个节点的地址
    size_t size;  // 当前学生数量
} StudentList;

// 容器生命周期

// 函数：初始化容器。
void list_init(StudentList *list);

// 函数：释放容器内存并重置容器。
void list_free(StudentList *list);

// 学生记录操作

// 函数：添加学生。
// 返回值：0 表示成功；-1 表示学号已存在；-2 表示内存不足；1 表示参数错误。
int list_add(StudentList *list, const Student *stu);

// 函数：根据学号删除学生。
// 返回值：0 表示成功；-1 表示未找到指定学号的学生；1 表示参数错误。
int list_remove_by_id(StudentList *list, const char *id);

// 函数：根据学号查找学生。
// 返回值：找到时返回容器内记录的地址；未找到或参数错误时返回 NULL。
// 注意：增删或排序后应重新查询，不宜长期保存返回的指针。
Student *list_find_by_id(const StudentList *list, const char *id);

// 函数：根据姓名查找学生（子串匹配）。
// 返回值：成功时返回指针数组，并将匹配数量写入 *out_count；无匹配时数量为 0。
//         参数错误或内存不足时返回 NULL。
// 注意：调用方负责 free 返回的数组；数组内的学生指针指向容器中的记录，不能单独释放。
//       增删或排序后应重新查询，不宜长期保存数组内的学生指针。
Student **student_list_find_by_name(const StudentList *list, const char *name, size_t *out_count);

// 排序

// 函数：按成绩排序。
// 参数：descending 非零表示降序，0 表示升序。
void list_sort_by_score(StudentList *list, int descending);

// 函数：按学号升序排序。
void list_sort_by_id(StudentList *list);

// 表格输出

// 函数：打印学生列表表头。
void student_print_header(void);

// 函数：打印学生列表分隔线。
void student_print_separator(void);

// 函数：打印单个学生信息。
void student_print(const Student *stu);

// 函数：打印学生列表。
void list_print(const StudentList *list);

#endif  // STUDENT_H
