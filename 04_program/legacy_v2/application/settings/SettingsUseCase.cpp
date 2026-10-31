#include "application/settings/SettingsUseCase.h"

#include <algorithm>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QStringList>
#include <QSqlQuery>

#include "database/DatabaseManager.h"
#include "database/HandlerDao.h"
#include "database/SettingsDao.h"
#include "database/SourceDictDao.h"
#include "database/TagDictDao.h"
#include "utils/Logger.h"

namespace {

constexpr auto kExportConfigKey = "export_config";
constexpr auto kUiFontPreferencesKey = "ui_font_preferences";

QString fontWeightName(UiFontWeight weight)
{
    switch (weight) {
    case UiFontWeight::Medium: return QStringLiteral("medium");
    case UiFontWeight::Semibold: return QStringLiteral("semibold");
    case UiFontWeight::Regular: return QStringLiteral("regular");
    }
    return QStringLiteral("regular");
}

template <typename Dict, typename Dao>
int insertNext(Dao &dao, QList<Dict> entries, const QString &name)
{
    int nextSort = 1;
    for (const Dict &entry : entries) {
        nextSort = std::max(nextSort, entry.sortOrder + 1);
    }
    Dict entry;
    entry.name = name;
    entry.sortOrder = nextSort;
    return dao.insert(entry);
}

template <typename Dict>
void sortByName(QList<Dict> &entries)
{
    std::sort(entries.begin(), entries.end(), [](const Dict &left, const Dict &right) {
        const int byName = QString::localeAwareCompare(left.name, right.name);
        return byName == 0 ? left.id < right.id : byName < 0;
    });
}

} // namespace

QList<SourceDict> SettingsUseCase::sources() const
{
    auto entries = SourceDictDao(DatabaseManager::instance().database()).findAll();
    sortByName(entries);
    return entries;
}

QList<TagDict> SettingsUseCase::tags() const
{
    auto entries = TagDictDao(DatabaseManager::instance().database()).findAll();
    sortByName(entries);
    return entries;
}

QList<Handler> SettingsUseCase::handlers() const
{
    auto entries = HandlerDao(DatabaseManager::instance().database()).findAll();
    sortByName(entries);
    const auto self = std::find_if(entries.begin(), entries.end(), [](const Handler &entry) {
        return entry.name == QStringLiteral("本人");
    });
    if (self != entries.end()) std::rotate(entries.begin(), self, self + 1);
    return entries;
}

int SettingsUseCase::addSource(const QString &name) const
{
    SourceDictDao dao(DatabaseManager::instance().database());
    return insertNext(dao, dao.findAll(), name);
}

int SettingsUseCase::addTag(const QString &name) const
{
    TagDictDao dao(DatabaseManager::instance().database());
    return insertNext(dao, dao.findAll(), name);
}

int SettingsUseCase::addHandler(const QString &name) const
{
    HandlerDao dao(DatabaseManager::instance().database());
    return insertNext(dao, dao.findAll(), name);
}

bool SettingsUseCase::deleteSource(int id) const
{
    return SourceDictDao(DatabaseManager::instance().database()).deleteById(id);
}

bool SettingsUseCase::deleteTag(int id) const
{
    return TagDictDao(DatabaseManager::instance().database()).deleteById(id);
}

bool SettingsUseCase::deleteHandler(int id) const
{
    return HandlerDao(DatabaseManager::instance().database()).deleteById(id);
}

ExportConfig SettingsUseCase::exportConfig() const
{
    const QString json = SettingsDao(DatabaseManager::instance().database()).getValue(QLatin1String(kExportConfigKey));
    if (!json.isEmpty()) {
        QJsonParseError error;
        QJsonDocument::fromJson(json.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError) {
            Logger::instance().warn(QStringLiteral("SettingsView: 导出配置解析失败，回退默认值: %1")
                                        .arg(error.errorString()));
        }
    }
    return ExportConfig::fromJson(json);
}

bool SettingsUseCase::saveExportConfig(const ExportConfig &config) const
{
    return SettingsDao(DatabaseManager::instance().database()).setValue(QLatin1String(kExportConfigKey), config.toJson());
}

UiFontPreferences SettingsUseCase::uiFontPreferences() const
{
    const QString json = SettingsDao(DatabaseManager::instance().database())
        .getValue(QLatin1String(kUiFontPreferencesKey));
    const QJsonObject stored = QJsonDocument::fromJson(json.toUtf8()).object();
    UiFontPreferences preferences;
    const int size = stored.value(QStringLiteral("pixelSize")).toInt(14);
    if (size == 13 || size == 14 || size == 15) preferences.pixelSize = size;
    const QString weight = stored.value(QStringLiteral("weight")).toString();
    if (weight == QStringLiteral("medium")) preferences.weight = UiFontWeight::Medium;
    else if (weight == QStringLiteral("semibold")) preferences.weight = UiFontWeight::Semibold;
    return preferences;
}

bool SettingsUseCase::saveUiFontPreferences(const UiFontPreferences &preferences) const
{
    if (preferences.pixelSize < 13 || preferences.pixelSize > 15) return false;
    QJsonObject stored;
    stored.insert(QStringLiteral("pixelSize"), preferences.pixelSize);
    stored.insert(QStringLiteral("weight"), fontWeightName(preferences.weight));
    return SettingsDao(DatabaseManager::instance().database()).setValue(
        QLatin1String(kUiFontPreferencesKey), QString::fromUtf8(QJsonDocument(stored).toJson(QJsonDocument::Compact)));
}

QString SettingsUseCase::workDir() const
{
    return SettingsDao(DatabaseManager::instance().database()).getValue(QStringLiteral("work_dir"));
}

QString SettingsUseCase::lastBackupTime() const
{
    return SettingsDao(DatabaseManager::instance().database()).getValue(QStringLiteral("last_backup_date"));
}

void SettingsUseCase::resetAllData() const
{
    QSqlQuery query(DatabaseManager::instance().database());
    const QStringList tables = {
        QStringLiteral("task"), QStringLiteral("task_remark"), QStringLiteral("reference_material"),
        QStringLiteral("achievement"), QStringLiteral("operation_log"), QStringLiteral("periodic_task"),
        QStringLiteral("source_dict"), QStringLiteral("tag_dict"), QStringLiteral("handler_dict")
    };
    for (const QString &table : tables) {
        query.exec(QStringLiteral("DELETE FROM %1").arg(table));
    }
    query.exec(QStringLiteral("DELETE FROM settings"));
    query.exec(QStringLiteral("DELETE FROM sqlite_sequence"));
}
