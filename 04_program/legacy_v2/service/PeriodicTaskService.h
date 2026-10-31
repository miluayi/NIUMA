/**
 * @file PeriodicTaskService.h
 * @brief 周期任务业务逻辑层（无状态），编排 PeriodicTaskDao / TaskService 完成模板到任务的转换
 *
 * 依赖：database/PeriodicTaskDao、database/DatabaseManager、service/TaskService、
 *       models/PeriodicTask、models/Task、utils/TaskNumberGenerator、utils/Logger
 * 文档：Phase 7、PRD 9.3、coding.md 第六章
 *
 * 关键约束：
 *  - PeriodicTaskService 为普通 C++ 类，不继承 QObject，不持有 db_ 成员，所有 DAO 在方法内构造
 *  - 严禁 UI 直接操作 DAO/DB，必须通过 Service 层
 *  - createTaskFromTemplate 根据办理人设置 task.status（本人→进行中，否则→他人办理）
 *  - 模板字段映射：name→title, category, source, tag, handler；isFromPeriodic=true
 *  - 任务编号由 TaskService::createTask 内部生成
 */
#pragma once

#include <QString>

#include "models/PeriodicTask.h"
#include "service/TaskService.h"

/**
 * @brief 周期任务提醒项（用于首页提醒区域展示）
 */
struct PeriodicReminderItem {
    int templateId = 0;
    QString name;
    QString category;
    QString source;
    QString handler;
    QDate nextReminderDate;
    bool dueToday = false;  // true=今日到期, false=未来7天到期
};

class PeriodicTaskService {
public:
    PeriodicTaskService() = default;
    ~PeriodicTaskService() = default;

    /**
     * @brief 从周期任务模板创建实际任务
     * @param periodicTaskId 周期任务模板 ID
     * @return TaskResult，成功时 taskId 为新任务 ID
     *
     * 流程：
     * 1. 通过 PeriodicTaskDao 查询模板
     * 2. 构造 Task（字段从模板映射）
     * 3. 通过 TaskService::createTask 入库（编号、状态、操作日志由其处理）
     * 4. 更新模板的 last_reminded_date 为今天
     */
    TaskResult createTaskFromTemplate(int periodicTaskId);

    /**
     * @brief 忽略本次提醒，推进到下一周期
     * @param periodicTaskId 周期任务模板 ID
     * @return TaskResult
     *
     * 流程：更新模板的 last_reminded_date 为今天，使下次提醒日期变为下一周期
     */
    TaskResult ignoreCurrent(int periodicTaskId);

    /**
     * @brief 查询到期提醒（今日到期 + 未来7天到期，2 个层级）
     * @return 提醒项列表，按到期日期升序排列
     */
    QList<PeriodicReminderItem> findDueReminders();

    /**
     * @brief 获取启用的周期任务模板列表
     */
    QList<PeriodicTask> listEnabled();

    /**
     * @brief 获取全部周期任务模板列表
     */
    QList<PeriodicTask> listAll();

    /**
     * @brief 查询单个周期任务模板
     */
    std::optional<PeriodicTask> getById(int id);

    /**
     * @brief 创建周期任务模板
     * @return 成功时 taskId 为新模板 ID
     */
    TaskResult createTemplate(const PeriodicTask &tmpl);

    /**
     * @brief 更新周期任务模板
     */
    TaskResult updateTemplate(const PeriodicTask &tmpl);

    /**
     * @brief 删除周期任务模板
     */
    TaskResult deleteTemplate(int id);

    /**
     * @brief 启用/禁用周期任务模板
     */
    TaskResult setEnabled(int id, bool enabled);

private:
    // 将模板字段映射为 Task 字段
    Task taskFromTemplate(const PeriodicTask &tmpl);
};
