/**
 * @file PeriodicTaskService.cpp
 * @brief 周期任务业务逻辑层实现：模板到任务的转换、模板 CRUD
 *
 * 依赖：service/PeriodicTaskService.h、service/TaskService.h、
 *       database/PeriodicTaskDao、database/DatabaseManager、
 *       models/PeriodicTask、models/Task、utils/Logger
 * 文档：Phase 7、PRD 9.3、coding.md 第六章
 */

#include "PeriodicTaskService.h"

#include <QDate>
#include <algorithm>

#include "DatabaseManager.h"
#include "PeriodicTaskDao.h"
#include "TaskService.h"

#include "utils/Logger.h"

// ============================================================================
// 模板到任务的转换
// ============================================================================

Task PeriodicTaskService::taskFromTemplate(const PeriodicTask &tmpl)
{
    Task task;
    // 模板字段映射
    task.title = tmpl.name;
    task.category = tmpl.category;
    task.source = tmpl.source;
    task.tag = tmpl.tag;
    // handler 为 null（未设置）时使用默认"本人"；空字符串 "" 表示"他人办理"，保留
    task.handler = tmpl.handler.isNull() ? QStringLiteral("本人") : tmpl.handler;

    // 状态由 TaskService::createTask 根据办理人设置（本人→进行中，否则→他人办理）
    // 此处不显式设置 status

    // 标记为来自周期任务
    task.isFromPeriodic = true;

    return task;
}

// ============================================================================
// 核心方法：从模板创建任务
// ============================================================================

TaskResult PeriodicTaskService::createTaskFromTemplate(int periodicTaskId)
{
    PeriodicTaskDao dao(DatabaseManager::instance().database());
    auto tmpl = dao.findById(periodicTaskId);
    if (!tmpl.has_value()) {
        Logger::instance().warn(
            QStringLiteral("PeriodicTaskService::createTaskFromTemplate 模板不存在: id=%1").arg(periodicTaskId));
        return {false, QStringLiteral("周期任务模板不存在"), 0};
    }

    // 构造 Task
    Task task = taskFromTemplate(tmpl.value());

    // 通过 TaskService 创建任务（含编号生成、状态设置、操作日志、目录创建）
    TaskService taskService;
    TaskResult result = taskService.createTask(task);
    if (!result.ok) {
        return result;
    }

    // 更新模板的 last_reminded_date 为今天
    dao.updateLastRemindedDate(periodicTaskId, QDate::currentDate());

    Logger::instance().info(
        QStringLiteral("PeriodicTaskService::createTaskFromTemplate 成功: tmplId=%1 newTaskId=%2")
            .arg(periodicTaskId).arg(result.taskId));

    return result;
}

// ============================================================================
// 忽略本次提醒
// ============================================================================

TaskResult PeriodicTaskService::ignoreCurrent(int periodicTaskId)
{
    PeriodicTaskDao dao(DatabaseManager::instance().database());
    auto tmpl = dao.findById(periodicTaskId);
    if (!tmpl.has_value()) {
        Logger::instance().warn(
            QStringLiteral("PeriodicTaskService::ignoreCurrent 模板不存在: id=%1").arg(periodicTaskId));
        return {false, QStringLiteral("周期任务模板不存在"), 0};
    }

    // 更新 last_reminded_date 为今天，使下次提醒日期推进到下一周期
    dao.updateLastRemindedDate(periodicTaskId, QDate::currentDate());

    Logger::instance().info(
        QStringLiteral("PeriodicTaskService::ignoreCurrent 成功: tmplId=%1 推进到下一周期").arg(periodicTaskId));

    return {true, QString(), periodicTaskId};
}

// ============================================================================
// 查询到期提醒（今日到期 + 未来7天到期）
// ============================================================================

QList<PeriodicReminderItem> PeriodicTaskService::findDueReminders()
{
    QList<PeriodicReminderItem> result;

    PeriodicTaskDao dao(DatabaseManager::instance().database());
    QList<PeriodicTask> templates = dao.findAllEnabled();

    QDate today = QDate::currentDate();
    QDate todayPlus7 = today.addDays(7);

    for (const PeriodicTask &tmpl : templates) {
        QDate nextDate = tmpl.calcNextReminderDate();
        if (!nextDate.isValid()) {
            continue;
        }

        PeriodicReminderItem item;
        item.templateId = tmpl.id;
        item.name = tmpl.name;
        item.category = tmpl.category;
        item.source = tmpl.source;
        item.handler = tmpl.handler;
        item.nextReminderDate = nextDate;

        if (nextDate <= today) {
            // 今日到期（含过期未处理）
            item.dueToday = true;
            result.append(item);
        } else if (nextDate <= todayPlus7) {
            // 未来7天到期
            item.dueToday = false;
            result.append(item);
        }
        // 超过7天的不展示
    }

    // 按到期日期升序排列（今日到期在前，再按日期排序）
    std::sort(result.begin(), result.end(),
              [](const PeriodicReminderItem &a, const PeriodicReminderItem &b) {
                  if (a.dueToday != b.dueToday) {
                      return a.dueToday;  // true(今日到期) 排在前
                  }
                  return a.nextReminderDate < b.nextReminderDate;
              });

    return result;
}

// ============================================================================
// 模板查询
// ============================================================================

QList<PeriodicTask> PeriodicTaskService::listEnabled()
{
    PeriodicTaskDao dao(DatabaseManager::instance().database());
    return dao.findAllEnabled();
}

QList<PeriodicTask> PeriodicTaskService::listAll()
{
    PeriodicTaskDao dao(DatabaseManager::instance().database());
    return dao.findAll();
}

std::optional<PeriodicTask> PeriodicTaskService::getById(int id)
{
    PeriodicTaskDao dao(DatabaseManager::instance().database());
    return dao.findById(id);
}

// ============================================================================
// 模板 CRUD
// ============================================================================

TaskResult PeriodicTaskService::createTemplate(const PeriodicTask &tmpl)
{
    PeriodicTaskDao dao(DatabaseManager::instance().database());
    int newId = dao.insert(tmpl);
    if (newId <= 0) {
        return {false, QStringLiteral("创建周期任务模板失败"), 0};
    }
    Logger::instance().info(
        QStringLiteral("PeriodicTaskService::createTemplate 成功: id=%1 name=%2").arg(newId).arg(tmpl.name));
    return {true, QString(), newId};
}

TaskResult PeriodicTaskService::updateTemplate(const PeriodicTask &tmpl)
{
    PeriodicTaskDao dao(DatabaseManager::instance().database());
    if (!dao.update(tmpl)) {
        return {false, QStringLiteral("更新周期任务模板失败"), 0};
    }
    Logger::instance().info(
        QStringLiteral("PeriodicTaskService::updateTemplate 成功: id=%1").arg(tmpl.id));
    return {true, QString(), tmpl.id};
}

TaskResult PeriodicTaskService::deleteTemplate(int id)
{
    PeriodicTaskDao dao(DatabaseManager::instance().database());
    if (!dao.deleteById(id)) {
        return {false, QStringLiteral("删除周期任务模板失败"), 0};
    }
    Logger::instance().info(
        QStringLiteral("PeriodicTaskService::deleteTemplate 成功: id=%1").arg(id));
    return {true, QString(), id};
}

TaskResult PeriodicTaskService::setEnabled(int id, bool enabled)
{
    PeriodicTaskDao dao(DatabaseManager::instance().database());
    if (!dao.setEnabled(id, enabled)) {
        return {false, QStringLiteral("修改启用状态失败"), 0};
    }
    return {true, QString(), id};
}
