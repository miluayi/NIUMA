#pragma once

#include <QList>
#include <QStringList>
#include <optional>

#include "models/PeriodicTask.h"
#include "service/TaskService.h"

struct PeriodicTaskFormOptions {
    QStringList sources;
    QStringList tags;
};

class PeriodicTaskUseCase {
public:
    QList<PeriodicTask> listAll() const;
    std::optional<PeriodicTask> find(int id) const;
    PeriodicTaskFormOptions loadFormOptions() const;
    TaskResult create(const PeriodicTask &task) const;
    TaskResult update(const PeriodicTask &task) const;
    TaskResult remove(int id) const;
    TaskResult setEnabled(int id, bool enabled) const;
};
