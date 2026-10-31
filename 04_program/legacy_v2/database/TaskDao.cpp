/**
 * @file TaskDao.cpp
 * @brief 任务数据访问层实现：CRUD、筛选查询、编号序列、统计
 *
 * 依赖：QtSql、models/Task、models/TaskFilter、utils/Logger、utils/DateUtil
 * 文档：TD 3.1（TaskDao 接口）、TD 5.1（编号规则）、coding.md 第五章（数据库访问规范）、task.md T1.7
 *
 * 关键约束：
 *  - 全部 SQL 使用参数化查询（QSqlQuery::prepare + addBindValue），禁止字符串拼接
 *  - getMaxSequenceForDate 不添加 status 条件，必须包含 status='已删除' 行防止序列回绕
 *  - activeTaskCondition() 返回 "status = '进行中'"
 *  - buildOrderByClause() 返回三档排序 SQL 片段
 */

#include "TaskDao.h"
#include "AttachmentSearch.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "utils/DateUtil.h"
#include "utils/Logger.h"

// ============================================================================
// 构造
// ============================================================================

TaskDao::TaskDao(QSqlDatabase db)
    : db_(db)
{
}

QString TaskDao::lastError() const
{
    return lastError_;
}

// ============================================================================
// 静态方法（唯一出处）
// ============================================================================

QString TaskDao::activeTaskCondition()
{
    return "status = '进行中'";
}

QString TaskDao::buildOrderByClause()
{
    // 三档状态；进行中未逾期截止日优先，已完成按实际完成时间倒序。
    return "CASE WHEN status = '他人办理' THEN 2 "
           "WHEN status = '已完成' THEN 1 ELSE 0 END ASC, "
           "CASE WHEN status = '进行中' AND deadline >= date('now', 'localtime') THEN 0 ELSE 1 END ASC, "
           "CASE WHEN status = '进行中' AND deadline >= date('now', 'localtime') THEN deadline END ASC, "
           "CASE WHEN status = '已完成' THEN julianday(finish_time) END DESC, "
           "task_no DESC";
}

// ============================================================================
// 单条查询
// ============================================================================

std::optional<Task> TaskDao::findById(int id)
{
    QSqlQuery query(db_);
    query.prepare("SELECT * FROM task WHERE id = ?");
    query.addBindValue(id);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TaskDao::findById 失败: %1").arg(query.lastError().text()));
        return std::nullopt;
    }
    if (query.next()) {
        return taskFromQuery(query);
    }
    return std::nullopt;
}

std::optional<Task> TaskDao::findByTaskNo(const QString &taskNo)
{
    QSqlQuery query(db_);
    query.prepare("SELECT * FROM task WHERE task_no = ?");
    query.addBindValue(taskNo);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TaskDao::findByTaskNo 失败: %1").arg(query.lastError().text()));
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

QList<Task> TaskDao::findByFilter(const TaskFilter &filter)
{
    QList<Task> result;
    QStringList bindValues;
    QString whereClause = buildWhereClause(filter, bindValues);

    QString sql = "SELECT * FROM task";
    if (!whereClause.isEmpty()) {
        sql += " " + whereClause;
    }
    // 首页显式请求截止时间排序；逾期任务与无截止日任务按编号倒序。
    if (filter.sortByDeadline && filter.status != QStringLiteral("已完成")) {
        sql += " ORDER BY CASE WHEN deadline >= date('now', 'localtime') THEN 0 ELSE 1 END ASC, "
               "CASE WHEN deadline >= date('now', 'localtime') THEN deadline END ASC, task_no DESC";
    } else {
        sql += " ORDER BY " + buildOrderByClause();
    }

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
        Logger::instance().error(QStringLiteral("TaskDao::findByFilter 失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        result.append(taskFromQuery(query));
    }
    return result;
}

int TaskDao::countByFilter(const TaskFilter &filter)
{
    QStringList bindValues;
    QString whereClause = buildWhereClause(filter, bindValues);

    QString sql = "SELECT COUNT(*) FROM task";
    if (!whereClause.isEmpty()) {
        sql += " " + whereClause;
    }

    QSqlQuery query(db_);
    query.prepare(sql);
    for (const QString &val : bindValues) {
        query.addBindValue(val);
    }
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TaskDao::countByFilter 失败: %1").arg(query.lastError().text()));
        return 0;
    }
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

QList<Task> TaskDao::findActive(int limit)
{
    QList<Task> result;
    QSqlQuery query(db_);
    QString sql = "SELECT * FROM task WHERE " + activeTaskCondition()
                  + " ORDER BY create_time DESC";
    if (limit > 0) {
        sql += " LIMIT ?";
    }
    query.prepare(sql);
    if (limit > 0) {
        query.addBindValue(limit);
    }
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TaskDao::findActive 失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        result.append(taskFromQuery(query));
    }
    return result;
}

QList<Task> TaskDao::findTodayFocus()
{
    QList<Task> result;
    QSqlQuery query(db_);
    // 今日专注仅查询 status='进行中' 任务，按 focus_order 正序
    query.prepare("SELECT * FROM task WHERE is_today_focus = 1 AND "
                  + activeTaskCondition()
                  + " ORDER BY focus_order ASC");
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TaskDao::findTodayFocus 失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        result.append(taskFromQuery(query));
    }
    return result;
}

QList<Task> TaskDao::findDeleted()
{
    QList<Task> result;
    QSqlQuery query(db_);
    query.prepare("SELECT * FROM task WHERE status = ? ORDER BY delete_time DESC");
    query.addBindValue(QStringLiteral("已删除"));
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TaskDao::findDeleted 失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        result.append(taskFromQuery(query));
    }
    return result;
}

QStringList TaskDao::getDistinctSources()
{
    QStringList result;
    QSqlQuery query(db_);
    query.prepare("SELECT DISTINCT source FROM task WHERE status != ? AND source IS NOT NULL AND source != '' ORDER BY source");
    query.addBindValue(QStringLiteral("已删除"));
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TaskDao::getDistinctSources 失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        result.append(query.value(0).toString());
    }
    return result;
}

QStringList TaskDao::getDistinctTags()
{
    QStringList result;
    QSqlQuery query(db_);
    query.prepare("SELECT DISTINCT tag FROM task WHERE status != ? AND tag IS NOT NULL AND tag != '' ORDER BY tag");
    query.addBindValue(QStringLiteral("已删除"));
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TaskDao::getDistinctTags 失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        result.append(query.value(0).toString());
    }
    return result;
}

QStringList TaskDao::getDistinctHandlers()
{
    QSqlQuery query(db_);
    query.prepare("SELECT DISTINCT handler FROM task WHERE status != ? AND handler IS NOT NULL "
                  "AND TRIM(handler) != '' ORDER BY handler");
    query.addBindValue(QStringLiteral("已删除"));
    QStringList result;
    if (query.exec()) while (query.next()) result.append(query.value(0).toString());
    return result;
}

QList<int> TaskDao::getDistinctYears()
{
    QSqlQuery query(db_);
    query.prepare("SELECT DISTINCT strftime('%Y', create_time) FROM task WHERE status != ?");
    query.addBindValue(QStringLiteral("已删除"));
    QList<int> result;
    if (query.exec()) while (query.next()) {
        const int year = query.value(0).toInt();
        if (year > 0) result.append(year);
    }
    return result;
}

QList<Task> TaskDao::findByDateRange(const QDate &start, const QDate &end)
{
    QList<Task> result;
    QSqlQuery query(db_);
    query.prepare("SELECT * FROM task WHERE create_time >= ? AND create_time <= ? "
                  "ORDER BY create_time DESC");
    query.addBindValue(start.toString("yyyy-MM-dd") + " 00:00:00");
    query.addBindValue(end.toString("yyyy-MM-dd") + " 23:59:59");
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TaskDao::findByDateRange 失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        result.append(taskFromQuery(query));
    }
    return result;
}

// ============================================================================
// 插入 / 更新 / 删除
// ============================================================================

int TaskDao::insert(const Task &task)
{
    QString now = DateUtil::currentDateTimeStr();

    QSqlQuery query(db_);
    query.prepare(
        "INSERT INTO task ("
        "  task_no, title, category, source, tag, handler, status, "
        "  original_status, folder_name, folder_path, is_key_task, has_subdirs, "
        "  is_today_focus, focus_order, deadline, description, is_from_periodic, "
        "  processing_days, create_time, finish_time, delete_time, update_time"
        ") VALUES ("
        "  ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?"
        ")");
    query.addBindValue(task.taskNo);
    query.addBindValue(task.title);
    query.addBindValue(task.category);
    query.addBindValue(task.source);
    query.addBindValue(task.tag);
    query.addBindValue(task.handler);
    query.addBindValue(task.status);
    query.addBindValue(task.originalStatus);
    query.addBindValue(task.folderName);
    query.addBindValue(task.folderPath);
    query.addBindValue(task.isKeyTask ? 1 : 0);
    query.addBindValue(task.hasSubdirs ? 1 : 0);
    query.addBindValue(task.isTodayFocus ? 1 : 0);
    query.addBindValue(task.focusOrder);
    // deadline 为空时存储 NULL
    query.addBindValue(task.deadline.isValid() ? QVariant(task.deadline.toString("yyyy-MM-dd")) : QVariant());
    query.addBindValue(task.description);
    query.addBindValue(task.isFromPeriodic ? 1 : 0);
    query.addBindValue(task.processingDays);
    query.addBindValue(task.createTime.isValid() ? DateUtil::formatDateTime(task.createTime) : now);
    query.addBindValue(task.finishTime.isValid() ? QVariant(DateUtil::formatDateTime(task.finishTime)) : QVariant());
    query.addBindValue(task.deleteTime.isValid() ? QVariant(DateUtil::formatDateTime(task.deleteTime)) : QVariant());
    query.addBindValue(now);  // update_time

    if (!query.exec()) {
        lastError_ = query.lastError().text();
        Logger::instance().error(QStringLiteral("TaskDao::insert 失败: %1").arg(lastError_));
        return -1;
    }
    QVariant lastId = query.lastInsertId();
    if (lastId.isValid()) {
        return lastId.toInt();
    }
    return -1;
}

bool TaskDao::update(const Task &task)
{
    QString now = DateUtil::currentDateTimeStr();

    QSqlQuery query(db_);
    query.prepare(
        "UPDATE task SET "
        "  task_no = ?, title = ?, category = ?, source = ?, tag = ?, handler = ?, "
        "  status = ?, original_status = ?, folder_name = ?, folder_path = ?, "
        "  is_key_task = ?, has_subdirs = ?, is_today_focus = ?, focus_order = ?, "
        "  deadline = ?, description = ?, is_from_periodic = ?, processing_days = ?, "
        "  finish_time = ?, delete_time = ?, update_time = ? "
        "WHERE id = ?");
    query.addBindValue(task.taskNo);
    query.addBindValue(task.title);
    query.addBindValue(task.category);
    query.addBindValue(task.source);
    query.addBindValue(task.tag);
    query.addBindValue(task.handler);
    query.addBindValue(task.status);
    query.addBindValue(task.originalStatus);
    query.addBindValue(task.folderName);
    query.addBindValue(task.folderPath);
    query.addBindValue(task.isKeyTask ? 1 : 0);
    query.addBindValue(task.hasSubdirs ? 1 : 0);
    query.addBindValue(task.isTodayFocus ? 1 : 0);
    query.addBindValue(task.focusOrder);
    query.addBindValue(task.deadline.isValid() ? QVariant(task.deadline.toString("yyyy-MM-dd")) : QVariant());
    query.addBindValue(task.description);
    query.addBindValue(task.isFromPeriodic ? 1 : 0);
    query.addBindValue(task.processingDays);
    query.addBindValue(task.finishTime.isValid() ? QVariant(DateUtil::formatDateTime(task.finishTime)) : QVariant());
    query.addBindValue(task.deleteTime.isValid() ? QVariant(DateUtil::formatDateTime(task.deleteTime)) : QVariant());
    query.addBindValue(now);  // update_time
    query.addBindValue(task.id);

    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TaskDao::update 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool TaskDao::updateStatus(int id, const QString &status)
{
    QSqlQuery query(db_);
    query.prepare("UPDATE task SET status = ?, update_time = ? WHERE id = ?");
    query.addBindValue(status);
    query.addBindValue(DateUtil::currentDateTimeStr());
    query.addBindValue(id);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TaskDao::updateStatus 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool TaskDao::updateHandler(int id, const QString &handler)
{
    QSqlQuery query(db_);
    query.prepare("UPDATE task SET handler = ?, update_time = ? WHERE id = ?");
    query.addBindValue(handler);
    query.addBindValue(DateUtil::currentDateTimeStr());
    query.addBindValue(id);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TaskDao::updateHandler 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool TaskDao::updateKeyTask(int id, bool isKey)
{
    QSqlQuery query(db_);
    query.prepare("UPDATE task SET is_key_task = ?, update_time = ? WHERE id = ?");
    query.addBindValue(isKey ? 1 : 0);
    query.addBindValue(DateUtil::currentDateTimeStr());
    query.addBindValue(id);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TaskDao::updateKeyTask 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool TaskDao::updateTodayFocus(int id, bool focused, int focusOrder)
{
    QSqlQuery query(db_);
    query.prepare("UPDATE task SET is_today_focus = ?, focus_order = ?, update_time = ? WHERE id = ?");
    query.addBindValue(focused ? 1 : 0);
    query.addBindValue(focusOrder);
    query.addBindValue(DateUtil::currentDateTimeStr());
    query.addBindValue(id);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TaskDao::updateTodayFocus 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool TaskDao::deleteById(int id)
{
    // 物理删除（逻辑删除由 TaskService 设置 status='已删除' 实现）
    QSqlQuery query(db_);
    query.prepare("DELETE FROM task WHERE id = ?");
    query.addBindValue(id);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TaskDao::deleteById 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

// ============================================================================
// 编号序列
// ============================================================================

int TaskDao::getMaxSequenceForDate(const QString &dateStr)
{
    // 重要：不添加 status 条件！必须包含 status='已删除' 行防止序列回绕
    // task_no 格式 YYYYMMDD-NNN，SUBSTR(task_no, 1, 8) 取日期，SUBSTR(task_no, 10, 3) 取序列号
    QSqlQuery query(db_);
    query.prepare("SELECT MAX(CAST(SUBSTR(task_no, 10, 3) AS INTEGER)) AS max_seq "
                  "FROM task WHERE SUBSTR(task_no, 1, 8) = ?");
    query.addBindValue(dateStr);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TaskDao::getMaxSequenceForDate 失败: %1").arg(query.lastError().text()));
        return 0;
    }
    if (query.next() && !query.isNull(0)) {
        return query.value(0).toInt();
    }
    return 0;
}

// ============================================================================
// 统计
// ============================================================================

int TaskDao::countByStatus(const QString &status)
{
    QSqlQuery query(db_);
    query.prepare("SELECT COUNT(*) FROM task WHERE status = ?");
    query.addBindValue(status);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TaskDao::countByStatus 失败: %1").arg(query.lastError().text()));
        return 0;
    }
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int TaskDao::countByDateRange(const QDate &start, const QDate &end)
{
    QSqlQuery query(db_);
    query.prepare("SELECT COUNT(*) FROM task WHERE create_time >= ? AND create_time <= ?");
    query.addBindValue(start.toString("yyyy-MM-dd") + " 00:00:00");
    query.addBindValue(end.toString("yyyy-MM-dd") + " 23:59:59");
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TaskDao::countByDateRange 失败: %1").arg(query.lastError().text()));
        return 0;
    }
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

// ============================================================================
// Phase 6 统计方法
// ============================================================================

TaskStats TaskDao::getStatsByDateRange(const QDate &start, const QDate &end)
{
    TaskStats stats;
    QSqlQuery query(db_);
    query.prepare("SELECT status, COUNT(*) FROM task "
                  "WHERE create_time >= ? AND create_time <= ? AND status != ? "
                  "GROUP BY status");
    query.addBindValue(start.toString("yyyy-MM-dd") + " 00:00:00");
    query.addBindValue(end.toString("yyyy-MM-dd") + " 23:59:59");
    query.addBindValue(QStringLiteral("已删除"));
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TaskDao::getStatsByDateRange 失败: %1").arg(query.lastError().text()));
        return stats;
    }
    while (query.next()) {
        QString status = query.value(0).toString();
        int count = query.value(1).toInt();
        stats.totalCount += count;
        if (status == QStringLiteral("已完成")) {
            stats.completedCount = count;
        } else if (status == QStringLiteral("进行中")) {
            stats.inProgressCount = count;
        } else if (status == QStringLiteral("他人办理")) {
            stats.otherHandlerCount = count;
        }
    }
    if (stats.totalCount > 0) {
        stats.completionRate = static_cast<double>(stats.completedCount) / stats.totalCount * 100.0;
    }
    return stats;
}

QList<CategoryStat> TaskDao::getCategoryDistribution(const QDate &start, const QDate &end)
{
    QList<CategoryStat> result;
    QSqlQuery query(db_);
    query.prepare("SELECT category, COUNT(*) AS cnt FROM task "
                  "WHERE create_time >= ? AND create_time <= ? AND status != ? "
                  "GROUP BY category ORDER BY cnt DESC");
    query.addBindValue(start.toString("yyyy-MM-dd") + " 00:00:00");
    query.addBindValue(end.toString("yyyy-MM-dd") + " 23:59:59");
    query.addBindValue(QStringLiteral("已删除"));
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TaskDao::getCategoryDistribution 失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        CategoryStat item;
        item.category = query.value(0).toString();
        item.count = query.value(1).toInt();
        result.append(item);
    }
    return result;
}

QList<SourceStat> TaskDao::getSourceDistribution(const QDate &start, const QDate &end)
{
    QList<SourceStat> result;
    QSqlQuery query(db_);
    query.prepare("SELECT source, COUNT(*) AS cnt FROM task "
                  "WHERE create_time >= ? AND create_time <= ? AND status != ? "
                  "GROUP BY source ORDER BY cnt DESC");
    query.addBindValue(start.toString("yyyy-MM-dd") + " 00:00:00");
    query.addBindValue(end.toString("yyyy-MM-dd") + " 23:59:59");
    query.addBindValue(QStringLiteral("已删除"));
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TaskDao::getSourceDistribution 失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        SourceStat item;
        item.source = query.value(0).toString();
        item.count = query.value(1).toInt();
        result.append(item);
    }
    return result;
}

QList<MonthlyTrend> TaskDao::getMonthlyTrend(int year)
{
    // 初始化 12 个月，默认 count=0
    QList<MonthlyTrend> result;
    result.reserve(12);
    for (int m = 1; m <= 12; ++m) {
        MonthlyTrend item;
        item.month = m;
        item.count = 0;
        result.append(item);
    }

    QSqlQuery query(db_);
    query.prepare("SELECT strftime('%m', create_time) AS mon, COUNT(*) AS cnt FROM task "
                  "WHERE status != ? AND strftime('%Y', create_time) = ? "
                  "GROUP BY mon ORDER BY mon");
    query.addBindValue(QStringLiteral("已删除"));
    query.addBindValue(QString::number(year));
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TaskDao::getMonthlyTrend 失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        int month = query.value(0).toInt();
        int count = query.value(1).toInt();
        if (month >= 1 && month <= 12) {
            result[month - 1].count = count;
        }
    }
    return result;
}

QList<HeatmapData> TaskDao::getHeatmapData(int year, const QString &dimension)
{
    QList<HeatmapData> result;
    // dimension 为列名，无法参数化，需白名单校验
    QString dimColumn = "create_time";
    if (dimension == QStringLiteral("finish_time")) {
        dimColumn = "finish_time";
    }

    QSqlQuery query(db_);
    query.prepare(QString("SELECT DATE(%1) AS d, COUNT(*) AS cnt FROM task "
                          "WHERE status != ? AND strftime('%Y', %1) = ? "
                          "GROUP BY d ORDER BY d").arg(dimColumn));
    query.addBindValue(QStringLiteral("已删除"));
    query.addBindValue(QString::number(year));
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("TaskDao::getHeatmapData 失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        HeatmapData item;
        item.date = QDate::fromString(query.value(0).toString(), "yyyy-MM-dd");
        item.count = query.value(1).toInt();
        if (item.date.isValid()) {
            result.append(item);
        }
    }
    return result;
}

// ============================================================================
// 私有辅助方法
// ============================================================================

Task TaskDao::taskFromQuery(QSqlQuery &query)
{
    Task task;
    task.id = query.value("id").toInt();
    task.taskNo = query.value("task_no").toString();
    task.title = query.value("title").toString();
    task.category = query.value("category").toString();
    task.source = query.value("source").toString();
    task.tag = query.value("tag").toString();
    task.handler = query.value("handler").toString();
    task.status = query.value("status").toString();
    task.originalStatus = query.value("original_status").toString();
    task.folderName = query.value("folder_name").toString();
    task.folderPath = query.value("folder_path").toString();
    // 布尔字段存储为 INTEGER 0/1
    task.isKeyTask = query.value("is_key_task").toInt() != 0;
    task.hasSubdirs = query.value("has_subdirs").toInt() != 0;
    task.isTodayFocus = query.value("is_today_focus").toInt() != 0;
    task.focusOrder = query.value("focus_order").toInt();
    // deadline 格式 yyyy-MM-dd
    QString deadlineStr = query.value("deadline").toString();
    if (!deadlineStr.isEmpty()) {
        task.deadline = QDate::fromString(deadlineStr, "yyyy-MM-dd");
    }
    task.description = query.value("description").toString();
    task.isFromPeriodic = query.value("is_from_periodic").toInt() != 0;
    task.processingDays = query.value("processing_days").toInt();
    // 时间字段格式 yyyy-MM-dd HH:mm:ss
    QString createTimeStr = query.value("create_time").toString();
    if (!createTimeStr.isEmpty()) {
        task.createTime = QDateTime::fromString(createTimeStr, "yyyy-MM-dd HH:mm:ss");
    }
    QString finishTimeStr = query.value("finish_time").toString();
    if (!finishTimeStr.isEmpty()) {
        task.finishTime = QDateTime::fromString(finishTimeStr, "yyyy-MM-dd HH:mm:ss");
        if (!task.finishTime.isValid()) task.finishTime = QDateTime::fromString(finishTimeStr, Qt::ISODate);
    }
    QString deleteTimeStr = query.value("delete_time").toString();
    if (!deleteTimeStr.isEmpty()) {
        task.deleteTime = QDateTime::fromString(deleteTimeStr, "yyyy-MM-dd HH:mm:ss");
    }
    QString updateTimeStr = query.value("update_time").toString();
    if (!updateTimeStr.isEmpty()) {
        task.updateTime = QDateTime::fromString(updateTimeStr, "yyyy-MM-dd HH:mm:ss");
    }
    return task;
}

QString TaskDao::buildWhereClause(const TaskFilter &filter, QStringList &bindValues)
{
    QStringList parts;

    // 分类
    if (!filter.category.isEmpty()) {
        parts << "category = ?";
        bindValues << filter.category;
    }

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

    // 办理人
    if (filter.handler == QStringLiteral("非本人")) {
        parts << "COALESCE(handler, '') <> ?";
        bindValues << QStringLiteral("本人");
    } else if (!filter.handler.isEmpty()) {
        parts << "handler = ?";
        bindValues << filter.handler;
    }

    // 状态映射（coding.md 5.5）
    if (filter.status == QStringLiteral("已删除")) {
        parts << "status = ?";
        bindValues << QStringLiteral("已删除");
    } else if (!filter.status.isEmpty()) {
        parts << "status = ?";
        bindValues << filter.status;
    } else {
        // 默认排除已删除（除非显式查询已删除）
        parts << "status != ?";
        bindValues << QStringLiteral("已删除");
    }

    // 日期范围（create_time 为 TEXT 格式 yyyy-MM-dd HH:mm:ss，字符串比较有效）
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
        const QStringList ids = AttachmentSearch::matchingIds(db_, "task", parts, bindValues, filter.keyword);
        QString match = "(title LIKE ? OR task_no LIKE ?";
        bindValues << ("%" + filter.keyword + "%") << ("%" + filter.keyword + "%");
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
