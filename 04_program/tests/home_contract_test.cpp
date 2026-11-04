#include <QApplication>
#include "text_selection_contract.h"
#include <QDate>
#include <QDir>
#include <QEvent>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSqlQuery>

#include "DatabaseManager.h"
#include "HomeView.h"
#include "AppTheme.h"
#include "ui/widgets/TaskItemDelegate.h"
#include "PathUtil.h"
#include "ui/widgets/PaginationWidget.h"

namespace {

bool insertTask(const QString &number, const QString &title, const QString &status,
                const QDate &deadline, const QString &created, bool keyTask = false)
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("INSERT INTO task (task_no, title, category, source, tag, handler, status, deadline, create_time, is_key_task) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(number);
    query.addBindValue(title);
    query.addBindValue(QStringLiteral("查"));
    query.addBindValue(QStringLiteral("S03 测试"));
    query.addBindValue(QStringLiteral("S03 标签"));
    query.addBindValue(QStringLiteral("本人"));
    query.addBindValue(status);
    query.addBindValue(deadline.isValid() ? deadline.toString(Qt::ISODate) : QString());
    query.addBindValue(created);
    query.addBindValue(keyTask ? 1 : 0);
    return query.exec();
}

bool rowContains(const QFrame *row, const QString &text)
{
    for (const QLabel *label : row->findChildren<QLabel *>()) {
        if (label->text().contains(text)) return true;
    }
    return false;
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
    if (!clear.exec("DELETE FROM task") || !clear.exec("DELETE FROM periodic_task")) return 3;

    HomeView emptyHome;
    emptyHome.show();
    emptyHome.refresh();
    app.processEvents();
    const auto *emptyFocusTitle = emptyHome.findChild<QLabel *>(QStringLiteral("FocusTitle"));
    const auto *emptyPagination = emptyHome.findChild<PaginationWidget *>();
    bool emptyHintVisible = false;
    for (const QLabel *label : emptyHome.findChildren<QLabel *>()) {
        if (label->objectName() == QStringLiteral("InProgressEmptyState")) {
            if (label->text() != QStringLiteral("目前没有任务，请添加任务")) return 4;
            emptyHintVisible = label->isVisible();
            break;
        }
    }
    if (!emptyFocusTitle || emptyFocusTitle->text() != QStringLiteral("今日专注  0/3")
        || !emptyPagination || emptyPagination->total() != 0 || !emptyHintVisible) return 4;
    emptyHome.hide();

    const QDate today = QDate::currentDate();
    const QString thisMonth = today.toString("yyyy-MM-dd") + " 10:00:00";
    for (int index = 1; index <= 10; ++index) {
        if (!insertTask(QStringLiteral("S03-%1").arg(index),
                        QStringLiteral("S03-%1").arg(index), QStringLiteral("进行中"),
                        today.addDays(11 - index), thisMonth, index == 1)) return 5;
    }
    if (!insertTask(QStringLiteral("S03-11"), QStringLiteral("S03-11"), QStringLiteral("进行中"),
                    QDate(), thisMonth)) return 6;
    if (!insertTask(QStringLiteral("S03-DONE-1"), QStringLiteral("S03-DONE-1"), QStringLiteral("已完成"),
                    QDate(), thisMonth)
        || !insertTask(QStringLiteral("S03-DONE-2"), QStringLiteral("S03-DONE-2"), QStringLiteral("已完成"),
                       QDate(), thisMonth)) return 7;

    QSqlQuery periodic(DatabaseManager::instance().database());
    periodic.prepare("INSERT INTO periodic_task (name, category, period_type, period_value, handler, is_enabled, create_time) "
                     "VALUES (?, ?, ?, ?, ?, ?, ?)");
    periodic.addBindValue(QStringLiteral("S03 周期提醒"));
    periodic.addBindValue(QStringLiteral("办"));
    periodic.addBindValue(QStringLiteral("daily"));
    periodic.addBindValue(1);
    periodic.addBindValue(QStringLiteral("本人"));
    periodic.addBindValue(1);
    periodic.addBindValue(thisMonth);
    if (!periodic.exec()) return 8;

    auto *home = new HomeView;
    QScrollArea scroll;
    scroll.setWidget(home);
    scroll.setWidgetResizable(true);
    scroll.resize(900, 600);
    scroll.show();
    home->refresh();
    app.processEvents();

    const auto rows = home->findChildren<QFrame *>(QStringLiteral("InProgressItem"));
    const auto *pagination = home->findChild<PaginationWidget *>();
    const auto *focusTitle = home->findChild<QLabel *>(QStringLiteral("FocusTitle"));
    if (rows.size() != 10 || !pagination || pagination->total() != 11 || pagination->totalPages() != 2
        || !focusTitle || focusTitle->text() != QStringLiteral("今日专注  0/3")) return 9;
    if (!rowContains(rows.first(), QStringLiteral("S03-10"))
        || !rowContains(rows.last(), QStringLiteral("S03-1"))) return 10;
    auto *homeCategory = rows.first()->findChild<QFrame *>(QStringLiteral("appUiCategoryTag"));
    const auto *homeCategoryCell = rows.first()->findChild<QWidget *>(QStringLiteral("HomeInProgressCategoryCell"));
    if (!homeCategory || homeCategory->height() != 24
        || !homeCategoryCell || homeCategoryCell->width() != 48
        || homeCategory->property("categoryText").toString() != QStringLiteral("查")) return 15;
    const int expectedCategoryTagWidth = TaskItemDelegate::categoryBadgeWidth(QStringLiteral("查"));
    const auto *categoryColumn = home->findChild<QLabel *>(QStringLiteral("HomeInProgressCategoryColumn"));
    if (homeCategory->width() != expectedCategoryTagWidth || !categoryColumn
        || categoryColumn->width() != 48
        || qAbs(homeCategory->mapTo(homeCategoryCell, QPoint(0, 0)).x()
                + homeCategory->width() / 2 - homeCategoryCell->width() / 2) > 1
        || qAbs(homeCategory->mapTo(rows.first(), QPoint(0, 0)).y()
                + homeCategory->height() / 2 - rows.first()->height() / 2) > 1) return 23;
    const QImage categoryImage = homeCategory->grab().toImage();
    int backgroundPixels = 0;
    int textPixels = 0;
    const QColor badgeText = QColor(QStringLiteral("#000000"));
    for (int y = 0; y < categoryImage.height(); ++y) {
        for (int x = 0; x < categoryImage.width(); ++x) {
            const QColor pixel = categoryImage.pixelColor(x, y);
            if (pixel == QColor(QStringLiteral("#DCDCAA"))) ++backgroundPixels;
            if (pixel.red() <= badgeText.red() + 8
                && pixel.green() <= badgeText.green() + 8
                && pixel.blue() <= badgeText.blue() + 8) ++textPixels;
        }
    }
    if (backgroundPixels < 50 || textPixels < 2) return 16;
    const auto *keyTaskTitle = rows.last()->findChild<QLabel *>(QStringLiteral("appUiTaskTitle"));
    if (!keyTaskTitle || !keyTaskTitle->text().contains(QStringLiteral("#FF0000"))) return 17;
    if (!home->findChild<QFrame *>(QStringLiteral("appUiColumnHeader"))
        || home->findChild<QWidget *>(QStringLiteral("InProgressList"))->height() != 480) return 11;
    const auto *columnHeader = home->findChild<QFrame *>(QStringLiteral("appUiColumnHeader"));
    QLabel *sourceHeader = nullptr;
    QLabel *tagHeader = nullptr;
    QLabel *deadlineHeader = nullptr;
    for (QLabel *label : columnHeader->findChildren<QLabel *>(QString(), Qt::FindDirectChildrenOnly)) {
        if (label->text() == QStringLiteral("来源")) sourceHeader = label;
        if (label->text() == QStringLiteral("事项标签")) tagHeader = label;
        if (label->text() == QStringLiteral("截止日期")) deadlineHeader = label;
    }
    QLabel *sourceCell = nullptr;
    QLabel *tagCell = nullptr;
    QLabel *deadlineCell = nullptr;
    for (QLabel *label : rows.first()->findChildren<QLabel *>(QString(), Qt::FindDirectChildrenOnly)) {
        if (label->text() == QStringLiteral("S03 测试")) sourceCell = label;
        if (label->text() == QStringLiteral("S03 标签")) tagCell = label;
        if (label->text().startsWith(QStringLiteral("截止 "))) deadlineCell = label;
    }
    if (!sourceHeader || !tagHeader || !deadlineHeader || !sourceCell || !tagCell || !deadlineCell) return 26;
    const int tagHeaderCenter = tagHeader->mapTo(home, QPoint(tagHeader->width() / 2, 0)).x();
    const int tagCellCenter = tagCell->mapTo(home, QPoint(tagCell->width() / 2, 0)).x();
    const int sourceHeaderCenter = sourceHeader->mapTo(home, QPoint(sourceHeader->width() / 2, 0)).x();
    const int sourceCellCenter = sourceCell->mapTo(home, QPoint(sourceCell->width() / 2, 0)).x();
    const int deadlineHeaderCenter = deadlineHeader->mapTo(home, QPoint(deadlineHeader->width() / 2, 0)).x();
    const int deadlineCellCenter = deadlineCell->mapTo(home, QPoint(deadlineCell->width() / 2, 0)).x();
    if (qAbs(tagHeaderCenter - tagCellCenter) > 1
        || qAbs(sourceHeaderCenter - sourceCellCenter) > 1
        || qAbs(deadlineHeaderCenter - deadlineCellCenter) > 1
        || !deadlineCell->styleSheet().contains(QStringLiteral("#FF0000"))) return 26;
    bool foundThreeDayWarning = false;
    for (QLabel *label : rows.at(2)->findChildren<QLabel *>(QString(), Qt::FindDirectChildrenOnly)) {
        if (label->text().startsWith(QStringLiteral("截止 "))
            && label->styleSheet().contains(QStringLiteral("#FAAD14"))) foundThreeDayWarning = true;
    }
    if (!foundThreeDayWarning) return 27;
    AppTheme::apply(app, *home);
    app.processEvents();
    if (deadlineCell->styleSheet().contains(QStringLiteral("color:#000000"))
        || !deadlineCell->styleSheet().contains(QStringLiteral("#FF0000"))
        || !deadlineCell->styleSheet().contains(QStringLiteral("background:transparent"))) return 28;
    rows.first()->setAttribute(Qt::WA_UnderMouse, true);
    QEvent enter(QEvent::Enter);
    QApplication::sendEvent(rows.first(), &enter);
    app.processEvents();
    const QImage hoverRow = rows.first()->grab().toImage();
    for (const QLabel *cell : {tagCell, sourceCell, deadlineCell}) {
        const QPoint blankCorner = cell->mapTo(rows.first(), QPoint(2, 12));
        if (hoverRow.pixelColor(blankCorner) != QColor(QStringLiteral("#F5F7FA"))) return 30;
    }
    if (!hoverRow.save(QDir(root).filePath(QStringLiteral("home-deadline-hover.png")))) return 31;
    if (home->findChildren<QFrame *>(QStringLiteral("appUiStatCell")).size() != 4
        || home->findChildren<QFrame *>(QStringLiteral("PeriodicReminderItem")).isEmpty()
        || home->findChild<QLabel *>(QStringLiteral("StatTotal"))->text() != QStringLiteral("13")
        || home->findChild<QLabel *>(QStringLiteral("StatCompleted"))->text() != QStringLiteral("2")) return 12;

    const QList<int> taskIds = {rows.at(0)->property("taskId").toInt(),
                                rows.at(1)->property("taskId").toInt(),
                                rows.at(2)->property("taskId").toInt()};
    const auto *focusArea = home->findChild<QFrame *>(QStringLiteral("FocusArea"));
    if (!focusArea || focusArea->height() != 230) return 18;
    const int focusAreaHeight = focusArea->height();
    const QRect titleGeometry = focusTitle->geometry();
    scroll.verticalScrollBar()->setValue(qMin(20, scroll.verticalScrollBar()->maximum()));
    const int scrollBefore = scroll.verticalScrollBar()->value();
    for (int index = 0; index < taskIds.size(); ++index) {
        bool clicked = false;
        for (QPushButton *button : home->findChildren<QPushButton *>()) {
            if (button->property("taskId").toInt() == taskIds.at(index)
                && !button->property("focused").toBool()) {
                button->click();
                clicked = true;
                break;
            }
        }
        app.processEvents();
        if (!clicked || focusTitle->text() != QStringLiteral("今日专注  %1/3").arg(index + 1)
            || focusArea->height() != focusAreaHeight
            || focusTitle->geometry() != titleGeometry
            || scroll.verticalScrollBar()->value() != scrollBefore) return 19;
        const auto currentFocusRows = home->findChildren<QFrame *>(QStringLiteral("FocusItem"));
        if (currentFocusRows.size() != index + 1
            || currentFocusRows.first()->geometry().top() <= focusTitle->geometry().bottom()) return 24;
        const auto *focusCategory = currentFocusRows.first()->findChild<QFrame *>(QStringLiteral("appUiCategoryTag"));
        if (!focusCategory || focusCategory->property("categoryText").toString() != QStringLiteral("查")
            || focusCategory->width() != expectedCategoryTagWidth
            || qAbs(focusCategory->mapTo(currentFocusRows.first(), QPoint(0, 0)).y()
                    + focusCategory->height() / 2 - currentFocusRows.first()->height() / 2) > 1) return 25;
        const auto focusTag = currentFocusRows.first()->findChildren<QLabel *>();
        bool hasTag = false, hasSource = false, hasDeadline = false;
        for (const QLabel *label : focusTag) {
            hasTag |= label->text() == QStringLiteral("S03 标签");
            hasSource |= label->text() == QStringLiteral("S03 测试");
            hasDeadline |= label->text().startsWith(QStringLiteral("截止 "));
        }
        if (!hasTag || !hasSource || !hasDeadline) return 29;
    }
    const auto focusRows = home->findChildren<QFrame *>(QStringLiteral("FocusItem"));
    if (focusRows.size() != 3) return 20;
    for (int index = 0; index < taskIds.size(); ++index) {
        if (focusRows.at(index)->property("taskId").toInt() != taskIds.at(index)) return 21;
    }
    if (!home->grab().save(QDir(root).filePath(QStringLiteral("home-focused.png")))) return 22;

    if (!clear.exec("DELETE FROM task")) return 32;
    for (const auto &row : {QStringLiteral("S03-101"), QStringLiteral("S03-102"),
                            QStringLiteral("S03-103"), QStringLiteral("S03-104")}) {
        const QDate deadline = row == QStringLiteral("S03-101") ? today.addDays(1)
            : row == QStringLiteral("S03-102") ? today
            : row == QStringLiteral("S03-104") ? today.addDays(-1) : QDate();
        if (!insertTask(row, row, QStringLiteral("进行中"), deadline, thisMonth)) return 33;
    }
    home->refresh();
    app.processEvents();
    const auto overdueRows = home->findChildren<QFrame *>(QStringLiteral("InProgressItem"));
    const QStringList expectedOrder = {QStringLiteral("S03-102"), QStringLiteral("S03-101"),
                                       QStringLiteral("S03-104"), QStringLiteral("S03-103")};
    if (overdueRows.size() != expectedOrder.size()) return 34;
    for (int i = 0; i < overdueRows.size(); ++i) {
        if (!rowContains(overdueRows.at(i), expectedOrder.at(i))) return 35;
    }
    const auto *overdueCell = overdueRows.at(2)->findChild<QLabel *>(QStringLiteral("HomeDeadlineCell"));
    if (!overdueCell || overdueCell->text() != QStringLiteral("逾期")) return 36;
    if (!home->grab().save(QDir(root).filePath(QStringLiteral("home-overdue-order.png")))) return 37;
    QLabel *copyTitle=nullptr;
    for(auto *label:home->findChildren<QLabel *>(QStringLiteral("appUiTaskTitle")))
        if(label->isVisible()){copyTitle=label;break;}
    if(!copyTitle || !verifyLabelSelection(copyTitle)) return 180;
    QObject::disconnect(home,&HomeView::taskClicked,nullptr,nullptr);
    int opened=0;
    QObject::connect(home,&HomeView::taskClicked,home,[&opened](int id){if(id>0) ++opened;});
    const QPoint click=copyTitle->rect().center();
    selectionMouse(copyTitle,QEvent::MouseButtonPress,click);
    selectionMouse(copyTitle,QEvent::MouseButtonRelease,click);
    app.processEvents();
    if(opened!=1) return 181;

    DatabaseManager::instance().close();
    return 0;
}
