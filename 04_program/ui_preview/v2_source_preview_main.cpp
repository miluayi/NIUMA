#include <QApplication>
#include <QAbstractItemView>
#include <QCalendarWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QDateEdit>
#include <QDate>
#include <QGuiApplication>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QFontInfo>
#include <QFrame>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QRegion>
#include <QScrollArea>
#include <QScrollBar>
#include <QScreen>
#include <QSqlQuery>
#include <QTableView>
#include <QTableWidget>
#include <QTextEdit>
#include <QTimer>
#include <QTest>
#include <QWheelEvent>
#include <functional>
#include <memory>

#include "database/DatabaseManager.h"
#include "SchemeBStyler.h"
#include "ui/HomeView.h"
#include "ui/MainWindow.h"
#include "ui/TaskCreateDialog.h"
#include "ui/TaskDetailView.h"
#include "ui/widgets/ToastWidget.h"
#include "ui/ReferenceCreateDialog.h"
#include "ui/ReferenceView.h"
#include "ui/SettingsView.h"
#include "ui/PeriodicTaskView.h"
#include "ui/TaskListView.h"
#include "ui/widgets/DragDropArea.h"
#include "ui/widgets/PaginationWidget.h"

namespace {

void seedPreviewData()
{
    auto db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.exec(QStringLiteral("DELETE FROM task_remark"));
    query.exec(QStringLiteral("DELETE FROM achievement"));
    query.exec(QStringLiteral("DELETE FROM operation_log"));
    query.exec(QStringLiteral("DELETE FROM task"));

    const QStringList categories = {QStringLiteral("查"), QStringLiteral("查"),
                                    QStringLiteral("报"), QStringLiteral("办"),
                                    QStringLiteral("查")};
    const QStringList titles = {
        QStringLiteral("123"), QStringLiteral("办好"),
        QStringLiteral("1号办"), QStringLiteral("8查"), QStringLiteral("7查")};
    const QStringList sources = {QStringLiteral(""), QStringLiteral("收入核算"),
                                 QStringLiteral("党办"), QStringLiteral(""), QStringLiteral("")};

    for (int index = 0; index < 13; ++index) {
        query.prepare(QStringLiteral(
            "INSERT INTO task (task_no,title,category,source,tag,handler,status," 
            "is_key_task,deadline,is_from_periodic,processing_days,create_time,update_time) "
            "VALUES (?,?,?,?,?,?,?,?,?,0,?,datetime('now'),datetime('now'))"));
        query.addBindValue(QStringLiteral("20260901-%1").arg(13 - index, 3, 10, QLatin1Char('0')));
        query.addBindValue(titles.at(index % titles.size()));
        query.addBindValue(categories.at(index % categories.size()));
        query.addBindValue(sources.at(index % sources.size()));
        // 事项标签与重点任务标记是两个独立字段。
        query.addBindValue(QString());
        query.addBindValue(QStringLiteral("本人"));
        query.addBindValue(QStringLiteral("进行中"));
        query.addBindValue(index == 1 ? 1 : 0);
        const int deadlineOffsets[] = {0, 1, 2, 3, 7, -1, 4, 10};
        query.addBindValue(QDate::currentDate().addDays(deadlineOffsets[index % 8]).toString(Qt::ISODate));
        query.addBindValue(index < 3 ? 1 : 0);
        query.exec();
        query.clear();
    }
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("NIUMA V2 Source UI Preview"));
    app.setApplicationVersion(QStringLiteral("v2-source-preview"));
    const QString configuredDataDir = qEnvironmentVariable("NIUMA_PREVIEW_DATA_DIR");
    const QString dataDir = configuredDataDir.isEmpty()
        ? QString::fromUtf8(NIUMA_PREVIEW_DATA_DIR)
        : QDir::cleanPath(configuredDataDir);
    QDir().mkpath(dataDir);
    const QString dbPath = QDir(dataDir).filePath(QStringLiteral("database.db"));
    if (!DatabaseManager::instance().initialize(dbPath)) {
        return 2;
    }
    seedPreviewData();

    MainWindow window;
    const QString schemeTitle = QStringLiteral("NIUMA · 方案B");
    window.setWindowTitle(schemeTitle);
    if (auto *titlebar = window.findChild<QWidget *>(QStringLiteral("titlebar"))) {
        const auto labels = titlebar->findChildren<QLabel *>();
        if (labels.size() >= 2) {
            labels.at(1)->setText(schemeTitle);
        }
    }
    new SchemeBStyler(app, window);
    // 1440×900 leaves comfortable margins on a 1920×1080 desktop while
    // keeping the dense V2 tables readable. Fit smaller available desktops.
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        const QRect available = screen->availableGeometry();
        const QSize initial(qMin(1440, available.width() - 64),
                            qMin(900, available.height() - 64));
        window.resize(initial.expandedTo(window.minimumSize()));
        window.move(available.left() + (available.width() - window.width()) / 2,
                    available.top() + (available.height() - window.height()) / 2);
    }
    window.show();
    QTimer::singleShot(250, &window, [&window]() {
        window.raise();
        window.activateWindow();
    });

    QString captureDir;
    const QStringList arguments = app.arguments();
    for (int i = 1; i < arguments.size(); ++i) {
        if (arguments.at(i) == QStringLiteral("--capture-dir") && i + 1 < arguments.size()) {
            captureDir = arguments.at(i + 1);
        } else if (arguments.at(i).startsWith(QStringLiteral("--capture-dir="))) {
            captureDir = arguments.at(i).mid(QStringLiteral("--capture-dir=").size());
        }
    }
    if (arguments.contains(QStringLiteral("--verify-font-and-controls")) && !captureDir.isEmpty()) {
        QDir().mkpath(captureDir);
        QTimer::singleShot(900, &window, [&app, &window, captureDir]() {
            int fontMismatches = 0;
            int comboCount = 0;
            int missingArrows = 0;
            int checkCount = 0;
            int offCenterChecks = 0;
            QStringList examples;
            for (QWidget *widget : window.findChildren<QWidget *>()) {
                if (widget->isVisible() && !QFontInfo(widget->font()).family().startsWith(
                        QStringLiteral("MiSans"), Qt::CaseInsensitive)) {
                    ++fontMismatches;
                    if (examples.size() < 8) examples << QString::fromLatin1(widget->metaObject()->className());
                }
                if (auto *combo = qobject_cast<QComboBox *>(widget)) {
                    ++comboCount;
                    auto *arrow = combo->findChild<QLabel *>(QStringLiteral("schemeBComboArrow"));
                    if (!arrow || !arrow->isVisibleTo(combo)
                        || qAbs(arrow->geometry().center().y() - combo->rect().center().y()) > 1) {
                        ++missingArrows;
                    }
                }
                if (auto *box = qobject_cast<QCheckBox *>(widget)) {
                    ++checkCount;
                    auto *mark = box->findChild<QLabel *>(QStringLiteral("schemeBCheckMark"));
                    if (!mark) ++offCenterChecks;
                }
            }
            QFile report(QDir(captureDir).filePath(QStringLiteral("font-and-controls.txt")));
            if (report.open(QIODevice::WriteOnly | QIODevice::Text)) {
                report.write(QStringLiteral("app-font=%1 resolved=%2 installed=%3 visible-font-mismatches=%4 "
                                            "combos=%5 missing-arrows=%6 checks=%7 missing-checkmarks=%8 examples=%9\n")
                    .arg(app.font().family(), QFontInfo(app.font()).family())
                    .arg(QFontDatabase().families().contains(QStringLiteral("MiSans")))
                    .arg(fontMismatches).arg(comboCount).arg(missingArrows)
                    .arg(checkCount).arg(offCenterChecks).arg(examples.join(','))
                    .toUtf8());
            }
            app.exit(fontMismatches == 0 && comboCount > 0 && missingArrows == 0
                && checkCount > 0 && offCenterChecks == 0 ? 0 : 3);
        });
    } else if (arguments.contains(QStringLiteral("--shoot-calendar")) && !captureDir.isEmpty()) {
        QDir().mkpath(captureDir);
        QTimer::singleShot(900, &window, [&app, &window, captureDir]() {
            if (auto *sidebar = window.findChild<QListWidget *>(QStringLiteral("sidebar"))) {
                sidebar->setCurrentRow(1);
            }
            QTimer::singleShot(300, &window, [&app, &window, captureDir]() {
                auto *edit = window.findChild<QDateEdit *>();
                if (!edit) { app.exit(2); return; }
                QTest::mouseClick(edit, Qt::LeftButton);
                auto *popup = edit->findChild<QWidget *>(QStringLiteral("schemeBDatePopup"));
                if (!popup || !popup->isVisible()) { app.exit(2); return; }
                QTimer::singleShot(120, &window, [&app, captureDir, popup, edit]() {
                    const bool saved = popup->grab().save(
                        QDir(captureDir).filePath(QStringLiteral("calendar.png")), "PNG");
                    const bool inputSaved = edit->grab().save(
                        QDir(captureDir).filePath(QStringLiteral("calendar-input.png")), "PNG");
                    auto *today = popup->findChild<QPushButton *>(QStringLiteral("schemeBDateQuick"));
                    if (today) QTest::mouseClick(today, Qt::LeftButton);
                    app.exit(saved && inputSaved && today && edit->date() == QDate::currentDate() ? 0 : 2);
                });
            });
        });
    } else if (arguments.contains(QStringLiteral("--shoot-combo")) && !captureDir.isEmpty()) {
        QDir().mkpath(captureDir);
        QTimer::singleShot(900, &window, [&app, &window, captureDir]() {
            QTimer::singleShot(0, &window, [&window]() {
                if (auto *create = window.findChild<QPushButton *>(QStringLiteral("shellCreate"))) {
                    create->click();
                }
            });
            QTimer::singleShot(350, &window, [&app, &window, captureDir]() {
                auto *task = window.findChild<TaskCreateDialog *>();
                QComboBox *combo = nullptr;
                if (task) {
                    for (QComboBox *candidate : task->findChildren<QComboBox *>()) {
                        if (candidate->isEditable()) { combo = candidate; break; }
                    }
                }
                if (!combo) { if (task) task->reject(); app.exit(2); return; }
                if (combo->count() < 5) {
                    combo->addItems({QStringLiteral("征管股"), QStringLiteral("风险管理股"),
                                     QStringLiteral("税政股"), QStringLiteral("纳税服务"),
                                     QStringLiteral("行政办公室"), QStringLiteral("法制股")});
                }
                combo->showPopup();
                QTimer::singleShot(250, task, [&app, task, combo, captureDir]() {
                    QWidget *popup = combo->view()->window();
                    const QImage image = popup->grab().toImage();
                    const bool saved = image.save(
                        QDir(captureDir).filePath(QStringLiteral("combo-popup.png")), "PNG");
                    const QRegion mask = popup->mask();
                    const bool roundedCorners = !mask.isEmpty()
                        && !mask.contains(QPoint(0, 0))
                        && !mask.contains(QPoint(image.width()-1, 0))
                        && !mask.contains(QPoint(0, image.height()-1))
                        && !mask.contains(QPoint(image.width()-1, image.height()-1));
                    QFile report(QDir(captureDir).filePath(QStringLiteral("combo-popup.txt")));
                    if (report.open(QIODevice::WriteOnly | QIODevice::Text)) {
                        report.write(QStringLiteral("popup=%1 view=%2 size=%3x%4 rounded-mask=%5 styled=%6 mask-empty=%7 corner-alpha=%8,%9,%10,%11\n")
                            .arg(popup->metaObject()->className(), combo->view()->metaObject()->className())
                            .arg(image.width()).arg(image.height())
                            .arg(roundedCorners)
                            .arg(popup->property("schemeBTransparentCorners").toBool())
                            .arg(mask.isEmpty())
                            .arg(image.pixelColor(0, 0).alpha())
                            .arg(image.pixelColor(image.width()-1, 0).alpha())
                            .arg(image.pixelColor(0, image.height()-1).alpha())
                            .arg(image.pixelColor(image.width()-1, image.height()-1).alpha()).toUtf8());
                    }
                    combo->hidePopup();
                    task->reject();
                    app.exit(saved && roundedCorners ? 0 : 3);
                });
            });
        });
    } else if (arguments.contains(QStringLiteral("--shoot-dialogs")) && !captureDir.isEmpty()) {
        QDir().mkpath(captureDir);
        QTimer::singleShot(900, &window, [&app, &window, captureDir]() {
            QTimer::singleShot(0, &window, [&window]() {
                if (auto *create = window.findChild<QPushButton *>(QStringLiteral("shellCreate"))) {
                    create->click();
                }
            });
            QTimer::singleShot(350, &window, [&app, &window, captureDir]() {
                auto *task = window.findChild<TaskCreateDialog *>();
                auto *description = task ? task->findChild<QTextEdit *>() : nullptr;
                auto *files = task ? task->findChild<DragDropArea *>() : nullptr;
                auto *date = task ? task->findChild<QDateEdit *>() : nullptr;
                if (!task || !description || !files || !date) { app.exit(2); return; }
                const bool taskSaved = task->grab().save(
                    QDir(captureDir).filePath(QStringLiteral("new-task.png")), "PNG");
                const bool taskSizes = description->height() == 200 && files->height() == 80;
                QTest::mouseClick(date, Qt::LeftButton);
                auto *popup = date->findChild<QWidget *>(QStringLiteral("schemeBDatePopup"));
                auto *heading = popup ? popup->findChild<QLabel *>(QStringLiteral("schemeBDateHeading")) : nullptr;
                const bool today = heading && heading->text() == QStringLiteral("%1年%2月")
                    .arg(QDate::currentDate().year()).arg(QDate::currentDate().month());
                const bool calendarSaved = popup && popup->grab().save(
                    QDir(captureDir).filePath(QStringLiteral("new-task-calendar.png")), "PNG");
                if (popup) popup->hide();
                task->reject();
                QTimer::singleShot(150, &window, [&app, &window, captureDir,
                                                   taskSaved, taskSizes, today, calendarSaved]() {
                    if (auto *sidebar = window.findChild<QListWidget *>(QStringLiteral("sidebar"))) {
                        sidebar->setCurrentRow(3);
                    }
                    QTimer::singleShot(0, &window, [&window]() {
                        if (auto *view = window.findChild<ReferenceView *>()) {
                            if (auto *create = view->findChild<QPushButton *>(QStringLiteral("btnPrimary"))) {
                                create->click();
                            }
                        }
                    });
                    QTimer::singleShot(350, &window, [&app, &window, captureDir,
                                                       taskSaved, taskSizes, today, calendarSaved]() {
                        auto *reference = window.findChild<ReferenceCreateDialog *>();
                        auto *description = reference ? reference->findChild<QTextEdit *>() : nullptr;
                        auto *files = reference ? reference->findChild<DragDropArea *>() : nullptr;
                        const bool referenceSaved = reference && reference->grab().save(
                            QDir(captureDir).filePath(QStringLiteral("new-reference.png")), "PNG");
                        const bool referenceSizes = description && files
                            && description->height() == 200 && files->height() == 96;
                        if (reference) reference->reject();
                        app.exit(taskSaved && taskSizes && today && calendarSaved
                                 && referenceSaved && referenceSizes ? 0 : 2);
                    });
                });
            });
        });
    } else if (arguments.contains(QStringLiteral("--shoot-home-bottom")) && !captureDir.isEmpty()) {
        QDir().mkpath(captureDir);
        QTimer::singleShot(900, &window, [&app, &window, captureDir]() {
            auto *home = window.findChild<HomeView *>();
            auto *pagination = home ? home->findChild<PaginationWidget *>() : nullptr;
            auto *scroll = home ? qobject_cast<QScrollArea *>(home->parentWidget()->parentWidget()) : nullptr;
            if (!pagination || !scroll) { app.exit(2); return; }
            scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum());
            QTimer::singleShot(120, &window, [&app, &window, captureDir, home, pagination]() {
                int firstCount = 0;
                for (QFrame *row : home->findChildren<QFrame *>(QStringLiteral("InProgressItem"))) {
                    if (row->isVisible()) ++firstCount;
                }
                const bool firstSaved = window.grab().save(
                    QDir(captureDir).filePath(QStringLiteral("home-bottom-page1.png")), "PNG");
                QFile report(QDir(captureDir).filePath(QStringLiteral("home-pagination.txt")));
                if (report.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    report.write(QStringLiteral("pagination=%1,%2 %3x%4\n")
                        .arg(pagination->x()).arg(pagination->y())
                        .arg(pagination->width()).arg(pagination->height()).toUtf8());
                    for (QPushButton *button : pagination->findChildren<QPushButton *>()) {
                        report.write(QStringLiteral("button=%1 x=%2 y=%3 visible=%4 enabled=%5\n")
                            .arg(button->text()).arg(button->x()).arg(button->y())
                            .arg(button->isVisible()).arg(button->isEnabled()).toUtf8());
                    }
                }
                QPushButton *secondPage = nullptr;
                for (QPushButton *button : pagination->findChildren<QPushButton *>()) {
                    if (button->text() == QStringLiteral("2") && button->isVisible()) {
                        secondPage = button;
                        break;
                    }
                }
                if (!secondPage) { app.exit(2); return; }
                QTest::mouseClick(secondPage, Qt::LeftButton);
                QTimer::singleShot(300, &window, [&app, &window, captureDir, home, firstCount, firstSaved]() {
                    int secondCount = 0;
                    for (QFrame *row : home->findChildren<QFrame *>(QStringLiteral("InProgressItem"))) {
                        if (row->isVisible()) ++secondCount;
                    }
                    const bool secondSaved = window.grab().save(
                        QDir(captureDir).filePath(QStringLiteral("home-bottom-page2.png")), "PNG");
                    app.exit(firstSaved && secondSaved && firstCount == 10 && secondCount == 3 ? 0 : 2);
                });
            });
        });
    } else if (arguments.contains(QStringLiteral("--verify-sidebar-resize")) && !captureDir.isEmpty()) {
        QDir().mkpath(captureDir);
        QTimer::singleShot(900, &window, [&app, &window, captureDir]() {
            auto *sidebar = window.findChild<QListWidget *>(QStringLiteral("sidebar"));
            if (!sidebar) { app.exit(2); return; }
            const QSize initial = window.size();
            const int original = sidebar->visualItemRect(sidebar->item(8)).top();
            window.resize(1000, 650);
            QApplication::processEvents();
            const int small = sidebar->visualItemRect(sidebar->item(8)).top();
            window.resize(1440, 900);
            QApplication::processEvents();
            const int large = sidebar->visualItemRect(sidebar->item(8)).top();
            QFile report(QDir(captureDir).filePath(QStringLiteral("sidebar-resize.txt")));
            if (report.open(QIODevice::WriteOnly | QIODevice::Text)) {
                report.write(QStringLiteral("initial=%1x%2 settings-y=%3,%4,%5\n")
                    .arg(initial.width()).arg(initial.height()).arg(original)
                    .arg(small).arg(large).toUtf8());
            }
            app.exit(initial.width() == 1440 && initial.height() == 900
                && original == small && small == large ? 0 : 3);
        });
    } else if (arguments.contains(QStringLiteral("--verify-nested-wheel")) && !captureDir.isEmpty()) {
        QDir().mkpath(captureDir);
        QTimer::singleShot(900, &window, [&app, &window, captureDir]() {
            window.findChild<QListWidget *>(QStringLiteral("sidebar"))->setCurrentRow(8);
            auto *settings = window.findChild<SettingsView *>();
            auto lists = settings ? settings->findChildren<QListWidget *>() : QList<QListWidget *>();
            QScrollArea *outer = nullptr;
            for (QWidget *parent = settings; parent; parent = parent->parentWidget()) {
                if ((outer = qobject_cast<QScrollArea *>(parent))) break;
            }
            if (lists.isEmpty() || !outer) { app.exit(2); return; }
            auto *inner = lists.first();
            for (int i = 0; i < 30; ++i) inner->addItem(QStringLiteral("测试来源 %1").arg(i));
            outer->verticalScrollBar()->setValue(100);
            const int outerBefore = outer->verticalScrollBar()->value();
            inner->verticalScrollBar()->setValue(0);
            QWidget *row = inner->itemWidget(inner->item(0));
            if (!row) { app.exit(2); return; }
            const QPointF rowLocal = row->rect().center();
            QWheelEvent wheel(rowLocal, row->mapToGlobal(rowLocal.toPoint()),
                              QPoint(), QPoint(0, -120), Qt::NoButton, Qt::NoModifier,
                              Qt::NoScrollPhase, false);
            QApplication::sendEvent(row, &wheel);
            QTimer::singleShot(260, &window, [&app, &window, captureDir, inner, outer, outerBefore]() {
                const int innerAfter = inner->verticalScrollBar()->value();
                inner->verticalScrollBar()->setValue(inner->verticalScrollBar()->maximum());
                const QPoint global = inner->viewport()->mapToGlobal(inner->viewport()->rect().center());
                const QPointF outerLocal = outer->viewport()->mapFromGlobal(global);
                QWheelEvent atEdge(outerLocal, global,
                                   QPoint(), QPoint(0, -120), Qt::NoButton, Qt::NoModifier,
                                   Qt::NoScrollPhase, false);
                QApplication::sendEvent(outer->viewport(), &atEdge);
                const int outerAtEdge = outer->verticalScrollBar()->value();
                inner->clear();
                outer->verticalScrollBar()->setValue(outerBefore);
                QApplication::sendEvent(outer->viewport(), &atEdge);
                const int outerWithEmptyList = outer->verticalScrollBar()->value();
                QFile report(QDir(captureDir).filePath(QStringLiteral("nested-wheel.txt")));
                if (report.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    report.write(QStringLiteral("inner=%1 outer-before=%2 outer-at-edge=%3 outer-empty-list=%4\n")
                        .arg(innerAfter).arg(outerBefore).arg(outerAtEdge)
                        .arg(outerWithEmptyList).toUtf8());
                }
                app.exit(innerAfter > 0 && outerBefore == outerAtEdge
                    && outerBefore == outerWithEmptyList ? 0 : 3);
            });
        });
    } else if (arguments.contains(QStringLiteral("--verify-horizontal-wheel")) && !captureDir.isEmpty()) {
        QDir().mkpath(captureDir);
        QTimer::singleShot(900, &window, [&app, &window, captureDir]() {
            auto bars = std::make_shared<QList<QScrollBar *>>();
            for (QWidget *page : {static_cast<QWidget *>(window.findChild<TaskListView *>()),
                                  static_cast<QWidget *>(window.findChild<PeriodicTaskView *>()),
                                  static_cast<QWidget *>(window.findChild<ReferenceView *>())}) {
                auto *table = page ? page->findChild<QTableView *>() : nullptr;
                if (!table) continue;
                table->verticalScrollBar()->setRange(0, 0);
                table->horizontalScrollBar()->setRange(0, 200);
                table->horizontalScrollBar()->setValue(0);
                const QPointF local = table->viewport()->rect().center();
                QWheelEvent wheel(local, table->viewport()->mapToGlobal(local.toPoint()),
                                  QPoint(), QPoint(0, -120), Qt::NoButton, Qt::NoModifier,
                                  Qt::NoScrollPhase, false);
                QApplication::sendEvent(table->viewport(), &wheel);
                bars->append(table->horizontalScrollBar());
            }
            QTimer::singleShot(300, &window, [&app, captureDir, bars]() {
                int passed = 0;
                QStringList lines;
                for (QScrollBar *bar : *bars) {
                    const int value = bar->value();
                    if (value == 36) ++passed;
                    lines << QString::number(value);
                }
                QFile report(QDir(captureDir).filePath(QStringLiteral("horizontal-wheel.txt")));
                if (report.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    report.write(QStringLiteral("passed=%1 %2\n").arg(passed).arg(lines.join(','))
                        .toUtf8());
                }
                app.exit(passed == 3 ? 0 : 3);
            });
        });
    } else if (arguments.contains(QStringLiteral("--verify-scroll")) && !captureDir.isEmpty()) {
        QDir().mkpath(captureDir);
        QTimer::singleShot(900, &window, [&app, &window, captureDir]() {
            auto *settings = window.findChild<SettingsView *>();
            QScrollArea *area = nullptr;
            for (QWidget *parent = settings; parent; parent = parent->parentWidget()) {
                if ((area = qobject_cast<QScrollArea *>(parent))) break;
            }
            if (!area || area->verticalScrollBar()->maximum() < 36) { app.exit(2); return; }
            area->verticalScrollBar()->setValue(0);
            const QPointF local = area->viewport()->rect().center();
            QWheelEvent wheel(local, area->viewport()->mapToGlobal(local.toPoint()),
                              QPoint(), QPoint(0, -120), Qt::NoButton, Qt::NoModifier,
                              Qt::NoScrollPhase, false);
            QApplication::sendEvent(area->viewport(), &wheel);
            QTimer::singleShot(80, &window, [&app, &window, area, captureDir]() {
                const int middle = area->verticalScrollBar()->value();
                QTimer::singleShot(180, &window, [&app, &window, area, captureDir, middle]() {
                    const int end = area->verticalScrollBar()->value();
                    bool pixelLists = true;
                    for (QAbstractItemView *view : window.findChildren<QAbstractItemView *>()) {
                        if (qobject_cast<QTableView *>(view)
                            || qobject_cast<QTableWidget *>(view)) {
                            pixelLists &= view->verticalScrollMode()
                                == QAbstractItemView::ScrollPerPixel;
                        }
                    }
                    QFile report(QDir(captureDir).filePath(QStringLiteral("scroll-check.txt")));
                    if (report.open(QIODevice::WriteOnly | QIODevice::Text)) {
                        report.write(QStringLiteral("middle=%1 end=%2 pixel-lists=%3\n")
                            .arg(middle).arg(end).arg(pixelLists).toUtf8());
                    }
                    app.exit(middle > 0 && middle < end && end == 36 && pixelLists ? 0 : 3);
                });
            });
        });
    } else if (arguments.contains(QStringLiteral("--shoot-refresh")) && !captureDir.isEmpty()) {
        QDir().mkpath(captureDir);
        QTimer::singleShot(900, &window, [&app, &window, captureDir]() {
            auto *titlebar = window.findChild<QWidget *>(QStringLiteral("titlebar"));
            QPushButton *reload = nullptr;
            if (titlebar) {
                for (QPushButton *button : titlebar->findChildren<QPushButton *>()) {
                    if (button->toolTip() == QStringLiteral("刷新当前页面")) {
                        reload = button;
                        break;
                    }
                }
            }
            if (!reload) { app.exit(2); return; }
            reload->click();
            QTimer::singleShot(250, &window, [&app, &window, captureDir]() {
                const bool toastVisible = !window.findChildren<ToastWidget *>().isEmpty();
                const bool saved = window.grab().save(
                    QDir(captureDir).filePath(QStringLiteral("refresh-toast.png")), "PNG");
                app.exit(toastVisible && saved ? 0 : 3);
            });
        });
    } else if ((arguments.contains(QStringLiteral("--shoot-detail"))
                || arguments.contains(QStringLiteral("--shoot-detail-long")))
               && !captureDir.isEmpty()) {
        QDir().mkpath(captureDir);
        const bool longTitle = arguments.contains(QStringLiteral("--shoot-detail-long"));
        QTimer::singleShot(900, &window, [&app, &window, captureDir, longTitle]() {
            QSqlQuery query;
            if (!query.exec(QStringLiteral("SELECT id FROM task WHERE is_key_task=1 LIMIT 1"))
                || !query.next()) { app.exit(2); return; }
            const int taskId = query.value(0).toInt();
            if (longTitle) {
                QSqlQuery update;
                update.prepare(QStringLiteral("UPDATE task SET title=? WHERE id=?"));
                update.addBindValue(QStringLiteral(
                    "关于推进重点项目资料核查、阶段汇总和后续事项落实情况的专项任务，"
                    "需持续跟踪并及时反馈办理结果"));
                update.addBindValue(taskId);
                if (!update.exec()) { app.exit(2); return; }
            }
            auto *detail = new TaskDetailView(taskId, &window);
            detail->show();
            QTimer::singleShot(450, detail, [&app, detail, captureDir, longTitle]() {
                auto *basic = detail->findChild<QFrame *>(QStringLiteral("basicInfoSection"));
                auto *button = detail->findChild<QPushButton *>(QStringLiteral("openFolderBtn"));
                auto *title = detail->findChild<QLabel *>(QStringLiteral("schemeBDetailTitle"));
                const bool moved = basic && button && basic->isAncestorOf(button);
                const bool saved = detail->grab().save(
                    QDir(captureDir).filePath(longTitle
                        ? QStringLiteral("task-detail-long.png")
                        : QStringLiteral("task-detail.png")), "PNG");
                QFile report(QDir(captureDir).filePath(longTitle
                    ? QStringLiteral("task-detail-long.txt")
                    : QStringLiteral("task-detail.txt")));
                if (report.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    report.write(QStringLiteral("folder-in-basic=%1 title-height=%2 wrapped=%3\n")
                        .arg(moved).arg(title ? title->height() : 0)
                        .arg(title && title->wordWrap()).toUtf8());
                }
                app.exit(moved && saved && title && title->wordWrap()
                    && (!longTitle || title->height() > 40) ? 0 : 3);
            });
        });
    } else if (arguments.contains(QStringLiteral("--shoot-all")) && !captureDir.isEmpty()) {
        QDir().mkpath(captureDir);
        QTimer::singleShot(900, &window, [&app, &window, captureDir]() {
            auto *sidebar = window.findChild<QListWidget *>(QStringLiteral("sidebar"));
            if (!sidebar) { app.exit(2); return; }
            auto capturePage = std::make_shared<std::function<void(int)>>();
            *capturePage = [&app, &window, captureDir, sidebar, capturePage](int page) {
                if (page >= 8) { app.exit(0); return; }
                sidebar->setCurrentRow(page == 7 ? 8 : page);
                QTimer::singleShot(250, &window, [&window, captureDir, capturePage, page]() {
                    window.grab().save(QDir(captureDir).filePath(
                        QStringLiteral("page-%1.png").arg(page)), "PNG");
                    (*capturePage)(page + 1);
                });
            };
            (*capturePage)(0);
        });
    } else if (arguments.contains(QStringLiteral("--shoot-focus")) && !captureDir.isEmpty()) {
        QDir().mkpath(captureDir);
        QTimer::singleShot(900, &window, [&app, &window, captureDir]() {
            auto *home = window.findChild<HomeView *>();
            auto *scroll = home ? qobject_cast<QScrollArea *>(home->parentWidget()->parentWidget()) : nullptr;
            auto *bar = scroll ? scroll->verticalScrollBar() : nullptr;
            QPushButton *star = nullptr;
            if (home) {
                for (QPushButton *button : home->findChildren<QPushButton *>()) {
                    if (button->property("taskId").isValid() && button->property("focused").isValid()) {
                        star = button;
                        break;
                    }
                }
            }
            if (!star || !bar) { app.exit(2); return; }
            const int before = bar->value();
            window.grab().save(QDir(captureDir).filePath(QStringLiteral("focus-before.png")), "PNG");
            QTest::mouseClick(star, Qt::LeftButton);
            QTimer::singleShot(200, &window, [&app, &window, captureDir, bar, before]() {
                const int after = bar->value();
                window.grab().save(QDir(captureDir).filePath(QStringLiteral("focus-after.png")), "PNG");
                QFile report(QDir(captureDir).filePath(QStringLiteral("focus-scroll.txt")));
                if (report.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    report.write(QStringLiteral("before=%1 after=%2\n").arg(before).arg(after).toUtf8());
                }
                app.exit(before == after ? 0 : 3);
            });
        });
    } else if (arguments.contains(QStringLiteral("--shoot")) && !captureDir.isEmpty()) {
        QDir().mkpath(captureDir);
        QTimer::singleShot(900, &window, [&app, &window, captureDir]() {
            const QString path = QDir(captureDir).filePath(QStringLiteral("v2-source-home-1440x900.png"));
            const bool saved = window.grab().save(path, "PNG");
            app.exit(saved ? 0 : 2);
        });
    }
    const int result = app.exec();
    DatabaseManager::instance().close();
    return result;
}
