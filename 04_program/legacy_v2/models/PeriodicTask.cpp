/**
 * @file PeriodicTask.cpp
 * @brief PeriodicTask 结构体方法实现：周期提醒日期计算
 *
 * 文档：TD 2.4、PRD 9.3
 *
 * 规则：
 *  - targetDay=0（不指定执行日）：按间隔计算
 *    - daily：lastRemindedDate + periodValue 天
 *    - weekly：lastRemindedDate + periodValue * 7 天
 *    - monthly：lastRemindedDate 加 periodValue 个月（处理月末边界，如 1/31 → 2/28）
 *    - yearly：lastRemindedDate 加 periodValue 年
 *  - targetDay>0（指定执行日）：按具体日期计算
 *    - weekly(1-7)：找到下一个目标星期几，再加 (periodValue-1)*7 天
 *    - monthly(1-31)：目标月份的第 targetDay 天（月末自动截断）
 *    - yearly(MMDD)：目标年份的 M月D日（闰年/月末自动截断）
 *  - lastRemindedDate 为空/无效时返回今天（即到期需提醒）
 */
#include "PeriodicTask.h"

#include <QDate>
#include <QtGlobal>

QDate PeriodicTask::calcNextReminderDate() const
{
    // lastRemindedDate 为空或无效：返回今天（即视为到期）
    if (!lastRemindedDate.isValid()) {
        return QDate::currentDate();
    }

    if (periodType == QStringLiteral("daily")) {
        return lastRemindedDate.addDays(periodValue);
    }

    if (periodType == QStringLiteral("weekly")) {
        if (targetDay >= 1 && targetDay <= 7) {
            // 指定星期几：找到下一个目标日
            int daysUntilTarget = (targetDay - lastRemindedDate.dayOfWeek() + 7) % 7;
            if (daysUntilTarget == 0) {
                daysUntilTarget = 7;  // 已在目标日，下一个是下周
            }
            return lastRemindedDate.addDays(daysUntilTarget + (periodValue - 1) * 7);
        }
        // 未指定星期几：按间隔
        return lastRemindedDate.addDays(periodValue * 7);
    }

    if (periodType == QStringLiteral("monthly")) {
        QDate baseDate = lastRemindedDate.addMonths(periodValue);
        if (targetDay >= 1 && targetDay <= 31) {
            // 指定日期：目标月份的第 targetDay 天（月末自动截断）
            int day = qMin(targetDay, baseDate.daysInMonth());
            return QDate(baseDate.year(), baseDate.month(), day);
        }
        // 未指定日期：按间隔（QDate::addMonths 已自动处理月末边界）
        return baseDate;
    }

    if (periodType == QStringLiteral("yearly")) {
        if (targetDay > 0) {
            // 指定日期：targetDay 编码为 MMDD（如 315 = 3月15日）
            int month = targetDay / 100;
            int day = targetDay % 100;
            if (month >= 1 && month <= 12 && day >= 1 && day <= 31) {
                QDate baseDate = lastRemindedDate.addYears(periodValue);
                QDate temp(baseDate.year(), month, 1);
                day = qMin(day, temp.daysInMonth());
                return QDate(baseDate.year(), month, day);
            }
        }
        // 未指定日期：按间隔（QDate::addYears 已自动处理闰年 2/29 边界）
        return lastRemindedDate.addYears(periodValue);
    }

    // 未知 periodType：返回今天作为安全回退
    return QDate::currentDate();
}
