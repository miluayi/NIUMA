#pragma once

#include <QList>
#include <optional>

#include "models/Achievement.h"
#include "models/OperationLog.h"
#include "models/Task.h"
#include "models/TaskRemark.h"
#include "service/TaskService.h"

class TaskDetailUseCase {
public:
    std::optional<Task> findTask(int taskId) const;
    QList<TaskRemark> findRemarks(int taskId) const;
    QList<OperationLog> findHistory(int taskId) const;
    std::optional<Achievement> findAchievement(int taskId) const;

    TaskResult completeTask(int taskId) const;
    TaskResult cancelComplete(int taskId) const;
    TaskResult deleteTask(int taskId) const;
    TaskResult addRemark(int taskId, const QString &content) const;
    TaskResult deleteRemark(int remarkId) const;
};
