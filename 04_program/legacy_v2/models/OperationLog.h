/**
 * @file OperationLog.h
 * @brief 操作日志数据模型，对齐 operation_log 表 schema
 *
 * 文档：TD 2.8（operation_log 表）、3.6.1
 * 约束：task_id 可为 0（全局操作），operator 是 C++ 关键字用 operator_
 */
#pragma once
#include <QString>
#include <QDateTime>

struct OperationLog {
    int id = 0;
    int taskId = 0;           // 可为 0（全局操作）
    QString action;           // create/update/complete/cancel_complete/delete/restore/...
    QString description;
    QString operator_ = "本人"; // operator 是 C++ 关键字，用 operator_
    QDateTime createTime;
};
