/**
 * @file FocusService.cpp
 * @brief 今日专注业务逻辑层实现：3 任务上限、排序、跨天清零
 *
 * 依赖：database/TaskDao、database/SettingsDao、database/DatabaseManager、
 *       models/Task、utils/Logger
 * 文档：task.md T5（今日专注）、T5.4（跨天清零）、coding.md 第六章
 */

#include "FocusService.h"

#include <QDate>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "DatabaseManager.h"
#include "SettingsDao.h"
#include "TaskDao.h"
#include "utils/Logger.h"

// ============================================================================
// 查询
// ============================================================================

QList<Task> FocusService::getTodayFocus()
{
    TaskDao taskDao(DatabaseManager::instance().database());
    return taskDao.findTodayFocus();
}

// ============================================================================
// 设置专注标记
// ============================================================================

FocusService::FocusResult FocusService::setFocus(int taskId, bool focused)
{
    TaskDao taskDao(DatabaseManager::instance().database());

    if (focused) {
        // 检查上限
        int count = countTodayFocus();
        if (count >= kMaxFocusTasks) {
            Logger::instance().warn(QStringLiteral("FocusService::setFocus 已达上限: taskId=%1 count=%2")
                                        .arg(taskId).arg(count));
            return {false, QStringLiteral("今日专注已达上限（3个）")};
        }

        // 计算 max(focus_order) + 1，追加到末尾
        QSqlQuery query(DatabaseManager::instance().database());
        query.prepare("SELECT MAX(focus_order) FROM task WHERE is_today_focus = 1");
        if (!query.exec()) {
            Logger::instance().error(QStringLiteral("FocusService::setFocus 查询 max(focus_order) 失败: %1")
                                         .arg(query.lastError().text()));
            return {false, QStringLiteral("查询专注排序失败")};
        }
        int maxOrder = 0;
        if (query.next() && !query.isNull(0)) {
            maxOrder = query.value(0).toInt();
        }

        if (!taskDao.updateTodayFocus(taskId, true, maxOrder + 1)) {
            Logger::instance().error(QStringLiteral("FocusService::setFocus 更新失败: taskId=%1").arg(taskId));
            return {false, QStringLiteral("设置专注失败")};
        }

        Logger::instance().info(QStringLiteral("FocusService::setFocus 已加入专注: taskId=%1 order=%2")
                                    .arg(taskId).arg(maxOrder + 1));
        return {true, QString()};
    }

    // 取消专注：始终允许，focus_order 清零
    if (!taskDao.updateTodayFocus(taskId, false, 0)) {
        Logger::instance().error(QStringLiteral("FocusService::setFocus 取消专注失败: taskId=%1").arg(taskId));
        return {false, QStringLiteral("取消专注失败")};
    }

    Logger::instance().info(QStringLiteral("FocusService::setFocus 已移出专注: taskId=%1").arg(taskId));
    return {true, QString()};
}

// ============================================================================
// 排序
// ============================================================================

FocusService::FocusResult FocusService::reorder(int taskId, int newOrder)
{
    TaskDao taskDao(DatabaseManager::instance().database());

    // 当前实现：仅更新指定任务的 focus_order，保留其 is_today_focus=1
    // 完整重排（其他任务顺序联动调整）后续按需实现
    if (!taskDao.updateTodayFocus(taskId, true, newOrder)) {
        Logger::instance().error(QStringLiteral("FocusService::reorder 更新失败: taskId=%1 newOrder=%2")
                                     .arg(taskId).arg(newOrder));
        return {false, QStringLiteral("调整顺序失败")};
    }

    return {true, QString()};
}

// ============================================================================
// 跨天清零
// ============================================================================

void FocusService::clearAllTodayFocus()
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("UPDATE task SET is_today_focus = 0, focus_order = 0 WHERE is_today_focus = 1");
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("FocusService::clearAllTodayFocus 失败: %1")
                                     .arg(query.lastError().text()));
        return;
    }

    int affected = query.numRowsAffected();
    Logger::instance().info(QStringLiteral("FocusService::clearAllTodayFocus 已清零 %1 条专注任务")
                                .arg(affected));

    // 同步更新 last_focus_clear_date
    updateLastClearDate();
}

bool FocusService::isNewDay() const
{
    SettingsDao settingsDao(DatabaseManager::instance().database());
    QString lastStr = settingsDao.getValue(QStringLiteral("last_focus_clear_date"));
    if (lastStr.isEmpty()) {
        // 首次启动或未记录：视为新的一天，触发清零并写入今天
        return true;
    }
    QDate lastDate = QDate::fromString(lastStr, Qt::ISODate);
    if (!lastDate.isValid()) {
        // 格式异常：视为新的一天
        return true;
    }
    return lastDate != QDate::currentDate();
}

void FocusService::updateLastClearDate()
{
    SettingsDao settingsDao(DatabaseManager::instance().database());
    QString today = QDate::currentDate().toString(Qt::ISODate);
    if (!settingsDao.setValue(QStringLiteral("last_focus_clear_date"), today)) {
        Logger::instance().error(QStringLiteral("FocusService::updateLastClearDate 写入失败: %1").arg(today));
    }
}

bool FocusService::clearIfNewDay()
{
    if (!isNewDay()) {
        return false;
    }

    clearAllTodayFocus();
    // clearAllTodayFocus 内部已调用 updateLastClearDate，这里无需重复
    Logger::instance().info(QStringLiteral("今日专注跨天清零已执行"));
    return true;
}

// ============================================================================
// 私有辅助
// ============================================================================

int FocusService::countTodayFocus()
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT COUNT(*) FROM task WHERE is_today_focus = 1 AND status = ?");
    query.addBindValue(QStringLiteral("进行中"));
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("FocusService::countTodayFocus 失败: %1")
                                     .arg(query.lastError().text()));
        return 0;
    }
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}
