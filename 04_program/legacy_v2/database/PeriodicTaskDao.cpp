/**
 * @file PeriodicTaskDao.cpp
 * @brief 周期任务数据访问层实现：periodic_task 表的 CRUD、按到期日查询
 *
 * 依赖：QtSql、models/PeriodicTask、utils/Logger
 * 文档：TD 2.4（periodic_task 表）、coding.md 第五章（数据库访问规范）、Phase 7
 *
 * 关键约束：
 *  - 全部 SQL 使用参数化查询（QSqlQuery::prepare + addBindValue），禁止字符串拼接
 *  - last_reminded_date 存储为 ISO 日期字符串（QDate::toString(Qt::ISODate)）
 *  - is_enabled 存储为 INTEGER 0/1
 *  - findByDueDate 在内存中调用 calcNextReminderDate 进行区间过滤
 */

#include "PeriodicTaskDao.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>

#include "utils/Logger.h"

// ============================================================================
// 构造
// ============================================================================

PeriodicTaskDao::PeriodicTaskDao(QSqlDatabase db)
    : db_(db)
{
}

// ============================================================================
// 单条查询
// ============================================================================

std::optional<PeriodicTask> PeriodicTaskDao::findById(int id)
{
    QSqlQuery query(db_);
    query.prepare("SELECT * FROM periodic_task WHERE id = ?");
    query.addBindValue(id);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("PeriodicTaskDao::findById 失败: %1").arg(query.lastError().text()));
        return std::nullopt;
    }
    if (query.next()) {
        return taskFromQuery(query);
    }
    return std::nullopt;
}

// ============================================================================
// 列表查询
// ============================================================================

QList<PeriodicTask> PeriodicTaskDao::findAll()
{
    QList<PeriodicTask> result;
    QSqlQuery query(db_);
    query.prepare("SELECT * FROM periodic_task ORDER BY id ASC");
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("PeriodicTaskDao::findAll 失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        result.append(taskFromQuery(query));
    }
    return result;
}

QList<PeriodicTask> PeriodicTaskDao::findAllEnabled()
{
    QList<PeriodicTask> result;
    QSqlQuery query(db_);
    query.prepare("SELECT * FROM periodic_task WHERE is_enabled = 1 ORDER BY id ASC");
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("PeriodicTaskDao::findAllEnabled 失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        result.append(taskFromQuery(query));
    }
    return result;
}

QList<PeriodicTask> PeriodicTaskDao::findByDueDate(const QDate &today, int daysAhead)
{
    QList<PeriodicTask> result;
    // 先取所有启用模板，再在内存中按 calcNextReminderDate() 过滤
    // 原因：下次提醒日期需通过 PeriodicTask::calcNextReminderDate 计算，
    //       SQLite 无法直接表达该逻辑
    QList<PeriodicTask> enabled = findAllEnabled();
    if (enabled.isEmpty()) {
        return result;
    }

    QDate endDate = today.addDays(daysAhead);
    for (const PeriodicTask &t : enabled) {
        QDate next = t.calcNextReminderDate();
        if (!next.isValid()) {
            continue;
        }
        if (next >= today && next <= endDate) {
            result.append(t);
        }
    }
    return result;
}

// ============================================================================
// 插入 / 更新 / 删除
// ============================================================================

int PeriodicTaskDao::insert(const PeriodicTask &task)
{
    QSqlQuery query(db_);
    query.prepare(
        "INSERT INTO periodic_task ("
        "  name, category, period_type, period_value, source, tag, handler, "
        "  reminder_time, last_reminded_date, is_enabled, create_time, target_day"
        ") VALUES ("
        "  ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?"
        ")");
    query.addBindValue(task.name);
    query.addBindValue(task.category);
    query.addBindValue(task.periodType);
    query.addBindValue(task.periodValue);
    query.addBindValue(task.source);
    query.addBindValue(task.tag);
    // handler NOT NULL DEFAULT '本人'，为空时回退为"本人"
    query.addBindValue(task.handler.isEmpty() ? QStringLiteral("本人") : task.handler);
    query.addBindValue(task.reminderTime);
    // last_reminded_date 存储为 ISO 日期字符串
    query.addBindValue(task.lastRemindedDate.isValid()
                           ? QVariant(task.lastRemindedDate.toString(Qt::ISODate))
                           : QVariant());
    query.addBindValue(task.isEnabled ? 1 : 0);
    // create_time 存储为完整时间字符串
    query.addBindValue(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
    query.addBindValue(task.targetDay);

    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("PeriodicTaskDao::insert 失败: %1").arg(query.lastError().text()));
        return -1;
    }
    QVariant lastId = query.lastInsertId();
    if (lastId.isValid()) {
        return lastId.toInt();
    }
    return -1;
}

bool PeriodicTaskDao::update(const PeriodicTask &task)
{
    QSqlQuery query(db_);
    query.prepare(
        "UPDATE periodic_task SET "
        "  name = ?, category = ?, period_type = ?, period_value = ?, source = ?, "
        "  tag = ?, handler = ?, reminder_time = ?, last_reminded_date = ?, "
        "  is_enabled = ?, target_day = ? "
        "WHERE id = ?");
    query.addBindValue(task.name);
    query.addBindValue(task.category);
    query.addBindValue(task.periodType);
    query.addBindValue(task.periodValue);
    query.addBindValue(task.source);
    query.addBindValue(task.tag);
    query.addBindValue(task.handler.isEmpty() ? QStringLiteral("本人") : task.handler);
    query.addBindValue(task.reminderTime);
    query.addBindValue(task.lastRemindedDate.isValid()
                           ? QVariant(task.lastRemindedDate.toString(Qt::ISODate))
                           : QVariant());
    query.addBindValue(task.isEnabled ? 1 : 0);
    query.addBindValue(task.targetDay);
    query.addBindValue(task.id);

    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("PeriodicTaskDao::update 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool PeriodicTaskDao::deleteById(int id)
{
    QSqlQuery query(db_);
    query.prepare("DELETE FROM periodic_task WHERE id = ?");
    query.addBindValue(id);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("PeriodicTaskDao::deleteById 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool PeriodicTaskDao::updateLastRemindedDate(int id, const QDate &date)
{
    QSqlQuery query(db_);
    query.prepare("UPDATE periodic_task SET last_reminded_date = ? WHERE id = ?");
    query.addBindValue(date.isValid() ? QVariant(date.toString(Qt::ISODate)) : QVariant());
    query.addBindValue(id);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("PeriodicTaskDao::updateLastRemindedDate 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool PeriodicTaskDao::setEnabled(int id, bool enabled)
{
    QSqlQuery query(db_);
    query.prepare("UPDATE periodic_task SET is_enabled = ? WHERE id = ?");
    query.addBindValue(enabled ? 1 : 0);
    query.addBindValue(id);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("PeriodicTaskDao::setEnabled 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

// ============================================================================
// 私有辅助方法
// ============================================================================

PeriodicTask PeriodicTaskDao::taskFromQuery(QSqlQuery &query)
{
    PeriodicTask task;
    task.id = query.value("id").toInt();
    task.name = query.value("name").toString();
    task.category = query.value("category").toString();
    task.periodType = query.value("period_type").toString();
    task.periodValue = query.value("period_value").toInt();
    task.source = query.value("source").toString();
    task.tag = query.value("tag").toString();
    task.handler = query.value("handler").toString();
    task.reminderTime = query.value("reminder_time").toString();
    // last_reminded_date 为 ISO 日期字符串
    QString lastDateStr = query.value("last_reminded_date").toString();
    if (!lastDateStr.isEmpty()) {
        task.lastRemindedDate = QDate::fromString(lastDateStr, Qt::ISODate);
    }
    // is_enabled 存储为 INTEGER 0/1
    task.isEnabled = query.value("is_enabled").toInt() != 0;
    // create_time 格式 yyyy-MM-dd HH:mm:ss
    QString createTimeStr = query.value("create_time").toString();
    if (!createTimeStr.isEmpty()) {
        task.createTime = QDateTime::fromString(createTimeStr, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    }
    // target_day：0=不指定，weekly:1-7，monthly:1-31，yearly:MMDD
    int targetDayIdx = query.record().indexOf("target_day");
    if (targetDayIdx >= 0) {
        task.targetDay = query.value(targetDayIdx).toInt();
    }
    return task;
}
