/**
 * @file AmountUtil.cpp
 * @brief 成果金额转换工具类实现：元/分/万元相互转换
 *
 * 依赖：Qt5 Core
 * 文档：TD 2.3（achievement 表 result_amount 字段）、coding.md 第五章
 * 约束：result_amount 单位为分（qint64 整数），禁止 REAL/FLOAT
 */

#include "AmountUtil.h"

#include <QtGlobal>

// ============================================================================
// 元 ↔ 分
// ============================================================================

qint64 AmountUtil::yuanToCents(const QString &yuan)
{
    // 空串或 null 返回 0
    if (yuan.isEmpty()) {
        return 0;
    }

    QString trimmed = yuan.trimmed();
    if (trimmed.isEmpty()) {
        return 0;
    }

    // 解析为 double，失败返回 0
    bool ok = false;
    double value = trimmed.toDouble(&ok);
    if (!ok) {
        return 0;
    }

    // 乘以 100 并四舍五入到整数分
    return qRound64(value * 100.0);
}

// ============================================================================
// 分 → 元
// ============================================================================

QString AmountUtil::centsToYuan(qint64 cents)
{
    // 使用整数运算避免浮点精度问题：
    // 整数部分 = cents / 100，小数部分 = |cents % 100|（2 位补零）
    qint64 yuanPart = cents / 100;
    qint64 fracPart = cents % 100;

    QString sign;
    if (cents < 0) {
        sign = QStringLiteral("-");
        // 负数取绝对值处理小数部分
        yuanPart = -yuanPart;
        fracPart = -fracPart;
    }

    return sign + QString::number(yuanPart) + QStringLiteral(".")
           + QString::number(fracPart).rightJustified(2, QLatin1Char('0'));
}

QString AmountUtil::centsToWanYuan(qint64 cents)
{
    // 万元 = 分 / 1000000（1 元 = 100 分，1 万元 = 10000 元 = 1000000 分）
    // 格式化为 2 位小数
    double wanYuan = cents / 1000000.0;
    return QString::number(wanYuan, 'f', 2);
}

double AmountUtil::centsToYuanDouble(qint64 cents)
{
    return cents / 100.0;
}
