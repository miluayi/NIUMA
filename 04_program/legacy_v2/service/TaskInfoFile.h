#pragma once

#include <QString>
#include <optional>

#include "models/Task.h"
#include "models/Achievement.h"

struct TaskInfoSnapshot {
    Task task;
    std::optional<Achievement> achievement;
};

// A recoverable copy of task details stored beside the task's attachments.
class TaskInfoFile {
public:
    static QString path(const QString &folderPath);
    static bool write(const Task &task);
    static std::optional<Task> read(const QString &folderPath);
    static std::optional<TaskInfoSnapshot> readSnapshot(const QString &folderPath);
    static bool syncAchievement(const Task &task);
    static bool reconcileAchievement(const Task &task);
};
