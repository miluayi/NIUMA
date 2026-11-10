#include "application/achievement/AchievementUseCase.h"
#include "database/AchievementDao.h"
#include "database/DatabaseManager.h"
#include "database/TaskDao.h"
#include "database/SettingsDao.h"
#include "models/ExportConfig.h"
#include "service/TaskInfoFile.h"
#include <QDir>

namespace {
template <typename Mutation>
int persistAchievement(int taskId, Mutation mutate)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    const auto task = TaskDao(db).findById(taskId);
    if (!task || !db.transaction()) return -1;

    AchievementDao dao(db);
    const int result = mutate(dao);
    const bool hasFolder = !task->folderPath.isEmpty() && QDir(task->folderPath).exists();
    if (result <= 0 || (hasFolder && !TaskInfoFile::syncAchievement(*task))) {
        db.rollback();
        return -1;
    }
    if (!db.commit()) {
        db.rollback();
        if (hasFolder) TaskInfoFile::syncAchievement(*task);
        return -1;
    }
    return result;
}
} // namespace

QList<Achievement> AchievementUseCase::findByYear(int year) const
{
    return AchievementDao(DatabaseManager::instance().database()).findByYear(year);
}

QList<AchievementListItem> AchievementUseCase::listByYear(int year) const
{
    const QList<Achievement> achievements = findByYear(year);
    TaskDao tasks(DatabaseManager::instance().database());
    QList<AchievementListItem> items;
    items.reserve(achievements.size());
    for (const Achievement &achievement : achievements) {
        const auto task = tasks.findById(achievement.taskId);
        items.append({achievement, task ? task->taskNo : QString(),
                      task ? task->category : QString()});
    }
    return items;
}

AchievementYearStats AchievementUseCase::stats(int year) const
{
    AchievementDao achievements(DatabaseManager::instance().database());
    TaskDao tasks(DatabaseManager::instance().database());
    return {achievements.countByYear(year), achievements.sumAmountByYear(year),
            achievements.countKeyAchievementByYear(year),
            tasks.countByStatus(QStringLiteral("已完成"))};
}

int AchievementUseCase::create(const Achievement &achievement) const
{
    return persistAchievement(achievement.taskId, [&](AchievementDao &dao) {
        return dao.insert(achievement);
    });
}

bool AchievementUseCase::update(const Achievement &achievement) const
{
    return persistAchievement(achievement.taskId, [&](AchievementDao &dao) {
        return dao.update(achievement);
    }) > 0;
}

bool AchievementUseCase::remove(int achievementId) const
{
    const auto record = AchievementDao(DatabaseManager::instance().database()).findById(achievementId);
    return record && persistAchievement(record->taskId, [&](AchievementDao &dao) {
        return dao.deleteById(achievementId);
    }) > 0;
}

WordExport::ExportResult AchievementUseCase::exportAnnualReport(int year, const QString &outputPath) const
{
    const QString configJson = SettingsDao(DatabaseManager::instance().database()).getValue("export_config");
    const ExportConfig config = configJson.isEmpty()
        ? ExportConfig::defaultValue()
        : ExportConfig::fromJson(configJson);
    return WordExport::exportAnnualReport(config, outputPath, year);
}
