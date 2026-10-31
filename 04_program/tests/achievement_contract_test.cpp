#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QSqlQuery>
#include <QTableView>
#include <QTextEdit>
#include <QTimer>
#include <QShortcut>

#include "AchievementDialog.h"
#include "AchievementView.h"
#include "application/achievement/AchievementUseCase.h"
#include "DatabaseManager.h"
#include "SettingsDao.h"
#include "TaskDetailView.h"
#include "database/AchievementDao.h"
#include "models/ExportConfig.h"
#include "models/Task.h"
#include "service/FileService.h"
#include "service/TaskService.h"
#include "utils/PathUtil.h"
#include "utils/WordExport.h"

namespace {

TaskResult createTask(const QString &title)
{
    Task task;
    task.title = title;
    task.category = QStringLiteral("报");
    task.source = QStringLiteral("S08 来源");
    task.handler = QStringLiteral("本人");
    task.description = QStringLiteral("S08 任务说明");
    TaskService service;
    return service.createTask(task);
}

void saveTriggeredAchievement()
{
    QTimer::singleShot(0, []() {
        auto *dialog = qobject_cast<AchievementDialog *>(QApplication::activeModalWidget());
        if (!dialog) return;
        dialog->findChild<QLineEdit *>(QStringLiteral("achievementWorkContentEdit"))
            ->setText(QStringLiteral("S08 完成触发成果"));
        dialog->findChild<QLineEdit *>(QStringLiteral("achievementResultCountEdit"))->setText(QStringLiteral("2"));
        dialog->findChild<QLineEdit *>(QStringLiteral("achievementResultAmountEdit"))->setText(QStringLiteral("9.50"));
        dialog->findChild<QCheckBox *>(QStringLiteral("achievementAnnualCheck"))->setChecked(true);
        QMetaObject::invokeMethod(dialog, "onSaveClicked", Qt::DirectConnection);
    });
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    const QString root = qEnvironmentVariable("NIUMA_TEST_ROOT");
    if (root.isEmpty()) return 1;
    qputenv("NIUMA_DATA_DIR", QDir(root).filePath("Data").toUtf8());
    if (!QDir().mkpath(root) || !DatabaseManager::instance().initialize(PathUtil::databasePath())) return 2;
    QSqlQuery query(DatabaseManager::instance().database());
    if (!query.exec("DELETE FROM achievement") || !query.exec("DELETE FROM task")) return 3;
    FileService fileService;
    const QString workRoot = QDir(root).filePath(QStringLiteral("work-root"));
    if (QDir(workRoot).exists() && !QDir(workRoot).removeRecursively()) return 27;
    if (!fileService.initializeWorkDir(workRoot).success) return 4;

    TaskResult directTask = createTask(QStringLiteral("S08 直接登记任务"));
    if (!directTask.ok) return 5;
    AchievementDialog skipped(directTask.taskId, QStringLiteral("报"), QStringLiteral("S08 直接登记任务"));
    skipped.show();
    app.processEvents();
    QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QApplication::sendEvent(&skipped, &escape);
    if (skipped.result() != QDialog::Rejected || skipped.getAchievement().has_value()) return 6;

    AchievementDialog dialog(directTask.taskId, QStringLiteral("报"), QStringLiteral("S08 直接登记任务"));
    dialog.show();
    app.processEvents();
    auto *content = dialog.findChild<QLineEdit *>(QStringLiteral("achievementWorkContentEdit"));
    auto *amount = dialog.findChild<QLineEdit *>(QStringLiteral("achievementResultAmountEdit"));
    auto *description = dialog.findChild<QTextEdit *>(QStringLiteral("achievementDescriptionEdit"));
    auto *annual = dialog.findChild<QCheckBox *>(QStringLiteral("achievementAnnualCheck"));
    auto *key = dialog.findChild<QCheckBox *>(QStringLiteral("achievementKeyCheck"));
    auto *saveShortcut = dialog.findChild<QShortcut *>(QStringLiteral("shortcutSaveAchievement"));
    if (!content || !amount || !description || !annual || !key || dialog.minimumWidth() != 560
        || !saveShortcut || saveShortcut->key() != QKeySequence::Save
        || description->height() != 72) return 7;
    content->setText(QStringLiteral("S08 成果内容"));
    amount->setText(QStringLiteral("12.34"));
    dialog.findChild<QLineEdit *>(QStringLiteral("achievementResultCountEdit"))->setText(QStringLiteral("3"));
    annual->setChecked(true);
    key->setChecked(true);
    if (!QMetaObject::invokeMethod(saveShortcut, "activated", Qt::DirectConnection)) return 8;
    AchievementDao achievementDao(DatabaseManager::instance().database());
    auto directAchievement = achievementDao.findByTaskId(directTask.taskId);
    if (!directAchievement.has_value() || directAchievement->resultAmount != 1234
        || !directAchievement->isAnnual || !directAchievement->isKeyAchievement) return 9;
    TaskService taskService;
    if (!taskService.completeTask(directTask.taskId).ok) return 10;

    TaskResult triggerTask = createTask(QStringLiteral("S08 完成触发任务"));
    if (!triggerTask.ok) return 11;
    TaskDetailView detail(triggerTask.taskId);
    detail.show();
    saveTriggeredAchievement();
    if (!QMetaObject::invokeMethod(&detail, "onCompleteClicked", Qt::DirectConnection)) return 12;
    auto completedTask = taskService.getTaskById(triggerTask.taskId);
    auto triggeredAchievement = achievementDao.findByTaskId(triggerTask.taskId);
    if (!completedTask.has_value() || !completedTask->isCompleted() || !triggeredAchievement.has_value()
        || triggeredAchievement->resultAmount != 950 || !triggeredAchievement->isAnnual) return 13;

    AchievementView view;
    view.show();
    view.refresh();
    app.processEvents();
    auto *table = view.findChild<QTableView *>(QStringLiteral("achievementTable"));
    auto *total = view.findChild<QLabel *>(QStringLiteral("achievementTotalStat"));
    auto *amountStat = view.findChild<QLabel *>(QStringLiteral("achievementAmountStat"));
    auto *keyStat = view.findChild<QLabel *>(QStringLiteral("achievementKeyStat"));
    if (!table || !total || !amountStat || !keyStat || table->model()->rowCount() != 2
        || total->text() != QStringLiteral("2") || amountStat->text() != QStringLiteral("¥21.84")
        || keyStat->text() != QStringLiteral("1")) return 14;
    if (table->model()->index(0, 1).data(Qt::TextAlignmentRole).toInt() != Qt::AlignCenter
        || table->model()->index(0, 2).data(Qt::TextAlignmentRole).toInt() != Qt::AlignCenter) return 26;

    ExportConfig config;
    config.reportTitle = QStringLiteral("S08 测试总结");
    if (!SettingsDao(DatabaseManager::instance().database()).setValue("export_config", config.toJson())) return 15;
    const QString outputPath = QDir(root).filePath("S08-summary.doc");
    WordExport::ExportResult exported = AchievementUseCase().exportAnnualReport(
        QDate::currentDate().year(), outputPath);
    if (!exported.success || !QFile::exists(outputPath) || exported.itemCount < 4) return 15;
    QFile output(outputPath);
    if (!output.open(QIODevice::ReadOnly)) return 16;
    const QByteArray document = output.readAll();
    if (!document.contains("S08") || !document.contains("\xE6\x88\x90\xE6\x9E\x9C")) return 17;
    WordExport::ExportResult failed = AchievementUseCase().exportAnnualReport(
        QDate::currentDate().year(), QDir(root).filePath("missing-parent/S08-summary.doc"));
    if (failed.success) return 18;

    DatabaseManager::instance().close();
    return 0;
}
