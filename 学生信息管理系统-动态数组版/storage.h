// storage.h —— 文件读写

// 头文件保护
#ifndef STORAGE_H
#define STORAGE_H

// 项目头文件
#include "student.h"

// 函数：将容器中的全部记录保存到文件。
// 返回值：0 表示成功；-1 表示参数错误；-2 表示文件无法打开、写入或关闭失败。
int storage_save(const char *filename, const StudentList *list);

// 函数：从文件加载记录，成功后替换容器现有内容。
// 返回值：0 表示成功；-1 表示参数错误；-2 表示文件无法打开或内存不足；
//         -3 表示文件格式不符或数据读取不完整。
// 注意：加载失败时保留容器原有内容。
int storage_load(const char *filename, StudentList *list);

#endif  // STORAGE_H
