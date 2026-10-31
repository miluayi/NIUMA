/**
 * @file AchievementDao.cpp
 * @brief 成果数据访问层实现：achievement 表的 CRUD、按年份查询、统计
 *
 * 依赖：QtSql、models/Achievement、utils/Logger
 * 文档：TD 2.3（achievement 表）、TD 3.x（AchievementDao 接口）、coding.md 第五章（数据库访问规范）
 *
 * 关键约束：
 *  - 全部 SQL 使用参数化查询（QSqlQuery::prepare + addBindValue），禁止字符串拼接
 *  - is_annual 和 is_key_achievement 存储为 INTEGER（0/1），读取时转为 bool
 *  - result_amount 为 qint64（分），禁止 float/REAL
 *  - create_time 存储为统一格式字符串 "yyyy-MM-dd HH:mm:ss"
 *  - 按年份查询使用 SUBSTR(create_time, 1, 4) = ? 匹配年份前缀
 */

#include "AchievementDao.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "utils/DateUtil.h"
#include "utils/Logger.h"

// ============================================================================
// 构造
// ============================================================================

AchievementDao::AchievementDao(QSqlDatabase db)
    : db_(db)
{
}

// ============================================================================
// 单条查询
// ============================================================================

std::optional<Achievement> AchievementDao::findById(int id)
{
    QSqlQuery query(db_);
    query.prepare("SELECT * FROM achievement WHERE id = ?");
    query.addBindValue(id);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("AchievementDao::findById 失败: %1").arg(query.lastError().text()));
        return std::nullopt;
    }
    if (query.next()) {
        return achievementFromQuery(query);
    }
    return std::nullopt;
}

std::optional<Achievement> AchievementDao::findByTaskId(int taskId)
{
    QSqlQuery query(db_);
    query.prepare("SELECT * FROM achievement WHERE task_id = ?");
    query.addBindValue(taskId);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("AchievementDao::findByTaskId 失败: %1").arg(query.lastError().text()));
        return std::nullopt;
    }
    if (query.next()) {
        return achievementFromQuery(query);
    }
    return std::nullopt;
}

// ============================================================================
// 列表查询
// ============================================================================

QList<Achievement> AchievementDao::findByYear(int year)
{
    QList<Achievement> result;
    QSqlQuery query(db_);
    // create_time 格式 "yyyy-MM-dd HH:mm:ss"，SUBSTR 取前 4 位为年份
    query.prepare("SELECT * FROM achievement WHERE SUBSTR(create_time, 1, 4) = ? "
                  "ORDER BY create_time DESC");
    query.addBindValue(QString::number(year));
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("AchievementDao::findByYear 失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        result.append(achievementFromQuery(query));
    }
    return result;
}

QList<Achievement> AchievementDao::findAll()
{
    QList<Achievement> result;
    QSqlQuery query(db_);
    query.prepare("SELECT * FROM achievement ORDER BY create_time DESC");
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("AchievementDao::findAll 失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        result.append(achievementFromQuery(query));
    }
    return result;
}

int AchievementDao::countByYear(int year)
{
    QSqlQuery query(db_);
    query.prepare("SELECT COUNT(*) FROM achievement WHERE SUBSTR(create_time, 1, 4) = ?");
    query.addBindValue(QString::number(year));
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("AchievementDao::countByYear 失败: %1").arg(query.lastError().text()));
        return 0;
    }
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

// ============================================================================
// 插入 / 更新 / 删除
// ============================================================================

int AchievementDao::insert(const Achievement &achievement)
{
    QSqlQuery query(db_);
    query.prepare(
        "INSERT INTO achievement ("
        "  task_id, work_content, work_object, result_count, result_amount, "
        "  description, is_annual, is_key_achievement, create_time"
        ") VALUES ("
        "  ?, ?, ?, ?, ?, ?, ?, ?, ?"
        ")");
    query.addBindValue(achievement.taskId);
    query.addBindValue(achievement.workContent);
    query.addBindValue(achievement.workObject);
    query.addBindValue(achievement.resultCount);
    query.addBindValue(achievement.resultAmount);  // qint64 分
    query.addBindValue(achievement.description);
    query.addBindValue(achievement.isAnnual ? 1 : 0);
    query.addBindValue(achievement.isKeyAchievement ? 1 : 0);
    // create_time 统一格式 "yyyy-MM-dd HH:mm:ss"
    query.addBindValue(DateUtil::currentDateTimeStr());

    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("AchievementDao::insert 失败: %1").arg(query.lastError().text()));
        return -1;
    }
    QVariant lastId = query.lastInsertId();
    if (lastId.isValid()) {
        return lastId.toInt();
    }
    return -1;
}

bool AchievementDao::update(const Achievement &achievement)
{
    QSqlQuery query(db_);
    query.prepare(
        "UPDATE achievement SET "
        "  task_id = ?, work_content = ?, work_object = ?, result_count = ?, "
        "  result_amount = ?, description = ?, is_annual = ?, is_key_achievement = ? "
        "WHERE id = ?");
    query.addBindValue(achievement.taskId);
    query.addBindValue(achievement.workContent);
    query.addBindValue(achievement.workObject);
    query.addBindValue(achievement.resultCount);
    query.addBindValue(achievement.resultAmount);  // qint64 分
    query.addBindValue(achievement.description);
    query.addBindValue(achievement.isAnnual ? 1 : 0);
    query.addBindValue(achievement.isKeyAchievement ? 1 : 0);
    query.addBindValue(achievement.id);

    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("AchievementDao::update 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool AchievementDao::deleteByTaskId(int taskId)
{
    // 物理删除（外键 ON DELETE CASCADE 会随 task 删除自动清理，此方法提供显式删除入口）
    QSqlQuery query(db_);
    query.prepare("DELETE FROM achievement WHERE task_id = ?");
    query.addBindValue(taskId);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("AchievementDao::deleteByTaskId 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool AchievementDao::deleteById(int id)
{
    // 物理删除
    QSqlQuery query(db_);
    query.prepare("DELETE FROM achievement WHERE id = ?");
    query.addBindValue(id);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("AchievementDao::deleteById 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

// ============================================================================
// 统计
// ============================================================================

qint64 AchievementDao::sumAmountByYear(int year)
{
    QSqlQuery query(db_);
    query.prepare("SELECT COALESCE(SUM(result_amount), 0) FROM achievement "
                  "WHERE SUBSTR(create_time, 1, 4) = ?");
    query.addBindValue(QString::number(year));
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("AchievementDao::sumAmountByYear 失败: %1").arg(query.lastError().text()));
        return 0;
    }
    if (query.next()) {
        return query.value(0).toLongLong();
    }
    return 0;
}

int AchievementDao::countKeyAchievementByYear(int year)
{
    QSqlQuery query(db_);
    query.prepare("SELECT COUNT(*) FROM achievement "
                  "WHERE SUBSTR(create_time, 1, 4) = ? AND is_key_achievement = 1");
    query.addBindValue(QString::number(year));
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("AchievementDao::countKeyAchievementByYear 失败: %1").arg(query.lastError().text()));
        return 0;
    }
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

QList<QPair<QString, int>> AchievementDao::categoryCountByYear(int year)
{
    QList<QPair<QString, int>> result;
    QSqlQuery query(db_);
    // JOIN task 表获取 category 字段，按分类分组计数
    query.prepare(
        "SELECT t.category, COUNT(*) AS cnt "
        "FROM achievement a "
        "JOIN task t ON a.task_id = t.id "
        "WHERE SUBSTR(a.create_time, 1, 4) = ? "
        "GROUP BY t.category "
        "ORDER BY cnt DESC");
    query.addBindValue(QString::number(year));
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("AchievementDao::categoryCountByYear 失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        QString category = query.value(0).toString();
        int count = query.value(1).toInt();
        result.append(qMakePair(category, count));
    }
    return result;
}

QList<QPair<QString, int>> AchievementDao::sourceCountByYear(int year)
{
    QList<QPair<QString, int>> result;
    QSqlQuery query(db_);
    // JOIN task 表获取 source 字段，按来源分组计数
    query.prepare(
        "SELECT t.source, COUNT(*) AS cnt "
        "FROM achievement a "
        "JOIN task t ON a.task_id = t.id "
        "WHERE SUBSTR(a.create_time, 1, 4) = ? "
        "GROUP BY t.source "
        "ORDER BY cnt DESC");
    query.addBindValue(QString::number(year));
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("AchievementDao::sourceCountByYear 失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        QString source = query.value(0).toString();
        int count = query.value(1).toInt();
        result.append(qMakePair(source, count));
    }
    return result;
}

// ============================================================================
// 私有辅助方法
// ============================================================================

Achievement AchievementDao::achievementFromQuery(QSqlQuery &query)
{
    Achievement achievement;
    achievement.id = query.value("id").toInt();
    achievement.taskId = query.value("task_id").toInt();
    achievement.workContent = query.value("work_content").toString();
    achievement.workObject = query.value("work_object").toString();
    achievement.resultCount = query.value("result_count").toInt();
    // result_amount 为 qint64（分），使用 toLongLong 避免精度丢失
    achievement.resultAmount = query.value("result_amount").toLongLong();
    achievement.description = query.value("description").toString();
    // 布尔字段存储为 INTEGER 0/1
    achievement.isAnnual = query.value("is_annual").toInt() != 0;
    achievement.isKeyAchievement = query.value("is_key_achievement").toInt() != 0;
    // 时间字段格式 "yyyy-MM-dd HH:mm:ss"
    QString createTimeStr = query.value("create_time").toString();
    if (!createTimeStr.isEmpty()) {
        achievement.createTime = DateUtil::parseDateTime(createTimeStr);
    }
    return achievement;
}
