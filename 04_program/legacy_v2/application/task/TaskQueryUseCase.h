#pragma once

#include <QList>
#include <QStringList>

#include "models/Task.h"
#include "models/TaskFilter.h"

struct TaskListData {
    QList<Task> tasks;
    int total = 0;
    QStringList sources;
    QStringList tags;
    QStringList handlers;
    QList<int> years;
};

struct TaskFormOptions {
    QStringList sources;
    QStringList tags;
    QStringList handlers;
};

class TaskQueryUseCase {
public:
    TaskListData loadList(const TaskFilter &filter) const;
    TaskFormOptions loadFormOptions() const;
};
