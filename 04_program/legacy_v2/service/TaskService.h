/**
 * @file TaskService.h
 * @brief 任务业务逻辑层（无状态），编排 TaskDao/TaskRemarkDao/OperationLogDao 完成任务生命周期管理
 *
 * 依赖：database/TaskDao、database/TaskRemarkDao、database/OperationLogDao、
 *       database/DatabaseManager、models/Task、models/TaskRemark、models/TaskFilter、
 *       utils/TaskNumberGenerator、utils/DateUtil、utils/Logger
 * 文档：task.md T2.1、PRD 14.5（状态转换规则）、coding.md 第六章（编号/日期规则）
 *
 * 关键约束：
 *  - TaskService 为普通 C++ 类，不继承 QObject，不持有 db_ 成员，所有 DAO 在方法内构造
 *  - 严禁 UI 直接操作 DAO/DB，必须通过 Service 层
 *  - 状态转换：进行中→已完成(completeTask)、已完成→进行中(cancelComplete)、
 *    任意→已删除(deleteTask,保存 original_status)、已删除→original_status(restoreTask)
 *  - 办理人变更：本人→非本人 status='他人办理'、非本人→本人 status='进行中'
 *  - 办理天数 = daysBetween(createTime, now) + 1（当天完成 = 1 天）
 *  - 文件目录移动由 FileService 负责（Phase 3 实现），当前用 TODO(phase3) 标注
 */
#pragma once

#include <QList>
#include <QString>
#include <QStringList>
#include <optional>

#include "models/Task.h"
#include "models/TaskFilter.h"
#include "models/TaskRemark.h"

/**
 * @brief 业务方法返回值
 */
struct TaskResult {
    bool ok = false;        // 是否成功
    QString error;          // 失败时的错误信息
    int taskId = 0;         // 成功时返回任务 ID
};

/**
 * @brief 任务业务逻辑层（无状态）
 *
 * 编排 TaskDao/TaskRemarkDao/OperationLogDao 完成任务的创建、更新、状态转换、
 * 备注管理和查询。所有 DAO 在每个方法内构造，不持有成员指针。
 *
 * 用法：
 *   TaskService service;
 *   TaskResult result = service.createTask(task);
 */
class TaskService {
public:
    TaskService() = default;
    ~TaskService() = default;

    // ---- 核心任务管理 ----

    /**
     * @brief 创建任务
     * @param task 任务数据（taskNo/status 由 Service 层设置，createTime 由 DAO 写入）
     * @param sourceFiles 源文件列表（Phase 3 实现后台复制，当前仅保留接口）
     * @return TaskResult，成功时 taskId 为新任务 ID
     */
    TaskResult createTask(const Task &task, const QStringList &sourceFiles = {});

    /**
     * @brief 更新任务（检测办理人变更并自动切换状态）
     * @param task 任务数据（需包含 id）
     * @return TaskResult
     */
    TaskResult updateTask(const Task &task);

    /**
     * @brief 完成任务（进行中→已完成）
     * @param taskId 任务 ID
     * @return TaskResult
     */
    TaskResult completeTask(int taskId);

    /**
     * @brief 取消完成（已完成→进行中）
     * @param taskId 任务 ID
     * @return TaskResult
     */
    TaskResult cancelComplete(int taskId);

    /**
     * @brief 删除任务（任意状态→已删除，保存 original_status）
     * @param taskId 任务 ID
     * @return TaskResult
     */
    TaskResult deleteTask(int taskId);

    /**
     * @brief 恢复任务（已删除→original_status，清空 original_status 和 delete_time）
     * @param taskId 任务 ID
     * @return TaskResult
     */
    TaskResult restoreTask(int taskId);

    /**
     * @brief 物理删除任务（备注由外键 CASCADE 清理，日志由 SET NULL 保留）
     *
     * 同时调用 FileService::deleteTaskFolder 删除物理目录（若存在）。
     * @param taskId 任务 ID
     * @return TaskResult
     */
    TaskResult permanentDelete(int taskId);

    /**
     * @brief 清理过期已删除任务（回收站 30 天自动清理）
     * @param retentionDays 保留天数（默认 30），deleteTime 距今天数 > 此值则永久删除
     * @return 实际清理的任务数
     *
     * 遍历 findDeleted() 列表，对每个任务计算 today - deleteTime.date() 天数，
     * 若天数 > retentionDays 则调用 permanentDelete 永久删除（含物理目录）。
     */
    int cleanExpiredDeleted(int retentionDays = 30);

    /**
     * @brief 更新重点任务标记
     * @param taskId 任务 ID
     * @param isKey 是否为重点任务
     * @return TaskResult
     */
    TaskResult updateKeyTask(int taskId, bool isKey);

    // ---- 备注管理 ----

    /** @brief 获取任务备注（按创建时间正序） */
    QList<TaskRemark> getRemarks(int taskId);

    /** @brief 添加备注 */
    TaskResult addRemark(int taskId, const QString &content);

    /** @brief 更新备注内容 */
    TaskResult updateRemark(int remarkId, const QString &content);

    /** @brief 删除备注 */
    TaskResult deleteRemark(int remarkId);

    // ---- 查询方法（委托 TaskDao） ----

    /** @brief 按 ID 查询任务 */
    std::optional<Task> getTaskById(int id);

    /** @brief 按编号查询任务 */
    std::optional<Task> getTaskByNo(const QString &taskNo);

    /** @brief 按筛选条件查询任务列表 */
    QList<Task> getTasksByFilter(const TaskFilter &filter);

    /** @brief 按筛选条件查询任务总数 */
    int countTasksByFilter(const TaskFilter &filter);

};
