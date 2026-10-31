/**
 * @file TaskService.cpp
 * @brief 任务业务逻辑层实现：编排 TaskDao/TaskRemarkDao/OperationLogDao 完成任务生命周期管理
 *
 * 依赖：database/TaskDao、database/TaskRemarkDao、database/OperationLogDao、
 *       database/DatabaseManager、models/Task、models/TaskRemark、models/TaskFilter、
 *       utils/TaskNumberGenerator、utils/DateUtil、utils/Logger
 * 文档：task.md T2.1、PRD 14.5（状态转换规则）、coding.md 第六章（编号/日期规则）
 */

#include "TaskService.h"

#include "DatabaseManager.h"
#include "FileService.h"
#include "OperationLogDao.h"
#include "TaskDao.h"
#include "TaskRemarkDao.h"
#include "TaskInfoFile.h"

#include <QDir>

#include "utils/DateUtil.h"
#include "utils/Logger.h"
#include "utils/TaskNumberGenerator.h"

namespace {
void syncInfo(const Task &task)
{
    if (!task.folderPath.isEmpty() && !TaskInfoFile::write(task)) {
        Logger::instance().warn(QStringLiteral("任务信息文件写入失败: %1").arg(task.folderPath));
    }
}

bool moveAndUpdate(Task &task, const Task &before, TaskDao &dao)
{
    FileService files;
    if (!files.moveTaskFolderForStatusChange(task, before.status)) return false;
    if (!dao.update(task)) {
        if (task.folderPath != before.folderPath && !task.folderPath.isEmpty()
            && !QDir().rename(task.folderPath, before.folderPath)) {
            Logger::instance().error(QStringLiteral("任务目录回退失败: %1").arg(task.folderPath));
        }
        return false;
    }
    syncInfo(task);
    return true;
}
}

// ============================================================================
// 核心任务管理
// ============================================================================

TaskResult TaskService::createTask(const Task &task, const QStringList &sourceFiles)
{
    Task newTask = task;

    // 生成编号
    newTask.taskNo = TaskNumberGenerator::nextTaskNo();

    // 根据办理人设置状态：本人→进行中，非本人→他人办理
    if (newTask.handler == QStringLiteral("本人")) {
        newTask.status = QStringLiteral("进行中");
    } else {
        newTask.status = QStringLiteral("他人办理");
        if (newTask.handler.trimmed().isEmpty())
            newTask.handler = QStringLiteral("非本人");
    }

    // 插入数据库（create_time 由 TaskDao::insert 内部写入）
    TaskDao taskDao(DatabaseManager::instance().database());
    int newId = taskDao.insert(newTask);
    if (newId <= 0) {
        QString detail = taskDao.lastError();
        Logger::instance().error(QStringLiteral("TaskService::createTask 插入失败: %1 | SQL错误: %2")
                                     .arg(newTask.title, detail));
        return {false, QStringLiteral("任务创建失败：%1").arg(detail), 0};
    }

    newTask.id = newId;

    // 创建任务目录 + 复制文件
    FileService fileService;
    const bool folderCreated = fileService.createTaskFolder(newTask);

    // 生成 folderName/folderPath 并更新数据库
    newTask.folderName = fileService.buildTaskFolderName(newTask);
    QString workDir = fileService.getWorkDir();
    if (!workDir.isEmpty() && folderCreated) {
        newTask.folderPath = fileService.buildTaskFolderPath(newTask, workDir);
        taskDao.update(newTask);
        const auto persisted = taskDao.findById(newId);
        if (persisted) syncInfo(*persisted);
    }

    // 复制源文件到任务目录（后台异步，避免大文件阻塞 UI）
    if (!sourceFiles.isEmpty() && !newTask.folderPath.isEmpty()) {
        FileService::copyFilesToTaskDirAsync(sourceFiles, newTask.folderPath);
    }

    // 记录操作日志
    OperationLogDao logDao(DatabaseManager::instance().database());
    logDao.logTaskAction(newId, QStringLiteral("create"),
                         QStringLiteral("创建任务: %1").arg(newTask.title));

    Logger::instance().info(QStringLiteral("TaskService::createTask 成功: id=%1 taskNo=%2")
                                .arg(newId).arg(newTask.taskNo));

    return {true, QString(), newId};
}

TaskResult TaskService::updateTask(const Task &task)
{
    Task newTask = task;

    TaskDao taskDao(DatabaseManager::instance().database());

    // 查询旧任务数据（检查办理人是否变更）
    auto oldTask = taskDao.findById(newTask.id);
    if (!oldTask) {
        Logger::instance().warn(QStringLiteral("TaskService::updateTask 任务不存在: id=%1").arg(newTask.id));
        return {false, QStringLiteral("任务不存在"), 0};
    }

    // 以数据库中的路径为准，避免编辑窗口回传过期目录信息。
    newTask.folderPath = oldTask->folderPath;
    newTask.folderName = oldTask->folderName;
    FileService files;
    // 办理人变更可能需要在两个工作区之间移动目录。
    if ((oldTask->handler == QStringLiteral("本人")) != (newTask.handler == QStringLiteral("本人"))) {
        const bool toSelf = newTask.handler == QStringLiteral("本人");
        newTask.status = toSelf ? QStringLiteral("进行中") : QStringLiteral("他人办理");
        if (!newTask.folderPath.isEmpty()
            && !files.moveTaskFolderForHandlerChange(newTask,
                toSelf ? FileService::kDirInProgress : FileService::kDirOtherHandler)) {
            return {false, QStringLiteral("任务目录移动失败，请等待附件复制完成或关闭占用的文件后重试"), 0};
        }
    }
    if (newTask.status == QStringLiteral("他人办理") && newTask.handler.trimmed().isEmpty())
        newTask.handler = QStringLiteral("非本人");

    if (!files.renameTaskFolder(newTask)) {
        if (newTask.folderPath != oldTask->folderPath && !newTask.folderPath.isEmpty())
            QDir().rename(newTask.folderPath, oldTask->folderPath);
        return {false, QStringLiteral("任务目录重命名失败，请关闭占用文件后重试"), 0};
    }

    // 更新任务
    if (!taskDao.update(newTask)) {
        if (newTask.folderPath != oldTask->folderPath && !newTask.folderPath.isEmpty()
            && !QDir().rename(newTask.folderPath, oldTask->folderPath)) {
            Logger::instance().error(QStringLiteral("办理人变更后目录回退失败: %1").arg(newTask.folderPath));
        }
        Logger::instance().error(QStringLiteral("TaskService::updateTask 更新失败: id=%1").arg(newTask.id));
        return {false, QStringLiteral("更新任务失败"), 0};
    }
    syncInfo(newTask);

    // 记录操作日志
    OperationLogDao logDao(DatabaseManager::instance().database());
    logDao.logTaskAction(newTask.id, QStringLiteral("update"),
                         QStringLiteral("更新任务: %1").arg(newTask.title));

    return {true, QString(), newTask.id};
}

TaskResult TaskService::completeTask(int taskId)
{
    TaskDao taskDao(DatabaseManager::instance().database());

    // 查询任务
    auto opt = taskDao.findById(taskId);
    if (!opt) {
        Logger::instance().warn(QStringLiteral("TaskService::completeTask 任务不存在: id=%1").arg(taskId));
        return {false, QStringLiteral("任务不存在"), 0};
    }

    // 验证状态：必须为进行中
    Task task = *opt;
    if (!task.isActive()) {
        return {false, QStringLiteral("任务状态不是进行中，无法完成"), 0};
    }

    // 计算办理天数 = daysBetween(createTime, now) + 1（当天完成 = 1 天）
    QDateTime now = QDateTime::currentDateTime();
    task.processingDays = DateUtil::daysBetween(task.createTime, now) + 1;

    // 设置完成字段
    task.status = QStringLiteral("已完成");
    task.finishTime = now;

    // 先移动目录，再写数据库；失败时保持原状态。
    if (!moveAndUpdate(task, *opt, taskDao)) {
        Logger::instance().error(QStringLiteral("TaskService::completeTask 更新失败: id=%1").arg(taskId));
        return {false, QStringLiteral("任务目录移动失败，请等待附件复制完成或关闭占用的文件后重试"), 0};
    }

    // 记录操作日志
    OperationLogDao logDao(DatabaseManager::instance().database());
    logDao.logTaskAction(taskId, QStringLiteral("complete"),
                         QStringLiteral("完成任务: %1").arg(task.title));

    return {true, QString(), taskId};
}

TaskResult TaskService::cancelComplete(int taskId)
{
    TaskDao taskDao(DatabaseManager::instance().database());

    auto opt = taskDao.findById(taskId);
    if (!opt) {
        Logger::instance().warn(QStringLiteral("TaskService::cancelComplete 任务不存在: id=%1").arg(taskId));
        return {false, QStringLiteral("任务不存在"), 0};
    }

    // 验证状态：必须为已完成
    Task task = *opt;
    if (!task.isCompleted()) {
        return {false, QStringLiteral("任务状态不是已完成，无法取消完成"), 0};
    }

    // 恢复进行中，清空 finish_time
    task.status = QStringLiteral("进行中");
    task.finishTime = QDateTime();

    if (!moveAndUpdate(task, *opt, taskDao)) {
        Logger::instance().error(QStringLiteral("TaskService::cancelComplete 更新失败: id=%1").arg(taskId));
        return {false, QStringLiteral("任务目录移动失败，请等待附件复制完成或关闭占用的文件后重试"), 0};
    }

    // 记录操作日志
    OperationLogDao logDao(DatabaseManager::instance().database());
    logDao.logTaskAction(taskId, QStringLiteral("cancel_complete"),
                         QStringLiteral("取消完成: %1").arg(task.title));

    return {true, QString(), taskId};
}

TaskResult TaskService::deleteTask(int taskId)
{
    TaskDao taskDao(DatabaseManager::instance().database());

    auto opt = taskDao.findById(taskId);
    if (!opt) {
        Logger::instance().warn(QStringLiteral("TaskService::deleteTask 任务不存在: id=%1").arg(taskId));
        return {false, QStringLiteral("任务不存在"), 0};
    }

    // 验证状态：不能重复删除
    Task task = *opt;
    if (task.isDeleted()) {
        return {false, QStringLiteral("任务已删除"), 0};
    }

    // 保存原状态，设置已删除
    task.originalStatus = task.status;
    task.status = QStringLiteral("已删除");
    task.deleteTime = QDateTime::currentDateTime();

    // 原目录确实遗失时，允许仅将数据库记录移入回收站。
    if (!task.folderPath.isEmpty() && !QDir(task.folderPath).exists()) {
        Logger::instance().warn(QStringLiteral("删除任务时目录已遗失: %1").arg(task.folderPath));
        task.folderPath.clear();
        task.folderName.clear();
    }
    if (!moveAndUpdate(task, *opt, taskDao)) {
        Logger::instance().error(QStringLiteral("TaskService::deleteTask 更新失败: id=%1").arg(taskId));
        return {false, QStringLiteral("任务目录移动失败，请等待附件复制完成或关闭占用的文件后重试"), 0};
    }

    // 记录操作日志
    OperationLogDao logDao(DatabaseManager::instance().database());
    logDao.logTaskAction(taskId, QStringLiteral("delete"),
                         QStringLiteral("删除任务: %1").arg(task.title));

    return {true, QString(), taskId};
}

TaskResult TaskService::restoreTask(int taskId)
{
    TaskDao taskDao(DatabaseManager::instance().database());

    auto opt = taskDao.findById(taskId);
    if (!opt) {
        Logger::instance().warn(QStringLiteral("TaskService::restoreTask 任务不存在: id=%1").arg(taskId));
        return {false, QStringLiteral("任务不存在"), 0};
    }

    // 验证状态：必须为已删除
    Task task = *opt;
    if (!task.isDeleted()) {
        return {false, QStringLiteral("任务不在回收站，无法恢复"), 0};
    }

    // 恢复原状态，清空 original_status 和 delete_time
    // 注意：先保存 originalStatus 用于目录移动，再清空（FileService 依赖 task.status 决定目标目录）
    task.status = task.originalStatus.isEmpty() ? QStringLiteral("进行中") : task.originalStatus;
    task.originalStatus.clear();
    task.deleteTime = QDateTime();

    if (!moveAndUpdate(task, *opt, taskDao)) {
        Logger::instance().error(QStringLiteral("TaskService::restoreTask 更新失败: id=%1").arg(taskId));
        return {false, QStringLiteral("任务目录移动失败，请等待附件复制完成或关闭占用的文件后重试"), 0};
    }

    // 记录操作日志
    OperationLogDao logDao(DatabaseManager::instance().database());
    logDao.logTaskAction(taskId, QStringLiteral("restore"),
                         QStringLiteral("恢复任务: %1").arg(task.title));

    return {true, QString(), taskId};
}

TaskResult TaskService::permanentDelete(int taskId)
{
    TaskDao taskDao(DatabaseManager::instance().database());

    // 查询任务（获取 folderPath 用于删除物理目录）
    auto opt = taskDao.findById(taskId);
    if (opt) {
        // 删除物理目录（若存在）
        FileService fileService;
        fileService.deleteTaskFolder(*opt);
    }

    // 物理删除（备注由外键 CASCADE 清理，日志由 SET NULL 保留）
    if (!taskDao.deleteById(taskId)) {
        Logger::instance().error(QStringLiteral("TaskService::permanentDelete 删除失败: id=%1").arg(taskId));
        return {false, QStringLiteral("物理删除失败"), 0};
    }

    Logger::instance().info(QStringLiteral("TaskService::permanentDelete 成功: id=%1").arg(taskId));

    return {true, QString(), taskId};
}

int TaskService::cleanExpiredDeleted(int retentionDays)
{
    TaskDao taskDao(DatabaseManager::instance().database());
    QList<Task> deletedTasks = taskDao.findDeleted();

    int cleanedCount = 0;
    QDate today = QDate::currentDate();

    for (const Task &task : deletedTasks) {
        if (!task.deleteTime.isValid()) {
            continue;
        }

        // 计算距今天数：today - deleteTime.date()
        int daysSinceDelete = task.deleteTime.date().daysTo(today);

        // 超过保留天数则永久删除
        if (daysSinceDelete > retentionDays) {
            permanentDelete(task.id);
            ++cleanedCount;
        }
    }

    if (cleanedCount > 0) {
        Logger::instance().info(
            QStringLiteral("TaskService::cleanExpiredDeleted 清理了 %1 条过期任务").arg(cleanedCount));
    }

    return cleanedCount;
}

TaskResult TaskService::updateKeyTask(int taskId, bool isKey)
{
    TaskDao taskDao(DatabaseManager::instance().database());

    if (!taskDao.updateKeyTask(taskId, isKey)) {
        Logger::instance().error(QStringLiteral("TaskService::updateKeyTask 更新失败: id=%1").arg(taskId));
        return {false, QStringLiteral("更新重点任务失败"), 0};
    }
    if (const auto updated = taskDao.findById(taskId)) syncInfo(*updated);

    // 记录操作日志
    OperationLogDao logDao(DatabaseManager::instance().database());
    logDao.logTaskAction(taskId, QStringLiteral("update_key_task"),
                         QStringLiteral("更新重点任务标记: %1")
                             .arg(isKey ? QStringLiteral("是") : QStringLiteral("否")));

    return {true, QString(), taskId};
}

// ============================================================================
// 备注管理
// ============================================================================

QList<TaskRemark> TaskService::getRemarks(int taskId)
{
    TaskRemarkDao remarkDao(DatabaseManager::instance().database());
    return remarkDao.findByTaskId(taskId);
}

TaskResult TaskService::addRemark(int taskId, const QString &content)
{
    TaskRemarkDao remarkDao(DatabaseManager::instance().database());

    TaskRemark remark;
    remark.taskId = taskId;
    remark.content = content;

    int newId = remarkDao.insert(remark);
    if (newId <= 0) {
        Logger::instance().error(QStringLiteral("TaskService::addRemark 插入失败: taskId=%1").arg(taskId));
        return {false, QStringLiteral("添加备注失败"), 0};
    }

    return {true, QString(), taskId};
}

TaskResult TaskService::updateRemark(int remarkId, const QString &content)
{
    TaskRemarkDao remarkDao(DatabaseManager::instance().database());

    TaskRemark remark;
    remark.id = remarkId;
    remark.content = content;

    if (!remarkDao.update(remark)) {
        Logger::instance().error(QStringLiteral("TaskService::updateRemark 更新失败: remarkId=%1").arg(remarkId));
        return {false, QStringLiteral("更新备注失败"), 0};
    }

    return {true, QString(), 0};
}

TaskResult TaskService::deleteRemark(int remarkId)
{
    TaskRemarkDao remarkDao(DatabaseManager::instance().database());

    if (!remarkDao.deleteById(remarkId)) {
        Logger::instance().error(QStringLiteral("TaskService::deleteRemark 删除失败: remarkId=%1").arg(remarkId));
        return {false, QStringLiteral("删除备注失败"), 0};
    }

    return {true, QString(), 0};
}

// ============================================================================
// 查询方法（委托 TaskDao）
// ============================================================================

std::optional<Task> TaskService::getTaskById(int id)
{
    TaskDao taskDao(DatabaseManager::instance().database());
    return taskDao.findById(id);
}

std::optional<Task> TaskService::getTaskByNo(const QString &taskNo)
{
    TaskDao taskDao(DatabaseManager::instance().database());
    return taskDao.findByTaskNo(taskNo);
}

QList<Task> TaskService::getTasksByFilter(const TaskFilter &filter)
{
    TaskDao taskDao(DatabaseManager::instance().database());
    return taskDao.findByFilter(filter);
}

int TaskService::countTasksByFilter(const TaskFilter &filter)
{
    TaskDao taskDao(DatabaseManager::instance().database());
    return taskDao.countByFilter(filter);
}
