// storage.c —— 文件读写

// 项目头文件
#include "storage.h"

// 标准库头文件
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// 文件格式常量（u 后缀表示无符号整数常量）
#define FILE_MAGIC "STU1"  // 文件标识（魔数）
#define FILE_VERSION 1u  // 文件格式版本号
#define MAX_RECORDS 1000000u  // 记录数上限

// 文件头结构体
typedef struct {
    char magic[4];  // 文件标识（魔数）
    uint32_t version;  // 文件格式版本号
    uint32_t count;  // 记录数
} FileHeader;

// 函数：将容器中的全部记录保存到文件。
// 返回值：0 表示成功；-1 表示参数错误；-2 表示文件无法打开、写入或关闭失败。
int storage_save(const char *filename, const StudentList *list) {
    FILE *fp;
    FileHeader header;
    StudentNode *cur;
    if (filename == NULL || list == NULL) {
        return -1;
    }
    fp = fopen(filename, "wb");
    if (fp == NULL) {
        return -2;
    }
    memcpy(header.magic, FILE_MAGIC, 4);
    header.version = FILE_VERSION;
    header.count = (uint32_t)list->size;
    // 写入文件头
    if (fwrite(&header, sizeof(header), 1, fp) != 1) {
        fclose(fp);
        return -2;
    }
    // 写入学生数据
    for (cur = list->head; cur != NULL; cur = cur->next) {
        if (fwrite(&cur->data, sizeof(Student), 1, fp) != 1) {
            fclose(fp);
            return -2;
        }
    }
    if (fclose(fp) != 0) {
        return -2;
    }
    return 0;
}

// 函数：从文件加载记录，成功后替换容器现有内容。
// 返回值：0 表示成功；-1 表示参数错误；-2 表示文件无法打开或内存不足；
//         -3 表示文件格式不符或数据读取不完整。
// 注意：加载失败时保留容器原有内容。
int storage_load(const char *filename, StudentList *list) {
    FILE *fp;
    FileHeader header;
    StudentList tmp;
    if (filename == NULL || list == NULL) {
        return -1;
    }
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        return -2;
    }
    // 检查文件头
    if (fread(&header, sizeof(header), 1, fp) != 1 ||  // 读取文件头
        memcmp(header.magic, FILE_MAGIC, 4) != 0 ||  // 检查文件标识（魔数）
        header.version != FILE_VERSION ||  // 检查文件格式版本号
        header.count > MAX_RECORDS) {  // 检查记录数上限
        fclose(fp);
        return -3;
    }
    // 先将学生数据读入临时容器，全部成功后再替换目标容器
    // 这样可在加载失败时保留原有数据
    list_init(&tmp);
    for (size_t i = 0; i < header.count; i++) {
        StudentNode *node = malloc(sizeof(StudentNode));
        if (node == NULL) {
            list_free(&tmp);
            fclose(fp);
            return -2;
        }
        if (fread(&node->data, sizeof(Student), 1, fp) != 1) {
            free(node);
            list_free(&tmp);
            fclose(fp);
            return -3;
        }
        node->next = NULL;
        if (tmp.tail == NULL) {
            tmp.head = node;
        } else {
            tmp.tail->next = node;
        }
        tmp.tail = node;
        tmp.size++;
    }
    fclose(fp);
    list_free(list);
    *list = tmp;
    return 0;
}
