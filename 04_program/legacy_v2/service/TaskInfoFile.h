#pragma once

#include <QString>
#include <optional>

#include "models/Task.h"

// A recoverable copy of task details stored beside the task's attachments.
class TaskInfoFile {
public:
    static QString path(const QString &folderPath);
    static bool write(const Task &task);
    static std::optional<Task> read(const QString &folderPath);
};
