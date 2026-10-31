/**
 * @file Achievement.h
 * @brief 成果数据模型，对齐 achievement 表 schema
 *
 * 文档：TD 2.3（achievement 表）、3.8
 * 约束：result_amount 单位为分（qint64 整数），禁止 REAL/FLOAT
 */
#pragma once
#include <QString>
#include <QDateTime>

struct Achievement {
    int id = 0;
    int taskId = 0;
    QString workContent;          // 工作内容
    QString workObject;           // 工作对象
    int resultCount = 0;          // 成果数量
    qint64 resultAmount = 0;      // 成果金额（单位：分，禁止 float）
    QString description;          // 成果说明
    bool isAnnual = false;        // 纳入年度成果
    bool isKeyAchievement = false;// 重点成果
    QDateTime createTime;

    // 便捷方法（金额转换，实际实现在 AmountUtil）
    double amountInYuan() const { return resultAmount / 100.0; }
    double amountInWanYuan() const { return resultAmount / 1000000.0; }
};
