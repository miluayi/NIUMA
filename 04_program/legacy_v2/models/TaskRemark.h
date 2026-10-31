/**
 * @file TaskRemark.h
 * @brief 任务备注数据模型，对齐 task_remark 表 schema
 *
 * 文档：TD 2.10（task_remark 表）、3.2
 * 约束：外键 ON DELETE CASCADE，按 create_time ASC 排序
 */
#pragma once
#include <QString>
#include <QDateTime>

struct TaskRemark {
    int id = 0;
    int taskId = 0;
    QString content;
    QDateTime createTime;
};
