#include <QApplication>
#include <QAbstractButton>
#include <QComboBox>
#include <QDir>
#include <QDialog>
#include <QDate>
#include <QDateTime>
#include <QDateEdit>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMetaObject>
#include <QSqlQuery>
#include <QScrollBar>
#include <QSet>
#include <QShortcut>
#include <QStackedWidget>
#include <QTableView>
#include <QTimer>
#include <QWheelEvent>

#include "DatabaseManager.h"
#include "TaskListView.h"
#include "models/TaskTableModel.h"
#include "ui/widgets/PaginationWidget.h"
#include "utils/PathUtil.h"

namespace {
bool insertTask(int index)
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("INSERT INTO task (task_no,title,category,source,tag,handler,status,deadline,create_time) "
                  "VALUES (?,?,?,?,?,?,?,?,?)");
    query.addBindValue(QStringLiteral("S04-%1").arg(index));
    query.addBindValue(QStringLiteral("S04 任务 %1").arg(index));
    query.addBindValue(index == 1 ? QStringLiteral("催") : QStringLiteral("查"));
    query.addBindValue(QStringLiteral("S04 来源"));
    query.addBindValue(QStringLiteral("S04 标签"));
    query.addBindValue(QStringLiteral("本人"));
    query.addBindValue(QStringLiteral("进行中"));
    query.addBindValue(QDate::currentDate().addDays(index).toString(Qt::ISODate));
    query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    return query.exec();
}

void closeConfirmation(bool confirm)
{
    QTimer::singleShot(0, [confirm]() {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!dialog) return;
        if (confirm) dialog->accept();
        else dialog->reject();
    });
}

QString taskStatus(int taskId)
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(QStringLiteral("SELECT status FROM task WHERE id=?"));
    query.addBindValue(taskId);
    return query.exec() && query.next() ? query.value(0).toString() : QString();
}

QWidget *yearPopup(QComboBox *year)
{
    year->showPopup();
    QApplication::processEvents();
    if (auto *popup = year->findChild<QWidget *>(QStringLiteral("appUiYearPopup"))) return popup;
    for (auto *widget : QApplication::topLevelWidgets())
        if (widget->objectName() == QStringLiteral("appUiYearPopup") && widget->isVisible()) return widget;
    return nullptr;
}

bool pickYearFromGrid(QComboBox *year, int chosenYear, const QString &screenshot)
{
    auto *popup = yearPopup(year);
    if (!popup || !popup->isVisible()) return false;
    auto *previous = popup->findChild<QAbstractButton *>(QStringLiteral("appUiYearPrevious"));
    auto *next = popup->findChild<QAbstractButton *>(QStringLiteral("appUiYearNext"));
    auto *all = popup->findChild<QAbstractButton *>(QStringLiteral("appUiYearAll"));
    const auto cells = popup->findChildren<QAbstractButton *>(QStringLiteral("appUiYearCell"));
    QSet<int> years;
    for (auto *cell : cells) years.insert(cell->property("year").toInt());
    if (!previous || !next || !all || cells.size() != 12 || years.size() != 12) return false;
    next->click();
    QSet<int> nextYears;
    for (auto *cell : popup->findChildren<QAbstractButton *>(QStringLiteral("appUiYearCell")))
        nextYears.insert(cell->property("year").toInt());
    for (int value : years) if (!nextYears.contains(value + 10)) return false;
    previous->click();
    if (!popup->grab().save(screenshot)) return false;
    for (auto *cell : popup->findChildren<QAbstractButton *>(QStringLiteral("appUiYearCell"))) {
        if (cell->property("year").toInt() == chosenYear) {
            cell->click();
            QApplication::processEvents();
            return year->currentData().toInt() == chosenYear && !popup->isVisible();
        }
    }
    return false;
}

int verifyFilterContract(TaskListView &view, QTableView *table)
{
    QSqlQuery query(DatabaseManager::instance().database());
    if (!query.exec("DELETE FROM task")
        || !query.exec("INSERT OR IGNORE INTO handler_dict (name,is_enabled,sort_order,create_time) "
                       "VALUES ('字典办理人',1,99,'2025-01-01 00:00:00')")) return 90;
    const int currentYear = QDate::currentDate().year();
    const int previousYear = currentYear - 1;
    struct FilterRow { const char *number; const char *category; const char *source;
                       const char *tag; const char *handler; const char *status; int year; };
    const FilterRow rows[] = {
        {"FILTER-001", "催", "来源甲", "标签甲", "张三", "进行中", currentYear},
        {"FILTER-002", "查", "来源乙", "标签乙", "李四", "已完成", currentYear},
        {"FILTER-003", "催", "来源甲", "标签甲", "往年办理人", "进行中", previousYear},
        {"FILTER-004", "催", "来源甲", "标签甲", "仅删除办理人", "已删除", currentYear}
    };
    for (const auto &row : rows) {
        query.prepare("INSERT INTO task (task_no,title,category,source,tag,handler,status,create_time) "
                      "VALUES (?,?,?,?,?,?,?,?)");
        query.addBindValue(QString::fromLatin1(row.number));
        query.addBindValue(QStringLiteral("筛选验证"));
        query.addBindValue(QString::fromUtf8(row.category));
        query.addBindValue(QString::fromUtf8(row.source));
        query.addBindValue(QString::fromUtf8(row.tag));
        query.addBindValue(QString::fromUtf8(row.handler));
        query.addBindValue(QString::fromUtf8(row.status));
        query.addBindValue(QStringLiteral("%1-06-15 12:00:00").arg(row.year));
        if (!query.exec()) return 91;
    }
    QMetaObject::invokeMethod(&view, "onResetClicked", Qt::DirectConnection);
    view.refresh();
    auto *year = view.findChild<QComboBox *>(QStringLiteral("taskYearCombo"));
    auto *handler = view.findChild<QComboBox *>(QStringLiteral("taskHandlerCombo"));
    auto *category = view.findChild<QComboBox *>(QStringLiteral("taskCategoryCombo"));
    auto *source = view.findChild<QComboBox *>(QStringLiteral("taskSourceCombo"));
    auto *tag = view.findChild<QComboBox *>(QStringLiteral("taskTagCombo"));
    auto *status = view.findChild<QComboBox *>(QStringLiteral("taskStatusCombo"));
    auto *search = view.findChild<QLineEdit *>(QStringLiteral("taskSearchEdit"));
    auto *start = view.findChildren<QDateEdit *>().value(0);
    auto *end = view.findChildren<QDateEdit *>().value(1);
    if (!year || !handler || !category || !source || !tag || !status || !search || !start || !end)
        return 92;
    if (year->currentText() != QString::number(currentYear)
        || start->date() != QDate(currentYear, 1, 1) || end->date() != QDate(currentYear, 12, 31)
        || table->model()->rowCount() != 2) return 93;
    if (handler->findText(QStringLiteral("张三")) < 0 || handler->findText(QStringLiteral("李四")) < 0
        || handler->findText(QStringLiteral("往年办理人")) < 0
        || handler->findText(QStringLiteral("字典办理人")) < 0
        || handler->findText(QStringLiteral("仅删除办理人")) >= 0) return 94;

    // Every discrete filter refreshes immediately, while unsent text/date changes stay pending.
    search->setText(QStringLiteral("未提交关键词"));
    start->setDate(QDate(previousYear, 1, 1));
    end->setDate(QDate(previousYear, 12, 31));
    const QList<QComboBox *> instantFilters = {category, source, tag, status, handler};
    const QStringList choices = {QStringLiteral("催"), QStringLiteral("来源甲"), QStringLiteral("标签甲"),
                                 QStringLiteral("进行中"), QStringLiteral("张三")};
    for (int index = 0; index < instantFilters.size(); ++index) {
        instantFilters.at(index)->setCurrentText(choices.at(index));
        if (table->model()->rowCount() != 1 || !view.currentFilter().keyword.isEmpty()
            || view.currentFilter().startDate != QDate(currentYear, 1, 1)) return 95 + index;
        view.refresh();
        if (instantFilters.at(index)->currentText() != choices.at(index)
            || search->text() != QStringLiteral("未提交关键词")
            || start->date() != QDate(previousYear, 1, 1)
            || end->date() != QDate(previousYear, 12, 31)
            || table->model()->rowCount() != 1) return 100 + index;
        instantFilters.at(index)->setCurrentIndex(0);
        if (table->model()->rowCount() != 2) return 105 + index;
    }
    start->setDate(QDate(currentYear, 7, 1));
    end->setDate(QDate(currentYear, 7, 31));
    if (!pickYearFromGrid(year, previousYear,
            QDir(qEnvironmentVariable("NIUMA_TEST_ROOT")).filePath(QStringLiteral("task-year-picker.png"))))
        return 118;
    if (start->date() != QDate(previousYear, 1, 1) || end->date() != QDate(previousYear, 12, 31)
        || table->model()->rowCount() != 1 || !view.currentFilter().keyword.isEmpty()
        || search->text() != QStringLiteral("未提交关键词")) return 110;
    view.refresh();
    if (year->currentText() != QString::number(previousYear) || table->model()->rowCount() != 1)
        return 111;
    category->setCurrentText(QStringLiteral("催"));
    if (table->model()->rowCount() != 1
        || table->model()->index(0, TaskTableModel::ColHandler).data().toString() != QStringLiteral("往年办理人"))
        return 117;
    category->setCurrentIndex(0);
    if (table->model()->rowCount() != 1
        || table->model()->index(0, TaskTableModel::ColHandler).data().toString()
            != QStringLiteral("往年办理人")) return 112;
    auto *popup = yearPopup(year);
    auto *all = popup ? popup->findChild<QAbstractButton *>(QStringLiteral("appUiYearAll")) : nullptr;
    if (!all) return 119;
    all->click();
    if (start->date() != QDate(currentYear, 1, 1) || end->date() != QDate(currentYear, 12, 31)
        || table->model()->rowCount() != 3 || year->currentData().toInt() != 0) return 113;
    if (table->model()->rowCount() != 3) return 114;
    search->clear();
    start->setDate(QDate(previousYear, 1, 1));
    end->setDate(QDate(previousYear, 12, 31));
    if (table->model()->rowCount() != 3) return 115;
    QMetaObject::invokeMethod(&view, "onSearchClicked", Qt::DirectConnection);
    if (table->model()->rowCount() != 1) return 116;

    // Submitted keywords survive an immediate year change; draft text waits for Search.
    year->setCurrentText(QString::number(currentYear));
    search->setText(QStringLiteral("FILTER-001"));
    QMetaObject::invokeMethod(&view, "onSearchClicked", Qt::DirectConnection);
    if (table->model()->rowCount() != 1) return 120;
    search->setText(QStringLiteral("FILTER-003"));
    start->setDate(QDate(currentYear, 7, 1));
    end->setDate(QDate(currentYear, 7, 31));
    year->setCurrentText(QString::number(previousYear));
    auto *empty = view.findChild<QLabel *>(QStringLiteral("taskListEmptyState"));
    if (table->model()->rowCount() != 0 || !empty || !empty->isVisible()
        || empty->text() != QStringLiteral("没有符合当前筛选条件的任务")
        || view.currentFilter().keyword != QStringLiteral("FILTER-001")
        || view.currentFilter().startDate != QDate(previousYear, 1, 1)
        || search->text() != QStringLiteral("FILTER-003")) return 121;
    year->setCurrentText(QStringLiteral("全部年度"));
    if (table->model()->rowCount() != 1
        || table->model()->index(0, TaskTableModel::ColTaskNo).data().toString() != QStringLiteral("FILTER-001"))
        return 122;
    QMetaObject::invokeMethod(&view, "onSearchClicked", Qt::DirectConnection);
    if (table->model()->rowCount() != 1
        || table->model()->index(0, TaskTableModel::ColTaskNo).data().toString() != QStringLiteral("FILTER-003"))
        return 123;
    category->setCurrentText(QStringLiteral("查"));
    if (table->model()->rowCount() != 0 || !empty->isVisible()) return 124;
    category->setCurrentIndex(0);
    year->setCurrentText(QString::number(currentYear));
    search->setText(QStringLiteral("筛选验证"));
    category->setCurrentText(QStringLiteral("催"));
    QMetaObject::invokeMethod(&view, "onSearchClicked", Qt::DirectConnection);
    if (table->model()->rowCount() != 1) return 125;
    source->setCurrentText(QStringLiteral("来源乙"));
    if (table->model()->rowCount() != 0 || !empty->isVisible()) return 126;
    source->setCurrentIndex(0);
    start->setDate(QDate(currentYear, 7, 1));
    end->setDate(QDate(currentYear, 7, 31));
    if (table->model()->rowCount() != 1) return 127;
    QMetaObject::invokeMethod(&view, "onSearchClicked", Qt::DirectConnection);
    if (table->model()->rowCount() != 0 || !empty->isVisible()) return 128;
    if (!view.grab().save(QDir(qEnvironmentVariable("NIUMA_TEST_ROOT"))
                             .filePath(QStringLiteral("task-filter-zero.png")))) return 129;
    popup = yearPopup(year);
    bool selectedSameYear = false;
    if (popup) for (auto *cell : popup->findChildren<QAbstractButton *>(QStringLiteral("appUiYearCell"))) {
        if (cell->property("year").toInt() == currentYear) {
            cell->click();
            selectedSameYear = true;
            break;
        }
    }
    if (!selectedSameYear || table->model()->rowCount() != 1
        || start->date() != QDate(currentYear, 1, 1) || end->date() != QDate(currentYear, 12, 31))
        return 130;
    return 0;
}

int verifyHandlerFilterContract(TaskListView &view, QTableView *table)
{
    QSqlQuery query(DatabaseManager::instance().database());
    if (!query.exec("CREATE TEMP TABLE task AS SELECT * FROM main.task WHERE 0")
        || !query.exec("CREATE TEMP TABLE handler_dict AS SELECT * FROM main.handler_dict WHERE 0")) return 140;
    const QStringList dictionary = {QStringLiteral("张三"), QStringLiteral("王五"), QStringLiteral("钱七"),
        QStringLiteral("李明"), QStringLiteral("陈亮"), QStringLiteral("阿明"),
        QStringLiteral("非本人"), QStringLiteral("本人")};
    for (int index = 0; index < dictionary.size(); ++index) {
        query.prepare("INSERT INTO handler_dict (id,name,is_enabled,sort_order,create_time) VALUES (?,?,1,?,?)");
        query.addBindValue(index + 1);
        query.addBindValue(dictionary.at(index));
        query.addBindValue(index);
        query.addBindValue(QStringLiteral("2026-01-01 00:00:00"));
        if (!query.exec()) return 141;
    }
    const QStringList handlers = {QStringLiteral("本人"), QStringLiteral("张三"), QStringLiteral("阿明"),
        QStringLiteral("王五"), QStringLiteral("陈亮"), QStringLiteral("李明"), QStringLiteral("非本人"),
        QStringLiteral(""), QString(), QStringLiteral("张三"), QStringLiteral("陈亮"),
        QStringLiteral("阿明"), QStringLiteral("王五"), QStringLiteral("李明"),
        QStringLiteral("仅删除办理人"), QStringLiteral("张三")};
    const int year = QDate::currentDate().year();
    for (int index = 0; index < handlers.size(); ++index) {
        query.prepare("INSERT INTO task (id,task_no,title,category,source,tag,handler,status,create_time,finish_time) "
                      "VALUES (?,?,?,?,?,?,?,?,?,?)");
        query.addBindValue(index + 1);
        query.addBindValue(QStringLiteral("HANDLER-%1").arg(index + 1, 3, 10, QChar('0')));
        query.addBindValue(QStringLiteral("办理人筛选"));
        query.addBindValue(index % 2 == 0 ? QStringLiteral("催") : QStringLiteral("查"));
        query.addBindValue(QStringLiteral("办理人来源"));
        query.addBindValue(QStringLiteral("办理人标签"));
        query.addBindValue(index == 8 ? QVariant() : QVariant(handlers.at(index)));
        query.addBindValue(index == 14 ? QStringLiteral("已删除")
                               : index == 1 ? QStringLiteral("已完成")
                               : index == 3 ? QStringLiteral("他人办理") : QStringLiteral("进行中"));
        query.addBindValue(QStringLiteral("%1-06-15 12:00:00").arg(index == 15 ? year - 1 : year));
        query.addBindValue(index == 1 ? QStringLiteral("%1-06-16 12:00:00").arg(year) : QString());
        if (!query.exec()) return 142;
    }
    QMetaObject::invokeMethod(&view, "onResetClicked", Qt::DirectConnection);
    view.refresh();
    auto *handler = view.findChild<QComboBox *>(QStringLiteral("taskHandlerCombo"));
    auto *status = view.findChild<QComboBox *>(QStringLiteral("taskStatusCombo"));
    auto *category = view.findChild<QComboBox *>(QStringLiteral("taskCategoryCombo"));
    auto *search = view.findChild<QLineEdit *>(QStringLiteral("taskSearchEdit"));
    auto *pagination = view.findChild<PaginationWidget *>();
    if (!handler || !status || !category || !search || !pagination) return 143;
    const QStringList expected = {QStringLiteral("全部办理人"), QStringLiteral("本人"), QStringLiteral("非本人"),
        QStringLiteral("阿明"), QStringLiteral("陈亮"), QStringLiteral("李明"), QStringLiteral("钱七"),
        QStringLiteral("王五"), QStringLiteral("张三")};
    QStringList observed;
    for (int index = 0; index < handler->count(); ++index) observed.append(handler->itemText(index));
    if (observed != expected || pagination->total() != 14) return 144;
    handler->setCurrentText(QStringLiteral("非本人"));
    if (pagination->total() != 13 || table->model()->rowCount() != 10
        || pagination->totalPages() != 2) return 145;
    QSet<int> ids;
    bool sawNullOrEmpty = false;
    bool sawLiteralOther = false;
    for (int page = 1; page <= 2; ++page) {
        if (!QMetaObject::invokeMethod(&view, "onPageChanged", Qt::DirectConnection, Q_ARG(int, page))) return 146;
        if (table->model()->rowCount() != (page == 1 ? 10 : 3)) return 147;
        for (int row = 0; row < table->model()->rowCount(); ++row) {
            auto *model = qobject_cast<TaskTableModel *>(table->model());
            const Task task = model->getTask(row);
            if (task.handler == QStringLiteral("本人") || ids.contains(task.id)) return 148;
            ids.insert(task.id);
            sawNullOrEmpty |= task.handler.isEmpty();
            sawLiteralOther |= task.handler == QStringLiteral("非本人");
        }
    }
    if (ids.size() != 13 || !ids.contains(8) || !ids.contains(9) || !sawNullOrEmpty || !sawLiteralOther)
        return 149;
    view.refresh();
    if (handler->currentText() != QStringLiteral("非本人") || pagination->total() != 13) return 150;
    status->setCurrentText(QStringLiteral("已完成"));
    if (pagination->total() != 1 || table->model()->rowCount() != 1
        || table->model()->index(0, TaskTableModel::ColHandler).data().toString() != QStringLiteral("张三"))
        return 151;
    category->setCurrentText(QStringLiteral("催"));
    if (pagination->total() != 0 || table->model()->rowCount() != 0) return 152;
    category->setCurrentIndex(0);
    status->setCurrentIndex(0);
    handler->setCurrentText(QStringLiteral("阿明"));
    if (pagination->total() != 2 || table->model()->rowCount() != 2) return 153;
    handler->setCurrentText(QStringLiteral("本人"));
    if (pagination->total() != 1 || table->model()->rowCount() != 1) return 154;
    handler->setCurrentText(QStringLiteral("非本人"));
    search->setText(QStringLiteral("HANDLER-001"));
    QMetaObject::invokeMethod(&view, "onSearchClicked", Qt::DirectConnection);
    if (pagination->total() != 0 || table->model()->rowCount() != 0) return 155;
    search->clear();
    QMetaObject::invokeMethod(&view, "onSearchClicked", Qt::DirectConnection);
    view.resize(1350, 800);
    QApplication::processEvents();
    handler->showPopup();
    QApplication::processEvents();
    if (!handler->view()->window()->grab().save(QDir(qEnvironmentVariable("NIUMA_TEST_ROOT"))
                             .filePath(QStringLiteral("task-handler-options.png")))) return 156;
    handler->hidePopup();
    if (!query.exec("DROP TABLE temp.task") || !query.exec("DROP TABLE temp.handler_dict")) return 157;
    return 0;
}
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    const QString root = qEnvironmentVariable("NIUMA_TEST_ROOT");
    if (root.isEmpty()) return 1;
    qputenv("NIUMA_DATA_DIR", QDir(root).filePath("Data").toUtf8());
    if (!QDir().mkpath(root) || !DatabaseManager::instance().initialize(PathUtil::databasePath())) return 2;
    QSqlQuery clear(DatabaseManager::instance().database());
    if (!clear.exec("DELETE FROM task")) return 3;

    TaskListView view;
    view.resize(1000, 800);
    view.show();
    view.refresh();
    app.processEvents();
    auto *state = view.findChild<QStackedWidget *>(QStringLiteral("taskTableState"));
    auto *empty = view.findChild<QLabel *>(QStringLiteral("taskListEmptyState"));
    auto *table = view.findChild<QTableView *>(QStringLiteral("taskTable"));
    auto *pagination = view.findChild<PaginationWidget *>();
    if (!state || !empty || !table || !pagination || state->currentWidget() != empty
        || empty->text() != QStringLiteral("没有符合当前筛选条件的任务") || pagination->total() != 0) return 4;
    auto *initialYear = view.findChild<QComboBox *>(QStringLiteral("taskYearCombo"));
    if (!initialYear || initialYear->currentText() != QString::number(QDate::currentDate().year())) return 88;

    for (int index = 1; index <= 11; ++index) if (!insertTask(index)) return 5;
    view.refresh();
    app.processEvents();
    auto *model = qobject_cast<TaskTableModel *>(table->model());
    if (!model || state->currentWidget() != table || model->rowCount() != 10
        || model->columnCount() != 10 || pagination->totalPages() != 2
        || table->verticalHeader()->defaultSectionSize() != 48
        || table->columnWidth(TaskTableModel::ColCategory) != 48
        || table->columnWidth(TaskTableModel::ColDays) != 56) return 6;
    if (model->data(model->index(0, TaskTableModel::ColTitle), Qt::TextAlignmentRole).toInt()
            != static_cast<int>(Qt::AlignVCenter | Qt::AlignLeft)
        || model->data(model->index(0, TaskTableModel::ColCategory), Qt::TextAlignmentRole).toInt()
            != static_cast<int>(Qt::AlignCenter)) return 7;

    const int selectedTaskId = model->getTaskId(0);
    auto *deleteShortcut = view.findChild<QShortcut *>(QStringLiteral("shortcutDeleteSelectedTask"));
    auto *search = view.findChild<QLineEdit *>(QStringLiteral("taskSearchEdit"));
    if (selectedTaskId <= 0 || !deleteShortcut || deleteShortcut->key() != QKeySequence::Delete
        || deleteShortcut->context() != Qt::WidgetShortcut || !search) return 71;
    table->setCurrentIndex(model->index(0, 0));
    search->setFocus();
    QKeyEvent deleteInSearch(QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier);
    QApplication::sendEvent(search, &deleteInSearch);
    app.processEvents();
    if (taskStatus(selectedTaskId) != QStringLiteral("进行中") || QApplication::activeModalWidget()) return 72;
    table->setFocus();
    closeConfirmation(false);
    if (!QMetaObject::invokeMethod(deleteShortcut, "activated", Qt::DirectConnection)
        || taskStatus(selectedTaskId) != QStringLiteral("进行中")) return 73;
    closeConfirmation(true);
    if (!QMetaObject::invokeMethod(deleteShortcut, "activated", Qt::DirectConnection)
        || taskStatus(selectedTaskId) != QStringLiteral("已删除")) return 74;

    view.resize(500, 900);
    app.processEvents();
    QScrollBar *horizontal = table->horizontalScrollBar();
    QScrollBar *vertical = table->verticalScrollBar();
    const int horizontalBefore = horizontal->value();
    QWheelEvent wheel(QPointF(10, 10), QPointF(10, 10), QPoint(), QPoint(0, -120),
                      Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QApplication::sendEvent(table->viewport(), &wheel);
    if (vertical->maximum() != vertical->minimum()
        || horizontal->maximum() == horizontal->minimum()
        || horizontal->value() == horizontalBefore) return 8;

    int openedTaskId = 0;
    QObject::connect(&view, &TaskListView::taskClicked, [&openedTaskId](int id) { openedTaskId = id; });
    QMetaObject::invokeMethod(&view, "onTaskClicked", Qt::DirectConnection,
                              Q_ARG(QModelIndex, model->index(0, 0)));
    if (openedTaskId <= 0) return 9;

    search->setText(QStringLiteral("S04 不存在"));
    QMetaObject::invokeMethod(&view, "onSearchClicked", Qt::DirectConnection);
    app.processEvents();
    if (state->currentWidget() != empty
        || empty->text() != QStringLiteral("没有符合当前筛选条件的任务")) return 10;
    if (!insertTask(12)) return 75;
    view.showCreatedTask();
    app.processEvents();
    if (!view.currentFilter().keyword.isEmpty() || !view.currentFilter().category.isEmpty()
        || !view.currentFilter().handler.isEmpty() || view.currentFilter().page != 1
        || pagination->total() != 11 || model->rowCount() != 10) return 76;
    QMetaObject::invokeMethod(&view, "onResetClicked", Qt::DirectConnection);
    app.processEvents();
    if (state->currentWidget() != table || model->rowCount() != 10) return 11;

    auto *startDate = view.findChildren<QDateEdit *>().value(0);
    auto *endDate = view.findChildren<QDateEdit *>().value(1);
    if (!startDate || !endDate || startDate->date() != QDate(QDate::currentDate().year(), 1, 1)
        || endDate->date() != QDate(QDate::currentDate().year(), 12, 31)) return 77;
    auto *year = view.findChild<QComboBox *>(QStringLiteral("taskYearCombo"));
    if (!year || year->findText(QStringLiteral("全部年度")) < 0) return 84;
    year->setCurrentText(QStringLiteral("全部年度"));
    QMetaObject::invokeMethod(&view, "onSearchClicked", Qt::DirectConnection);
    if (!clear.exec("DELETE FROM task")) return 78;
    struct Row { const char *number; const char *status; QDate deadline; };
    const QDate today = QDate::currentDate();
    const Row rows[] = {
        {"20200101-003", "进行中", QDate()}, {"20200101-001", "进行中", today.addDays(1)},
        {"20200101-004", "进行中", QDate()}, {"20200101-002", "进行中", today},
        {"20200101-005", "进行中", today.addDays(-1)},
        {"20200101-008", "进行中", today.addDays(-2)},
        {"20200101-006", "已完成", today.addDays(-3)},
        {"20200101-000", "已完成", QDate()},
        {"20200101-007", "他人办理", today.addDays(-4)}
    };
    for (const Row &row : rows) {
        QSqlQuery insert(DatabaseManager::instance().database());
        insert.prepare("INSERT INTO task (task_no,title,handler,status,deadline,create_time) VALUES (?,?,?,?,?,?)");
        insert.addBindValue(QString::fromLatin1(row.number));
        insert.addBindValue(QStringLiteral("排序验证"));
        insert.addBindValue(QStringLiteral("本人"));
        insert.addBindValue(QString::fromUtf8(row.status));
        insert.addBindValue(row.deadline.isValid() ? row.deadline.toString(Qt::ISODate) : QString());
        insert.addBindValue(QStringLiteral("2020-01-01 00:00:00"));
        if (!insert.exec()) return 79;
    }
    if (!clear.exec("UPDATE task SET finish_time='2026-02-01 12:00:00' WHERE task_no='20200101-006'")
        || !clear.exec("UPDATE task SET finish_time='2026-03-01 12:00:00' WHERE task_no='20200101-000'"))
        return 85;
    view.refresh();
    const QStringList expected = {QStringLiteral("20200101-002"), QStringLiteral("20200101-001"),
        QStringLiteral("20200101-008"), QStringLiteral("20200101-005"),
        QStringLiteral("20200101-004"), QStringLiteral("20200101-003"),
        QStringLiteral("20200101-000"), QStringLiteral("20200101-006"), QStringLiteral("20200101-007")};
    if (pagination->total() != expected.size() || model->rowCount() != expected.size()) return 80;
    for (int row = 0; row < expected.size(); ++row)
        if (model->data(model->index(row, TaskTableModel::ColTaskNo)).toString() != expected.at(row))
            return 81;
    if (model->data(model->index(2, TaskTableModel::ColDeadline)).toString() != QStringLiteral("逾期")
        || model->data(model->index(3, TaskTableModel::ColDeadline)).toString() != QStringLiteral("逾期")
        || model->data(model->index(7, TaskTableModel::ColDeadline)).toString()
            != today.addDays(-3).toString(Qt::ISODate)
        || model->data(model->index(8, TaskTableModel::ColDeadline)).toString()
            != today.addDays(-4).toString(Qt::ISODate)) return 82;
    view.resize(1000, 800);
    app.processEvents();
    if (!view.grab().save(QDir(root).filePath(QStringLiteral("task-list-overdue-order.png")))) return 83;
    const int completionColumn = TaskTableModel::ColDeadline + 1;
    if (model->headerData(completionColumn, Qt::Horizontal).toString() != QStringLiteral("完成日期")
        || model->data(model->index(6, completionColumn)).toString() != QStringLiteral("2026-03-01")
        || model->data(model->index(7, completionColumn)).toString() != QStringLiteral("2026-02-01"))
        return 86;
    auto *status = view.findChild<QComboBox *>(QStringLiteral("taskStatusCombo"));
    status->setCurrentText(QStringLiteral("已完成"));
    if (model->rowCount() != 2
        || model->data(model->index(0, TaskTableModel::ColTaskNo)).toString() != QStringLiteral("20200101-000"))
        return 87;
    const int filterResult = verifyFilterContract(view, table);
    if (filterResult != 0) return filterResult;
    const int handlerResult = verifyHandlerFilterContract(view, table);
    if (handlerResult != 0) return handlerResult;

    DatabaseManager::instance().close();
    return 0;
}
