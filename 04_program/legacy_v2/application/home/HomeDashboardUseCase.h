#pragma once

#include <QDate>
#include <QList>

#include "models/Task.h"

struct HomeMonthlyStats {
    int total = 0;
    int completed = 0;
    int completionRate = 0;
    int comparedToLastMonth = 0;
};

struct HomePeriodicReminder {
    int templateId = 0;
    QString name;
    QDate nextReminderDate;
    bool dueToday = false;
};

struct HomeDashboard {
    QList<Task> focusTasks;
    QList<Task> inProgressTasks;
    int inProgressTotal = 0;
    QList<HomePeriodicReminder> periodicReminders;
    HomeMonthlyStats monthlyStats;
};

class HomeDashboardUseCase {
public:
    HomeDashboard load(int inProgressPage) const;
};
