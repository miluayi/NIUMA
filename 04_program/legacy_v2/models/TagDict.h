/**
 * @file TagDict.h
 * @brief 事项标签字典数据模型，对齐 tag_dict 表 schema
 *
 * 文档：TD 2.6（tag_dict 表）、3.8
 * 约束：逻辑删除（enabled=0），历史任务保留原记录
 */
#pragma once
#include <QString>
#include <QDateTime>

enum class TagScope { Task, Periodic, Reference };

struct TagDict {
    int id = 0;
    QString name;
    bool isEnabled = true;
    int sortOrder = 0;
    QDateTime createTime;
};
