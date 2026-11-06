#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QSqlQuery>

#include "DatabaseManager.h"
#include "PathUtil.h"
#include "SettingsView.h"
#include "application/settings/SettingsUseCase.h"
#include "database/SettingsDao.h"
#include "models/ExportConfig.h"

namespace {

bool clearFixture()
{
    QSqlQuery query(DatabaseManager::instance().database());
    return query.exec("DELETE FROM source_dict") && query.exec("DELETE FROM tag_dict")
        && query.exec("DELETE FROM handler_dict") && query.exec("DELETE FROM settings");
}

int dictionaryId(const QString &table, const QString &name)
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(QStringLiteral("SELECT id FROM %1 WHERE name = ?").arg(table));
    query.addBindValue(name);
    return query.exec() && query.next() ? query.value(0).toInt() : -1;
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    const QString root = qEnvironmentVariable("NIUMA_TEST_ROOT");
    if (root.isEmpty()) return 1;
    qputenv("NIUMA_DATA_DIR", QDir(root).filePath("Data").toUtf8());
    if (!QDir().mkpath(root) || !DatabaseManager::instance().initialize(PathUtil::databasePath())) return 2;
    const QStringList expectedBuiltIns = {
        QStringLiteral("征收管理股"), QStringLiteral("风险管理股"), QStringLiteral("税政股"),
        QStringLiteral("纳税服务"), QStringLiteral("法制股"), QStringLiteral("收入核算股"),
        QStringLiteral("党委办公室"), QStringLiteral("人事股"), QStringLiteral("税源管理"),
        QStringLiteral("社保和非税股"), QStringLiteral("办公室")
    };
    const auto builtIns = SettingsUseCase().sources();
    if (builtIns.size() != expectedBuiltIns.size()) return 19;
    for (int i = 0; i < builtIns.size(); ++i) {
        if (!expectedBuiltIns.contains(builtIns.at(i).name)
            || (i > 0 && QString::localeAwareCompare(builtIns.at(i - 1).name, builtIns.at(i).name) > 0)) return 20;
    }
    const auto builtInTags = SettingsUseCase().tags();
    if (builtInTags.size() != 2
        || builtInTags.at(0).name != QStringLiteral("欠税管理")
        && builtInTags.at(1).name != QStringLiteral("欠税管理")) return 21;
    const auto builtInHandlers = SettingsUseCase().handlers();
    if (builtInHandlers.size() != 2
        || builtInHandlers.at(0).name != QStringLiteral("本人")
        || builtInHandlers.at(1).name != QStringLiteral("非本人")) return 28;
    if (!clearFixture()) return 3;

    SettingsUseCase dictionaries;
    dictionaries.addSource(QStringLiteral("Z source"));
    dictionaries.addSource(QStringLiteral("A source"));
    dictionaries.addTag(QStringLiteral("Z tag"));
    dictionaries.addTag(QStringLiteral("A tag"));
    dictionaries.addHandler(QStringLiteral("Z handler"));
    dictionaries.addHandler(QStringLiteral("本人"));
    dictionaries.addHandler(QStringLiteral("A handler"));
    if (dictionaries.sources().first().name != QStringLiteral("A source")
        || dictionaries.tags().first().name != QStringLiteral("A tag")
        || dictionaries.handlers().first().name != QStringLiteral("本人")
        || dictionaries.handlers().at(1).name != QStringLiteral("A handler")) return 22;
    if (!clearFixture()) return 23;

    SettingsView view;
    view.resize(960, 900);
    view.show();
    app.processEvents();
    if (!view.findChild<QLabel *>("settingsTitle") || !view.findChild<QLabel *>("settingsSubtitle")
        || !view.findChild<QFrame *>("fontSettingsCard")
        || !view.findChild<QLabel *>("settingsWorkDirLabel") || !view.findChild<QListWidget *>("settingsSourceList")
        || !view.findChild<QListWidget *>("settingsTagList") || !view.findChild<QListWidget *>("settingsHandlerList")
        || !view.findChild<QPushButton *>("settingsSave") || !view.findChild<QPushButton *>("settingsBackup")
        || !view.findChild<QPushButton *>("settingsInitialize")) return 4;
    if (!view.findChild<QLabel *>("settingsInternalCode")
        || view.findChild<QLabel *>("settingsInternalCode")->text()
            != QStringLiteral("内部代号：铁军贰号")
        || !view.findChild<QLabel *>("settingsVersion")
        || view.findChild<QLabel *>("settingsVersion")->text()
            != QStringLiteral("版本号：V3.2.1")) return 26;
    auto *about = view.findChild<QFrame *>("aboutCard");
    if (!about || !about->grab().save(QDir(root).filePath(QStringLiteral("settings-about-v3.png"))))
        return 27;

    auto *sourceInput = view.findChild<QLineEdit *>("settingsSourceInput");
    auto *tagInput = view.findChild<QLineEdit *>("settingsTagInput");
    auto *handlerInput = view.findChild<QLineEdit *>("settingsHandlerInput");
    if (!sourceInput || !tagInput || !handlerInput) return 5;
    sourceInput->setText(QStringLiteral("S11 来源"));
    tagInput->setText(QStringLiteral("S11 标签"));
    handlerInput->setText(QStringLiteral("S11 办理人"));
    auto *addSource = view.findChild<QPushButton *>("settingsAddSource");
    auto *addTag = view.findChild<QPushButton *>("settingsAddTag");
    auto *addHandler = view.findChild<QPushButton *>("settingsAddHandler");
    if (!addSource || !addTag || !addHandler) return 6;
    addSource->click();
    addTag->click();
    addHandler->click();
    const int sourceId = dictionaryId("source_dict", QStringLiteral("S11 来源"));
    const int tagId = dictionaryId("tag_dict", QStringLiteral("S11 标签"));
    const int handlerId = dictionaryId("handler_dict", QStringLiteral("S11 办理人"));
    if (sourceId < 0 || tagId < 0 || handlerId < 0
        || view.findChild<QListWidget *>("settingsSourceList")->count() != 1
        || view.findChild<QListWidget *>("settingsTagList")->count() != 1
        || view.findChild<QListWidget *>("settingsHandlerList")->count() != 1) return 7;
    for (const QString &objectName : {QStringLiteral("settingsDeleteSource_%1").arg(sourceId),
                                     QStringLiteral("settingsDeleteTag_%1").arg(tagId),
                                     QStringLiteral("settingsDeleteHandler_%1").arg(handlerId)}) {
        auto *button = view.findChild<QPushButton *>(objectName);
        if (!button) return 8;
        button->click();
        app.processEvents();
    }
    if (dictionaryId("source_dict", QStringLiteral("S11 来源")) >= 0
        || dictionaryId("tag_dict", QStringLiteral("S11 标签")) >= 0
        || dictionaryId("handler_dict", QStringLiteral("S11 办理人")) >= 0) return 9;

    auto *reportTitle = view.findChild<QLineEdit *>("settingsReportTitle");
    auto *achievements = view.findChild<QCheckBox *>("settingsIncludeAchievements");
    auto *sortBy = view.findChild<QComboBox *>("settingsSortBy");
    auto *amountUnit = view.findChild<QComboBox *>("settingsAmountUnit");
    if (!reportTitle || !achievements || !sortBy || !amountUnit) return 10;
    auto *fontSize = view.findChild<QComboBox *>("settingsFontSize");
    auto *fontRegular = view.findChild<QRadioButton *>("settingsFontWeightRegular");
    auto *fontMedium = view.findChild<QRadioButton *>("settingsFontWeightMedium");
    auto *fontSemibold = view.findChild<QRadioButton *>("settingsFontWeightSemibold");
    if (!fontSize || !fontRegular || !fontMedium || !fontSemibold
        || fontSize->currentData().toInt() != 14 || !fontRegular->isChecked()) return 17;
    fontSize->setCurrentIndex(fontSize->findData(15));
    fontMedium->click();
    const UiFontPreferences fontPreferences = SettingsUseCase().uiFontPreferences();
    if (fontPreferences.pixelSize != 15 || fontPreferences.weight != UiFontWeight::Medium) return 18;
    reportTitle->setText(QStringLiteral("S11 导出标题"));
    achievements->setChecked(false);
    sortBy->setCurrentIndex(sortBy->findData(QStringLiteral("source")));
    amountUnit->setCurrentIndex(amountUnit->findData(QStringLiteral("wanyuan")));
    if (!QMetaObject::invokeMethod(&view, "onSaveClicked", Qt::DirectConnection)) return 11;
    SettingsDao settingsDao(DatabaseManager::instance().database());
    const ExportConfig saved = ExportConfig::fromJson(settingsDao.getValue(QStringLiteral("export_config")));
    if (saved.reportTitle != QStringLiteral("S11 导出标题") || saved.includeAchievements
        || saved.sortBy != QStringLiteral("source") || saved.amountUnit != QStringLiteral("wanyuan")) return 12;
    if (!QMetaObject::invokeMethod(&view, "onResetDefaultsClicked", Qt::DirectConnection)) return 13;
    const ExportConfig reset = ExportConfig::fromJson(settingsDao.getValue(QStringLiteral("export_config")));
    if (reset.reportTitle != ExportConfig::defaultValue().reportTitle || !reset.includeAchievements) return 14;

    view.setWorkDir(QDir(root).filePath("work-root"));
    if (view.findChild<QLabel *>("settingsWorkDirLabel")->text() != QDir(root).filePath("work-root")) return 15;
    int changeRequests = 0;
    int backupRequests = 0;
    int restoreRequests = 0;
    int scanRequests = 0;
    QObject::connect(&view, &SettingsView::workDirChangeRequested, [&]() { ++changeRequests; });
    QObject::connect(&view, &SettingsView::backupRequested, [&]() { ++backupRequests; });
    QObject::connect(&view, &SettingsView::restoreRequested, [&]() { ++restoreRequests; });
    QObject::connect(&view, &SettingsView::scanImportRequested, [&]() { ++scanRequests; });
    if (!QMetaObject::invokeMethod(&view, "onChangeDirClicked", Qt::DirectConnection)
        || !QMetaObject::invokeMethod(&view, "onBackupClicked", Qt::DirectConnection)
        || !QMetaObject::invokeMethod(&view, "onRestoreClicked", Qt::DirectConnection)
        || !QMetaObject::invokeMethod(&view, "onScanImportClicked", Qt::DirectConnection)
        || changeRequests != 1 || backupRequests != 1 || restoreRequests != 1 || scanRequests != 1) return 16;
    return 0;
}
