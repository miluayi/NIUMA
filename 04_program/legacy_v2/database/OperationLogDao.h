/**
 * @file OperationLogDao.h
 * @brief 操作日志数据访问层（无状态），提供 operation_log 表的插入、查询、清理及便捷记录方法
 *
 * 依赖：QtSql（QSqlDatabase/QSqlQuery）、models/OperationLog、utils/Logger、utils/DateUtil
 * 文档：TD 2.8（operation_log 表 schema）、TD 3.3（OperationLogDao 接口）、coding.md 第五章（数据库访问规范）、task.md T2.3
 *
 * 关键约束：
 *  - 全部 SQL 使用参数化查询（QSqlQuery::prepare + addBindValue），禁止字符串拼接
 *  - DAO 无状态，db_ 为构造时传入的连接，不持有业务状态
 *  - 时间格式：yyyy-MM-dd HH:mm:ss
 *  - task_id 为 0（全局操作）时存储 NULL，使用 QVariant() 绑定
 *  - 外键 ON DELETE SET NULL：任务被物理删除时日志保留，task_id 自动置 NULL
 *  - findByTaskId / findByTimeRange 均按 create_time DESC 排序
 */
#pragma once

#include <QSqlDatabase>
#include <QList>
#include <QDateTime>

#include "models/OperationLog.h"

class OperationLogDao {
public:
    explicit OperationLogDao(QSqlDatabase db);

    // 插入日志（返回新 ID，失败返回 -1）。task_id 为 0 时存储 NULL
    int insert(const OperationLog &log);

    // 按 task_id 查询（按 create_time DESC 排序）
    QList<OperationLog> findByTaskId(int taskId);

    // 按时间范围查询（按 create_time DESC 排序）
    QList<OperationLog> findByTimeRange(const QDateTime &start, const QDateTime &end);

    // 按时间范围删除（用于日志清理）
    bool deleteByTimeRange(const QDateTime &start, const QDateTime &end);

    // 便捷方法：构造 OperationLog 并插入，返回 insert 结果
    int logTaskAction(int taskId, const QString &action, const QString &description);

private:
    QSqlDatabase db_;

    // 辅助：从 QSqlQuery 读取一行构造 OperationLog
    OperationLog logFromQuery(QSqlQuery &query);
};
