#include <QApplication>
#include <QAction>
#include <QAbstractItemView>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDateEdit>
#include <QDir>
#include <QDebug>
#include <QGraphicsOpacityEffect>
#include <QImage>
#include <QListWidget>
#include <QLabel>
#include <QKeyEvent>
#include <QFontDatabase>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QShortcut>
#include <QStackedWidget>
#include <QStatusBar>

#include "DatabaseManager.h"
#include "MainWindow.h"
#include "HomeView.h"
#include "PathUtil.h"
#include "AppTheme.h"
#include "TaskListView.h"
#include "widgets/ToastWidget.h"
#include "smooth_scroll_contract.h"
#include "native_mouse_contract.h"

class ResizeStyleCounter : public QObject {
public:
    int changes = 0;
    bool eventFilter(QObject *, QEvent *event) override {
        if (event->type() == QEvent::StyleChange) ++changes;
        return false;
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    const bool legacyInput=app.arguments().contains(QStringLiteral("--legacy-input"));
    if (legacyInput) {
        app.setProperty("niumaLegacyWindowChrome",true);
        qInstallMessageHandler([](QtMsgType,const QMessageLogContext &,const QString &message) {
            fprintf(stderr,"Qt: %s\n",message.toUtf8().constData());
        });
    }
    if (!DatabaseManager::instance().initialize(PathUtil::databasePath())) {
        return 1;
    }

    MainWindow window;
    const auto *titleBar = window.findChild<QWidget *>(QStringLiteral("titlebar"));
    auto *sidebar = window.findChild<QListWidget *>(QStringLiteral("sidebar"));
    const auto *workspace = window.findChild<QStackedWidget *>(QStringLiteral("appUiWorkspace"));
    const auto *statusBar = window.findChild<QStatusBar *>();

    window.show();
    app.processEvents();
    if (legacyInput || app.arguments().contains(QStringLiteral("--native-input"))) {
        QElapsedTimer startupWait;
        startupWait.start();
        while (startupWait.elapsed()<500) { app.processEvents(); QThread::msleep(5); }
        const int input=verifyPhysicalNavigation(window,sidebar);
        if (input) return input;
#ifdef Q_OS_WIN
        if (legacyInput) {
            const HWND hwnd=reinterpret_cast<HWND>(window.winId());
            if (!window.testAttribute(Qt::WA_TranslucentBackground)
                || !window.property("appUiLegacyCorners").toBool()
                || (GetWindowLongPtrW(hwnd,GWL_STYLE)&WS_CAPTION)) return 301;
            const HMENU systemMenu=GetSystemMenu(hwnd,FALSE);
            const UINT minimizeState=systemMenu ? GetMenuState(systemMenu,SC_MINIMIZE,MF_BYCOMMAND) : UINT(-1);
            if (minimizeState==UINT(-1) || (minimizeState&(MF_GRAYED|MF_DISABLED))) return 307;
            window.showMaximized();
            app.processEvents();
            if (!window.isMaximized() || (GetWindowLongPtrW(hwnd,GWL_STYLE)&WS_CAPTION)) return 302;
            window.showMinimized();
            app.processEvents();
            if (!IsIconic(hwnd)) return 303;
            window.showNormal();
            app.processEvents();
            if (window.isMaximized() || IsIconic(hwnd)
                || (GetWindowLongPtrW(hwnd,GWL_STYLE)&WS_CAPTION)
                || verifyPhysicalNavigation(window,sidebar)) return 304;
        }
#endif
        DatabaseManager::instance().close();
        return 0;
    }
    const auto *newTaskShortcut = window.findChild<QShortcut *>(QStringLiteral("shortcutNewTask"));
    // Resizing must not repolish every descendant's style sheet.
    ResizeStyleCounter resizeStyles;
    app.installEventFilter(&resizeStyles);
    const QSize savedSize = window.size();
    for (int index = 0; index < 12; ++index) {
        window.resize(1200+index*8, 750+index*4);
        app.processEvents();
    }
    window.resize(savedSize);
    app.processEvents();
    app.removeEventFilter(&resizeStyles);
    if (resizeStyles.changes != 0) return 284;
    const auto *findTaskShortcut = window.findChild<QShortcut *>(QStringLiteral("shortcutFindTask"));
    const auto *refreshShortcut = window.findChild<QShortcut *>(QStringLiteral("shortcutRefreshAll"));
    auto *taskSearch = window.findChild<QLineEdit *>(QStringLiteral("taskSearchEdit"));
    auto *shellSearch = window.findChild<QLineEdit *>(QStringLiteral("shellSearch"));
    auto *backButton = window.findChild<QPushButton *>(QStringLiteral("shellBack"));
    auto *forwardButton = window.findChild<QPushButton *>(QStringLiteral("shellForward"));
    auto *refreshButton = window.findChild<QPushButton *>(QStringLiteral("shellRefresh"));
    auto *createButton = window.findChild<QPushButton *>(QStringLiteral("shellCreate"));
    auto *home = window.findChild<HomeView *>();
    auto *taskList = window.findChild<TaskListView *>();
    auto *filterCombo = window.findChild<QComboBox *>();
    auto *categoryCombo = window.findChild<QComboBox *>(QStringLiteral("taskCategoryCombo"));
    auto *checkBox = window.findChild<QCheckBox *>(QStringLiteral("settingsIncludeAchievements"));
    auto *focusArea = window.findChild<QWidget *>(QStringLiteral("FocusArea"));
    const auto titleLabels = titleBar ? titleBar->findChildren<QLabel *>(QString(), Qt::FindDirectChildrenOnly)
                                      : QList<QLabel *>();

    if (!titleBar || titleBar->minimumHeight() != 70 || titleBar->maximumHeight() != 70) return 2;
    if (window.testAttribute(Qt::WA_TranslucentBackground)!=window.property("appUiLegacyCorners").toBool()
        || !window.mask().isEmpty()) return 24;
    if (titleLabels.size() < 2 || titleLabels.at(1)->text() != QStringLiteral("NIUMA")
        || titleLabels.at(1)->font().bold() || !titleLabels.at(1)->styleSheet().contains(QStringLiteral("font-size:17pt"))
        || titleLabels.at(1)->width() != 126) return 16;
    if (!sidebar || sidebar->minimumWidth() != 184 || sidebar->maximumWidth() != 184) return 3;
    if (!workspace || workspace->count() != 8) return 4;
    auto *clearSearch = shellSearch ? shellSearch->findChild<QAction *>(QStringLiteral("shellSearchClearAction")) : nullptr;
    if (!clearSearch || clearSearch->isVisible()) return 280;
    shellSearch->setText(QStringLiteral("清除按钮检查"));
    app.processEvents();
    const QImage clearIcon = clearSearch->icon().pixmap(16, 16).toImage();
    int redPixels = 0;
    for (int y = 0; y < clearIcon.height(); ++y) for (int x = 0; x < clearIcon.width(); ++x) {
        const QColor color = clearIcon.pixelColor(x, y);
        if (color.alpha() > 128 && color.red() > 150 && color.green() < 100 && color.blue() < 100) ++redPixels;
    }
    if (!clearSearch->isVisible() || redPixels < 10
        || !window.grab().save(QDir(qEnvironmentVariable("NIUMA_DATA_DIR")).filePath("shell-red-clear.png"))) return 281;
    clearSearch->trigger();
    if (!shellSearch->text().isEmpty() || clearSearch->isVisible()) return 282;
    if (!statusBar || statusBar->minimumHeight() != 36 || statusBar->maximumHeight() != 36) return 5;
    if (!app.property("appUiFontFallback").isValid()
        || AppTheme::preferredFontFamily(QStringList{QStringLiteral("Segoe UI")})
            != QStringLiteral("Segoe UI")
        || AppTheme::preferredFontFamily(QStringList{QStringLiteral("Microsoft YaHei")})
            != QStringLiteral("Microsoft YaHei")) return 6;
    if (QDir(QStringLiteral(":/fonts")).exists()
        || app.font().family() != QStringLiteral("Segoe UI")) return 15;
    if (!newTaskShortcut || newTaskShortcut->key() != QKeySequence::New
        || !findTaskShortcut || findTaskShortcut->key() != QKeySequence::Find
        || !refreshShortcut || refreshShortcut->key() != QKeySequence(Qt::Key_F5)
        || !taskSearch || !shellSearch || !backButton || !forwardButton || !refreshButton
        || !createButton || !home || !taskList) return 7;
    const auto *minimize = titleBar->findChild<QPushButton *>(QStringLiteral("btnMinimize"));
    const auto *maximize = titleBar->findChild<QPushButton *>(QStringLiteral("btnMaximize"));
    const auto *close = titleBar->findChild<QPushButton *>(QStringLiteral("btnClose"));
    if (!minimize || !maximize || !close) return 251;
    if (minimize->size() != QSize(46, 32) || maximize->size() != QSize(46, 32)
        || close->size() != QSize(46, 32) || !minimize->text().isEmpty()
        || !maximize->text().isEmpty() || !close->text().isEmpty()
        || maximize->x()-minimize->x() != 46 || close->x()-maximize->x() != 46) return 252;
    auto *titleIcon = titleBar->findChild<QLabel *>("titleIcon");
    if (!titleIcon || titleIcon->size() != QSize(30, 30)
        || window.findChild<QWidget *>("windowOutline")) return 283;
    const QPoint closeOrigin = close->mapTo(&window, QPoint());
    if (closeOrigin.y()*2+close->height() !=
        (window.width()-closeOrigin.x())*2-close->width()
        || minimize->y() != close->y() || maximize->y() != close->y()) return 285;
    if (closeOrigin.y()+close->height()/2 != qRound((titleBar->height()/2.0-5)*0.6+5)) return 289;
    for (int size : {14,16,20,24,28,32}) {
        const QImage glyph = QIcon(":/icons/window-close.svg").pixmap(size,size).toImage();
        if (glyph.isNull()) return 286;
        for (int y=0; y<glyph.height(); ++y) for (int x=0; x<glyph.width(); ++x) {
            const int alpha = glyph.pixelColor(x,y).alpha();
            if (qAbs(alpha-glyph.pixelColor(glyph.width()-1-x,y).alpha()) > 8
                || qAbs(alpha-glyph.pixelColor(x,glyph.height()-1-y).alpha()) > 8) return 287;
        }
    }
    for (const QImage &mark : {titleIcon->grab().toImage(), window.windowIcon().pixmap(30,30).toImage()}) {
        QRect white;
        for (int y=0; y<mark.height(); ++y) for (int x=0; x<mark.width(); ++x) {
            const QColor color = mark.pixelColor(x,y);
            if (color.alpha()>200 && color.red()>245 && color.green()>245 && color.blue()>245)
                white |= QRect(x,y,1,1);
        }
        if (white.isEmpty() || qAbs(white.left()+white.right()-(mark.width()-1))>1
            || qAbs(white.top()+white.bottom()-(mark.height()-1))>1) return 288;
    }
    if (!titleBar->styleSheet().contains(QStringLiteral("QPushButton#btnMinimize:pressed"))
        || !titleBar->styleSheet().contains(QStringLiteral("QPushButton#btnMaximize:pressed"))
        || !titleBar->styleSheet().contains(QStringLiteral("QPushButton#btnClose:pressed"))) return 253;
    const QPoint originalWindowPos = window.pos();
    const QPoint searchPos = shellSearch->geometry().center();
    const QPoint searchGlobal = titleBar->mapToGlobal(searchPos);
    QMouseEvent inputPress(QEvent::MouseButtonPress, QPointF(searchPos), QPointF(searchGlobal),
                           Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(const_cast<QWidget *>(titleBar), &inputPress);
    QMouseEvent inputMove(QEvent::MouseMove, QPointF(searchPos + QPoint(24, 0)),
                          QPointF(searchGlobal + QPoint(24, 0)), Qt::NoButton,
                          Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(const_cast<QWidget *>(titleBar), &inputMove);
    QMouseEvent inputRelease(QEvent::MouseButtonRelease, QPointF(searchPos + QPoint(24, 0)),
                             QPointF(searchGlobal + QPoint(24, 0)), Qt::LeftButton,
                             Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(const_cast<QWidget *>(titleBar), &inputRelease);
    if (window.pos() != originalWindowPos) return 254;
    QPoint blankPos;
    bool foundBlankTitleBarArea = false;
    for (int x = titleBar->width() / 2; x < titleBar->width(); ++x) {
        if (!titleBar->childAt(QPoint(x, titleBar->height() / 2))) {
            blankPos = QPoint(x, titleBar->height() / 2);
            foundBlankTitleBarArea = true;
            break;
        }
    }
    if (!foundBlankTitleBarArea) return 256;
    const QPoint blankGlobal = titleBar->mapToGlobal(blankPos);
    QMouseEvent blankPress(QEvent::MouseButtonPress, QPointF(blankPos), QPointF(blankGlobal),
                           Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(const_cast<QWidget *>(titleBar), &blankPress);
    QMouseEvent blankMove(QEvent::MouseMove, QPointF(blankPos + QPoint(24, 0)),
                          QPointF(blankGlobal + QPoint(24, 0)), Qt::NoButton,
                          Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(const_cast<QWidget *>(titleBar), &blankMove);
    QMouseEvent blankRelease(QEvent::MouseButtonRelease, QPointF(blankPos + QPoint(24, 0)),
                             QPointF(blankGlobal + QPoint(24, 0)), Qt::LeftButton,
                             Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(const_cast<QWidget *>(titleBar), &blankRelease);
    if (!blankPress.isAccepted()) return 255;
    if (!focusArea) return 17;
    if (focusArea->height() != 230) return 18;
    if (!filterCombo) return 19;
    if (!filterCombo->findChild<QLabel *>(QStringLiteral("appUiComboArrow"))) return 20;
    if (!categoryCombo || categoryCombo->view()->window() == categoryCombo) return 23;
    if (!categoryCombo->grab().save(QDir(qEnvironmentVariable("NIUMA_DATA_DIR"))
            .filePath(QStringLiteral("combo-control.png")))) return 272;
    if (!checkBox) return 21;
    if (!checkBox->styleSheet().contains(QStringLiteral(":/icons/app_check.svg"))) return 22;
    if (home->findChild<QWidget *>(QStringLiteral("btnPrimary"))->isVisible()
        || backButton->isEnabled() || forwardButton->isEnabled()) return 10;
    QKeyEvent findTask(QEvent::KeyPress, Qt::Key_F, Qt::ControlModifier);
    QApplication::sendEvent(&window, &findTask);
    app.processEvents();
    if (sidebar->currentRow() != 1 || QApplication::focusWidget() != taskSearch) return 8;
    sidebar->setCurrentRow(2);
    app.processEvents();
    if (!backButton->isEnabled() || forwardButton->isEnabled()) return 11;
    backButton->click();
    app.processEvents();
    if (sidebar->currentRow() != 1 || !forwardButton->isEnabled()) return 12;
    forwardButton->click();
    app.processEvents();
    if (sidebar->currentRow() != 2) return 13;
    shellSearch->setText(QStringLiteral("shell-contract"));
    QMetaObject::invokeMethod(shellSearch, "returnPressed");
    app.processEvents();
    if (sidebar->currentRow() != 1 || taskList->currentFilter().keyword != QStringLiteral("shell-contract")) return 14;
    refreshButton->click();
    app.processEvents();
    const auto toasts = window.findChildren<ToastWidget *>();
    if (toasts.isEmpty()) return 27;
    auto *opacity = qobject_cast<QGraphicsOpacityEffect *>(toasts.constLast()->graphicsEffect());
    if (!opacity) return 268;
    opacity->setOpacity(1.0);
    const QImage toastImage = toasts.constLast()->grab().toImage();
    if (!toastImage.save(QDir(qEnvironmentVariable("NIUMA_DATA_DIR"))
            .filePath(QStringLiteral("refresh-toast.png")))) return 266;
    const QColor toastPixel = toastImage.pixelColor(15, toastImage.height() / 2);
    if (toastPixel != QColor(QStringLiteral("#D1FAE5"))) return 267;
    bool foundRefreshMessage = false;
    for (const QLabel *label : toasts.constLast()->findChildren<QLabel *>()) {
        if (label->text() == QStringLiteral("刷新完成")) {
            foundRefreshMessage = !label->wordWrap()
                && label->styleSheet().contains(QStringLiteral("#166534"));
        }
    }
    if (!foundRefreshMessage) return 28;
    auto *maximizeButton = titleBar->findChild<QPushButton *>(QStringLiteral("btnMaximize"));
    if (!maximizeButton || maximizeButton->toolTip() != QStringLiteral("最大化")) return 269;
    maximizeButton->click();
    app.processEvents();
    if (!window.isMaximized() || !maximizeButton->property("windowControlRestore").toBool()
        || maximizeButton->toolTip() != QStringLiteral("还原")) return 270;
    maximizeButton->click();
    app.processEvents();
    if (window.isMaximized() || maximizeButton->property("windowControlRestore").toBool()
        || maximizeButton->toolTip() != QStringLiteral("最大化")) return 271;
    sidebar->setCurrentRow(3);
    app.processEvents();
    QDateEdit *visibleDate = nullptr;
    for (QDateEdit *edit : window.findChildren<QDateEdit *>()) {
        if (edit->isVisible()) { visibleDate = edit; break; }
    }
    if (!visibleDate || visibleDate->buttonSymbols() != QAbstractSpinBox::NoButtons
        || !visibleDate->grab().save(QDir(qEnvironmentVariable("NIUMA_DATA_DIR"))
            .filePath(QStringLiteral("date-control.png")))) return 273;
    const int scrolling=verifySmoothScrolling(window,sidebar);
    if (scrolling) return scrolling;
    sidebar->setCurrentRow(1);
    app.processEvents();
    categoryCombo->showPopup();
    QWidget *comboPopup = categoryCombo->view()->window();
    // Qt's native combo fade can defer the actual Show event.
    QElapsedTimer popupWait;
    popupWait.start();
    while (!comboPopup->isVisible() && popupWait.elapsed()<1500) {
        app.processEvents(); QThread::msleep(5);
    }
    if (!comboPopup->isVisible() || !comboPopup->mask().isEmpty()
        || !comboPopup->property("nativePopupHandle").toULongLong()) return 260;
    const QString comboCapture = QDir(qEnvironmentVariable("NIUMA_DATA_DIR"))
        .filePath(QStringLiteral("combo-popup.png"));
    if (!comboPopup->grab().save(comboCapture)) return 265;
    categoryCombo->hidePopup();
    DatabaseManager::instance().close();
    const QString style = app.styleSheet();
    return style.contains(QStringLiteral("QComboBox::drop-down"))
        && style.contains(QStringLiteral("QDateEdit::drop-down"))
        && style.contains(QStringLiteral("QCheckBox::indicator:checked"))
        && style.contains(QStringLiteral(":/icons/app_check.svg"))
        && style.contains(QStringLiteral("QHeaderView::section"))
        && style.contains(QStringLiteral("QMessageBox QPushButton")) ? 0 : 9;
}
