/**
 * @file Task.h
 * @brief 任务数据模型，字段严格对齐 task 表 schema
 *
 * 文档：TD 2.2（task 表）、3.8（核心数据模型）
 * 约束：status 使用直接字符串值，handler NOT NULL DEFAULT '本人'，
 *       不包含废弃字段 is_urgent/plan_start_date/sort_order
 */
#pragma once
#include <QString>
#include <QDateTime>
#include <QDate>
#include <QMetaType>
#include <optional>

struct Task {
    int id = 0;
    QString taskNo;              // task_no YYYYMMDD-NNN
    QString title;               // 任务名称
    QString category;            // 分类 催/查/报/办
    QString source;              // 来源
    QString tag;                 // 事项标签
    QString handler = "本人";    // 办理人，默认"本人"
    QString status = "进行中";   // 状态：进行中/已完成/已删除/他人办理
    QString originalStatus;      // 删除时保存的原状态
    QString folderName;          // 编号_[分类]_任务名称
    QString folderPath;          // 完整目录路径
    bool isKeyTask = false;      // 重点任务
    bool hasSubdirs = false;     // 是否有子目录
    bool isTodayFocus = false;   // 今日专注
    int focusOrder = 0;          // 专注排序
    QDate deadline;              // 截止日期
    QString description;         // 任务说明
    bool isFromPeriodic = false; // 是否来自周期任务
    int processingDays = 0;      // 办理天数
    QDateTime createTime;        // 创建时间
    QDateTime finishTime;        // 完成时间
    QDateTime deleteTime;        // 删除时间
    QDateTime updateTime;        // 更新时间

    // 便捷方法
    bool isActive() const { return status == "进行中"; }
    bool isCompleted() const { return status == "已完成"; }
    bool isDeleted() const { return status == "已删除"; }
    bool isHandledByOther() const { return status == "他人办理"; }
    bool isOverdue() const;
    int daysLeft() const;        // 距截止日期剩余天数
};

Q_DECLARE_METATYPE(Task)
