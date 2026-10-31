#include "application/periodic/PeriodicTaskUseCase.h"

#include "database/DatabaseManager.h"
#include "database/SourceDictDao.h"
#include "database/TagDictDao.h"
#include "service/PeriodicTaskService.h"

namespace {
template <typename Entry>
QStringList names(const QList<Entry> &entries)
{
    QStringList result;
    for (const Entry &entry : entries) result.append(entry.name);
    return result;
}
}

QList<PeriodicTask> PeriodicTaskUseCase::listAll() const { return PeriodicTaskService().listAll(); }
std::optional<PeriodicTask> PeriodicTaskUseCase::find(int id) const { return PeriodicTaskService().getById(id); }
PeriodicTaskFormOptions PeriodicTaskUseCase::loadFormOptions() const
{
    const QSqlDatabase database = DatabaseManager::instance().database();
    return {names(SourceDictDao(database).findEnabledByRecentUse()),
            names(TagDictDao(database).findEnabledByRecentUse())};
}
TaskResult PeriodicTaskUseCase::create(const PeriodicTask &task) const { return PeriodicTaskService().createTemplate(task); }
TaskResult PeriodicTaskUseCase::update(const PeriodicTask &task) const { return PeriodicTaskService().updateTemplate(task); }
TaskResult PeriodicTaskUseCase::remove(int id) const { return PeriodicTaskService().deleteTemplate(id); }
TaskResult PeriodicTaskUseCase::setEnabled(int id, bool enabled) const { return PeriodicTaskService().setEnabled(id, enabled); }
