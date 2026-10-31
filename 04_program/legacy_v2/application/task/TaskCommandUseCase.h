#pragma once

#include <optional>

#include "models/Task.h"
#include "service/TaskService.h"

class TaskCommandUseCase {
public:
    std::optional<Task> findTask(int taskId) const;
    TaskResult createTask(const Task &task, const QStringList &sourceFiles) const;
    TaskResult updateTask(const Task &task) const;
    TaskResult deleteTask(int taskId) const;
};
