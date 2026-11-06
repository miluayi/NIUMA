#include <QApplication>
#include "detail_title_contract.h"
#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QPalette>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QShortcut>
#include <QSqlQuery>
#include <QTextEdit>
#include <QTimer>

#include "DatabaseManager.h"
#include "TaskCreateDialog.h"
#include "TaskDetailView.h"
#include "TaskEditDialog.h"
#include "ui/widgets/DragDropArea.h"
#include "service/TaskService.h"
#include "utils/PathUtil.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    const QString root = qEnvironmentVariable("NIUMA_TEST_ROOT");
    if (root.isEmpty()) return 1;
    qputenv("NIUMA_DATA_DIR", QDir(root).filePath("Data").toUtf8());
    if (!QDir().mkpath(root) || !DatabaseManager::instance().initialize(PathUtil::databasePath())) return 2;
    QSqlQuery query(DatabaseManager::instance().database());
    if (!query.exec("DELETE FROM task")) return 3;

    TaskCreateDialog create;
    create.show();
    app.processEvents();
    app.processEvents();
    auto *createTitle = create.findChild<QLineEdit *>(QStringLiteral("taskTitleEdit"));
    auto *createDescription = create.findChild<QTextEdit *>(QStringLiteral("taskDescriptionEdit"));
    auto *attachment = create.findChild<QWidget *>(QStringLiteral("taskAttachmentArea"));
    auto *source = create.findChild<QComboBox *>(QStringLiteral("taskSourceCombo"));
    auto *tag = create.findChild<QComboBox *>(QStringLiteral("taskTagCombo"));
    auto *handler = create.findChild<QComboBox *>(QStringLiteral("taskHandlerCombo"));
    QLabel *uploadHint = nullptr;
    if (attachment) {
        for (QLabel *label : attachment->findChildren<QLabel *>()) {
            if (label->text().contains(QStringLiteral("拖拽文件"))) uploadHint = label;
        }
    }
    if (!createTitle || !createDescription || create.minimumWidth() != 720
        || createDescription->height() != 256 || !attachment || attachment->height() != 128
        || !source || !source->currentText().isEmpty() || !tag || !tag->currentText().isEmpty()
        || !handler || handler->currentText() != QStringLiteral("本人")
        || !uploadHint || uploadHint->font().bold()
        || create.testAttribute(Qt::WA_TranslucentBackground) || !create.mask().isEmpty()
        || !create.property("nativePopupHandle").toULongLong()) return 4;
    if (!create.grab().save(QDir(qEnvironmentVariable("NIUMA_TEST_ROOT"))
            .filePath(QStringLiteral("task-create-dialog.png")))) return 29;
    createTitle->setFocus();
    auto *clearDeadline = create.findChild<QPushButton *>(QStringLiteral("deadlineClearBtn"));
    auto *upload = qobject_cast<DragDropArea *>(attachment);
    QFile attachmentFile(QDir(root).filePath(QStringLiteral("red-remove-fixture.txt")));
    if (!attachmentFile.open(QIODevice::WriteOnly)) return 36;
    attachmentFile.write("test attachment");
    attachmentFile.close();
    if (!upload || !upload->pasteFiles({attachmentFile.fileName()})) return 37;
    app.processEvents();
    auto *remove = upload->findChild<QPushButton *>(QStringLiteral("attachmentRemoveButton"));
    if (!remove || !clearDeadline || remove->palette().color(QPalette::ButtonText) != QColor("#DC2626")
        || clearDeadline->palette().color(QPalette::ButtonText) != QColor("#DC2626")) return 38;
    if (!create.grab().save(QDir(root).filePath(QStringLiteral("task-create-red-removal.png")))) return 39;
    remove->click();
    if (!upload->filePaths().isEmpty()) return 40;
    QKeyEvent tab(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
    QApplication::sendEvent(createTitle, &tab);
    app.processEvents();
    if (QApplication::focusWidget() == createTitle) return 5;
    createDescription->setFocus();
    QKeyEvent returnInDescription(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    QApplication::sendEvent(createDescription, &returnInDescription);
    app.processEvents();
    if (!create.isVisible()) return 6;
    QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QApplication::sendEvent(&create, &escape);
    if (create.result() != QDialog::Rejected) return 7;

    TaskCreateDialog shortcutCreate;
    shortcutCreate.show();
    auto *shortcutTitle = shortcutCreate.findChild<QLineEdit *>(QStringLiteral("taskTitleEdit"));
    auto *shortcutSave = shortcutCreate.findChild<QShortcut *>(QStringLiteral("shortcutSaveTask"));
    if (!shortcutTitle || !shortcutSave || shortcutSave->key() != QKeySequence::Save) return 8;
    shortcutTitle->setText(QStringLiteral("S12 快捷键新建任务"));
    shortcutTitle->setFocus();
    if (!QMetaObject::invokeMethod(shortcutSave, "activated", Qt::DirectConnection)) return 9;
    app.processEvents();
    if (shortcutCreate.result() != QDialog::Accepted) return 10;

    query.prepare("INSERT INTO task (task_no,title,category,source,handler,status,is_key_task,description,create_time) "
                  "VALUES (?,?,?,?,?,?,?,?,?)");
    query.addBindValue(QStringLiteral("S05-1"));
    query.addBindValue(QStringLiteral("S05 长标题任务"));
    query.addBindValue(QStringLiteral("查"));
    query.addBindValue(QStringLiteral("S05 来源"));
    query.addBindValue(QStringLiteral("本人"));
    query.addBindValue(QStringLiteral("进行中"));
    query.addBindValue(1);
    query.addBindValue(QStringLiteral("S05 说明"));
    query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    if (!query.exec()) return 11;
    const int taskId = query.lastInsertId().toInt();

    TaskEditDialog edit(taskId);
    auto *editTitle = edit.findChild<QLineEdit *>(QStringLiteral("taskTitleEdit"));
    auto *editDescription = edit.findChild<QTextEdit *>(QStringLiteral("taskDescriptionEdit"));
    if (!editTitle || editTitle->text() != QStringLiteral("S05 长标题任务")
        || !editDescription || editDescription->height() != 200) return 12;
    auto *editSave = edit.findChild<QShortcut *>(QStringLiteral("shortcutSaveTask"));
    if (!editSave || editSave->key() != QKeySequence::Save) return 13;
    edit.show();
    app.processEvents();
    auto *editClearDeadline = edit.findChild<QPushButton *>(QStringLiteral("deadlineClearBtn"));
    if (!editClearDeadline || editClearDeadline->palette().color(QPalette::ButtonText) != QColor("#DC2626")) return 41;
    editTitle->setText(QStringLiteral("S12 快捷键保存任务"));
    editTitle->setFocus();
    if (!QMetaObject::invokeMethod(editSave, "activated", Qt::DirectConnection)) return 14;
    app.processEvents();
    if (edit.result() != QDialog::Accepted
        || TaskService().getTaskById(taskId)->title != QStringLiteral("S12 快捷键保存任务")) return 15;

    TaskService service;
    TaskDetailView detail(taskId);
    auto *detailTitle = detail.findChild<QLabel *>(QStringLiteral("appUiDetailTitle"));
    auto *keyTag = detail.findChild<QLabel *>(QStringLiteral("keyTagLabel"));
    auto *detailStatus = detail.findChild<QLabel *>(QStringLiteral("appUiDetailStatus"));
    auto *openFolder = detail.findChild<QPushButton *>(QStringLiteral("openFolderBtn"));
    const QStringList sections = {QStringLiteral("basicInfoSection"), QStringLiteral("descSection"),
                                  QStringLiteral("attachmentSection"), QStringLiteral("remarksSection"),
                                  QStringLiteral("historySection"), QStringLiteral("achievementSection")};
    if (!detailTitle || !detailTitle->wordWrap() || !detailTitle->text().contains(QStringLiteral("S12 快捷键保存任务"))
        || detailTitle->font().family() != QStringLiteral("Segoe UI")
        || detailTitle->font().pixelSize() != 22
        || !detailTitle->styleSheet().contains(QStringLiteral("#0F172A"))
        || !keyTag || keyTag->objectName() != QStringLiteral("keyTagLabel")
        || !detailStatus || !detailStatus->styleSheet().contains(QStringLiteral("#FFF3C4"))) return 16;
    if (!openFolder || !openFolder->parentWidget()
        || openFolder->parentWidget()->objectName() != QStringLiteral("basicInfoSection")) return 27;
    auto *detailCategory = detail.findChild<QLabel *>(QStringLiteral("appUiCategoryTag"));
    if (!detailCategory || detailCategory->text() != QStringLiteral("查")
        || detailCategory->size() != QSize(30, 24)
        || detailCategory->property("categoryText").toString()!=QStringLiteral("查")) return 25;
    for (const QString &section : sections) {
        if (!detail.findChild<QWidget *>(section)) return 17;
    }

    detail.show();
    app.processEvents();
    auto *basicInfo = detail.findChild<QWidget *>(QStringLiteral("basicInfoSection"));
    auto *finishTime = detail.findChild<QLabel *>(QStringLiteral("finishTimeValue"));
    auto *processingDays = detail.findChild<QLabel *>(QStringLiteral("processingDaysValue"));
    if (!basicInfo || !finishTime || !processingDays || !keyTag->isVisible()
        || detailTitle->font().pixelSize() != keyTag->font().pixelSize()) return 30;
    const QPoint buttonTopLeft = openFolder->mapTo(basicInfo, QPoint(0, 0));
    const QPoint finishTimeTopLeft = finishTime->mapTo(basicInfo, QPoint(0, 0));
    const QPoint processingDaysTopLeft = processingDays->mapTo(basicInfo, QPoint(0, 0));
    if (buttonTopLeft.x() < processingDaysTopLeft.x() + processingDays->width()
        || buttonTopLeft.y() < finishTimeTopLeft.y() + finishTime->height()
        || qAbs(buttonTopLeft.y() + openFolder->height() / 2
                - processingDaysTopLeft.y() - processingDays->height() / 2) > 1
        || qAbs(buttonTopLeft.x() + openFolder->width()
                - finishTimeTopLeft.x() - finishTime->width()) > 2) return 31;
    if (!detail.grab().save(QDir(qEnvironmentVariable("NIUMA_TEST_ROOT"))
            .filePath(QStringLiteral("task-detail-dialog.png")))) return 26;
    const QDate expiredDeadline = QDate::currentDate().addDays(-1);
    query.prepare(QStringLiteral("UPDATE task SET deadline = ? WHERE id = ?"));
    query.addBindValue(expiredDeadline.toString(Qt::ISODate));
    query.addBindValue(taskId);
    if (!query.exec() || !QMetaObject::invokeMethod(&detail, "refreshData", Qt::DirectConnection)) return 34;
    auto *detailDeadline = detail.findChild<QLabel *>(QStringLiteral("deadlineValue"));
    if (!detailDeadline || detailDeadline->text() != expiredDeadline.toString(Qt::ISODate)) return 35;
    detailTitle->setText(QString(45, QChar(0x725B)));
    app.processEvents();
    if (!verifyThreeLineTitle(detailTitle)
        || !detail.grab().save(QDir(qEnvironmentVariable("NIUMA_TEST_ROOT"))
            .filePath(QStringLiteral("task-detail-long-title.png")))) return 33;
    QTimer::singleShot(0, []() {
        if (auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget())) dialog->reject();
    });
    if (!QMetaObject::invokeMethod(&detail, "onDeleteClicked", Qt::DirectConnection)
        || service.getTaskById(taskId)->status != QStringLiteral("进行中")) return 18;

    auto task = service.getTaskById(taskId);
    if (!task.has_value()) return 19;
    task->description = QStringLiteral("S05 更新后的说明");
    if (!service.updateTask(*task).ok
        || !service.getTaskById(taskId)->description.contains(QStringLiteral("更新后"))) return 20;
    if (!service.completeTask(taskId).ok
        || service.getTaskById(taskId)->status != QStringLiteral("已完成")) return 21;
    if (!QMetaObject::invokeMethod(&detail, "refreshData", Qt::DirectConnection)) return 32;
    if (!detailStatus->styleSheet().contains(QStringLiteral("#DDF8E7"))) return 32;
    if (!service.cancelComplete(taskId).ok
        || service.getTaskById(taskId)->status != QStringLiteral("进行中")) return 22;
    if (!service.deleteTask(taskId).ok
        || service.getTaskById(taskId)->status != QStringLiteral("已删除")) return 23;
    if (!service.restoreTask(taskId).ok
        || service.getTaskById(taskId)->status != QStringLiteral("进行中")) return 24;

    DatabaseManager::instance().close();
    return 0;
}
