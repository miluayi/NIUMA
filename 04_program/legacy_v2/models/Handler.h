/**
 * @file Handler.h
 * @brief 办理人字典数据模型，对齐 handler_dict 表 schema
 *
 * 约束：逻辑删除（enabled=0），历史任务保留原记录
 */
#pragma once
#include <QString>
#include <QDateTime>

struct Handler {
    int id = 0;
    QString name;
    bool isEnabled = true;
    int sortOrder = 0;
    QDateTime createTime;
};
