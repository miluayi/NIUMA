#include <QApplication>
#include "detail_title_contract.h"
#include "category_badge_contract.h"
#include <QComboBox>
#include <QCheckBox>
#include <QDialog>
#include <QDir>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSqlQuery>
#include <QTableWidget>
#include <QHeaderView>
#include <QTimer>
#include <QShortcut>

#include "DatabaseManager.h"
#include "PeriodicTaskCreateDialog.h"
#include "PeriodicTaskView.h"
#include "models/PeriodicTask.h"
#include "service/FileService.h"
#include "service/PeriodicTaskService.h"
#include "utils/PathUtil.h"

namespace {

PeriodicTask newTemplate(const QString &name)
{
    PeriodicTask tmpl;
    tmpl.name = name;
    tmpl.category = QStringLiteral("报");
    tmpl.periodType = QStringLiteral("daily");
    tmpl.periodValue = 1;
    tmpl.source = QStringLiteral("S06 来源");
    tmpl.tag = QStringLiteral("S06 标签");
    tmpl.handler = QStringLiteral("本人");
    tmpl.reminderTime = QStringLiteral("09:00");
    tmpl.isEnabled = true;
    return tmpl;
}

void closeConfirmation(bool accept)
{
    QTimer::singleShot(0, [accept]() {
        if (auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget())) {
            accept ? dialog->accept() : dialog->reject();
        }
    });
}

bool hasDueToday(const QList<PeriodicReminderItem> &reminders)
{
    for (const PeriodicReminderItem &item : reminders) {
        if (item.dueToday) return true;
    }
    return false;
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
    if (!query.exec("DELETE FROM periodic_task") || !query.exec("DELETE FROM task")) return 3;

    PeriodicTaskView view;
    view.resize(1200, 700);
    view.show();
    app.processEvents();
    auto *table = view.findChild<QTableWidget *>(QStringLiteral("periodicTaskTable"));
    auto *empty = view.findChild<QLabel *>(QStringLiteral("periodicTaskEmptyState"));
    if (!table || table->rowCount() != 0 || !empty || !empty->isVisible()) return 4;

    PeriodicTaskCreateDialog createDialog;
    createDialog.show();
    app.processEvents();
    app.processEvents();
    auto *templateName = createDialog.findChild<QPlainTextEdit *>(QStringLiteral("periodicTemplateNameEdit"));
    auto *templateEnabled = createDialog.findChild<QCheckBox *>(QStringLiteral("periodicTemplateEnabledCheck"));
    auto *templateSource = createDialog.findChild<QComboBox *>(QStringLiteral("periodicSourceCombo"));
    auto *templateTag = createDialog.findChild<QComboBox *>(QStringLiteral("periodicTagCombo"));
    auto *createSave = createDialog.findChild<QShortcut *>(QStringLiteral("shortcutSavePeriodicTask"));
    if (!templateName || !templateEnabled || !templateSource || !templateTag
        || !templateSource->currentText().isEmpty() || !templateTag->currentText().isEmpty()
        || !createSave || createSave->key() != QKeySequence::Save
        || createDialog.minimumWidth() != 640) return 5;
    if (createDialog.testAttribute(Qt::WA_TranslucentBackground) || !createDialog.mask().isEmpty()
        || !createDialog.property("nativePopupHandle").toULongLong()) return 21;
    QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QApplication::sendEvent(&createDialog, &escape);
    if (createDialog.result() != QDialog::Rejected) return 6;

    PeriodicTaskService periodicService;
    TaskResult visualTemplate = periodicService.createTemplate(newTemplate(QStringLiteral("S06 表格模板")));
    if (!visualTemplate.ok) return 7;
    view.refresh();
    app.processEvents();
    if (table->rowCount() != 1 || !table->isVisible() || empty->isVisible()
        || table->item(0, 0)->textAlignment() != (Qt::AlignLeft | Qt::AlignVCenter)
        || table->item(0, 1)->textAlignment() != Qt::AlignCenter
        || !table->cellWidget(0, 5)) return 8;
    const QRect categoryCell=table->visualItemRect(table->item(0,1));
    const QImage categoryImage=table->viewport()->grab(categoryCell).toImage();
    if(!verifyCategoryPixels(categoryImage,QRect(QPoint(),categoryCell.size()),table->item(0,1)->text())
        || !categoryImage.save(QDir(root).filePath("periodic-category-standard.png"))) return 27;
    if (table->horizontalHeader()->sectionResizeMode(0) != QHeaderView::Stretch
        || table->horizontalHeader()->sectionResizeMode(6) != QHeaderView::ResizeToContents
        || table->columnWidth(6) >= table->columnWidth(0)) return 22;
    const auto actions = table->cellWidget(0, 6)->findChildren<QPushButton *>();
    if (actions.size() != 2) return 24;
    const int sectionLeft = table->horizontalHeader()->sectionViewportPosition(6);
    const int sectionRight = sectionLeft + table->columnWidth(6) - 1;
    const int buttonLeft = actions.at(0)->mapTo(table->viewport(), QPoint(0, 0)).x();
    const int buttonRight = actions.at(1)->mapTo(table->viewport(),
                                                 QPoint(actions.at(1)->width() - 1, 0)).x();
    if (qAbs((buttonLeft - sectionLeft) - (sectionRight - buttonRight)) > 1) return 24;
    if (!view.grab().save(QDir(root).filePath(QStringLiteral("periodic-card.png")))) return 23;

    PeriodicTaskCreateDialog editDialog(visualTemplate.taskId);
    auto *editName = editDialog.findChild<QPlainTextEdit *>(QStringLiteral("periodicTemplateNameEdit"));
    auto *editSave = editDialog.findChild<QShortcut *>(QStringLiteral("shortcutSavePeriodicTask"));
    if (!editName || !editSave || editSave->key() != QKeySequence::Save
        || editName->toPlainText() != QStringLiteral("S06 表格模板")) return 9;
    editName->setPlainText(QString(45,QChar(0x725B)));
    editDialog.show(); app.processEvents();
    if (!verifyThreeLineNameEdit(editName)
        || !editDialog.grab().save(QDir(root).filePath("periodic-name-45.png"))) return 25;
    QKeyEvent saveName(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier);
    QApplication::sendEvent(editName,&saveName);
    if (!periodicService.getById(visualTemplate.taskId)
        || periodicService.getById(visualTemplate.taskId)->name!=QString(45,QChar(0x725B))) return 26;

    if (!periodicService.setEnabled(visualTemplate.taskId, false).ok
        || periodicService.getById(visualTemplate.taskId)->isEnabled) return 10;
    if (!periodicService.setEnabled(visualTemplate.taskId, true).ok
        || !periodicService.getById(visualTemplate.taskId)->isEnabled) return 11;
    view.refresh();
    app.processEvents();
    auto deleteButtons = view.findChildren<QPushButton *>();
    QPushButton *deleteButton = nullptr;
    for (QPushButton *button : deleteButtons) {
        if (button->text() == QStringLiteral("删除")) {
            deleteButton = button;
            break;
        }
    }
    if (!deleteButton) return 12;
    closeConfirmation(false);
    deleteButton->click();
    if (!periodicService.getById(visualTemplate.taskId).has_value()) return 13;
    view.refresh();
    app.processEvents();
    deleteButton = nullptr;
    for (QPushButton *button : view.findChildren<QPushButton *>()) {
        if (button->text() == QStringLiteral("删除")) {
            deleteButton = button;
            break;
        }
    }
    if (!deleteButton) return 14;
    closeConfirmation(true);
    deleteButton->click();
    if (periodicService.getById(visualTemplate.taskId).has_value()) return 15;

    FileService fileService;
    if (!fileService.initializeWorkDir(QDir(root).filePath("work-root")).success) return 16;
    TaskResult dueTemplate = periodicService.createTemplate(newTemplate(QStringLiteral("S06 首页提醒模板")));
    if (!dueTemplate.ok || periodicService.findDueReminders().isEmpty()) return 17;
    if (!periodicService.createTaskFromTemplate(dueTemplate.taskId).ok) return 18;
    if (!query.exec("SELECT COUNT(*) FROM task") || !query.next() || query.value(0).toInt() != 1) return 190;
    if (!periodicService.getById(dueTemplate.taskId)->lastRemindedDate.isValid()) return 191;
    if (hasDueToday(periodicService.findDueReminders())) return 192;

    PeriodicTask ignored = newTemplate(QStringLiteral("S06 忽略提醒模板"));
    TaskResult ignoredTemplate = periodicService.createTemplate(ignored);
    if (!ignoredTemplate.ok || periodicService.findDueReminders().isEmpty()) return 20;
    if (!periodicService.ignoreCurrent(ignoredTemplate.taskId).ok
        || !periodicService.getById(ignoredTemplate.taskId)->lastRemindedDate.isValid()
        || hasDueToday(periodicService.findDueReminders())) return 21;

    DatabaseManager::instance().close();
    return 0;
}
