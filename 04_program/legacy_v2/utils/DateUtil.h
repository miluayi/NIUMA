/**
 * @file DateUtil.h
 * @brief 日期时间工具类（静态方法集合）
 *
 * 提供天数差计算、日期格式化/解析、剩余天数计算等工具方法。
 *
 * 依赖：Qt5 Core
 * 文档：task.md T8.12、coding.md 6.2
 */
#pragma once

#include <QDate>
#include <QDateTime>
#include <QString>

/**
 * @brief 日期时间工具类
 *
 * 全部为静态方法，无需实例化。
 */
class DateUtil {
public:
    /**
     * @brief 计算两个时间点之间的天数差
     * @param start 起始时间
     * @param end 结束时间
     * @return 天数差（end - start，向下取整）
     */
    static int daysBetween(const QDateTime &start, const QDateTime &end);

    /**
     * @brief 计算两个日期之间的天数差
     * @param start 起始日期
     * @param end 结束日期
     * @return 天数差（end - start）
     */
    static int daysBetween(const QDate &start, const QDate &end);

    /**
     * @brief 获取当前日期字符串
     * @return yyyy-MM-dd 格式
     */
    static QString currentDateStr();

    /**
     * @brief 获取当前日期时间字符串
     * @return yyyy-MM-dd HH:mm:ss 格式
     */
    static QString currentDateTimeStr();

    /**
     * @brief 解析日期时间字符串
     * @param str 日期时间字符串（yyyy-MM-dd HH:mm:ss）
     * @return QDateTime，解析失败返回无效时间
     */
    static QDateTime parseDateTime(const QString &str);

    /**
     * @brief 解析日期字符串
     * @param str 日期字符串（yyyy-MM-dd）
     * @return QDate，解析失败返回无效日期
     */
    static QDate parseDate(const QString &str);

    /**
     * @brief 格式化日期时间
     * @param dt 日期时间
     * @return yyyy-MM-dd HH:mm:ss 格式字符串
     */
    static QString formatDateTime(const QDateTime &dt);

    /**
     * @brief 计算距今天剩余天数
     * @param deadline 截止日期
     * @return 剩余天数（负数表示已过期，0 表示今天到期）
     */
    static int daysLeft(const QDate &deadline);
};
