/**
 * @file ReferenceMaterialDao.cpp
 * @brief 参考资料数据访问层实现：CRUD、筛选查询、编号序列
 *
 * 依赖：QtSql、models/ReferenceMaterial、utils/Logger
 * 文档：TD 2.9（reference_material 表）、TD 3.5（ReferenceMaterialDao 接口）、coding.md 第五章（数据库访问规范）
 *
 * 关键约束：
 *  - 全部 SQL 使用参数化查询（QSqlQuery::prepare + addBindValue），禁止字符串拼接
 *  - create_time/update_time 使用统一格式 "yyyy-MM-dd HH:mm:ss" 存储
 *  - getMaxSequenceForDate 同时查询 task 和 reference_material 表（共用每日序列）
 *  - keyword 在 name 和 description 两个字段中搜索（LIKE '%keyword%'）
 */

#include "ReferenceMaterialDao.h"
#include "AttachmentSearch.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "utils/DateUtil.h"
#include "utils/Logger.h"

// ============================================================================
// 构造
// ============================================================================

ReferenceMaterialDao::ReferenceMaterialDao(QSqlDatabase db)
    : db_(db)
{
}

// ============================================================================
// 单条查询
// ============================================================================

std::optional<ReferenceMaterial> ReferenceMaterialDao::findById(int id)
{
    QSqlQuery query(db_);
    query.prepare("SELECT * FROM reference_material WHERE id = ?");
    query.addBindValue(id);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("ReferenceMaterialDao::findById 失败: %1").arg(query.lastError().text()));
        return std::nullopt;
    }
    if (query.next()) {
        return refFromQuery(query);
    }
    return std::nullopt;
}

std::optional<ReferenceMaterial> ReferenceMaterialDao::findByRefNo(const QString &refNo)
{
    QSqlQuery query(db_);
    query.prepare("SELECT * FROM reference_material WHERE ref_no = ?");
    query.addBindValue(refNo);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("ReferenceMaterialDao::findByRefNo 失败: %1").arg(query.lastError().text()));
        return std::nullopt;
    }
    if (query.next()) {
        return refFromQuery(query);
    }
    return std::nullopt;
}

// ============================================================================
// 列表查询
// ============================================================================

QList<ReferenceMaterial> ReferenceMaterialDao::findByFilter(const ReferenceFilter &filter)
{
    QList<ReferenceMaterial> result;
    QStringList bindValues;
    QString whereClause = buildWhereClause(filter, bindValues);

    QString sql = "SELECT * FROM reference_material";
    if (!whereClause.isEmpty()) {
        sql += " " + whereClause;
    }
    sql += " ORDER BY ref_no DESC";

    // 分页（pageSize <= 0 表示不分页）
    bool usePaging = filter.pageSize > 0;
    if (usePaging) {
        sql += " LIMIT ? OFFSET ?";
    }

    QSqlQuery query(db_);
    query.prepare(sql);
    // 绑定 WHERE 子句参数（字符串值）
    for (const QString &val : bindValues) {
        query.addBindValue(val);
    }
    // 绑定分页参数（整型）
    if (usePaging) {
        int offset = (filter.page > 0 ? filter.page - 1 : 0) * filter.pageSize;
        query.addBindValue(filter.pageSize);
        query.addBindValue(offset);
    }
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("ReferenceMaterialDao::findByFilter 失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        result.append(refFromQuery(query));
    }
    return result;
}

int ReferenceMaterialDao::countByFilter(const ReferenceFilter &filter)
{
    QStringList bindValues;
    QString whereClause = buildWhereClause(filter, bindValues);

    QString sql = "SELECT COUNT(*) FROM reference_material";
    if (!whereClause.isEmpty()) {
        sql += " " + whereClause;
    }

    QSqlQuery query(db_);
    query.prepare(sql);
    for (const QString &val : bindValues) {
        query.addBindValue(val);
    }
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("ReferenceMaterialDao::countByFilter 失败: %1").arg(query.lastError().text()));
        return 0;
    }
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

QList<ReferenceMaterial> ReferenceMaterialDao::findAll()
{
    QList<ReferenceMaterial> result;
    QSqlQuery query(db_);
    query.prepare("SELECT * FROM reference_material ORDER BY ref_no DESC");
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("ReferenceMaterialDao::findAll 失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        result.append(refFromQuery(query));
    }
    return result;
}

// ============================================================================
// 插入 / 更新 / 删除
// ============================================================================

int ReferenceMaterialDao::insert(const ReferenceMaterial &ref)
{
    QString now = DateUtil::currentDateTimeStr();

    QSqlQuery query(db_);
    query.prepare(
        "INSERT INTO reference_material ("
        "  ref_no, name, source, tag, description, folder_name, folder_path, "
        "  create_time, update_time"
        ") VALUES ("
        "  ?, ?, ?, ?, ?, ?, ?, ?, ?"
        ")");
    query.addBindValue(ref.refNo);
    query.addBindValue(ref.name);
    query.addBindValue(ref.source);
    query.addBindValue(ref.tag);
    query.addBindValue(ref.description);
    query.addBindValue(ref.folderName);
    query.addBindValue(ref.folderPath);
    query.addBindValue(now);  // create_time
    query.addBindValue(now);  // update_time

    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("ReferenceMaterialDao::insert 失败: %1").arg(query.lastError().text()));
        return -1;
    }
    QVariant lastId = query.lastInsertId();
    if (lastId.isValid()) {
        return lastId.toInt();
    }
    return -1;
}

bool ReferenceMaterialDao::update(const ReferenceMaterial &ref)
{
    QString now = DateUtil::currentDateTimeStr();

    QSqlQuery query(db_);
    query.prepare(
        "UPDATE reference_material SET "
        "  ref_no = ?, name = ?, source = ?, tag = ?, description = ?, "
        "  folder_name = ?, folder_path = ?, update_time = ? "
        "WHERE id = ?");
    query.addBindValue(ref.refNo);
    query.addBindValue(ref.name);
    query.addBindValue(ref.source);
    query.addBindValue(ref.tag);
    query.addBindValue(ref.description);
    query.addBindValue(ref.folderName);
    query.addBindValue(ref.folderPath);
    query.addBindValue(now);  // update_time
    query.addBindValue(ref.id);

    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("ReferenceMaterialDao::update 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool ReferenceMaterialDao::deleteById(int id)
{
    // 物理删除（参考资料无回收站）
    QSqlQuery query(db_);
    query.prepare("DELETE FROM reference_material WHERE id = ?");
    query.addBindValue(id);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("ReferenceMaterialDao::deleteById 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

// ============================================================================
// 编号序列
// ============================================================================

int ReferenceMaterialDao::getMaxSequenceForDate(const QString &dateStr)
{
    // 任务和参考资料共用同一每日序列，需同时查询 task 和 reference_material 表
    // 编号格式 YYYYMMDD-NNN，SUBSTR(x, 1, 8) 取日期，SUBSTR(x, 10) 取序列号部分
    QSqlQuery query(db_);
    query.prepare(
        "SELECT MAX(seq) FROM ("
        "  SELECT CAST(SUBSTR(task_no, 10) AS INTEGER) AS seq FROM task "
        "  WHERE SUBSTR(task_no, 1, 8) = ? "
        "  UNION ALL "
        "  SELECT CAST(SUBSTR(ref_no, 10) AS INTEGER) AS seq FROM reference_material "
        "  WHERE SUBSTR(ref_no, 1, 8) = ?"
        ")");
    query.addBindValue(dateStr);
    query.addBindValue(dateStr);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("ReferenceMaterialDao::getMaxSequenceForDate 失败: %1").arg(query.lastError().text()));
        return 0;
    }
    if (query.next() && !query.isNull(0)) {
        return query.value(0).toInt();
    }
    return 0;
}

// ============================================================================
// 私有辅助方法
// ============================================================================

ReferenceMaterial ReferenceMaterialDao::refFromQuery(QSqlQuery &query)
{
    ReferenceMaterial ref;
    ref.id = query.value("id").toInt();
    ref.refNo = query.value("ref_no").toString();
    ref.name = query.value("name").toString();
    ref.source = query.value("source").toString();
    ref.tag = query.value("tag").toString();
    ref.description = query.value("description").toString();
    ref.folderName = query.value("folder_name").toString();
    ref.folderPath = query.value("folder_path").toString();
    // 时间字段格式 "yyyy-MM-dd HH:mm:ss"
    QString createTimeStr = query.value("create_time").toString();
    if (!createTimeStr.isEmpty()) {
        ref.createTime = DateUtil::parseDateTime(createTimeStr);
    }
    QString updateTimeStr = query.value("update_time").toString();
    if (!updateTimeStr.isEmpty()) {
        ref.updateTime = DateUtil::parseDateTime(updateTimeStr);
    }
    return ref;
}

QString ReferenceMaterialDao::buildWhereClause(const ReferenceFilter &filter, QStringList &bindValues)
{
    QStringList parts;

    // 来源
    if (!filter.source.isEmpty()) {
        parts << "source = ?";
        bindValues << filter.source;
    }

    // 标签
    if (!filter.tag.isEmpty()) {
        parts << "tag = ?";
        bindValues << filter.tag;
    }

    // 日期范围（create_time 为 TEXT 格式，字符串比较有效）
    if (filter.year > 0) {
        parts << "create_time >= ?" << "create_time <= ?";
        bindValues << (QDate(filter.year, 1, 1).toString("yyyy-MM-dd") + " 00:00:00")
                   << (QDate(filter.year, 12, 31).toString("yyyy-MM-dd") + " 23:59:59");
    }
    if (filter.startDate.isValid()) {
        parts << "create_time >= ?";
        bindValues << (filter.startDate.toString("yyyy-MM-dd") + " 00:00:00");
    }
    if (filter.endDate.isValid()) {
        parts << "create_time <= ?";
        bindValues << (filter.endDate.toString("yyyy-MM-dd") + " 23:59:59");
    }

    if (!filter.keyword.isEmpty()) {
        const QStringList ids = AttachmentSearch::matchingIds(db_, "reference_material", parts, bindValues, filter.keyword);
        QString match = "(ref_no LIKE ? OR name LIKE ? OR description LIKE ?";
        bindValues << ("%" + filter.keyword + "%") << ("%" + filter.keyword + "%") << ("%" + filter.keyword + "%");
        if (!ids.isEmpty()) {
            QStringList placeholders;
            for (const QString &id : ids) { placeholders << "?"; bindValues << id; }
            match += " OR id IN (" + placeholders.join(',') + ")";
        }
        parts << match + ")";
    }
    if (parts.isEmpty()) {
        return QString();
    }
    return "WHERE " + parts.join(" AND ");
}
