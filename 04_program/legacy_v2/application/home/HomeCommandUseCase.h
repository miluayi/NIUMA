#pragma once

#include <QString>

struct HomeCommandResult {
    bool ok = false;
    QString error;
};

class HomeCommandUseCase {
public:
    HomeCommandResult setFocus(int taskId, bool focused) const;
    HomeCommandResult createTaskFromPeriodicTemplate(int templateId) const;
    HomeCommandResult ignorePeriodicReminder(int templateId) const;
};
