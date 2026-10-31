/**
 * @file SettingsDao.cpp
 * @brief 系统设置数据访问层实现：settings 表的读写
 *
 * 依赖：QtSql、utils/Logger、utils/DateUtil
 * 文档：task.md T8.10（SettingsDao 实现）、TD 2.7（settings 表 schema）、TD 3.6.1（SettingsDao 接口）、coding.md 第五章（数据库访问规范）
 *
 * 实现说明：
 *  - 全部 SQL 使用参数化查询（QSqlQuery::prepare + addBindValue），禁止字符串拼接
 *  - setValue 采用 INSERT OR REPLACE 语义：key 存在则更新 value 与 update_time，不存在则插入新行
 *  - getValue 找不到 key 时静默返回 defaultValue，不记录错误日志
 *  - 时间格式：yyyy-MM-dd HH:mm:ss（通过 DateUtil::currentDateTimeStr 获取）
 *  - 所有写操作失败时调用 Logger::instance().error() 记录并返回 false
 */

#include "SettingsDao.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "utils/DateUtil.h"
#include "utils/Logger.h"

// ============================================================================
// 构造
// ============================================================================

SettingsDao::SettingsDao(QSqlDatabase db)
    : db_(db)
{
}

// ============================================================================
// 查询
// ============================================================================

QString SettingsDao::getValue(const QString &key, const QString &defaultValue) const
{
    QSqlQuery query(db_);
    query.prepare("SELECT value FROM settings WHERE key = ?");
    query.addBindValue(key);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("SettingsDao::getValue 失败: %1").arg(query.lastError().text()));
        return defaultValue;
    }
    if (query.next()) {
        return query.value(0).toString();
    }
    // key 不存在，静默返回 defaultValue（不报错）
    return defaultValue;
}

bool SettingsDao::hasKey(const QString &key) const
{
    QSqlQuery query(db_);
    query.prepare("SELECT 1 FROM settings WHERE key = ? LIMIT 1");
    query.addBindValue(key);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("SettingsDao::hasKey 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.next();
}

// ============================================================================
// 写入 / 删除
// ============================================================================

bool SettingsDao::setValue(const QString &key, const QString &value)
{
    QString now = DateUtil::currentDateTimeStr();

    QSqlQuery query(db_);
    // INSERT OR REPLACE：key 存在则更新 value 与 update_time，不存在则插入
    query.prepare("INSERT OR REPLACE INTO settings (key, value, update_time) VALUES (?, ?, ?)");
    query.addBindValue(key);
    query.addBindValue(value);
    query.addBindValue(now);  // update_time

    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("SettingsDao::setValue 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return true;
}

bool SettingsDao::removeKey(const QString &key)
{
    QSqlQuery query(db_);
    query.prepare("DELETE FROM settings WHERE key = ?");
    query.addBindValue(key);
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("SettingsDao::removeKey 失败: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}
