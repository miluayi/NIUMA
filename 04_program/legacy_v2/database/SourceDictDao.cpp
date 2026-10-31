/**
 * @file SourceDictDao.cpp
 * @brief 来源字典数据访问层实现：source_dict 表的 CRUD 与逻辑删除
 *
 * 依赖：QtSql、models/SourceDict、utils/Logger、utils/DateUtil
 * 文档：task.md T8.9、TD 2.5、TD 3.3、TD 3.6.1、coding.md 第五章
 *
 * 实现说明：
 *  - 全部 SQL 使用参数化查询（QSqlQuery::prepare + addBindValue），禁止字符串拼接
 *  - create_time 格式：yyyy-MM-dd HH:mm:ss（通过 DateUtil::currentDateTimeStr 获取）
 *  - isUsed 通过 name 匹配 task.source 和 reference_material.source（TEXT 字段，非外键）
 */

#include "SourceDictDao.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "utils/DateUtil.h"
#include "utils/Logger.h"

// ============================================================================
// 构造
// ============================================================================

SourceDictDao::SourceDictDao(QSqlDatabase db)
    : db_(db)
{
}

// ============================================================================
// 列表查询
// ============================================================================

QList<SourceDict> SourceDictDao::findAll()
{
    QList<SourceDict> result;
    QSqlQuery query(db_);
    query.prepare("SELECT id, name, is_enabled, sort_order, create_time "
                  "FROM source_dict ORDER BY id ASC");
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("SourceDictDao::findAll 失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        result.append(sourceFromQuery(query));
    }
    return result;
}

QList<SourceDict> SourceDictDao::findEnabled()
{
    QList<SourceDict> result;
    QSqlQuery query(db_);
    query.prepare("SELECT id, name, is_enabled, sort_order, create_time "
                  "FROM source_dict WHERE is_enabled = 1 ORDER BY sort_order ASC, id ASC");
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("SourceDictDao::findEnabled 失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        result.append(sourceFromQuery(query));
    }
    return result;
}

QList<SourceDict> SourceDictDao::findEnabledByRecentUse()
{
    QList<SourceDict> result;
    QSqlQuery query(db_);
    query.prepare(
        "SELECT s.id, s.name, s.is_enabled, s.sort_order, s.create_time "
        "FROM source_dict s LEFT JOIN ("
        "  SELECT source, MAX(used_at) AS last_used FROM ("
        "    SELECT source, COALESCE(update_time, create_time) AS used_at FROM task WHERE source IS NOT NULL AND source <> '' "
        "    UNION ALL SELECT source, create_time FROM periodic_task WHERE source IS NOT NULL AND source <> '' "
        "    UNION ALL SELECT source, COALESCE(update_time, create_time) FROM reference_material WHERE source IS NOT NULL AND source <> ''"
        "  ) GROUP BY source"
        ") recent ON recent.source = s.name "
        "WHERE s.is_enabled = 1 "
        "ORDER BY CASE WHEN recent.last_used IS NULL THEN 1 ELSE 0 END, "
        "recent.last_used DESC, s.sort_order ASC, s.id ASC");
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("SourceDictDao::findEnabledByRecentUse 失败: %1")
                                     .arg(query.lastError().text()));
        return result;
    }
    while (query.next()) result.append(sourceFromQuery(query));
    return result;
}

// ============================================================================
// 单条查询
// ============================================================================

std::optional<SourceDict> SourceDictDao::findById(int id)
{
    QSqlQuery query(db_);
    query.prepare("SELECT id, name, is_enabled, sort_order, create_time "
                  "FROM source_dict WHERE id = ?");
    query.addBindValue(id);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("SourceDictDao::findById 失败: %1").arg(query.lastError().text()));
        return std::nullopt;
    }
    if (query.next()) {
        return sourceFromQuery(query);
    }
    return std::nullopt;
}

std::optional<SourceDict> SourceDictDao::findByName(const QString &name)
{
    QSqlQuery query(db_);
    query.prepare("SELECT id, name, is_enabled, sort_order, create_time "
                  "FROM source_dict WHERE name = ?");
    query.addBindValue(name);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("SourceDictDao::findByName 失败: %1").arg(query.lastError().text()));
        return std::nullopt;
    }
    if (query.next()) {
        return sourceFromQuery(query);
    }
    return std::nullopt;
}

// ============================================================================
// 插入 / 更新 / 删除
// ============================================================================

int SourceDictDao::insert(const SourceDict &source)
{
    QString now = DateUtil::currentDateTimeStr();

    QSqlQuery query(db_);
    query.prepare("INSERT INTO source_dict (name, is_enabled, sort_order, create_time) "
                  "VALUES (?, ?, ?, ?)");
    query.addBindValue(source.name);
    query.addBindValue(source.isEnabled ? 1 : 0);
    query.addBindValue(source.sortOrder);
    query.addBindValue(now);

    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("SourceDictDao::insert 失败: %1").arg(query.lastError().text()));
        return -1;
    }
    QVariant lastId = query.lastInsertId();
    if (lastId.isValid()) {
        return lastId.toInt();
    }
    return -1;
}

bool SourceDictDao::update(const SourceDict &source)
{
    QSqlQuery query(db_);
    query.prepare("UPDATE source_dict SET name = ?, sort_order = ? WHERE id = ?");
    query.addBindValue(source.name);
    query.addBindValue(source.sortOrder);
    query.addBindValue(source.id);

    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("SourceDictDao::update 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool SourceDictDao::setEnabled(int id, bool enabled)
{
    QSqlQuery query(db_);
    query.prepare("UPDATE source_dict SET is_enabled = ? WHERE id = ?");
    query.addBindValue(enabled ? 1 : 0);
    query.addBindValue(id);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("SourceDictDao::setEnabled 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool SourceDictDao::deleteById(int id)
{
    QSqlQuery query(db_);
    query.prepare("DELETE FROM source_dict WHERE id = ?");
    query.addBindValue(id);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("SourceDictDao::deleteById 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

// ============================================================================
// 引用检查
// ============================================================================

bool SourceDictDao::isUsed(int id)
{
    // 先查出 name，再按 name 匹配 task.source 和 reference_material.source
    auto opt = findById(id);
    if (!opt) {
        return false;
    }
    QString name = opt->name;

    // 检查 task 表
    QSqlQuery taskQuery(db_);
    taskQuery.prepare("SELECT 1 FROM task WHERE source = ? LIMIT 1");
    taskQuery.addBindValue(name);
    if (!taskQuery.exec()) {
        Logger::instance().error(QStringLiteral("SourceDictDao::isUsed 查询 task 失败: %1").arg(taskQuery.lastError().text()));
        return true;  // 查询失败时保守返回 true（不允许删除）
    }
    if (taskQuery.next()) {
        return true;
    }

    // 检查 reference_material 表
    QSqlQuery refQuery(db_);
    refQuery.prepare("SELECT 1 FROM reference_material WHERE source = ? LIMIT 1");
    refQuery.addBindValue(name);
    if (!refQuery.exec()) {
        Logger::instance().error(QStringLiteral("SourceDictDao::isUsed 查询 reference_material 失败: %1").arg(refQuery.lastError().text()));
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

SourceDict SourceDictDao::sourceFromQuery(QSqlQuery &query)
{
    SourceDict source;
    source.id = query.value(0).toInt();
    source.name = query.value(1).toString();
    source.isEnabled = query.value(2).toInt() != 0;
    source.sortOrder = query.value(3).toInt();
    QString createTimeStr = query.value(4).toString();
    if (!createTimeStr.isEmpty()) {
        source.createTime = QDateTime::fromString(createTimeStr, "yyyy-MM-dd HH:mm:ss");
    }
    return source;
}
