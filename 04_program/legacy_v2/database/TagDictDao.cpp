/**
 * @file TagDictDao.cpp
 * @brief 事项标签字典数据访问层实现：tag_dict 表的 CRUD 与逻辑删除
 *
 * 依赖：QtSql、models/TagDict、utils/Logger、utils/DateUtil
 * 文档：task.md T8.9、TD 2.6、TD 3.3、TD 3.6.1、coding.md 第五章
 *
 * 实现说明：
 *  - 全部 SQL 使用参数化查询（QSqlQuery::prepare + addBindValue），禁止字符串拼接
 *  - create_time 格式：yyyy-MM-dd HH:mm:ss（通过 DateUtil::currentDateTimeStr 获取）
 *  - isUsed 通过 name 匹配 task.tag 和 reference_material.tag（TEXT 字段，非外键）
 */

#include "TagDictDao.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "utils/DateUtil.h"
#include "utils/Logger.h"

// ============================================================================
// 构造
// ============================================================================

TagDictDao::TagDictDao(QSqlDatabase db, TagScope scope)
    : db_(db),scope_(scope)
{
    // Identifiers come only from this closed mapping, never from user text.
    switch(scope) {
    case TagScope::Periodic: tableName_="periodic_tag_dict";recordTable_="periodic_task";break;
    case TagScope::Reference: tableName_="reference_tag_dict";recordTable_="reference_material";break;
    default: tableName_="tag_dict";recordTable_="task";break;
    }
}

// ============================================================================
// 列表查询
// ============================================================================

QList<TagDict> TagDictDao::findAll()
{
    QList<TagDict> result;
    QSqlQuery query(db_);
    query.prepare(QStringLiteral("SELECT id, name, is_enabled, sort_order, create_time "
                  "FROM %1 ORDER BY id ASC").arg(tableName_));
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TagDictDao::findAll 失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        result.append(tagFromQuery(query));
    }
    return result;
}

QList<TagDict> TagDictDao::findEnabled()
{
    QList<TagDict> result;
    QSqlQuery query(db_);
    query.prepare(QStringLiteral("SELECT id, name, is_enabled, sort_order, create_time "
                  "FROM %1 WHERE is_enabled = 1 ORDER BY sort_order ASC, id ASC").arg(tableName_));
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TagDictDao::findEnabled 失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        result.append(tagFromQuery(query));
    }
    return result;
}

QList<TagDict> TagDictDao::findEnabledByRecentUse()
{
    QList<TagDict> result;
    QSqlQuery query(db_);
    query.prepare(QStringLiteral(
        "SELECT d.id, d.name, d.is_enabled, d.sort_order, d.create_time "
        "FROM %1 d LEFT JOIN ("
        "  SELECT tag, MAX(%3) AS last_used FROM %2 WHERE tag IS NOT NULL AND tag <> '' GROUP BY tag"
        ") recent ON recent.tag = d.name "
        "WHERE d.is_enabled = 1 "
        "ORDER BY CASE WHEN recent.last_used IS NULL THEN 1 ELSE 0 END, "
        "recent.last_used DESC, d.sort_order ASC, d.id ASC").arg(tableName_,recordTable_,
            scope_==TagScope::Periodic ? QStringLiteral("create_time") : QStringLiteral("COALESCE(update_time, create_time)")));
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TagDictDao::findEnabledByRecentUse 失败: %1")
                                     .arg(query.lastError().text()));
        return result;
    }
    while (query.next()) result.append(tagFromQuery(query));
    return result;
}

// ============================================================================
// 单条查询
// ============================================================================

std::optional<TagDict> TagDictDao::findById(int id)
{
    QSqlQuery query(db_);
    query.prepare(QStringLiteral("SELECT id, name, is_enabled, sort_order, create_time "
                  "FROM %1 WHERE id = ?").arg(tableName_));
    query.addBindValue(id);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TagDictDao::findById 失败: %1").arg(query.lastError().text()));
        return std::nullopt;
    }
    if (query.next()) {
        return tagFromQuery(query);
    }
    return std::nullopt;
}

std::optional<TagDict> TagDictDao::findByName(const QString &name)
{
    QSqlQuery query(db_);
    query.prepare(QStringLiteral("SELECT id, name, is_enabled, sort_order, create_time "
                  "FROM %1 WHERE name = ?").arg(tableName_));
    query.addBindValue(name);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TagDictDao::findByName 失败: %1").arg(query.lastError().text()));
        return std::nullopt;
    }
    if (query.next()) {
        return tagFromQuery(query);
    }
    return std::nullopt;
}

// ============================================================================
// 插入 / 更新 / 删除
// ============================================================================

int TagDictDao::insert(const TagDict &tag)
{
    QString now = DateUtil::currentDateTimeStr();

    QSqlQuery query(db_);
    query.prepare(QStringLiteral("INSERT INTO %1 (name, is_enabled, sort_order, create_time) "
                  "VALUES (?, ?, ?, ?)").arg(tableName_));
    query.addBindValue(tag.name);
    query.addBindValue(tag.isEnabled ? 1 : 0);
    query.addBindValue(tag.sortOrder);
    query.addBindValue(now);

    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TagDictDao::insert 失败: %1").arg(query.lastError().text()));
        return -1;
    }
    QVariant lastId = query.lastInsertId();
    if (lastId.isValid()) {
        return lastId.toInt();
    }
    return -1;
}

bool TagDictDao::update(const TagDict &tag)
{
    QSqlQuery query(db_);
    query.prepare(QStringLiteral("UPDATE %1 SET name = ?, sort_order = ? WHERE id = ?").arg(tableName_));
    query.addBindValue(tag.name);
    query.addBindValue(tag.sortOrder);
    query.addBindValue(tag.id);

    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TagDictDao::update 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool TagDictDao::setEnabled(int id, bool enabled)
{
    QSqlQuery query(db_);
    query.prepare(QStringLiteral("UPDATE %1 SET is_enabled = ? WHERE id = ?").arg(tableName_));
    query.addBindValue(enabled ? 1 : 0);
    query.addBindValue(id);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TagDictDao::setEnabled 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool TagDictDao::deleteById(int id)
{
    QSqlQuery query(db_);
    query.prepare(QStringLiteral("DELETE FROM %1 WHERE id = ?").arg(tableName_));
    query.addBindValue(id);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TagDictDao::deleteById 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

// ============================================================================
// 引用检查
// ============================================================================

bool TagDictDao::isUsed(int id)
{
    // 先查出 name，再按 name 匹配 task.tag 和 reference_material.tag
    auto opt = findById(id);
    if (!opt) {
        return false;
    }
    QString name = opt->name;

    // Only records in this dictionary's domain can keep a tag in use.
    QSqlQuery taskQuery(db_);
    taskQuery.prepare(QStringLiteral("SELECT 1 FROM %1 WHERE tag = ? LIMIT 1").arg(recordTable_));
    taskQuery.addBindValue(name);
    if (!taskQuery.exec()) {
        Logger::instance().error(QStringLiteral("TagDictDao::isUsed 查询 task 失败: %1").arg(taskQuery.lastError().text()));
        return true;  // 查询失败时保守返回 true（不允许删除）
    }
    if (taskQuery.next()) {
        return true;
    }

    return false;
}

// ============================================================================
// 私有辅助方法
// ============================================================================

TagDict TagDictDao::tagFromQuery(QSqlQuery &query)
{
    TagDict tag;
    tag.id = query.value(0).toInt();
    tag.name = query.value(1).toString();
    tag.isEnabled = query.value(2).toInt() != 0;
    tag.sortOrder = query.value(3).toInt();
    QString createTimeStr = query.value(4).toString();
    if (!createTimeStr.isEmpty()) {
        tag.createTime = QDateTime::fromString(createTimeStr, "yyyy-MM-dd HH:mm:ss");
    }
    return tag;
}
