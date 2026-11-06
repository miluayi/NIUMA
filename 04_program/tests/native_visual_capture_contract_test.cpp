#include <QApplication>
#include "text_selection_contract.h"
#include "detail_title_contract.h"
#include "category_badge_contract.h"
#include "widgets/CategoryBadge.h"
#include <QTextEdit>
#include "ReferenceDetailDialog.h"
#include <QPlainTextEdit>
#include "widgets/TaskItemDelegate.h"
#include "models/Task.h"
#include <QScreen>
#include <QTextLayout>
#include <QGlyphRun>
#include <QThread>
#include <QComboBox>
#include <QCompleter>
#include <QAbstractItemView>
#include <QElapsedTimer>
#include <QDateEdit>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFrame>
#include <QFontInfo>
#include <QFontDatabase>
#include <QRawFont>
#include <QSet>
#include <QLabel>
#include <QSqlQuery>
#include <QTextStream>
#include <QListWidget>
#include <QMouseEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QPixmap>
#include <QStackedWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QStyleFactory>
#include <QTableView>
#include <QTableWidget>
#include <QHelpEvent>
#include <QToolTip>
#include <QCursor>

#include "DatabaseManager.h"
#include "MainWindow.h"
#include "AppTheme.h"
#include "PathUtil.h"
#include "SettingsView.h"
#include "TaskDetailView.h"
#include "TaskCreateDialog.h"
#include "ReferenceCreateDialog.h"
#include "TaskEditDialog.h"
#include "ReferenceEditDialog.h"
#include "PeriodicTaskCreateDialog.h"
#include "widgets/YearPicker.h"
#include "ui/widgets/DragDropArea.h"

static bool capturePopupFrame(QWidget *popup, const QString &directory, const QString &name)
{
    if (!popup || !popup->isVisible() || !popup->mask().isEmpty()
        || !popup->property("nativePopupHandle").toULongLong()) return false;
    popup->move(popup->screen()->availableGeometry().topLeft()+QPoint(24,24));
    popup->raise();
    QApplication::processEvents();
    QThread::msleep(100);
    const QPoint corner=popup->mapToGlobal(QPoint());
    return popup->screen()->grabWindow(0,corner.x()-6,corner.y()-6,64,64)
        .save(QDir(directory).filePath(name+"-native-frame.png"));
}

static bool verifyPopupImage(QWidget *popup, const QString &directory, const QString &name)
{
    QElapsedTimer wait;
    wait.start();
    while (!popup->isVisible() && wait.elapsed()<1500) {
        QApplication::processEvents(); QThread::msleep(5);
    }
    QApplication::processEvents();
    const QImage image=popup->grab().toImage().convertToFormat(QImage::Format_ARGB32);
    bool valid=popup->isVisible() && !image.isNull()
        && image.save(QDir(directory).filePath(name+"-dropdown.png"));
    const QPoint position=popup->mapToGlobal(QPoint());
    const QImage desktop=popup->screen()->grabWindow(0,position.x(),position.y(),popup->width(),popup->height()).toImage();
    valid=valid && !desktop.isNull() && desktop.save(QDir(directory).filePath(name+"-desktop.png"));
    if (!desktop.isNull()) {
        const QColor surface=desktop.pixelColor(qRound(12*popup->devicePixelRatioF()),desktop.height()/2);
        valid=valid && surface.red()>235 && surface.green()>235 && surface.blue()>235;
    }
    if (valid) {
        // The surface and its shadow must both end in transparent corners.
        for (QPoint point : {QPoint(0,0),QPoint(image.width()-1,0),
                             QPoint(0,image.height()-1),QPoint(image.width()-1,image.height()-1)})
            valid=valid && qAlpha(image.pixel(point))==0;
        const qreal scale=popup->devicePixelRatioF();
        const int inset=qRound(9*scale);
        // No white rectangle in the arcs, and a genuine soft alpha shadow below.
        for (QPoint point : {QPoint(inset,inset),QPoint(image.width()-1-inset,inset),
                             QPoint(inset,image.height()-1-inset),
                             QPoint(image.width()-1-inset,image.height()-1-inset)})
            valid=valid && qAlpha(image.pixel(point))<90;
        const int shadow=qAlpha(image.pixel(image.width()/2,image.height()-qRound(4*scale)));
        valid=valid && shadow>0 && shadow<90
            && qAlpha(image.pixel(image.width()/2,image.height()/2))==255;
#ifdef Q_OS_WIN
        const HWND handle=reinterpret_cast<HWND>(popup->internalWinId());
        valid=valid && !(GetWindowLongPtrW(handle,GWL_STYLE)&(WS_CAPTION|WS_THICKFRAME))
            && popup->mask().isEmpty();
        RECT client,bounds;
        GetClientRect(handle,&client);
        GetWindowRect(handle,&bounds);
        valid=valid && client.right==bounds.right-bounds.left && client.bottom==bounds.bottom-bounds.top
            && client.right==qRound(popup->width()*scale) && client.bottom==qRound(popup->height()*scale);
#endif
    }
    if (!valid) fprintf(stderr,"Dropdown corner/shadow failed: %s surface=%d visible=%d type=%x size=%dx%d position=%d,%d style=%s\n",
        qPrintable(name),popup->property("appUiPopupSurface").toBool(),popup->isVisible(),int(popup->windowType()),
        popup->width(),popup->height(),position.x(),position.y(),qPrintable(popup->styleSheet()));
    return valid;
}

static bool verifyComboCorners(QComboBox *combo, const QString &directory, const QString &name)
{
    combo->showPopup();
    // Qt's opaque QRollEffect window must never cover the alpha popup, even
    // during the first 150 ms (the old steady-state grabs missed this).
    for (auto *window : QApplication::topLevelWidgets())
        if (window->isVisible() && QByteArray(window->metaObject()->className())=="QRollEffect") return false;
    QWidget *popup=combo->view()->window();
    if (name=="Windows-single") {
        for (int frame=0;frame<3;++frame) {
            if (frame) QThread::msleep(35);
            QApplication::processEvents();
            if (!popup->grab().save(QDir(directory).filePath(QString("combo-opening-%1.png").arg(frame)))) return false;
        }
    }
    const bool valid=verifyPopupImage(popup,directory,name);
    const int original=combo->currentIndex();
    // Retain normal keyboard choice and Escape dismissal with the new surface.
    QKeyEvent end(QEvent::KeyPress,Qt::Key_End,Qt::NoModifier);
    QKeyEvent enter(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier);
    QApplication::sendEvent(combo->view(),&end);
    QApplication::sendEvent(combo->view(),&enter);
    const bool selected=!popup->isVisible() && combo->currentIndex()==combo->count()-1;
    combo->setCurrentIndex(original);
    combo->showPopup();
    QKeyEvent escape(QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier);
    QApplication::sendEvent(combo->view(),&escape);
    const bool dismissed=!popup->isVisible() && combo->currentIndex()==original;
    combo->hidePopup();
    QApplication::processEvents();
    return valid && selected && dismissed;
}

static bool waitForTooltipHidden()
{
    QElapsedTimer timer;
    timer.start();
    while (QToolTip::isVisible() && timer.elapsed()<1500) {
        QApplication::processEvents(); QThread::msleep(10);
    }
    return !QToolTip::isVisible();
}

static bool verifyOverflowTooltips(const QString &directory)
{
    QTableWidget table(1,2);
    table.resize(520,180);
    table.move(80,80);
    const QString longText=QStringLiteral("完整名称 <原文> ")+QString(45,QChar(0x725B));
    table.setItem(0,0,new QTableWidgetItem(QStringLiteral("短名称")));
    table.setItem(0,1,new QTableWidgetItem(longText));
    table.setColumnWidth(0,180);
    table.setColumnWidth(1,180);
    table.show(); table.raise(); table.activateWindow(); QApplication::processEvents();
    auto hover=[&](int column) {
        const QPoint pos=table.visualItemRect(table.item(0,column)).center();
        const QPoint global=table.viewport()->mapToGlobal(pos);
        QCursor::setPos(global);
        QApplication::processEvents();
        QHelpEvent event(QEvent::ToolTip,pos,global);
        QApplication::sendEvent(table.viewport(),&event);
    };
    hover(0);
    if (!QToolTip::text().isEmpty()) {fprintf(stderr,"Unexpected short tooltip: %s\n",qPrintable(QToolTip::text()));return false;}
    hover(1);
    if (!QToolTip::text().contains(QString(45,QChar(0x725B)))
        || !QToolTip::text().contains("&lt;原文&gt;")) {fprintf(stderr,"Missing long tooltip: %s\n",qPrintable(QToolTip::text()));return false;}
    if (!table.grab().save(QDir(directory).filePath("overflow-table.png"))) {fprintf(stderr,"Failed table capture\n");return false;}
    Task task;
    task.title=longText;
    task.isKeyTask=true;
    table.item(0,1)->setData(Qt::UserRole,QVariant::fromValue(task));
    table.setItemDelegateForColumn(1,new TaskItemDelegate(&table));
    hover(1);
    if (!QToolTip::text().contains(QStringLiteral("【重点】"))) {fprintf(stderr,"Missing custom tooltip: %s\n",qPrintable(QToolTip::text()));return false;}
    QElapsedTimer paintTimer;
    paintTimer.start();
    while (paintTimer.elapsed()<300) {QApplication::processEvents();QThread::msleep(10);}
    bool capturedBubble=false;
    for (auto *window : QApplication::topLevelWidgets()) {
        if (window->isVisible() && QByteArray(window->metaObject()->className())=="QTipLabel") {
            const QImage bubble=window->grab().toImage();
            int lightPixels=0;
            for (int y=0;y<bubble.height();++y) for (int x=0;x<bubble.width();++x)
                if (bubble.pixelColor(x,y).lightness()>200) ++lightPixels;
            capturedBubble=lightPixels>bubble.width()*bubble.height()/2
                && bubble.save(QDir(directory).filePath("overflow-bubble.png"));
            const QPoint pos=window->mapToGlobal(QPoint());
            window->screen()->grabWindow(0,pos.x(),pos.y(),window->width(),window->height())
                .save(QDir(directory).filePath("overflow-bubble-desktop.png"));
        }
    }
    if (!capturedBubble) {fprintf(stderr,"Missing bubble capture\n");return false;}
    QToolTip::hideText();
    table.setColumnWidth(1,1200);
    table.resize(1500,180);
    QApplication::processEvents();
    hover(1);
    if (!waitForTooltipHidden()) {fprintf(stderr,"Wide cell tooltip remained\n");return false;}
    table.hide();
    QWidget homeRow;
    homeRow.setObjectName("InProgressItem");
    homeRow.resize(220,60);
    QLabel label(&homeRow);
    label.setTextFormat(Qt::RichText);
    label.setText("<span>"+longText.toHtmlEscaped()+"</span>");
    label.setGeometry(0,0,180,40);
    homeRow.show();homeRow.raise();homeRow.activateWindow();QApplication::processEvents();
    QCursor::setPos(label.mapToGlobal(QPoint(20,20)));
    QApplication::processEvents();
    QHelpEvent event(QEvent::ToolTip,QPoint(20,20),label.mapToGlobal(QPoint(20,20)));
    QApplication::sendEvent(&label,&event);
    if (!QToolTip::text().contains(QString(45,QChar(0x725B)))) {fprintf(stderr,"Missing home tooltip: %s\n",qPrintable(QToolTip::text()));return false;}
    label.setText(QStringLiteral("已完整显示"));
    QApplication::sendEvent(&label,&event);
    if (!waitForTooltipHidden()) {fprintf(stderr,"Short home tooltip remained\n");return false;}
    return true;
}

static bool verifyAllCombos(QWidget *root, const QString &directory, const QString &prefix)
{
    int number=0;
    for (auto *combo : root->findChildren<QComboBox *>()) {
        if (!combo->isVisible() || combo->count()==0 || dynamic_cast<YearPicker *>(combo)) continue;
        for (QWidget *ancestor=combo->parentWidget();ancestor;ancestor=ancestor->parentWidget())
            if (auto *scroll=qobject_cast<QScrollArea *>(ancestor)) scroll->ensureWidgetVisible(combo,0,0);
        QApplication::processEvents();
        // At high DPI the test main window's minimum size can exceed the screen.
        // Bring the tested control onto the monitor before opening its popup.
        const QRect available=combo->screen()->availableGeometry();
        const QPoint anchor=combo->mapToGlobal(combo->rect().center());
        if (!available.adjusted(20,40,-20,-40).contains(anchor)) {
            QWidget *window=combo->window();
            window->move(window->pos()+available.center()-anchor);
            QApplication::processEvents();
        }
        const QString name=prefix+"-"+QString::number(++number)+"-"+combo->objectName();
        if (!verifyComboCorners(combo,directory,name)) return false;
        if (auto *completer=combo->completer()) {
            // Inline completion has no dropdown in the product.
            if (completer->completionMode()==QCompleter::InlineCompletion) continue;
            completer->setCompletionPrefix(QString());
            completer->complete();
            const bool valid=verifyPopupImage(completer->popup(),directory,name+"-completion");
            if (completer->completionCount()<=completer->maxVisibleItems()
                && completer->popup()->verticalScrollBar()->maximum()>0) return false;
            completer->popup()->hide();
            if (!valid) return false;
        }
    }
    return true;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    if (app.arguments().contains(QStringLiteral("--legacy-chrome")))
        app.setProperty("niumaLegacyWindowChrome",true);
    const QString root = qEnvironmentVariable("NIUMA_TEST_ROOT");
    const QString captureDir = qEnvironmentVariable("NIUMA_SCREENSHOT_DIR");
    if (root.isEmpty() || captureDir.isEmpty()) return 1;
    qputenv("NIUMA_DATA_DIR", QDir(root).filePath("Data").toUtf8());
    if (!QDir().mkpath(root) || !QDir().mkpath(captureDir)
        || !DatabaseManager::instance().initialize(PathUtil::databasePath())) return 2;
    QSqlQuery fixture(DatabaseManager::instance().database());
    if (!fixture.exec(QStringLiteral("DELETE FROM task"))) return 29;
    fixture.prepare(QStringLiteral(
        "INSERT INTO task (task_no, title, category, source, handler, status, create_time) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)"));
    fixture.addBindValue(QStringLiteral("VISUAL-1"));
    fixture.addBindValue(QStringLiteral("字体与分类标志检查"));
    fixture.addBindValue(QStringLiteral("查"));
    fixture.addBindValue(QStringLiteral("测试来源"));
    fixture.addBindValue(QStringLiteral("本人"));
    fixture.addBindValue(QStringLiteral("进行中"));
    fixture.addBindValue(QDate::currentDate().toString("yyyy-MM-dd") + " 10:00:00");
    if (!fixture.exec()) return 29;
    const int taskId = fixture.lastInsertId().toInt();
    const int currentYear = QDate::currentDate().year();
    fixture.prepare("INSERT INTO task (task_no,title,category,source,handler,status,finish_time,create_time) "
                    "VALUES (?, ?, '办', '办公室', '张三', '已完成', ?, ?)");
    for (int day : {1, 2}) {
        fixture.bindValue(0, QStringLiteral("COMPLETE-%1").arg(3 - day));
        fixture.bindValue(1, QStringLiteral("完成日期排序验证 %1").arg(day));
        fixture.bindValue(2, QStringLiteral("%1-02-%2 12:00:00").arg(currentYear).arg(day, 2, 10, QLatin1Char('0')));
        fixture.bindValue(3, QStringLiteral("%1-01-01 10:00:00").arg(currentYear));
        if (!fixture.exec()) return 35;
    }
    struct HandlerFixture {
        const char *number;
        const char *title;
        const char *handler;
        const char *status;
        const char *finished;
    };
    const HandlerFixture handlerRows[] = {
        {"COMPLETE-3", "同日较早完成（09:00）", "李四", "已完成", "02-02T09:00:00"},
        {"COMPLETE-4", "非本人已完成", "非本人", "已完成", "02-03 12:00:00"},
        {"COMPLETE-5", "本人已完成", "本人", "已完成", "02-04 12:00:00"},
        {"OTHERS-1", "他人办理（安一）", "安一", "他人办理", ""},
        {"OTHERS-2", "他人办理（王五）", "王五", "他人办理", ""}
    };
    fixture.prepare("INSERT INTO task (task_no,title,category,source,handler,status,finish_time,create_time) "
                    "VALUES (?, ?, '办', '办公室', ?, ?, ?, ?)");
    for (const HandlerFixture &row : handlerRows) {
        fixture.bindValue(0, QString::fromLatin1(row.number));
        fixture.bindValue(1, QString::fromUtf8(row.title));
        fixture.bindValue(2, QString::fromUtf8(row.handler));
        fixture.bindValue(3, QString::fromUtf8(row.status));
        fixture.bindValue(4, *row.finished ? QVariant(QStringLiteral("%1-%2").arg(currentYear).arg(row.finished)) : QVariant());
        fixture.bindValue(5, QStringLiteral("%1-01-01 10:00:00").arg(currentYear));
        if (!fixture.exec()) return 53;
    }
    if (!fixture.exec("DELETE FROM reference_material")) return 36;
    fixture.prepare("INSERT INTO reference_material (ref_no,name,source,tag,create_time) "
                    "VALUES (?, ?, '办公室', '政策资料', ?)");
    for (int year : {currentYear, currentYear - 1}) {
        fixture.bindValue(0, QStringLiteral("%1-001").arg(year));
        fixture.bindValue(1, QStringLiteral("%1 年度参考资料").arg(year));
        fixture.bindValue(2, QStringLiteral("%1-06-01 10:00:00").arg(year));
        if (!fixture.exec()) return 37;
    }

    QApplication::setEffectEnabled(Qt::UI_AnimateCombo,true);
    MainWindow window;
    if (!verifyCategoryCentering(captureDir,window.devicePixelRatioF())) return 201;
    window.resize(1440, 900);
    window.show();
    app.processEvents();
    window.raise();
    window.activateWindow();
    QThread::msleep(150);
    for (int width : {1440, 1100, 1300}) {
        window.resize(width, 800);
        // Synthetic 200% scaling can make the window larger than the monitor;
        // keep the inspected corner on-screen instead of capturing off-screen black.
        window.move(window.screen()->availableGeometry().topLeft() + QPoint(24, 24));
        app.processEvents();
        QThread::msleep(100);
        const QPoint corner = window.mapToGlobal(QPoint(0, 0));
        if (!window.screen()->grabWindow(0, corner.x(), corner.y(), 48, 48).save(
                QDir(captureDir).filePath(QStringLiteral("native-corner-%1.png").arg(width)))) return 61;
    }
    window.resize(1440, 900);
    app.processEvents();
    auto *title = window.findChild<QWidget *>(QStringLiteral("titlebar"));
    auto *logo = window.findChild<QLabel *>(QStringLiteral("titleIcon"));
    if (!title || !logo || !logo->grab().save(QDir(captureDir).filePath("canonical-logo.png"))
        || !window.windowIcon().pixmap(32,32).save(QDir(captureDir).filePath("window-logo-32.png"))
        || !title->grab(QRect(title->width()-160, 0, 160, title->height())).save(
                QDir(captureDir).filePath("window-controls.png"))) return 63;
    QFile fontDiagnostics(QDir(captureDir).filePath(QStringLiteral("font-diagnostics.txt")));
    if (!fontDiagnostics.open(QIODevice::WriteOnly | QIODevice::Text)) return 26;
    QTextStream fontReport(&fontDiagnostics);
    fontReport << "app=" << QFontInfo(app.font()).family() << '/' << QFontInfo(app.font()).styleName()
               << " Han=" << QRawFont::fromFont(app.font()).supportsCharacter(0x4EFB)
               << " Latin=" << QRawFont::fromFont(app.font()).supportsCharacter('N')
               << " Rare=" << QRawFont::fromFont(app.font()).supportsCharacter(0x20000);
    const QByteArray fontWeightTable = QRawFont::fromFont(app.font()).fontTable("OS/2");
    if (fontWeightTable.size() >= 6) {
        fontReport << " weight="
                   << (static_cast<unsigned char>(fontWeightTable.at(4)) * 256
                       + static_cast<unsigned char>(fontWeightTable.at(5)));
    }
    fontReport << '\n';
    QTextLayout mixed(QStringLiteral("任务 NIUMA 0123456789 ，。+-()"), app.font());
    mixed.beginLayout();
    mixed.createLine();
    mixed.endLayout();
    for (const QGlyphRun &run : mixed.glyphRuns()) {
        fontReport << "resolved-run=" << run.rawFont().familyName() << '\n';
        if (run.glyphIndexes().contains(0)) return 62;
    }
    if (!QFile::exists(QStringLiteral(":/fonts/segoeui.ttf"))
        || app.property("niumaBundledFontIds").toList().size()!=6
        || QFontInfo(app.font()).family() != QStringLiteral("Segoe UI")
        || (!QFontDatabase().families().contains(QStringLiteral("Microsoft YaHei UI"))
            && !QFontDatabase().families().contains(QStringLiteral("Microsoft YaHei")))
        || !QRawFont::fromFont(app.font()).supportsCharacter('N')) return 27;
    if (fontWeightTable.size() < 6
        || static_cast<unsigned char>(fontWeightTable.at(4)) * 256
                + static_cast<unsigned char>(fontWeightTable.at(5)) != 400) return 33;
    QSet<const QWidget *> auditedWidgets;
    auto *sidebar = window.findChild<QListWidget *>(QStringLiteral("sidebar"));
    auto *workspace = window.findChild<QStackedWidget *>(QStringLiteral("appUiWorkspace"));
    if (!sidebar || !workspace || workspace->count() != 8) return 3;

    for (int page = 0; page < workspace->count(); ++page) {
        const int sidebarRow = page == 7 ? 8 : page;
        sidebar->setCurrentRow(sidebarRow);
        app.processEvents();
        if (workspace->currentIndex() != page) return 4 + page;
        if (page == 0) {
            auto *homeTitle = window.findChild<QLabel *>(QStringLiteral("HomeTaskName"));
            if (!homeTitle) {
                for (auto *label : workspace->currentWidget()->findChildren<QLabel *>()) {
                    if (label->isVisible() && label->text().contains(QStringLiteral("字体与分类标志检查"))) {
                        homeTitle = label; break;
                    }
                }
            }
            if (!verifyLabelSelection(homeTitle)) return 180;
        }
        int badgeNumber=0;
        for (auto *badge : workspace->currentWidget()->findChildren<QLabel *>("appUiCategoryTag"))
            if (!verifyCategoryWidget(badge,badge->property("categoryText").toString(),
                    QDir(captureDir).filePath(QString("page-%1-category-%2.png").arg(page).arg(++badgeNumber)))) return 202;
        for (const QWidget *widget : window.findChildren<QWidget *>()) {
            if (widget->isVisible() && QFontInfo(widget->font()).family() != QStringLiteral("Segoe UI")) {
                fontReport << "page=" << page << " widget=" << widget->metaObject()->className()
                           << " object=" << widget->objectName()
                           << " resolved=" << QFontInfo(widget->font()).family() << '\n';
                return 28;
            }
            if (widget->isVisible() && !auditedWidgets.contains(widget)) {
                auditedWidgets.insert(widget);
                const QByteArray weightTable = QRawFont::fromFont(widget->font()).fontTable("OS/2");
                const int fontWeight = weightTable.size() < 6 ? 0
                    : static_cast<unsigned char>(weightTable.at(4)) * 256
                        + static_cast<unsigned char>(weightTable.at(5));
                if (fontWeight != 400) {
                    fontReport << "weight=" << fontWeight << " widget=" << widget->metaObject()->className()
                               << " object=" << widget->objectName() << '\n';
                    if (widget->objectName() != QStringLiteral("fontWeightSample")) return 34;
                }
            }
        }
        const QPixmap capture = window.grab();
        const QString path = QDir(captureDir).filePath(QStringLiteral("page-%1.png").arg(page));
        if (capture.isNull() || !capture.save(path, "PNG") || !QFileInfo::exists(path)) return 12 + page;
    }

    sidebar->setCurrentRow(1);
    app.processEvents();
    auto *taskStatus = window.findChild<QComboBox *>(QStringLiteral("taskStatusCombo"));
    auto *taskHandler = window.findChild<QComboBox *>(QStringLiteral("taskHandlerCombo"));
    auto *taskYear = window.findChild<QComboBox *>(QStringLiteral("taskYearCombo"));
    auto *taskSearchButton = window.findChild<QPushButton *>(QStringLiteral("taskSearchButton"));
    auto *taskReset = window.findChild<QPushButton *>(QStringLiteral("taskResetButton"));
    if (!taskStatus || !taskHandler || !taskYear || !taskSearchButton || !taskReset
        || taskHandler->findText(QStringLiteral("张三")) < 0) return 38;
    taskStatus->setCurrentText(QStringLiteral("已完成"));
    app.processEvents();
    auto *completedTable = window.findChild<QTableView *>(QStringLiteral("taskTable"));
    const QStringList completedNumbers = {QStringLiteral("COMPLETE-5"), QStringLiteral("COMPLETE-4"),
        QStringLiteral("COMPLETE-1"), QStringLiteral("COMPLETE-3"), QStringLiteral("COMPLETE-2")};
    if (!completedTable || completedTable->model()->rowCount() != completedNumbers.size()) return 54;
    for (int row = 0; row < completedNumbers.size(); ++row) {
        if (completedTable->model()->index(row, 0).data().toString() != completedNumbers.at(row)) return 55;
    }
    if (taskHandler->itemText(1) != QStringLiteral("本人") || taskHandler->itemText(2) != QStringLiteral("非本人")
        || taskHandler->findText(QStringLiteral("李四")) >= taskHandler->findText(QStringLiteral("张三"))) return 56;
    if (!window.grab().save(QDir(captureDir).filePath("task-completed-dates.png"))) return 39;
    taskHandler->showPopup();
    app.processEvents();
    if (!taskHandler->view()->window()->grab().save(QDir(captureDir).filePath("task-handler-options.png"))) return 40;
    taskHandler->hidePopup();
    taskHandler->setCurrentText(QStringLiteral("非本人"));
    app.processEvents();
    if (completedTable->model()->rowCount() != 4
        || !window.grab().save(QDir(captureDir).filePath("task-nonself-completed.png"))) return 57;
    taskStatus->setCurrentIndex(0);
    app.processEvents();
    if (completedTable->model()->rowCount() != 6) return 58;
    for (int row = 0; row < completedTable->model()->rowCount(); ++row) {
        if (completedTable->model()->index(row, 5).data().toString() == QStringLiteral("本人")) return 59;
    }
    if (!window.grab().save(QDir(captureDir).filePath("task-nonself-all-statuses.png"))) return 60;
    taskStatus->setCurrentText(QStringLiteral("已完成"));
    QMouseEvent openYear(QEvent::MouseButtonPress,
                         QPointF(taskYear->width() - 12, taskYear->height() / 2),
                         Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(taskYear, &openYear);
    app.processEvents();
    auto *taskYearPopup = taskYear->findChild<QWidget *>(QStringLiteral("appUiYearPopup"));
    if (!taskYearPopup || !taskYearPopup->isVisible()
        || taskYearPopup->findChildren<QPushButton *>(QStringLiteral("appUiYearCell")).size() != 12
        || !taskYearPopup->grab().save(QDir(captureDir).filePath("task-year-picker.png"))
        || !capturePopupFrame(taskYearPopup,captureDir,"year-picker")) return 47;
    taskYear->hidePopup();
    taskYear->setCurrentText(QString::number(currentYear - 1));
    app.processEvents();
    auto *taskTable = window.findChild<QTableView *>(QStringLiteral("taskTable"));
    if (!taskTable || taskTable->model()->rowCount() != 0) return 48;
    if (!window.grab().save(QDir(captureDir).filePath("task-previous-year.png"))) return 41;
    taskReset->click();

    sidebar->setCurrentRow(3);
    app.processEvents();
    auto *referenceYear = window.findChild<QComboBox *>(QStringLiteral("referenceYearCombo"));
    auto *referenceSearch = window.findChild<QPushButton *>(QStringLiteral("referenceSearchButton"));
    auto *referenceReset = window.findChild<QPushButton *>(QStringLiteral("referenceResetButton"));
    auto *referenceSource = window.findChild<QComboBox *>(QStringLiteral("referenceSourceCombo"));
    auto *referenceText = window.findChild<QLineEdit *>(QStringLiteral("referenceSearchEdit"));
    auto *referenceTable = window.findChild<QTableView *>(QStringLiteral("referenceTable"));
    if (!referenceYear || !referenceSearch || !referenceReset || !referenceSource || !referenceText || !referenceTable)
        return 49;
    referenceYear->showPopup();
    app.processEvents();
    auto *referenceYearPopup = referenceYear->findChild<QWidget *>(QStringLiteral("appUiYearPopup"));
    if (!referenceYearPopup || !referenceYearPopup->grab().save(QDir(captureDir).filePath("reference-year-picker.png")))
        return 50;
    referenceYear->hidePopup();
    referenceYear->setCurrentText(QString::number(currentYear - 1));
    app.processEvents();
    if (referenceTable->model()->rowCount() != 1
        || referenceTable->model()->index(0, 0).data().toString() != QStringLiteral("%1-001").arg(currentYear - 1)
        || !window.grab().save(QDir(captureDir).filePath("reference-previous-year.png"))) return 51;
    referenceSource->setCurrentText(QStringLiteral("办公室"));
    referenceText->setText(QStringLiteral("无匹配编号"));
    referenceSearch->click();
    app.processEvents();
    auto *referenceEmpty = window.findChild<QLabel *>(QStringLiteral("referenceEmptyState"));
    if (referenceTable->model()->rowCount() != 0 || !referenceEmpty || !referenceEmpty->isVisible()
        || !window.grab().save(QDir(captureDir).filePath("reference-filter-zero.png"))) return 52;
    referenceReset->click();

    QFile attachmentFixture(QDir(root).filePath("visual-attachment.txt"));
    if (!attachmentFixture.open(QIODevice::WriteOnly)) return 42;
    attachmentFixture.write("Qt visual fixture");
    attachmentFixture.close();
    {
        TaskCreateDialog taskCreate(&window);
        auto *upload = taskCreate.findChild<DragDropArea *>(QStringLiteral("taskAttachmentArea"));
        if (!upload || !upload->pasteFiles({attachmentFixture.fileName()})) return 43;
        taskCreate.show();
        app.processEvents();
        if (!capturePopupFrame(&taskCreate,captureDir,"task-create")) return 64;
        if (!taskCreate.grab().save(QDir(captureDir).filePath("task-red-removal.png"))) return 44;
        taskCreate.reject();
    }
    {
        ReferenceCreateDialog referenceCreate(&window);
        auto *upload = referenceCreate.findChild<DragDropArea *>(QStringLiteral("referenceAttachmentArea"));
        if (!upload || !upload->pasteFiles({attachmentFixture.fileName()})) return 45;
        referenceCreate.show();
        app.processEvents();
        if (!capturePopupFrame(&referenceCreate,captureDir,"reference-create")) return 65;
        if (!referenceCreate.grab().save(QDir(captureDir).filePath("reference-red-removal.png"))) return 46;
        referenceCreate.reject();
    }

    sidebar->setCurrentRow(8);
    app.processEvents();
    auto *settings = window.findChild<SettingsView *>();
    QScrollArea *settingsScroll = nullptr;
    if (settings) {
        for (QScrollArea *candidate : window.findChildren<QScrollArea *>()) {
            if (candidate->widget() == settings) {
                settingsScroll = candidate;
                break;
            }
        }
    }
    auto *exportCard = settings ? settings->findChild<QFrame *>(QStringLiteral("exportCard")) : nullptr;
    auto *fontCard = settings ? settings->findChild<QFrame *>(QStringLiteral("fontSettingsCard")) : nullptr;
    auto *dataCard = settings ? settings->findChild<QFrame *>(QStringLiteral("dataMgmtCard")) : nullptr;
    if (!settingsScroll || !exportCard || !fontCard || !dataCard
        || !(exportCard->y() < fontCard->y() && fontCard->y() < dataCard->y())) return 20;
    settingsScroll->verticalScrollBar()->setValue(settingsScroll->verticalScrollBar()->maximum());
    app.processEvents();
    const QPixmap fontCardCapture = window.grab();
    if (fontCardCapture.isNull() || !fontCardCapture.save(
            QDir(captureDir).filePath(QStringLiteral("settings-font-card.png")), "PNG")) return 21;

    sidebar->setCurrentRow(1);
    app.processEvents();
    QComboBox *combo = nullptr;
    for (QComboBox *candidate : window.findChildren<QComboBox *>()) {
        if (candidate->isVisible() && candidate->isEnabled()) {
            combo = candidate;
            break;
        }
    }
    QDateEdit *dateEdit = nullptr;
    for (QDateEdit *candidate : window.findChildren<QDateEdit *>()) {
        if (candidate->isVisible() && candidate->isEnabled()) {
            dateEdit = candidate;
            break;
        }
    }
    if (!combo || !dateEdit) return 20;
    for (int page=0;page<sidebar->count();++page) {
        if (page==7) continue;
        sidebar->setCurrentRow(page);
        app.processEvents();
        if (!verifyAllCombos(&window,captureDir,QString("page-%1").arg(page))) return 183;
    }
    {
        TaskCreateDialog create(&window);
        create.show(); app.processEvents();
        if (!verifyAllCombos(&create,captureDir,"task-create")) return 184;
        create.reject();
    }
    {
        TaskEditDialog edit(taskId,&window);
        edit.show(); app.processEvents();
        if (!verifyAllCombos(&edit,captureDir,"task-edit")) return 185;
        edit.reject();
    }
    {
        ReferenceCreateDialog create(&window);
        create.show(); app.processEvents();
        if (!verifyAllCombos(&create,captureDir,"reference-create")) return 186;
        create.reject();
    }
    {
        QSqlQuery referenceId(DatabaseManager::instance().database());
        if (!referenceId.exec("SELECT id FROM reference_material ORDER BY id LIMIT 1") || !referenceId.next()) return 187;
        ReferenceEditDialog edit(referenceId.value(0).toInt(),&window);
        edit.show(); app.processEvents();
        if (!verifyAllCombos(&edit,captureDir,"reference-edit")) return 187;
        edit.reject();
    }
    {
        PeriodicTaskCreateDialog create(&window);
        create.show(); app.processEvents();
        if (!verifyAllCombos(&create,captureDir,"periodic-create")) return 188;
        create.reject();
    }
    sidebar->setCurrentRow(1);
    app.processEvents();
    if (!combo->findChild<QWidget *>(QStringLiteral("appUiComboArrow"))) return 21;
    const QPixmap comboCapture = combo->grab();
    if (comboCapture.isNull()
        || !comboCapture.save(QDir(captureDir).filePath(QStringLiteral("combo-control.png")), "PNG")) return 25;
    const QPoint localDatePoint = dateEdit->rect().center();
    QMouseEvent datePress(QEvent::MouseButtonPress, QPointF(localDatePoint), Qt::LeftButton,
                          Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(dateEdit, &datePress);
    app.processEvents();
    auto *datePopup = dateEdit->findChild<QWidget *>(QStringLiteral("appUiDatePopup"));
    if (!datePopup || !datePopup->isVisible()) return 22;
    if (!capturePopupFrame(datePopup,captureDir,"date-picker")) return 66;
    const QPixmap dateCapture = datePopup->grab();
    if (dateCapture.isNull()
        || !dateCapture.save(QDir(captureDir).filePath(QStringLiteral("date-popup.png")), "PNG")) return 23;

    fixture.prepare("UPDATE task SET title=? WHERE id=?");
    fixture.addBindValue(QString(45,QChar(0x725B)));
    fixture.addBindValue(taskId);
    if (!fixture.exec()) return 191;
    TaskDetailView detail(taskId);
    detail.show();
    const QRect initialTitle=detail.findChild<QLabel *>(QStringLiteral("appUiDetailTitle"))->geometry();
    const QRect initialBody=detail.findChild<QWidget *>(QStringLiteral("detailBody"))->geometry();
    app.processEvents();
    QThread::msleep(100);
    app.processEvents();
    if(initialTitle!=detail.findChild<QLabel *>(QStringLiteral("appUiDetailTitle"))->geometry()
        || initialBody!=detail.findChild<QWidget *>(QStringLiteral("detailBody"))->geometry()) return 182;
    if (!capturePopupFrame(&detail,captureDir,"task-detail")) return 67;
    const auto *detailTitle = detail.findChild<QLabel *>(QStringLiteral("appUiDetailTitle"));
    if (!detailTitle || !detailTitle->isVisible()
        || QFontInfo(detailTitle->font()).family() != QStringLiteral("Segoe UI")
        || QFontInfo(detailTitle->font()).pixelSize() != 22) return 30;
    if (!verifyLabelSelection(const_cast<QLabel *>(detailTitle))) return 181;
    auto *badge=dynamic_cast<CategoryBadgeLabel *>(detail.findChild<QLabel *>("appUiCategoryTag"));
    if (!badge) return 202;
    for (const QString &category : {QStringLiteral("催"),QStringLiteral("查"),QStringLiteral("报"),QStringLiteral("办")}) {
        badge->setCategory(category);
        if (!verifyCategoryWidget(badge,category,QDir(captureDir).filePath(QString("detail-category-%1.png").arg(category)))) return 202;
    }
    badge->setCategory(QStringLiteral("查"));
    // A normal click must not leave a blinking insertion caret. Compare actual
    // Qt pixels across a blink interval, while preserving the copy checks above.
    auto *copyTitle=const_cast<QLabel *>(detailTitle);
    const QPoint click(12,copyTitle->height()/2);
    selectionMouse(copyTitle,QEvent::MouseButtonPress,click);
    selectionMouse(copyTitle,QEvent::MouseButtonRelease,click);
    app.processEvents();
    const QImage beforeBlink=copyTitle->grab().toImage();
    QElapsedTimer blink;
    blink.start();
    while(blink.elapsed()<qMax(100,QApplication::cursorFlashTime()/2+80)) {
        app.processEvents();QThread::msleep(10);
    }
    if (beforeBlink!=copyTitle->grab().toImage()
        || !detail.grab().save(QDir(captureDir).filePath("detail-click-no-caret.png"))) return 203;
    for (auto *edit : detail.findChildren<QTextEdit *>()) {
        if (!edit->isReadOnly() || edit->cursorWidth()!=0
            || (edit->textInteractionFlags() & Qt::TextSelectableByKeyboard)) return 203;
        const QString original=edit->toPlainText();
        edit->setFocus();edit->selectAll();selectionCopy(edit);
        if(QApplication::clipboard()->text()!=original || edit->toPlainText()!=original) return 203;
        QTextCursor cursor=edit->textCursor();cursor.clearSelection();edit->setTextCursor(cursor);
    }
    for (const QWidget *widget : detail.findChildren<QWidget *>()) {
        if (widget->isVisible() && QFontInfo(widget->font()).family() != QStringLiteral("Segoe UI")) {
            fontReport << "detail widget=" << widget->metaObject()->className()
                       << " object=" << widget->objectName()
                       << " resolved=" << QFontInfo(widget->font()).family() << '\n';
            return 31;
        }
        if (widget->isVisible()) {
            const QByteArray weightTable = QRawFont::fromFont(widget->font()).fontTable("OS/2");
            const int fontWeight = weightTable.size() < 6 ? 0
                : static_cast<unsigned char>(weightTable.at(4)) * 256
                    + static_cast<unsigned char>(weightTable.at(5));
            if (fontWeight != 400) {
                fontReport << "detail weight=" << fontWeight
                           << " widget=" << widget->metaObject()->className()
                           << " object=" << widget->objectName() << '\n';
                return 35;
            }
        }
    }
    if (!detail.grab().save(QDir(captureDir).filePath(QStringLiteral("task-detail-font.png")), "PNG")) return 32;
    if (!verifyThreeLineTitle(const_cast<QLabel *>(detailTitle))) return 191;
    detail.hide();

    if (!fixture.exec("SELECT id FROM reference_material ORDER BY id LIMIT 1") || !fixture.next()) return 192;
    const int referenceId=fixture.value(0).toInt();
    fixture.prepare("UPDATE reference_material SET name=? WHERE id=?");
    fixture.addBindValue(QString(45,QChar(0x725B)));
    fixture.addBindValue(referenceId);
    if (!fixture.exec()) return 192;
    {
        ReferenceDetailDialog reference(referenceId);
        reference.show(); app.processEvents();
        if (!verifyThreeLineTitle(reference.findChild<QLabel *>("appUiReferenceDetailTitle"))
            || !reference.grab().save(QDir(captureDir).filePath("reference-title-45.png"))) return 192;
    }
    {
        PeriodicTaskCreateDialog periodic;
        auto *name=periodic.findChild<QPlainTextEdit *>("periodicTemplateNameEdit");
        if (!name) return 193;
        name->setPlainText(QString(45,QChar(0x725B)));
        periodic.show(); app.processEvents();
        if (!verifyThreeLineNameEdit(name)
            || !periodic.grab().save(QDir(captureDir).filePath("periodic-title-45.png"))) return 193;
    }

    // Exercise classic Windows and Fusion menu painting as well as the host's
    // default theme, with both a one-item popup and a scrolling list.
    for (const QString &theme : {QString("Windows"),QString("Fusion")}) {
        QComboBox fixture;
        fixture.setObjectName("dropdownThemeFixture");
        fixture.resize(240,36);
        fixture.move(window.screen()->availableGeometry().topLeft()+QPoint(80,100));
        fixture.setMaxVisibleItems(5);
        auto *style=QStyleFactory::create(theme);
        if (!style) return 189;
        style->setParent(&fixture);
        fixture.setStyle(style);
        fixture.view()->setStyle(style);
        fixture.view()->window()->setStyle(style);
        fixture.addItem(QStringLiteral("单条选项 NIUMA 0123"));
        AppTheme::prepareWindow(fixture);
        fixture.show(); app.processEvents();
        if (!verifyComboCorners(&fixture,captureDir,theme+"-single")) return 189;
        for (int row=1;row<30;++row) fixture.addItem(QStringLiteral("滚动选项 %1").arg(row));
        if (!verifyComboCorners(&fixture,captureDir,theme+"-scrolling")) return 190;
    }

    if (!verifyOverflowTooltips(captureDir)) return 200;
    DatabaseManager::instance().close();
    return 0;
}
