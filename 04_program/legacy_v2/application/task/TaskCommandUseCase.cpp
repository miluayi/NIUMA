#include "application/task/TaskCommandUseCase.h"

std::optional<Task> TaskCommandUseCase::findTask(int taskId) const
{
    return TaskService().getTaskById(taskId);
}

TaskResult TaskCommandUseCase::createTask(const Task &task, const QStringList &sourceFiles) const
{
    return TaskService().createTask(task, sourceFiles);
}

TaskResult TaskCommandUseCase::updateTask(const Task &task) const
{
    return TaskService().updateTask(task);
}

TaskResult TaskCommandUseCase::deleteTask(int taskId) const
{
    return TaskService().deleteTask(taskId);
}
