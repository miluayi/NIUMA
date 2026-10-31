/**
 * @file MainWindow.cpp
 * @brief 主窗口实现（T1.3/T1.4 完整实现）
 *
 * 布局结构：
 *   MainWindow (FramelessWindowHint)
 *   ├── TitleBar (52px)
 *   ├── centralWidget
 *   │   ├── QHBoxLayout (margins=0, spacing=0)
 *   │   │   ├── Sidebar (QListWidget, fixedWidth=220)
 *   │   │   └── QStackedWidget (8 pages)
 *   └── QStatusBar (36px)
 *
 * 依赖：ui/MainWindow.h、ui/widgets/TitleBar、ui/widgets/SidebarDelegate
 * 文档：PRD 14.1、qt_implementation_guide.md 3.1、coding.md 7.1
 */
#include "MainWindow.h"
#include "AppTheme.h"

#include "widgets/TitleBar.h"
#include "widgets/SidebarDelegate.h"
#include "widgets/ToastWidget.h"
#include "widgets/DialogScrim.h"
#include "widgets/StyledMessageBox.h"
#include "widgets/RoundedCornerFilter.h"
#include "widgets/WindowOutline.h"
#include "TaskListView.h"
#include "TaskCreateDialog.h"
#include "TaskDetailView.h"
#include "TaskEditDialog.h"
#include "ReferenceView.h"
#include "ReferenceCreateDialog.h"
#include "ReferenceDetailDialog.h"
#include "ReferenceEditDialog.h"
#include "AchievementView.h"
#include "application/achievement/AchievementUseCase.h"
#include "SettingsView.h"
#include "HomeView.h"
#include "StatsView.h"
#include "PeriodicTaskView.h"
#include "PeriodicTaskCreateDialog.h"
#include "RecycleBinView.h"
#include "ScanImportDialog.h"
#include "application/home/HomeCommandUseCase.h"
#include "application/settings/SettingsUseCase.h"
#include "application/maintenance/MainWindowMaintenanceUseCase.h"
#include "service/BackupService.h"
#include "utils/Logger.h"
#include "src/SingleInstance.h"

#include <QApplication>
#include <QDate>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QComboBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLocale>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainterPath>
#include <QRegion>
#include <QSpinBox>
#include <QTextEdit>
#include <QTimer>
#include <QListWidget>
#include <QListWidgetItem>
#include <QProgressDialog>
#include <QScreen>
#include <QScrollArea>
#include <QShortcut>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dwmapi.h>
#ifdef _MSC_VER
#pragma comment(lib, "dwmapi.lib")
#endif
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , titleBar_(nullptr)
    , sidebar_(nullptr)
    , stackedWidget_(nullptr)
    , dbStatusLabel_(nullptr)
    , taskCountLabel_(nullptr)
    , workDirLabel_(nullptr)
    , taskListView_(nullptr)
    , referenceView_(nullptr)
    , achievementView_(nullptr)
    , settingsView_(nullptr)
    , homeView_(nullptr)
    , statsView_(nullptr)
    , periodicTaskView_(nullptr)
    , recycleBinView_(nullptr)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowSystemMenuHint);
#ifdef Q_OS_WIN
    WindowsChrome::prepare(this);
    const HWND nativeWindow = reinterpret_cast<HWND>(winId());
    SetWindowLongPtrW(nativeWindow, GWL_STYLE, GetWindowLongPtrW(nativeWindow, GWL_STYLE)
                     | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
#endif
    // Let DWM composite the corners; a bitmap region disables native rounding.
    setWindowTitle("NIUMA");
    setWindowIcon(QIcon(":/icons/app-icon.svg"));
    resize(1440, 900);
    setMinimumSize(940, 580);

    setupUI();
    setupStyle();
    centerOnScreen();
    setupWindowShadow();
    setupTaskManagementPage();
    setupReferencePage();
    setupAchievementPage();
    setupSettingsPage();
    setupHomePage();
    if (auto *button = homeView_->findChild<QWidget *>(QStringLiteral("btnPrimary"))) {
        button->hide();
    }
    setupStatsPage();
    setupPeriodicTaskPage();
    setupRecycleBinPage();
    refreshStatusBar();
    updateSidebarSpacer();
    const UiFontPreferences fontPreferences = SettingsUseCase().uiFontPreferences();
    AppTheme::applyTypography(*qApp, *this, fontPreferences.pixelSize,
                                  static_cast<int>(fontPreferences.weight));

    // QTimer for cross-day detection (checks every minute)
    QTimer *dayCheckTimer = new QTimer(this);
    connect(dayCheckTimer, &QTimer::timeout, this, &MainWindow::onCheckNewDay);
    dayCheckTimer->start(60000);  // 60 seconds

    // 全局事件过滤器：点击文本输入控件以外区域时自动失焦
    qApp->installEventFilter(this);
#ifndef Q_OS_WIN
    new WindowOutline(this);
#endif
}

MainWindow::~MainWindow() = default;

#ifdef Q_OS_WIN
bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    const auto *nativeMessage = static_cast<MSG *>(message);
    if (WindowsChrome::suppressLegacyPainting(this,nativeMessage->message,result)) return true;
    if (nativeMessage->message==0x02E0 || nativeMessage->message==WM_THEMECHANGED
        || nativeMessage->message==WM_DWMCOMPOSITIONCHANGED || nativeMessage->message==WM_SETTINGCHANGE)
        WindowsChrome::scheduleFrameSync(this);
    if (nativeMessage->message == WM_NCCALCSIZE && !property("appUiQtCustomFrame").toBool()) {
        *result = 0; // Keep our custom title bar while retaining native frame styles.
        return true;
    }
    if (nativeMessage->message == WM_NCHITTEST && !isMaximized() && !isFullScreen()) {
        RECT bounds;
        GetWindowRect(reinterpret_cast<HWND>(winId()), &bounds);
        const int x = static_cast<short>(LOWORD(nativeMessage->lParam));
        const int y = static_cast<short>(HIWORD(nativeMessage->lParam));
        const int edge = qMax(5, qRound(8 * devicePixelRatioF()));
        const bool left = x >= bounds.left && x < bounds.left + edge;
        const bool right = x < bounds.right && x >= bounds.right - edge;
        const bool top = y >= bounds.top && y < bounds.top + edge;
        const bool bottom = y < bounds.bottom && y >= bounds.bottom - edge;
        const int hit = top ? (left ? HTTOPLEFT : right ? HTTOPRIGHT : HTTOP)
            : bottom ? (left ? HTBOTTOMLEFT : right ? HTBOTTOMRIGHT : HTBOTTOM)
            : left ? HTLEFT : right ? HTRIGHT : HTCLIENT;
        *result = hit;
        return true;
    }
    if (nativeMessage->message == WM_GETMINMAXINFO) {
        const HMONITOR monitor = MonitorFromWindow(reinterpret_cast<HWND>(winId()), MONITOR_DEFAULTTONEAREST);
        MONITORINFO info = {sizeof(MONITORINFO)};
        if (GetMonitorInfoW(monitor, &info)) {
            auto *limits = reinterpret_cast<MINMAXINFO *>(nativeMessage->lParam);
            limits->ptMaxPosition = {info.rcWork.left - info.rcMonitor.left, info.rcWork.top - info.rcMonitor.top};
            limits->ptMaxSize = {info.rcWork.right - info.rcWork.left, info.rcWork.bottom - info.rcWork.top};
            limits->ptMinTrackSize = {qRound(minimumWidth() * devicePixelRatioF()), qRound(minimumHeight() * devicePixelRatioF())};
            *result = 0;
            return true;
        }
    }
    if (nativeMessage->message == SingleInstance::activationMessage()) {
        if (isMinimized()) setWindowState(windowState() & ~Qt::WindowMinimized);
        show();
        raise();
        activateWindow();
        // Preserve an open modal editor and bring it forward with its owner.
        QWidget *target = QApplication::activeModalWidget();
        if (!target) target = this;
        target->raise();
        target->activateWindow();
        SetForegroundWindow(reinterpret_cast<HWND>(target->winId()));
        *result = 0;
        return true;
    }
    return QMainWindow::nativeEvent(eventType, message, result);
}
#endif

void MainWindow::setupUI()
{
    auto *newTaskShortcut = new QShortcut(QKeySequence::New, this);
    newTaskShortcut->setObjectName(QStringLiteral("shortcutNewTask"));
    connect(newTaskShortcut, &QShortcut::activated, this, &MainWindow::onCreateTaskClicked);

    auto *findTaskShortcut = new QShortcut(QKeySequence::Find, this);
    findTaskShortcut->setObjectName(QStringLiteral("shortcutFindTask"));
    connect(findTaskShortcut, &QShortcut::activated, this, [this]() {
        sidebar_->setCurrentRow(1);
        if (auto *searchEdit = taskListView_->findChild<QLineEdit *>(QStringLiteral("taskSearchEdit"))) {
            searchEdit->setFocus();
        }
    });

    auto *refreshShortcut = new QShortcut(QKeySequence(Qt::Key_F5), this);
    refreshShortcut->setObjectName(QStringLiteral("shortcutRefreshAll"));
    connect(refreshShortcut, &QShortcut::activated, this, &MainWindow::refreshAllViews);

    // === Central widget ===
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // === TitleBar (52px) ===
    titleBar_ = new TitleBar(this);
    mainLayout->addWidget(titleBar_);

    // === Content area (sidebar + stacked widget) ===
    QWidget *contentWidget = new QWidget(this);
    QHBoxLayout *contentLayout = new QHBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    // Sidebar (QListWidget, 220px fixed, 8 items + dynamic spacer)
    sidebar_ = new QListWidget(this);
    sidebar_->setObjectName("sidebar");
    sidebar_->setFixedWidth(220);
    sidebar_->setFont(QFont(QStringLiteral("Segoe UI"), 15));
    sidebar_->setIconSize(QSize(18, 18));
    sidebar_->setFocusPolicy(Qt::NoFocus);
    sidebar_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sidebar_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sidebar_->setItemDelegate(new SidebarDelegate(this));

    // 7 navigation items + dynamic spacer + settings
    sidebar_->addItem("首页");
    sidebar_->addItem("任务管理");
    sidebar_->addItem("周期任务");
    sidebar_->addItem("参考资料");
    sidebar_->addItem("年度成果");
    sidebar_->addItem("统计");
    sidebar_->addItem("回收站");

    // Dynamic spacer item (recalculated on resize to push settings to bottom)
    sidebarSpacerItem_ = new QListWidgetItem();
    sidebarSpacerItem_->setFlags(Qt::NoItemFlags);
    sidebarSpacerItem_->setSizeHint(QSize(0, 0));
    sidebar_->addItem(sidebarSpacerItem_);

    QListWidgetItem *settingsItem = new QListWidgetItem("设置中心");
    settingsItem->setIcon(QIcon(":/icons/nav-settings.svg"));
    sidebar_->addItem(settingsItem);

    // Stacked widget (8 pages; all pages are populated by setup methods)
    stackedWidget_ = new QStackedWidget(this);
    for (int i = 0; i < 8; ++i) {
        QWidget *page = new QWidget(this);
        stackedWidget_->addWidget(page);
    }

    contentLayout->addWidget(sidebar_);
    contentLayout->addWidget(stackedWidget_, 1);
    mainLayout->addWidget(contentWidget, 1);

    // === Status bar (36px) ===
    QStatusBar *statusBar = new QStatusBar(this);
    statusBar->setSizeGripEnabled(false);
    statusBar->setMinimumHeight(36);
    statusBar->setMaximumHeight(36);
    setStatusBar(statusBar);

    // Status bar content: left=DB, center=task count, right=work dir
    QWidget *statusContent = new QWidget(this);
    QHBoxLayout *statusLayout = new QHBoxLayout(statusContent);
    statusLayout->setContentsMargins(16, 0, 16, 0);
    statusLayout->setSpacing(0);

    QFont statusFont(QStringLiteral("Segoe UI"), 13);
    dbStatusLabel_ = new QLabel("数据库: 就绪", statusContent);
    taskCountLabel_ = new QLabel("任务: 0 条", statusContent);
    workDirLabel_ = new QLabel("工作目录: 未设置", statusContent);

    for (QLabel *label : {dbStatusLabel_, taskCountLabel_, workDirLabel_}) {
        label->setFont(statusFont);
        label->setStyleSheet("color: #475569;");
    }

    statusLayout->addWidget(dbStatusLabel_);
    statusLayout->addStretch();
    statusLayout->addWidget(taskCountLabel_);
    statusLayout->addStretch();
    statusLayout->addWidget(workDirLabel_);
    statusBar->addWidget(statusContent, 1);

    // === Signal connections ===
    // Sidebar row change → switch stacked widget page (row 8=设置中心 → page index 7; row 7=spacer, skip)
    connect(sidebar_, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row == 8) {
            stackedWidget_->setCurrentIndex(7);
        } else if (row >= 0 && row < 7) {
            stackedWidget_->setCurrentIndex(row);
        }
        onSidebarRowChanged(row);
    });
    // TitleBar buttons
    connect(titleBar_, &TitleBar::minimizeClicked, this, &MainWindow::onMinimizeClicked);
    connect(titleBar_, &TitleBar::maximizeClicked, this, &MainWindow::onMaximizeClicked);
    connect(titleBar_, &TitleBar::closeClicked, this, &MainWindow::onCloseClicked);
    connect(titleBar_, &TitleBar::navigateBackClicked, this, [this]() {
        if (navigationPosition_ <= 0) return;
        navigatingHistory_ = true;
        sidebar_->setCurrentRow(navigationRows_.at(--navigationPosition_));
        navigatingHistory_ = false;
        updateNavigationButtons();
    });
    connect(titleBar_, &TitleBar::navigateForwardClicked, this, [this]() {
        if (navigationPosition_ + 1 >= navigationRows_.size()) return;
        navigatingHistory_ = true;
        sidebar_->setCurrentRow(navigationRows_.at(++navigationPosition_));
        navigatingHistory_ = false;
        updateNavigationButtons();
    });
    connect(titleBar_, &TitleBar::refreshClicked, this, [this]() {
        refreshAllViews();
        ToastWidget::show(this, ToastWidget::Success, QStringLiteral("刷新完成"));
    });
    connect(titleBar_, &TitleBar::createTaskClicked, this, &MainWindow::onCreateTaskClicked);
    connect(titleBar_, &TitleBar::searchSubmitted, this, [this](const QString &text) {
        sidebar_->setCurrentRow(1);
        if (taskListView_) taskListView_->searchFor(text);
    });

    // Default to first page (首页)
    sidebar_->setCurrentRow(0);
}

void MainWindow::setupStyle()
{
    AppTheme::apply(*qApp, *this);
}

void MainWindow::centerOnScreen()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect geometry = screen->availableGeometry();
        move(geometry.center().x() - width() / 2,
             geometry.center().y() - height() / 2);
    }
}

void MainWindow::setupWindowShadow()
{
#ifdef Q_OS_WIN
    WindowsChrome::configure(this);
#endif
}

void MainWindow::onMinimizeClicked()
{
    showMinimized();
}

void MainWindow::onMaximizeClicked()
{
    if (isMaximized()) {
        showNormal();
    } else {
        showMaximized();
    }
    titleBar_->setMaximized(isMaximized());
}

void MainWindow::onCloseClicked()
{
    close();
}

void MainWindow::setupTaskManagementPage()
{
    // Replace the placeholder Task Management page (index 1) with TaskListView
    QWidget *placeholderPage = stackedWidget_->widget(1);
    QScrollArea *scrollArea = new QScrollArea(placeholderPage);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    taskListView_ = new TaskListView(this);
    taskListView_->setAutoFillBackground(true);
    scrollArea->setWidget(taskListView_);
    QVBoxLayout *pageLayout = new QVBoxLayout(placeholderPage);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->addWidget(scrollArea);

    // Connect signals
    connect(taskListView_, &TaskListView::createTaskClicked,
            this, &MainWindow::onCreateTaskClicked);
    connect(taskListView_, &TaskListView::taskClicked,
            this, &MainWindow::onTaskClicked);
    connect(taskListView_, &TaskListView::taskDeleted,
            this, [this](int) { refreshAllViews(); });

    // Initial refresh
    taskListView_->refresh();
}

void MainWindow::setupReferencePage()
{
    // Replace the placeholder 参考资料 page (index 3) with ReferenceView
    QWidget *placeholderPage = stackedWidget_->widget(3);
    QScrollArea *scrollArea = new QScrollArea(placeholderPage);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    referenceView_ = new ReferenceView(this);
    referenceView_->setAutoFillBackground(true);
    scrollArea->setWidget(referenceView_);
    QVBoxLayout *pageLayout = new QVBoxLayout(placeholderPage);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->addWidget(scrollArea);

    // Connect signals
    connect(referenceView_, &ReferenceView::createReferenceClicked,
            this, &MainWindow::onCreateReferenceClicked);
    connect(referenceView_, &ReferenceView::referenceClicked,
            this, &MainWindow::onReferenceClicked);

    // Initial refresh
    referenceView_->refresh();
}

void MainWindow::setupAchievementPage()
{
    // Replace the placeholder 年度成果 page (index 4) with AchievementView
    QWidget *placeholderPage = stackedWidget_->widget(4);
    QScrollArea *scrollArea = new QScrollArea(placeholderPage);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    achievementView_ = new AchievementView(this);
    achievementView_->setAutoFillBackground(true);
    scrollArea->setWidget(achievementView_);
    QVBoxLayout *pageLayout = new QVBoxLayout(placeholderPage);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->addWidget(scrollArea);

    // Connect signals
    connect(achievementView_, &AchievementView::exportRequested,
            this, &MainWindow::onExportAchievements);

    // Initial refresh
    achievementView_->refresh();
}

void MainWindow::setupSettingsPage()
{
    // Replace the placeholder 设置中心 page (index 7) with SettingsView
    QWidget *placeholderPage = stackedWidget_->widget(7);
    QScrollArea *scrollArea = new QScrollArea(placeholderPage);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    settingsView_ = new SettingsView(this);
    settingsView_->setAutoFillBackground(true);
    scrollArea->setWidget(settingsView_);
    QVBoxLayout *pageLayout = new QVBoxLayout(placeholderPage);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->addWidget(scrollArea);

    // Connect signals for backup/restore/workdir change
    connect(settingsView_, &SettingsView::workDirChangeRequested,
            this, &MainWindow::onWorkDirChangeRequested);
    connect(settingsView_, &SettingsView::backupRequested,
            this, &MainWindow::onBackupRequested);
    connect(settingsView_, &SettingsView::restoreRequested,
            this, &MainWindow::onRestoreRequested);
    connect(settingsView_, &SettingsView::scanImportRequested,
            this, &MainWindow::onScanImportRequested);
    connect(settingsView_, &SettingsView::initCompleted,
            this, &MainWindow::onInitCompleted);
    connect(settingsView_, &SettingsView::fontPreferencesChanged,
            this, [this](int pixelSize, int weight) {
                AppTheme::applyTypography(*qApp, *this, pixelSize, weight);
            });

    // Initial refresh
    settingsView_->refresh();
}

void MainWindow::setupHomePage()
{
    // Replace the placeholder 首页 page (index 0) with HomeView
    QWidget *placeholderPage = stackedWidget_->widget(0);
    QScrollArea *scrollArea = new QScrollArea(placeholderPage);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    homeView_ = new HomeView(this);
    homeView_->setAutoFillBackground(true);
    scrollArea->setWidget(homeView_);
    QVBoxLayout *pageLayout = new QVBoxLayout(placeholderPage);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->addWidget(scrollArea);

    // Connect signals
    connect(homeView_, &HomeView::taskClicked,
            this, &MainWindow::onTaskClicked);
    connect(homeView_, &HomeView::createTaskClicked,
            this, &MainWindow::onCreateTaskClicked);
    connect(homeView_, &HomeView::viewAllTasksRequested,
            this, [this]() { sidebar_->setCurrentRow(1); });  // 切换到任务管理页面
    connect(homeView_, &HomeView::createPeriodicTaskFromTemplate,
            this, &MainWindow::onCreatePeriodicTaskFromTemplate);
    connect(homeView_, &HomeView::ignorePeriodicReminder,
            this, &MainWindow::onIgnorePeriodicReminder);

    // Initial refresh
    homeView_->refresh();
}

void MainWindow::setupStatsPage()
{
    // Replace the placeholder 统计 page (index 5) with StatsView
    QWidget *placeholderPage = stackedWidget_->widget(5);
    QScrollArea *scrollArea = new QScrollArea(placeholderPage);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    statsView_ = new StatsView(this);
    statsView_->setAutoFillBackground(true);
    scrollArea->setWidget(statsView_);
    QVBoxLayout *pageLayout = new QVBoxLayout(placeholderPage);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->addWidget(scrollArea);

    // 统计页面无交互信号（纯展示），初始刷新
    statsView_->refresh();
}

void MainWindow::setupPeriodicTaskPage()
{
    // Replace the placeholder 周期任务 page (index 2) with PeriodicTaskView
    QWidget *placeholderPage = stackedWidget_->widget(2);
    QScrollArea *scrollArea = new QScrollArea(placeholderPage);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    periodicTaskView_ = new PeriodicTaskView(this);
    periodicTaskView_->setAutoFillBackground(true);
    scrollArea->setWidget(periodicTaskView_);
    QVBoxLayout *pageLayout = new QVBoxLayout(placeholderPage);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->addWidget(scrollArea);

    // Connect signals
    connect(periodicTaskView_, &PeriodicTaskView::createPeriodicTaskClicked,
            this, &MainWindow::onCreatePeriodicTaskClicked);
    connect(periodicTaskView_, &PeriodicTaskView::editPeriodicTask,
            this, &MainWindow::onEditPeriodicTask);

    // Initial refresh
    periodicTaskView_->refresh();
}

void MainWindow::setupRecycleBinPage()
{
    // Replace the placeholder 回收站 page (index 6) with RecycleBinView
    QWidget *placeholderPage = stackedWidget_->widget(6);
    QScrollArea *scrollArea = new QScrollArea(placeholderPage);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    recycleBinView_ = new RecycleBinView(this);
    recycleBinView_->setAutoFillBackground(true);
    scrollArea->setWidget(recycleBinView_);
    QVBoxLayout *pageLayout = new QVBoxLayout(placeholderPage);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->addWidget(scrollArea);

    // Connect signals - refresh task list and status bar when recycle bin changes
    connect(recycleBinView_, &RecycleBinView::tasksRestored,
            this, &MainWindow::onRecycleBinChanged);
    connect(recycleBinView_, &RecycleBinView::tasksDeleted,
            this, &MainWindow::onRecycleBinChanged);

    // Initial refresh
    recycleBinView_->refresh();
}

void MainWindow::onCheckNewDay()
{
    if (MainWindowMaintenanceUseCase().clearFocusIfNewDay()) {
        Logger::instance().info(QStringLiteral("运行时检测到跨天，已清零今日专注"));
        if (homeView_) {
            homeView_->refresh();
        }
    }
}

void MainWindow::refreshStatusBar()
{
    const MainWindowStatus status = MainWindowMaintenanceUseCase().status();
    if (dbStatusLabel_) {
        dbStatusLabel_->setText(status.workDir.isEmpty()
            ? QStringLiteral("数据库: 就绪  ·  工作目录: 未设置")
            : QStringLiteral("数据库: 就绪  ·  工作目录: %1")
                .arg(QLocale().formattedDataSize(status.workDirBytes, 1)));
    }
    if (taskCountLabel_ || workDirLabel_) {
        if (taskCountLabel_) {
            taskCountLabel_->setText(QString("任务: %1 条").arg(status.taskCount));
        }
        if (workDirLabel_ && status.workDir.isEmpty()) {
            workDirLabel_->setText("工作目录: 未设置");
        } else if (workDirLabel_) {
            workDirLabel_->setText("工作目录: " + status.workDir);
        }
    }
}

void MainWindow::onCreateTaskClicked()
{
    DialogScrim scrim(this);
    scrim.show();
    TaskCreateDialog dialog(this);
    connect(&dialog, &TaskCreateDialog::taskCreated, [this](const QString &) {
        if (taskListView_) taskListView_->showCreatedTask();
        if (sidebar_->currentRow() == 1) refreshStatusBar();
        else refreshAllViews();
    });
    dialog.exec();
    scrim.hide();
}

void MainWindow::onTaskClicked(int taskId)
{
    bool reopenAfterEdit = false;
    do {
        TaskDetailView *detailView = new TaskDetailView(taskId, this);
        detailView->setAttribute(Qt::WA_DeleteOnClose);

        reopenAfterEdit = false;
        connect(detailView, &TaskDetailView::editRequested,
                this, [this, taskId, &reopenAfterEdit](int) {
            // 打开编辑弹窗（emit 同步调用此 lambda，编辑弹窗关闭后返回）
            TaskEditDialog *editDialog = new TaskEditDialog(taskId, this);
            editDialog->setAttribute(Qt::WA_DeleteOnClose);
            connect(editDialog, &TaskEditDialog::taskUpdated, this, [this]() {
                refreshAllViews();
            });
            DialogScrim editScrim(this);
            editScrim.show();
            editDialog->exec();
            editScrim.hide();
            // 编辑弹窗关闭后（无论保存还是取消），标记需要重新打开详情
            reopenAfterEdit = true;
        });
        connect(detailView, &TaskDetailView::taskUpdated, this, [this]() {
            refreshAllViews();
        });
        connect(detailView, &TaskDetailView::taskDeleted, this, [&reopenAfterEdit](int) {
            reopenAfterEdit = false;
        });

        DialogScrim scrim(this);
        scrim.show();
        detailView->exec();
        scrim.hide();
    } while (reopenAfterEdit);
}

void MainWindow::onEditRequested(int taskId)
{
    TaskEditDialog *editDialog = new TaskEditDialog(taskId, this);
    editDialog->setAttribute(Qt::WA_DeleteOnClose);

    connect(editDialog, &TaskEditDialog::taskUpdated, this, [this]() {
        refreshAllViews();
    });

    DialogScrim scrim(this);
    scrim.show();
    editDialog->exec();
    scrim.hide();
}

void MainWindow::onCreateReferenceClicked()
{
    DialogScrim scrim(this);
    scrim.show();
    ReferenceCreateDialog dialog(this);
    connect(&dialog, &ReferenceCreateDialog::referenceCreated, this, [this](int) {
        if (referenceView_) {
            referenceView_->showCreatedReference();
        }
    });
    dialog.exec();
    scrim.hide();
}

void MainWindow::onReferenceClicked(int refId)
{
    // Loop allows reopening the detail dialog after editing to show fresh data
    while (true) {
        ReferenceDetailDialog detailDialog(refId, this);

        bool reopen = false;

        connect(&detailDialog, &ReferenceDetailDialog::editRequested, this,
                [this, refId, &reopen]() {
            ReferenceEditDialog editDialog(refId, this);
            connect(&editDialog, &ReferenceEditDialog::referenceUpdated, this, [this](int) {
                if (referenceView_) {
                    referenceView_->refresh();
                }
            });
            DialogScrim editScrim(this);
            editScrim.show();
            editDialog.exec();
            editScrim.hide();
            // 无论保存还是取消，都重新打开详情弹窗（显示最新数据）
            reopen = true;
        });

        connect(&detailDialog, &ReferenceDetailDialog::referenceDeleted, this, [this](int) {
            if (referenceView_) {
                referenceView_->refresh();
            }
        });

        DialogScrim scrim(this);
        scrim.show();
        detailDialog.exec();
        scrim.hide();

        if (!reopen) {
            break;
        }
    }
}

void MainWindow::onExportAchievements(int year)
{
    QString defaultFileName = QString("年度工作总结_%1.doc").arg(year);
    QString outputPath = QFileDialog::getSaveFileName(
        this, "导出年度工作总结", defaultFileName,
        "Word 文档 (*.doc);;所有文件 (*.*)");

    if (outputPath.isEmpty()) {
        return;  // user cancelled
    }

    // Show progress dialog during export
    QProgressDialog progress("正在导出年度工作总结...", QString(), 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    progress.setCancelButton(nullptr);
    progress.show();

    WordExport::ExportResult result = AchievementUseCase().exportAnnualReport(year, outputPath);

    progress.close();

    if (result.success) {
        StyledMessageBox::information(this, "导出成功",
            QString("已成功导出 %1 条记录到：\n%2")
                .arg(result.itemCount)
                .arg(result.outputPath));
    } else {
        StyledMessageBox::warning(this, "导出失败", result.errorMessage);
    }
}

void MainWindow::onCreatePeriodicTaskClicked()
{
    DialogScrim scrim(this);
    scrim.show();
    PeriodicTaskCreateDialog dialog(this);
    connect(&dialog, &PeriodicTaskCreateDialog::templateSaved, this, [this]() {
        if (periodicTaskView_) {
            periodicTaskView_->refresh();
        }
    });
    dialog.exec();
    scrim.hide();
}

void MainWindow::onEditPeriodicTask(int id)
{
    DialogScrim scrim(this);
    scrim.show();
    PeriodicTaskCreateDialog dialog(id, this);
    connect(&dialog, &PeriodicTaskCreateDialog::templateSaved, this, [this]() {
        if (periodicTaskView_) {
            periodicTaskView_->refresh();
        }
    });
    dialog.exec();
    scrim.hide();
}

void MainWindow::onCreatePeriodicTaskFromTemplate(int templateId)
{
    // 从周期任务模板创建实际任务
    HomeCommandResult result = HomeCommandUseCase().createTaskFromPeriodicTemplate(templateId);
    if (!result.ok) {
        StyledMessageBox::warning(this, QStringLiteral("创建失败"), result.error);
        return;
    }

    ToastWidget::show(this, ToastWidget::Success, QStringLiteral("已从周期任务创建新任务"));
    refreshAllViews();
}

void MainWindow::onIgnorePeriodicReminder(int templateId)
{
    // 忽略本次提醒，推进到下一周期
    HomeCommandResult result = HomeCommandUseCase().ignorePeriodicReminder(templateId);
    if (!result.ok) {
        StyledMessageBox::warning(this, QStringLiteral("操作失败"), result.error);
        return;
    }

    ToastWidget::show(this, ToastWidget::Success, QStringLiteral("已忽略本次提醒"));
    refreshAllViews();
}

void MainWindow::onWorkDirChangeRequested()
{
    QString dir = QFileDialog::getExistingDirectory(
        this, QStringLiteral("选择工作目录的存放位置"),
        QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir.isEmpty()) {
        return;
    }

    const WorkDirChangeResult result = MainWindowMaintenanceUseCase().changeWorkDir(dir);
    if (result.success) {
        if (!result.previousWorkDir.isEmpty() && result.previousWorkDir != result.workDir) {
            Logger::instance().info(QStringLiteral("工作目录变更，已批量更新任务路径: %1 → %2")
                                        .arg(result.previousWorkDir, result.workDir));
        }
        if (settingsView_) {
            settingsView_->setWorkDir(result.workDir);
        }
        refreshStatusBar();
        refreshAllViews();
        ToastWidget::show(this, ToastWidget::Success, QStringLiteral("工作目录已更新"));
        Logger::instance().info(QStringLiteral("工作目录已更新: ") + result.workDir);
    } else {
        ToastWidget::show(this, ToastWidget::Error,
                          QStringLiteral("工作目录设置失败: ") + result.errorMessage);
        Logger::instance().error(QStringLiteral("工作目录设置失败: ") + result.errorMessage);
    }
}

void MainWindow::onInitCompleted()
{
    // 初始化完成：刷新所有页面，提示成功，引导重新设置工作目录
    refreshAllViews();
    refreshStatusBar();
    if (settingsView_) {
        settingsView_->refresh();
    }
    ToastWidget::show(this, ToastWidget::Success, QStringLiteral("软件已初始化，请重新设置工作目录"));
    Logger::instance().info(QStringLiteral("MainWindow: 软件初始化完成，等待重新设置工作目录"));

    // 自动弹出工作目录设置弹窗，引导用户重新设置
    StyledMessageBox::information(this, QStringLiteral("初始化成功"),
        QStringLiteral("软件已恢复到初始状态。\n\n请重新设置工作目录以开始使用。"));
    onWorkDirChangeRequested();
}

void MainWindow::onBackupRequested()
{
    const MainWindowStatus status = MainWindowMaintenanceUseCase().status();
    if (status.workDir.isEmpty()) {
        StyledMessageBox::warning(this, QStringLiteral("无法备份"),
                                  QStringLiteral("请先设置工作目录。"));
        return;
    }
    const int mode = StyledMessageBox::choice(this, QStringLiteral("选择备份方式"),
        QStringLiteral("全量备份包含全部任务附件；简易备份保留任务数据、目录结构和信息文件。\n当前工作目录体积：%1")
            .arg(QLocale().formattedDataSize(status.workDirBytes, 1)),
        QStringLiteral("全量备份"), QStringLiteral("简易备份"));
    if (mode == 0) return;
    if (mode == 1 && StyledMessageBox::question(this, QStringLiteral("确认全量备份"),
            QStringLiteral("将复制整个工作目录（%1），大文件可能需要较长时间。是否继续？")
                .arg(QLocale().formattedDataSize(status.workDirBytes, 1)),
            QStringLiteral("开始备份"), QStringLiteral("取消")) != StyledMessageBox::Yes) return;
    if (MainWindowMaintenanceUseCase().manualBackup(mode == 1)) {
        ToastWidget::show(this, ToastWidget::Success,
                          mode == 1 ? QStringLiteral("全量备份成功") : QStringLiteral("简易备份成功"));
        Logger::instance().info(QStringLiteral("手动备份成功"));
        if (settingsView_) {
            settingsView_->refresh();
        }
    } else {
        ToastWidget::show(this, ToastWidget::Error, QStringLiteral("备份失败，请重试"));
        Logger::instance().error(QStringLiteral("手动备份失败"));
    }
}

void MainWindow::onRestoreRequested()
{
    QString niumaPath = QFileDialog::getExistingDirectory(
        this, QStringLiteral("选择 .niuma 备份目录"),
        QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (niumaPath.isEmpty()) {
        return;
    }

    if (!BackupService::verifyNiuma(niumaPath)) {
        StyledMessageBox::warning(this, QStringLiteral("备份无效"),
                                  QStringLiteral("数据库校验失败，无法恢复。"));
        return;
    }
    const QString backupType = BackupService::readNiumaMeta(niumaPath)
        .value(QStringLiteral("backup_type")).toString();
    const QString typeText = backupType == QStringLiteral("full")
        ? QStringLiteral("全量备份（含附件）")
        : backupType == QStringLiteral("simple")
            ? QStringLiteral("简易备份（不含附件）") : QStringLiteral("旧版数据备份");

    // 二次确认
    auto btn = StyledMessageBox::question(
        this, QStringLiteral("确认恢复"),
        QStringLiteral("备份类型：%1。恢复将替换当前数据库和相应工作目录内容；原工作目录会保留在同级回退目录。是否继续？")
            .arg(typeText),
        QStringLiteral("确认恢复"), QStringLiteral("取消"));
    if (btn != StyledMessageBox::Yes) {
        return;
    }

    if (MainWindowMaintenanceUseCase().restoreBackup(niumaPath)) {
        ToastWidget::show(this, ToastWidget::Success, QStringLiteral("恢复成功，请重启应用以加载恢复的数据"));
        Logger::instance().info(QStringLiteral("从备份恢复成功: ") + niumaPath);
        if (settingsView_) {
            settingsView_->refresh();
        }
    } else {
        ToastWidget::show(this, ToastWidget::Error, QStringLiteral("恢复失败，请检查备份文件"));
        Logger::instance().error(QStringLiteral("从备份恢复失败: ") + niumaPath);
    }
}

void MainWindow::onScanImportRequested()
{
    DialogScrim scrim(this);
    scrim.show();
    ScanImportDialog dialog(this);
    connect(&dialog, &ScanImportDialog::importCompleted,
            this, [this](int success, int skip, int fail) {
        if (taskListView_) {
            taskListView_->refresh();
        }
        if (referenceView_) {
            referenceView_->refresh();
        }
        refreshStatusBar();
        QString msg = QStringLiteral("导入完成：成功 %1 条，跳过 %2 条，失败 %3 条")
                          .arg(success).arg(skip).arg(fail);
        ToastWidget::show(this, ToastWidget::Info, msg);
    });
    dialog.exec();
    scrim.hide();
}

void MainWindow::onRecycleBinChanged()
{
    // 回收站发生变化后刷新任务列表与状态栏
    if (taskListView_) {
        taskListView_->refresh();
    }
    refreshStatusBar();
}

void MainWindow::onSidebarRowChanged(int row)
{
    if (row >= 0 && row != 7) {
        if (navigationRows_.isEmpty()) {
            navigationRows_.append(row);
            navigationPosition_ = 0;
        } else if (!navigatingHistory_ && navigationRows_.at(navigationPosition_) != row) {
            navigationRows_.resize(navigationPosition_ + 1);
            navigationRows_.append(row);
            navigationPosition_ = navigationRows_.size() - 1;
        }
        updateNavigationButtons();
    }

    // Refresh the corresponding view when switching to its page
    // row 7 = spacer (not selectable), row 8 = 设置中心
    if (row == 0 && homeView_) {
        homeView_->refresh();
    } else if (row == 1 && taskListView_) {
        taskListView_->refresh();
    } else if (row == 2 && periodicTaskView_) {
        periodicTaskView_->refresh();
    } else if (row == 3 && referenceView_) {
        referenceView_->refresh();
    } else if (row == 4 && achievementView_) {
        achievementView_->refresh();
    } else if (row == 5 && statsView_) {
        statsView_->refresh();
    } else if (row == 6 && recycleBinView_) {
        recycleBinView_->refresh();
    } else if (row == 8 && settingsView_) {
        settingsView_->refresh();
    }
}

void MainWindow::updateNavigationButtons()
{
    if (titleBar_) {
        titleBar_->setNavigationEnabled(navigationPosition_ > 0,
                                        navigationPosition_ + 1 < navigationRows_.size());
    }
}

void MainWindow::updateSidebarSpacer()
{
    if (!sidebar_ || !sidebarSpacerItem_) {
        return;
    }
    // 7 nav items (rows 0-6) + 1 spacer (row 7) + 1 settings (row 8)
    // Each item height is 48px (from SidebarDelegate::sizeHint)
    const int kItemHeight = 48;
    int navHeight = 7 * kItemHeight;   // rows 0-6
    int settingsHeight = kItemHeight;  // row 8
    int available = sidebar_->height() - navHeight - settingsHeight;
    int spacerHeight = available > 0 ? available : 0;
    const QSize hint(0, spacerHeight);
    if (sidebarSpacerItem_->sizeHint() != hint) sidebarSpacerItem_->setSizeHint(hint);
}

void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange && titleBar_) {
        titleBar_->setMaximized(isMaximized());
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (auto *outline = findChild<QWidget *>("windowOutline", Qt::FindDirectChildrenOnly)) {
        outline->setGeometry(rect());
        outline->raise();
    }
    updateSidebarSpacer();
    // No integer-pixel region: DWM maintains smooth corners during resizing.
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    // 全局点击失焦：点击文本输入控件以外区域时，清除当前焦点控件的焦点
    if (event->type() == QEvent::MouseButtonPress) {
        QWidget *focusWidget = qApp->focusWidget();
        if (focusWidget) {
            // 判断当前焦点控件是否为文本输入类型
            bool isTextInput = qobject_cast<QLineEdit *>(focusWidget)
                             || qobject_cast<QTextEdit *>(focusWidget)
                             || qobject_cast<QSpinBox *>(focusWidget)
                             || qobject_cast<QDoubleSpinBox *>(focusWidget);
            // QComboBox 可编辑时焦点在其 lineEdit 上
            if (!isTextInput) {
                auto *combo = qobject_cast<QComboBox *>(focusWidget);
                if (combo && combo->isEditable()) {
                    isTextInput = true;
                }
            }
            // QDateEdit/QTimeEdit/QDateTimeEdit 继承自 QAbstractSpinBox
            if (!isTextInput) {
                isTextInput = (focusWidget->inherits("QAbstractSpinBox"));
            }

            if (isTextInput) {
                // 检查点击目标是否是该焦点控件本身或其子控件
                // watched 是事件接收者，需要找到实际的 widget
                // Use the receiver, not another screen-coordinate hit test:
                // custom native frames can report a different global origin.
                QWidget *targetWidget = qobject_cast<QWidget *>(watched);
                if (targetWidget && targetWidget != focusWidget
                    && !focusWidget->isAncestorOf(targetWidget)) {
                    // 点击了外部区域，清除焦点
                    focusWidget->clearFocus();
                }
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::refreshAllViews()
{
    // 保存后只同步刷新当前页面；其它页面切换进入时已有对应刷新逻辑。
    const int row = sidebar_->currentRow();
    if (row == 0 && homeView_) homeView_->refresh();
    else if (row == 1 && taskListView_) taskListView_->refresh();
    else if (row == 2 && periodicTaskView_) periodicTaskView_->refresh();
    else if (row == 3 && referenceView_) referenceView_->refresh();
    else if (row == 4 && achievementView_) achievementView_->refresh();
    else if (row == 5 && statsView_) statsView_->refresh();
    else if (row == 6 && recycleBinView_) recycleBinView_->refresh();
    else if (row == 8 && settingsView_) settingsView_->refresh();
    refreshStatusBar();
}
