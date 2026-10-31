#include "application/recyclebin/RecycleBinUseCase.h"

#include "database/DatabaseManager.h"
#include "database/TaskDao.h"

QList<Task> RecycleBinUseCase::listDeleted() const
{
    return TaskDao(DatabaseManager::instance().database()).findDeleted();
}

TaskResult RecycleBinUseCase::restoreTask(int taskId) const
{
    return TaskService().restoreTask(taskId);
}

TaskResult RecycleBinUseCase::permanentlyDeleteTask(int taskId) const
{
    return TaskService().permanentDelete(taskId);
}

int RecycleBinUseCase::restoreTasks(const QList<int> &taskIds) const
{
    TaskService service;
    int restored = 0;
    for (int taskId : taskIds) {
        if (service.restoreTask(taskId).ok) {
            ++restored;
        }
    }
    return restored;
}

int RecycleBinUseCase::permanentlyDeleteTasks(const QList<int> &taskIds) const
{
    TaskService service;
    int deleted = 0;
    for (int taskId : taskIds) {
        if (service.permanentDelete(taskId).ok) {
            ++deleted;
        }
    }
    return deleted;
}
