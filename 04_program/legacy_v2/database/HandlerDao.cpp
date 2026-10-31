/**
 * @file HandlerDao.cpp
 * @brief 办理人字典数据访问层实现：handler_dict 表的 CRUD 与逻辑删除
 */

#include "HandlerDao.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "utils/DateUtil.h"
#include "utils/Logger.h"

// ============================================================================
// 构造
// ============================================================================

HandlerDao::HandlerDao(QSqlDatabase db)
    : db_(db)
{
}

// ============================================================================
// 列表查询
// ============================================================================

QList<Handler> HandlerDao::findAll()
{
    QList<Handler> result;
    QSqlQuery query(db_);
    query.prepare("SELECT id, name, is_enabled, sort_order, create_time "
                  "FROM handler_dict ORDER BY id ASC");
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("HandlerDao::findAll 失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        result.append(handlerFromQuery(query));
    }
    return result;
}

QList<Handler> HandlerDao::findEnabled()
{
    QList<Handler> result;
    QSqlQuery query(db_);
    query.prepare("SELECT id, name, is_enabled, sort_order, create_time "
                  "FROM handler_dict WHERE is_enabled = 1 ORDER BY sort_order ASC, id ASC");
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("HandlerDao::findEnabled 失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        result.append(handlerFromQuery(query));
    }
    return result;
}

QList<Handler> HandlerDao::findEnabledByRecentUse()
{
    QList<Handler> result;
    QSqlQuery query(db_);
    query.prepare(
        "SELECT d.id, d.name, d.is_enabled, d.sort_order, d.create_time "
        "FROM handler_dict d LEFT JOIN ("
        "  SELECT handler, MAX(used_at) AS last_used FROM ("
        "    SELECT handler, COALESCE(update_time, create_time) AS used_at FROM task WHERE handler IS NOT NULL AND handler <> '' "
        "    UNION ALL SELECT handler, create_time FROM periodic_task WHERE handler IS NOT NULL AND handler <> ''"
        "  ) GROUP BY handler"
        ") recent ON recent.handler = d.name "
        "WHERE d.is_enabled = 1 "
        "ORDER BY CASE WHEN d.name = ? THEN 0 ELSE 1 END, "
        "CASE WHEN recent.last_used IS NULL THEN 1 ELSE 0 END, "
        "recent.last_used DESC, d.sort_order ASC, d.id ASC");
    query.addBindValue(QStringLiteral("本人"));
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("HandlerDao::findEnabledByRecentUse 失败: %1")
                                     .arg(query.lastError().text()));
        return result;
    }
    while (query.next()) result.append(handlerFromQuery(query));
    return result;
}

// ============================================================================
// 单条查询
// ============================================================================

std::optional<Handler> HandlerDao::findById(int id)
{
    QSqlQuery query(db_);
    query.prepare("SELECT id, name, is_enabled, sort_order, create_time "
                  "FROM handler_dict WHERE id = ?");
    query.addBindValue(id);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("HandlerDao::findById 失败: %1").arg(query.lastError().text()));
        return std::nullopt;
    }
    if (query.next()) {
        return handlerFromQuery(query);
    }
    return std::nullopt;
}

std::optional<Handler> HandlerDao::findByName(const QString &name)
{
    QSqlQuery query(db_);
    query.prepare("SELECT id, name, is_enabled, sort_order, create_time "
                  "FROM handler_dict WHERE name = ?");
    query.addBindValue(name);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("HandlerDao::findByName 失败: %1").arg(query.lastError().text()));
        return std::nullopt;
    }
    if (query.next()) {
        return handlerFromQuery(query);
    }
    return std::nullopt;
}

// ============================================================================
// 插入 / 更新 / 删除
// ============================================================================

int HandlerDao::insert(const Handler &handler)
{
    QString now = DateUtil::currentDateTimeStr();

    QSqlQuery query(db_);
    query.prepare("INSERT INTO handler_dict (name, is_enabled, sort_order, create_time) "
                  "VALUES (?, ?, ?, ?)");
    query.addBindValue(handler.name);
    query.addBindValue(handler.isEnabled ? 1 : 0);
    query.addBindValue(handler.sortOrder);
    query.addBindValue(now);

    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("HandlerDao::insert 失败: %1").arg(query.lastError().text()));
        return -1;
    }
    QVariant lastId = query.lastInsertId();
    if (lastId.isValid()) {
        return lastId.toInt();
    }
    return -1;
}

bool HandlerDao::update(const Handler &handler)
{
    QSqlQuery query(db_);
    query.prepare("UPDATE handler_dict SET name = ?, sort_order = ? WHERE id = ?");
    query.addBindValue(handler.name);
    query.addBindValue(handler.sortOrder);
    query.addBindValue(handler.id);

    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("HandlerDao::update 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool HandlerDao::setEnabled(int id, bool enabled)
{
    QSqlQuery query(db_);
    query.prepare("UPDATE handler_dict SET is_enabled = ? WHERE id = ?");
    query.addBindValue(enabled ? 1 : 0);
    query.addBindValue(id);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("HandlerDao::setEnabled 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool HandlerDao::deleteById(int id)
{
    QSqlQuery query(db_);
    query.prepare("DELETE FROM handler_dict WHERE id = ?");
    query.addBindValue(id);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("HandlerDao::deleteById 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

// ============================================================================
// 引用检查
// ============================================================================

bool HandlerDao::isUsed(int id)
{
    auto opt = findById(id);
    if (!opt) {
        return false;
    }
    QString name = opt->name;

    // 检查 task 表
    QSqlQuery taskQuery(db_);
    taskQuery.prepare("SELECT 1 FROM task WHERE handler = ? LIMIT 1");
    taskQuery.addBindValue(name);
    if (!taskQuery.exec()) {
        Logger::instance().error(QStringLiteral("HandlerDao::isUsed 查询 task 失败: %1").arg(taskQuery.lastError().text()));
        return true;
    }
    if (taskQuery.next()) {
        return true;
    }

    return false;
}

// ============================================================================
// 私有辅助方法
// ============================================================================

Handler HandlerDao::handlerFromQuery(QSqlQuery &query)
{
    Handler handler;
    handler.id = query.value(0).toInt();
    handler.name = query.value(1).toString();
    handler.isEnabled = query.value(2).toInt() != 0;
    handler.sortOrder = query.value(3).toInt();
    QString createTimeStr = query.value(4).toString();
    if (!createTimeStr.isEmpty()) {
        handler.createTime = QDateTime::fromString(createTimeStr, "yyyy-MM-dd HH:mm:ss");
    }
    return handler;
}
