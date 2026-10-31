/**
 * @file PeriodicTask.h
 * @brief 周期任务数据模型，对齐 periodic_task 表 schema
 *
 * 文档：TD 2.4（periodic_task 表）、3.8
 * 约束：handler NOT NULL DEFAULT '本人'
 */
#pragma once
#include <QString>
#include <QDateTime>
#include <QDate>

struct PeriodicTask {
    int id = 0;
    QString name;
    QString category;
    QString periodType;        // daily/weekly/monthly/yearly
    int periodValue = 1;
    QString source;
    QString tag;
    QString handler = "本人";   // NOT NULL DEFAULT '本人'
    QString reminderTime;      // HH:mm
    QDate lastRemindedDate;
    bool isEnabled = true;
    QDateTime createTime;
    int targetDay = 0;         // 执行日：0=不指定(按间隔)，weekly:1-7(周一~周日)，monthly:1-31，yearly:MMDD(如315=3月15日)

    // 计算下次提醒日期（Phase 7 实现，此处声明）
    QDate calcNextReminderDate() const;
};
