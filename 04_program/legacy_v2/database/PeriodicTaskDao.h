/**
 * @file PeriodicTaskDao.h
 * @brief 周期任务数据访问层（无状态），提供 periodic_task 表的 CRUD、按到期日查询等方法
 *
 * 依赖：QtSql（QSqlDatabase/QSqlQuery）、models/PeriodicTask、utils/Logger
 * 文档：TD 2.4（periodic_task 表）、coding.md 第五章（数据库访问规范）、Phase 7
 *
 * 关键约束：
 *  - 全部 SQL 使用参数化查询（QSqlQuery::prepare + addBindValue），禁止字符串拼接
 *  - DAO 无状态，db_ 为构造时传入的连接，不持有业务状态
 *  - handler 字段 TEXT NOT NULL DEFAULT '本人'
 *  - period_type 取值：'daily' / 'weekly' / 'monthly' / 'yearly'
 *  - last_reminded_date 存储为 ISO 日期字符串（QDate::toString(Qt::ISODate)）
 *  - is_enabled 存储为 INTEGER 0/1
 *  - findByDueDate：返回 enabled 且 calcNextReminderDate() 落在 [today, today+daysAhead] 区间的模板
 */
#pragma once

#include <QSqlDatabase>
#include <QList>
#include <QDate>
#include <optional>

#include "models/PeriodicTask.h"

class PeriodicTaskDao {
public:
    explicit PeriodicTaskDao(QSqlDatabase db);

    // 按 ID 查询
    std::optional<PeriodicTask> findById(int id);

    // 查询全部模板（按 id 正序）
    QList<PeriodicTask> findAll();

    // 查询所有启用的模板
    QList<PeriodicTask> findAllEnabled();

    // 查询启用模板中下次提醒日期落在 [today, today+daysAhead] 区间的记录
    QList<PeriodicTask> findByDueDate(const QDate &today, int daysAhead = 7);

    // 插入模板（返回新 ID，失败返回 -1）
    int insert(const PeriodicTask &task);

    // 更新模板（更新除 id、create_time 外的所有字段）
    bool update(const PeriodicTask &task);

    // 按 ID 删除（物理删除）
    bool deleteById(int id);

    // 更新上次提醒日期
    bool updateLastRemindedDate(int id, const QDate &date);

    // 启用/禁用模板
    bool setEnabled(int id, bool enabled);

private:
    QSqlDatabase db_;

    // 辅助：从 QSqlQuery 读取一行构造 PeriodicTask
    PeriodicTask taskFromQuery(QSqlQuery &query);
};
