/**
 * @file AchievementDao.h
 * @brief 成果数据访问层（无状态），提供 achievement 表的 CRUD、按年份查询、统计等方法
 *
 * 依赖：QtSql（QSqlDatabase/QSqlQuery）、models/Achievement、utils/Logger
 * 文档：TD 2.3（achievement 表）、TD 3.x（AchievementDao 接口）、coding.md 第五章（数据库访问规范）
 *
 * 关键约束：
 *  - 全部 SQL 使用参数化查询（QSqlQuery::prepare + addBindValue），禁止字符串拼接
 *  - DAO 无状态，db_ 为构造时传入的连接，不持有业务状态
 *  - is_annual 和 is_key_achievement 存储为 INTEGER（0/1），读取时转为 bool
 *  - result_amount 为 qint64（分），禁止 float/REAL
 *  - create_time 存储为 ISO 格式字符串（QDateTime::toString(Qt::ISODate)）
 *  - 一个 task 对应一条 achievement（findByTaskId 返回单条）
 *  - categoryCountByYear / sourceCountByYear 需 JOIN task 表获取 category/source 字段
 */
#pragma once

#include <QSqlDatabase>
#include <QList>
#include <QPair>
#include <optional>

#include "models/Achievement.h"

class AchievementDao {
public:
    explicit AchievementDao(QSqlDatabase db);

    // 按 ID 查询
    std::optional<Achievement> findById(int id);

    // 按 task_id 查询（一个 task 对应一条 achievement）
    std::optional<Achievement> findByTaskId(int taskId);

    // 按年份查询（create_time 年份匹配）
    QList<Achievement> findByYear(int year);

    // 查询全部
    QList<Achievement> findAll();

    // 按年份统计数量
    int countByYear(int year);

    // 插入成果（返回新 ID，失败返回 -1）
    int insert(const Achievement &achievement);

    // 更新成果（更新除 id、create_time 外的所有字段）
    bool update(const Achievement &achievement);

    // 按 task_id 删除（物理删除，外键 CASCADE 也会随 task 删除自动清理）
    bool deleteByTaskId(int taskId);

    // 按 ID 删除（物理删除）
    bool deleteById(int id);

    // ============================================================================
    // 统计
    // ============================================================================

    // 按年份统计金额总和（返回分）
    qint64 sumAmountByYear(int year);

    // 按年份统计重点成果数量
    int countKeyAchievementByYear(int year);

    // 分类统计：JOIN task 表，返回 (category, count) 列表
    QList<QPair<QString, int>> categoryCountByYear(int year);

    // 来源统计：JOIN task 表，返回 (source, count) 列表
    QList<QPair<QString, int>> sourceCountByYear(int year);

private:
    QSqlDatabase db_;

    // 辅助：从 QSqlQuery 读取一行构造 Achievement
    Achievement achievementFromQuery(QSqlQuery &query);
};
