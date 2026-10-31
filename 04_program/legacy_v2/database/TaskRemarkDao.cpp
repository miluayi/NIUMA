/**
 * @file TaskRemarkDao.cpp
 * @brief 任务备注数据访问层实现：task_remark 表的 CRUD
 *
 * 依赖：QtSql、models/TaskRemark、utils/Logger、utils/DateUtil
 * 文档：TD 2.10（task_remark 表 schema）、TD 3.2（TaskRemarkDao 接口）、coding.md 第五章（数据库访问规范）、task.md T2.2
 *
 * 关键约束：
 *  - 全部 SQL 使用参数化查询（QSqlQuery::prepare + addBindValue），禁止字符串拼接
 *  - 时间格式：yyyy-MM-dd HH:mm:ss
 *  - findByTaskId 按 create_time DESC 排序
 */

#include "TaskRemarkDao.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "utils/DateUtil.h"
#include "utils/Logger.h"

// ============================================================================
// 构造
// ============================================================================

TaskRemarkDao::TaskRemarkDao(QSqlDatabase db)
    : db_(db)
{
}

// ============================================================================
// 查询
// ============================================================================

QList<TaskRemark> TaskRemarkDao::findByTaskId(int taskId)
{
    QList<TaskRemark> result;
    QSqlQuery query(db_);
    query.prepare("SELECT * FROM task_remark WHERE task_id = ? ORDER BY create_time DESC");
    query.addBindValue(taskId);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TaskRemarkDao::findByTaskId 失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        result.append(remarkFromQuery(query));
    }
    return result;
}

// ============================================================================
// 插入 / 更新 / 删除
// ============================================================================

int TaskRemarkDao::insert(const TaskRemark &remark)
{
    QString now = DateUtil::currentDateTimeStr();

    QSqlQuery query(db_);
    query.prepare("INSERT INTO task_remark (task_id, content, create_time) VALUES (?, ?, ?)");
    query.addBindValue(remark.taskId);
    query.addBindValue(remark.content);
    query.addBindValue(now);  // create_time

    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TaskRemarkDao::insert 失败: %1").arg(query.lastError().text()));
        return -1;
    }
    QVariant lastId = query.lastInsertId();
    if (lastId.isValid()) {
        return lastId.toInt();
    }
    return -1;
}

bool TaskRemarkDao::update(const TaskRemark &remark)
{
    QSqlQuery query(db_);
    query.prepare("UPDATE task_remark SET content = ? WHERE id = ?");
    query.addBindValue(remark.content);
    query.addBindValue(remark.id);

    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TaskRemarkDao::update 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool TaskRemarkDao::deleteById(int id)
{
    // 物理删除
    QSqlQuery query(db_);
    query.prepare("DELETE FROM task_remark WHERE id = ?");
    query.addBindValue(id);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TaskRemarkDao::deleteById 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool TaskRemarkDao::deleteByTaskId(int taskId)
{
    // 物理删除（外键 ON DELETE CASCADE 会随 task 删除自动清理，此方法提供显式删除入口）
    QSqlQuery query(db_);
    query.prepare("DELETE FROM task_remark WHERE task_id = ?");
    query.addBindValue(taskId);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TaskRemarkDao::deleteByTaskId 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

// ============================================================================
// 私有辅助方法
// ============================================================================

TaskRemark TaskRemarkDao::remarkFromQuery(QSqlQuery &query)
{
    TaskRemark remark;
    remark.id = query.value("id").toInt();
    remark.taskId = query.value("task_id").toInt();
    remark.content = query.value("content").toString();
    // 时间字段格式 yyyy-MM-dd HH:mm:ss
    QString createTimeStr = query.value("create_time").toString();
    if (!createTimeStr.isEmpty()) {
        remark.createTime = QDateTime::fromString(createTimeStr, "yyyy-MM-dd HH:mm:ss");
    }
    return remark;
}
