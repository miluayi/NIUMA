/**
 * @file AmountUtil.h
 * @brief 成果金额转换工具类（静态方法集合）
 *
 * 提供元/分/万元之间的相互转换。成果金额在数据库中以分为单位存储（qint64 整数），
 * 禁止使用 float/REAL 以避免精度丢失。本工具类负责 UI 输入（元字符串）与存储（分）之间的转换。
 *
 * 依赖：Qt5 Core
 * 文档：TD 2.3（achievement 表 result_amount 字段）、coding.md 第五章
 * 约束：result_amount 单位为分（qint64 整数），禁止 REAL/FLOAT
 */
#pragma once

#include <QString>

/**
 * @brief 成果金额转换工具类
 *
 * 全部为静态方法，无需实例化。
 *
 * 单位说明：
 *  - 元（yuan）：用户输入的金额字符串，如 "120.50"
 *  - 分（cents）：数据库存储单位，qint64 整数，如 12050
 *  - 万元（wanYuan）：统计展示单位，如 "0.01"（12050 分 = 0.01 万元）
 */
class AmountUtil {
public:
    /**
     * @brief 元转分
     * @param yuan 元字符串，如 "120.50"
     * @return 分（qint64），如 12050；空串或解析失败返回 0
     *
     * 示例：
     *   "120.50" → 12050
     *   "120"    → 12000
     *   "120.5"  → 12050
     *   "120.505"→ 12051（四舍五入）
     *   ""       → 0
     */
    static qint64 yuanToCents(const QString &yuan);

    /**
     * @brief 分转元（字符串，2 位小数）
     * @param cents 分（qint64）
     * @return 元字符串，如 "120.50"
     *
     * 示例：
     *   12050 → "120.50"
     *   12000 → "120.00"
     *   0     → "0.00"
     */
    static QString centsToYuan(qint64 cents);

    /**
     * @brief 分转万元（字符串，2 位小数）
     * @param cents 分（qint64）
     * @return 万元字符串，如 "0.01"
     *
     * 示例：
     *   12050     → "0.01"（12050 / 1000000 = 0.012050，格式化为 0.01）
     *   100000000 → "100.00"（100 万元 = 1 亿元 = 100000000 分）
     */
    static QString centsToWanYuan(qint64 cents);

    /**
     * @brief 分转元（数字）
     * @param cents 分（qint64）
     * @return 元（double），如 120.50
     */
    static double centsToYuanDouble(qint64 cents);
};
