/**
 * @file OperationLogDao.cpp
 * @brief 操作日志数据访问层实现：operation_log 表的插入、查询、清理及便捷记录
 *
 * 依赖：QtSql、models/OperationLog、utils/Logger、utils/DateUtil
 * 文档：TD 2.8（operation_log 表 schema）、TD 3.3（OperationLogDao 接口）、coding.md 第五章（数据库访问规范）、task.md T2.3
 *
 * 关键约束：
 *  - 全部 SQL 使用参数化查询（QSqlQuery::prepare + addBindValue），禁止字符串拼接
 *  - 时间格式：yyyy-MM-dd HH:mm:ss
 *  - task_id 为 0（全局操作）时存储 NULL，使用 QVariant() 绑定
 *  - findByTaskId / findByTimeRange 均按 create_time DESC 排序
 */

#include "OperationLogDao.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "utils/DateUtil.h"
#include "utils/Logger.h"

// ============================================================================
// 构造
// ============================================================================

OperationLogDao::OperationLogDao(QSqlDatabase db)
    : db_(db)
{
}

// ============================================================================
// 插入
// ============================================================================

int OperationLogDao::insert(const OperationLog &log)
{
    QString now = DateUtil::currentDateTimeStr();

    QSqlQuery query(db_);
    query.prepare(
        "INSERT INTO operation_log ("
        "  task_id, action, description, operator, create_time"
        ") VALUES (?, ?, ?, ?, ?)");
    // task_id 为 0（全局操作）时存储 NULL
    query.addBindValue(log.taskId != 0 ? QVariant(log.taskId) : QVariant());
    query.addBindValue(log.action);
    query.addBindValue(log.description);
    query.addBindValue(log.operator_);
    query.addBindValue(now);  // create_time

    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("OperationLogDao::insert 失败: %1").arg(query.lastError().text()));
        return -1;
    }
    QVariant lastId = query.lastInsertId();
    if (lastId.isValid()) {
        return lastId.toInt();
    }
    return -1;
}

// ============================================================================
// 查询
// ============================================================================

QList<OperationLog> OperationLogDao::findByTaskId(int taskId)
{
    QList<OperationLog> result;
    QSqlQuery query(db_);
    query.prepare("SELECT * FROM operation_log WHERE task_id = ? ORDER BY create_time DESC");
    query.addBindValue(taskId);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("OperationLogDao::findByTaskId 失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        result.append(logFromQuery(query));
    }
    return result;
}

QList<OperationLog> OperationLogDao::findByTimeRange(const QDateTime &start, const QDateTime &end)
{
    QList<OperationLog> result;
    QSqlQuery query(db_);
    query.prepare("SELECT * FROM operation_log WHERE create_time >= ? AND create_time <= ? "
                  "ORDER BY create_time DESC");
    query.addBindValue(DateUtil::formatDateTime(start));
    query.addBindValue(DateUtil::formatDateTime(end));
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("OperationLogDao::findByTimeRange 失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        result.append(logFromQuery(query));
    }
    return result;
}

// ============================================================================
// 删除
// ============================================================================

bool OperationLogDao::deleteByTimeRange(const QDateTime &start, const QDateTime &end)
{
    // 物理删除（用于日志清理）
    QSqlQuery query(db_);
    query.prepare("DELETE FROM operation_log WHERE create_time >= ? AND create_time <= ?");
    query.addBindValue(DateUtil::formatDateTime(start));
    query.addBindValue(DateUtil::formatDateTime(end));
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("OperationLogDao::deleteByTimeRange 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

// ============================================================================
// 便捷方法
// ============================================================================

int OperationLogDao::logTaskAction(int taskId, const QString &action, const QString &description)
{
    OperationLog log;
    log.taskId = taskId;
    log.action = action;
    log.description = description;
    // operator_ 默认 "本人"（见 OperationLog 模型默认值），createTime 由 insert 内部写入
    return insert(log);
}

// ============================================================================
// 私有辅助方法
// ============================================================================

OperationLog OperationLogDao::logFromQuery(QSqlQuery &query)
{
    OperationLog log;
    log.id = query.value("id").toInt();
    // task_id 可为 NULL（全局操作或任务被物理删除后置 NULL），NULL 转 0
    log.taskId = query.isNull("task_id") ? 0 : query.value("task_id").toInt();
    log.action = query.value("action").toString();
    log.description = query.value("description").toString();
    log.operator_ = query.value("operator").toString();
    // 时间字段格式 yyyy-MM-dd HH:mm:ss
    QString createTimeStr = query.value("create_time").toString();
    if (!createTimeStr.isEmpty()) {
        log.createTime = QDateTime::fromString(createTimeStr, "yyyy-MM-dd HH:mm:ss");
    }
    return log;
}
