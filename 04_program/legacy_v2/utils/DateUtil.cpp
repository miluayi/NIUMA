/**
 * @file DateUtil.cpp
 * @brief 日期时间工具实现
 *
 * 依赖：utils/DateUtil.h
 * 文档：task.md T8.12、coding.md 6.2
 */
#include "DateUtil.h"

int DateUtil::daysBetween(const QDateTime &start, const QDateTime &end)
{
    if (!start.isValid() || !end.isValid()) {
        return 0;
    }
    qint64 secs = start.secsTo(end);
    // 向下取整：负数向下取整更符合业务（跨天即算 1 天差）
    if (secs >= 0) {
        return static_cast<int>(secs / 86400);
    } else {
        return static_cast<int>((secs - 86399) / 86400);
    }
}

int DateUtil::daysBetween(const QDate &start, const QDate &end)
{
    if (!start.isValid() || !end.isValid()) {
        return 0;
    }
    return start.daysTo(end);
}

QString DateUtil::currentDateStr()
{
    return QDate::currentDate().toString("yyyy-MM-dd");
}

QString DateUtil::currentDateTimeStr()
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
}

QDateTime DateUtil::parseDateTime(const QString &str)
{
    return QDateTime::fromString(str, "yyyy-MM-dd HH:mm:ss");
}

QDate DateUtil::parseDate(const QString &str)
{
    return QDate::fromString(str, "yyyy-MM-dd");
}

QString DateUtil::formatDateTime(const QDateTime &dt)
{
    if (!dt.isValid()) {
        return QString();
    }
    return dt.toString("yyyy-MM-dd HH:mm:ss");
}

int DateUtil::daysLeft(const QDate &deadline)
{
    if (!deadline.isValid()) {
        return 0;
    }
    return QDate::currentDate().daysTo(deadline);
}
