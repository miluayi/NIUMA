#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QFrame>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSqlQuery>
#include <QTableView>
#include <QTextEdit>
#include <QTimer>
#include <QShortcut>

#include "AchievementDialog.h"
#include "AchievementView.h"
#include "AppTheme.h"
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
#include "widgets/StyledMessageBox.h"

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

bool editAchievement(QPushButton *button, const Achievement &expected,
                     const QString &newContent, const QString &newAmount,
                     const QString &capturePath)
{
    bool verified = false;
    QObject timerContext;
    QTimer::singleShot(0, &timerContext, [&]() {
        auto *dialog = qobject_cast<AchievementDialog *>(QApplication::activeModalWidget());
        if (!dialog) return;
        auto *content = dialog->findChild<QLineEdit *>("achievementWorkContentEdit");
        auto *object = dialog->findChild<QLineEdit *>("achievementWorkObjectEdit");
        auto *count = dialog->findChild<QLineEdit *>("achievementResultCountEdit");
        auto *amount = dialog->findChild<QLineEdit *>("achievementResultAmountEdit");
        auto *description = dialog->findChild<QTextEdit *>("achievementDescriptionEdit");
        auto *annual = dialog->findChild<QCheckBox *>("achievementAnnualCheck");
        auto *key = dialog->findChild<QCheckBox *>("achievementKeyCheck");
        auto *save = dialog->findChild<QPushButton *>("btnPrimary");
        if (!content || !object || !count || !amount || !description || !annual || !key || !save
            || content->text() != expected.workContent || object->text() != expected.workObject
            || count->text().toInt() != expected.resultCount
            || amount->text() != QString::number(expected.amountInYuan(), 'f', 2)
            || description->toPlainText() != expected.description
            || annual->isChecked() != expected.isAnnual || key->isChecked() != expected.isKeyAchievement
            || save->text() != QStringLiteral("保存") || !dialog->grab().save(capturePath)) {
            dialog->reject();
            return;
        }
        content->setText(newContent);
        amount->setText(newAmount);
        verified = true;
        save->click();
    });
    button->click();
    return verified;
}

bool answerDelete(QPushButton *button, bool confirm)
{
    bool answered = false;
    QObject timerContext;
    QTimer::singleShot(0, &timerContext, [&]() {
        auto *dialog = qobject_cast<StyledMessageBox *>(QApplication::activeModalWidget());
        if (!dialog) return;
        const QString text = confirm ? QStringLiteral("确认删除") : QStringLiteral("取消");
        for (auto *choice : dialog->findChildren<QPushButton *>()) {
            if (choice->text() == text) {
                answered = true;
                choice->click();
                return;
            }
        }
        dialog->reject();
    });
    button->click();
    return answered;
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
    dialog.findChild<QLineEdit *>("achievementWorkObjectEdit")->setText(QStringLiteral("S08 办理对象"));
    description->setPlainText(QStringLiteral("第一行说明\n第二行说明"));
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
    AppTheme::apply(app, view);
    view.resize(1200, 700);
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

    TaskDetailView recorded(directTask.taskId);
    recorded.show();
    auto *section = recorded.findChild<QFrame *>("achievementSection");
    auto *scroll = recorded.findChild<QScrollArea *>("detailScroll");
    auto *modify = recorded.findChild<QPushButton *>("addAchievementBtn");
    auto *remove = recorded.findChild<QPushButton *>("deleteAchievementBtn");
    auto *headerEdit = recorded.findChild<QPushButton *>("headerEditBtn");
    auto *headerDelete = recorded.findChild<QPushButton *>("headerDeleteBtn");
    if (!section || !scroll || !modify || !remove || !headerEdit || !headerDelete) return 30;
    scroll->ensureWidgetVisible(section);
    app.processEvents();
    if (!modify->isVisible() || !remove->isVisible() || modify->text() != QStringLiteral("修改成果")
        || modify->y() != remove->y() || modify->geometry().right() >= remove->x()
        || modify->height() != headerEdit->height() || remove->height() != headerDelete->height()
        || modify->font().pixelSize() != headerEdit->font().pixelSize()
        || remove->styleSheet() != headerDelete->styleSheet()
        || !section->grab().save(QDir(root).filePath("achievement-detail-actions.png"))) return 31;
    const QString detailContent = QStringLiteral("详情修改成果");
    if (!editAchievement(modify, *directAchievement, detailContent, QStringLiteral("22.50"),
                         QDir(root).filePath("achievement-edit-detail.png"))) return 32;
    const auto detailEdited = achievementDao.findByTaskId(directTask.taskId);
    if (!detailEdited || detailEdited->id != directAchievement->id
        || detailEdited->createTime != directAchievement->createTime
        || detailEdited->workContent != detailContent || detailEdited->resultAmount != 2250) return 33;
    recorded.close();

    view.refresh();
    app.processEvents();
    auto *model = table->model();
    if (model->columnCount() != 8 || model->headerData(7, Qt::Horizontal).toString() != QStringLiteral("操作")
        || total->text() != QStringLiteral("2") || amountStat->text() != QStringLiteral("¥32.00")) return 34;
    const auto rowAction = [table, model](const QString &text, const char *name) -> QPushButton * {
        for (int row = 0; row < model->rowCount(); ++row) {
            if (model->index(row, 1).data().toString() != text) continue;
            auto *actions = table->indexWidget(model->index(row, 7));
            return actions ? actions->findChild<QPushButton *>(name) : nullptr;
        }
        return nullptr;
    };
    auto *rowEdit = rowAction(detailContent, "achievementRowEdit");
    const QString annualContent = QStringLiteral("年度页面编辑成果");
    if (!rowEdit || !rowEdit->isVisible()
        || !view.grab().save(QDir(root).filePath("achievement-annual-actions.png"))
        || !editAchievement(rowEdit, *detailEdited, annualContent, QStringLiteral("30.05"),
                            QDir(root).filePath("achievement-edit-annual.png"))) return 35;
    const auto annualEdited = achievementDao.findByTaskId(directTask.taskId);
    if (!annualEdited || annualEdited->id != directAchievement->id
        || annualEdited->createTime != directAchievement->createTime || annualEdited->resultAmount != 3005
        || annualEdited->workContent != annualContent || amountStat->text() != QStringLiteral("¥39.55")) return 36;

    auto *rowDelete = rowAction(annualContent, "achievementRowDelete");
    if (!rowDelete || !answerDelete(rowDelete, false) || !achievementDao.findByTaskId(directTask.taskId)
        || !answerDelete(rowDelete, true) || achievementDao.findByTaskId(directTask.taskId)
        || table->model()->rowCount() != 1 || amountStat->text() != QStringLiteral("¥9.50")
        || keyStat->text() != QStringLiteral("0")) return 37;
    const auto unchangedTask = taskService.getTaskById(directTask.taskId);
    if (!unchangedTask || !unchangedTask->isCompleted()) return 38;

    TaskDetailView otherDetail(triggerTask.taskId);
    otherDetail.show();
    auto *otherRemove = otherDetail.findChild<QPushButton *>("deleteAchievementBtn");
    auto *supplement = otherDetail.findChild<QPushButton *>("addAchievementBtn");
    auto *otherSection = otherDetail.findChild<QFrame *>("achievementSection");
    if (!otherRemove || !supplement || !otherSection || !answerDelete(otherRemove, true)
        || achievementDao.findByTaskId(triggerTask.taskId)) return 39;
    app.processEvents();
    if (otherRemove->isVisible() || !supplement->isVisible() || supplement->text() != QStringLiteral("补录成果")
        || supplement->height() != headerEdit->height()
        || !otherSection->grab().save(QDir(root).filePath("achievement-detail-supplement.png"))) return 40;
    saveTriggeredAchievement();
    supplement->click();
    if (!achievementDao.findByTaskId(triggerTask.taskId)
        || supplement->text() != QStringLiteral("修改成果") || !otherRemove->isVisible()) return 41;

    DatabaseManager::instance().close();
    return 0;
}
