#include "application/task/TaskQueryUseCase.h"

#include <QCollator>
#include <QLocale>
#include <algorithm>

#include "database/DatabaseManager.h"
#include "database/HandlerDao.h"
#include "database/SourceDictDao.h"
#include "database/TagDictDao.h"
#include "database/TaskDao.h"
#include "service/TaskService.h"

namespace {
template <typename Dictionary>
QStringList names(const QList<Dictionary> &entries)
{
    QStringList result;
    result.reserve(entries.size());
    for (const Dictionary &entry : entries) result.append(entry.name);
    return result;
}
}

TaskListData TaskQueryUseCase::loadList(const TaskFilter &filter) const
{
    TaskService service;
    TaskDao taskDao(DatabaseManager::instance().database());
    TaskListData result;
    result.tasks = service.getTasksByFilter(filter);
    result.total = service.countTasksByFilter(filter);
    result.sources = taskDao.getDistinctSources();
    result.tags = taskDao.getDistinctTags();
    QStringList otherHandlers;
    const QStringList enabledHandlers = names(HandlerDao(DatabaseManager::instance().database()).findEnabledByRecentUse());
    for (const QString &handler : enabledHandlers + taskDao.getDistinctHandlers()) {
        if (!handler.trimmed().isEmpty() && handler != QStringLiteral("本人")
            && handler != QStringLiteral("非本人") && !otherHandlers.contains(handler)) otherHandlers.append(handler);
    }
    const QCollator collator(QLocale(QLocale::Chinese, QLocale::China));
    std::sort(otherHandlers.begin(), otherHandlers.end(), [&collator](const QString &left, const QString &right) {
        const int comparison = collator.compare(left, right);
        return comparison == 0 ? left < right : comparison < 0;
    });
    result.handlers << QStringLiteral("本人") << QStringLiteral("非本人");
    result.handlers.append(otherHandlers);
    result.years = taskDao.getDistinctYears();
    return result;
}

TaskFormOptions TaskQueryUseCase::loadFormOptions() const
{
    const QSqlDatabase database = DatabaseManager::instance().database();
    SourceDictDao sourceDao(database);
    TagDictDao tagDao(database);
    HandlerDao handlerDao(database);
    return {names(sourceDao.findEnabledByRecentUse()), names(tagDao.findEnabledByRecentUse()),
            names(handlerDao.findEnabledByRecentUse())};
}
