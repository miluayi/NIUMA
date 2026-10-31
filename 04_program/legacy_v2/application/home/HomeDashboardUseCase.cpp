#include "application/home/HomeDashboardUseCase.h"

#include <QDate>

#include "database/DatabaseManager.h"
#include "database/TaskDao.h"
#include "models/TaskFilter.h"
#include "service/FocusService.h"
#include "service/PeriodicTaskService.h"

HomeDashboard HomeDashboardUseCase::load(int inProgressPage) const
{
    HomeDashboard dashboard;
    FocusService focusService;
    PeriodicTaskService periodicTaskService;
    TaskDao taskDao(DatabaseManager::instance().database());

    dashboard.focusTasks = focusService.getTodayFocus();

    TaskFilter filter;
    filter.status = QStringLiteral("进行中");
    filter.page = inProgressPage;
    filter.pageSize = 10;
    filter.sortByDeadline = true;
    dashboard.inProgressTasks = taskDao.findByFilter(filter);
    dashboard.inProgressTotal = taskDao.countByFilter(filter);
    const QList<PeriodicReminderItem> reminders = periodicTaskService.findDueReminders();
    dashboard.periodicReminders.reserve(reminders.size());
    for (const PeriodicReminderItem &reminder : reminders) {
        dashboard.periodicReminders.append({reminder.templateId, reminder.name,
                                            reminder.nextReminderDate, reminder.dueToday});
    }

    const QDate today = QDate::currentDate();
    const QDate thisMonthStart(today.year(), today.month(), 1);
    const QDate thisMonthEnd = thisMonthStart.addMonths(1).addDays(-1);
    const QList<Task> thisMonthTasks = taskDao.findByDateRange(thisMonthStart, thisMonthEnd);
    dashboard.monthlyStats.total = thisMonthTasks.size();
    for (const Task &task : thisMonthTasks) {
        if (task.isCompleted()) ++dashboard.monthlyStats.completed;
    }
    if (dashboard.monthlyStats.total > 0) {
        dashboard.monthlyStats.completionRate =
            dashboard.monthlyStats.completed * 100 / dashboard.monthlyStats.total;
    }

    const QDate lastMonthStart = thisMonthStart.addMonths(-1);
    const QDate lastMonthEnd = thisMonthStart.addDays(-1);
    dashboard.monthlyStats.comparedToLastMonth = dashboard.monthlyStats.total
        - taskDao.findByDateRange(lastMonthStart, lastMonthEnd).size();
    return dashboard;
}
