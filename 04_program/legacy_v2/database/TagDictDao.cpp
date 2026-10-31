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

TagDictDao::TagDictDao(QSqlDatabase db)
    : db_(db)
{
}

// ============================================================================
// 列表查询
// ============================================================================

QList<TagDict> TagDictDao::findAll()
{
    QList<TagDict> result;
    QSqlQuery query(db_);
    query.prepare("SELECT id, name, is_enabled, sort_order, create_time "
                  "FROM tag_dict ORDER BY id ASC");
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
    query.prepare("SELECT id, name, is_enabled, sort_order, create_time "
                  "FROM tag_dict WHERE is_enabled = 1 ORDER BY sort_order ASC, id ASC");
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
    query.prepare(
        "SELECT d.id, d.name, d.is_enabled, d.sort_order, d.create_time "
        "FROM tag_dict d LEFT JOIN ("
        "  SELECT tag, MAX(used_at) AS last_used FROM ("
        "    SELECT tag, COALESCE(update_time, create_time) AS used_at FROM task WHERE tag IS NOT NULL AND tag <> '' "
        "    UNION ALL SELECT tag, create_time FROM periodic_task WHERE tag IS NOT NULL AND tag <> '' "
        "    UNION ALL SELECT tag, COALESCE(update_time, create_time) FROM reference_material WHERE tag IS NOT NULL AND tag <> ''"
        "  ) GROUP BY tag"
        ") recent ON recent.tag = d.name "
        "WHERE d.is_enabled = 1 "
        "ORDER BY CASE WHEN recent.last_used IS NULL THEN 1 ELSE 0 END, "
        "recent.last_used DESC, d.sort_order ASC, d.id ASC");
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
    query.prepare("SELECT id, name, is_enabled, sort_order, create_time "
                  "FROM tag_dict WHERE id = ?");
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
    query.prepare("SELECT id, name, is_enabled, sort_order, create_time "
                  "FROM tag_dict WHERE name = ?");
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
    query.prepare("INSERT INTO tag_dict (name, is_enabled, sort_order, create_time) "
                  "VALUES (?, ?, ?, ?)");
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
    query.prepare("UPDATE tag_dict SET name = ?, sort_order = ? WHERE id = ?");
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
    query.prepare("UPDATE tag_dict SET is_enabled = ? WHERE id = ?");
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
    query.prepare("DELETE FROM tag_dict WHERE id = ?");
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

    // 检查 task 表
    QSqlQuery taskQuery(db_);
    taskQuery.prepare("SELECT 1 FROM task WHERE tag = ? LIMIT 1");
    taskQuery.addBindValue(name);
    if (!taskQuery.exec()) {
        Logger::instance().error(QStringLiteral("TagDictDao::isUsed 查询 task 失败: %1").arg(taskQuery.lastError().text()));
        return true;  // 查询失败时保守返回 true（不允许删除）
    }
    if (taskQuery.next()) {
        return true;
    }

    // 检查 reference_material 表
    QSqlQuery refQuery(db_);
    refQuery.prepare("SELECT 1 FROM reference_material WHERE tag = ? LIMIT 1");
    refQuery.addBindValue(name);
    if (!refQuery.exec()) {
        Logger::instance().error(QStringLiteral("TagDictDao::isUsed 查询 reference_material 失败: %1").arg(refQuery.lastError().text()));
        return true;
    }
    if (refQuery.next()) {
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
