#pragma once

#include <QList>

#include "models/Task.h"
#include "service/TaskService.h"

class RecycleBinUseCase {
public:
    QList<Task> listDeleted() const;
    TaskResult restoreTask(int taskId) const;
    TaskResult permanentlyDeleteTask(int taskId) const;
    int restoreTasks(const QList<int> &taskIds) const;
    int permanentlyDeleteTasks(const QList<int> &taskIds) const;
};
