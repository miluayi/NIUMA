#include "application/task/TaskDetailUseCase.h"

#include "database/AchievementDao.h"
#include "database/DatabaseManager.h"
#include "database/OperationLogDao.h"

std::optional<Task> TaskDetailUseCase::findTask(int taskId) const
{
    return TaskService().getTaskById(taskId);
}

QList<TaskRemark> TaskDetailUseCase::findRemarks(int taskId) const
{
    return TaskService().getRemarks(taskId);
}

QList<OperationLog> TaskDetailUseCase::findHistory(int taskId) const
{
    return OperationLogDao(DatabaseManager::instance().database()).findByTaskId(taskId);
}

std::optional<Achievement> TaskDetailUseCase::findAchievement(int taskId) const
{
    return AchievementDao(DatabaseManager::instance().database()).findByTaskId(taskId);
}

TaskResult TaskDetailUseCase::completeTask(int taskId) const
{
    return TaskService().completeTask(taskId);
}

TaskResult TaskDetailUseCase::cancelComplete(int taskId) const
{
    return TaskService().cancelComplete(taskId);
}

TaskResult TaskDetailUseCase::deleteTask(int taskId) const
{
    return TaskService().deleteTask(taskId);
}

TaskResult TaskDetailUseCase::addRemark(int taskId, const QString &content) const
{
    return TaskService().addRemark(taskId, content);
}

TaskResult TaskDetailUseCase::deleteRemark(int remarkId) const
{
    return TaskService().deleteRemark(remarkId);
}
