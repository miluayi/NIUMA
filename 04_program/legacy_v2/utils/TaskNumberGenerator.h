/**
 * @file TaskNumberGenerator.h
 * @brief 任务编号生成器（静态工具类），格式 YYYYMMDD-NNN
 *
 * 依赖：database/DatabaseManager、database/TaskDao
 * 文档：TD 5.1（编号规则）、coding.md 6.3（编号生成规则）、task.md T1.7
 *
 * 规则：
 *  - 格式 YYYYMMDD-NNN（如 20260603-001），每日从 001 开始
 *  - 编号不复用：删除任务后当日序列出现空缺属正常，新建任务按当日最大序列号+1续编
 *  - SQL 查询 MAX(seq) 必须包含 status='已删除' 行防止回绕
 *  - 任务和参考资料共用同一每日序列
 */
#pragma once

#include <QDate>
#include <QString>

class TaskNumberGenerator {
public:
    /**
     * @brief 生成下一个任务编号
     * @return YYYYMMDD-NNN 格式编号
     *
     * 规则：YYYYMMDD-NNN，每日从 001 开始，不复用空缺
     * SQL 查询 MAX(seq) 必须包含 status='已删除' 行防止回绕
     * 任务和参考资料共用同一每日序列
     */
    static QString nextTaskNo();

    /**
     * @brief 生成下一个参考资料编号（与任务共用序列）
     * @return YYYYMMDD-NNN 格式编号
     */
    static QString nextRefNo();

    /**
     * @brief 生成指定日期的下一个编号（测试用）
     * @param date 指定日期
     * @return YYYYMMDD-NNN 格式编号
     */
    static QString nextTaskNoForDate(const QDate &date);

    /**
     * @brief 格式化编号
     * @param date 日期
     * @param seq 序列号（1-999）
     * @return YYYYMMDD-NNN 格式编号（3 位补零）
     */
    static QString format(const QDate &date, int seq);
};
