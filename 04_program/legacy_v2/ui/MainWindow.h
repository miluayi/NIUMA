/**
 * @file MainWindow.h
 * @brief 主窗口类（T1.3/T1.4 完整实现）
 *
 * 无边框主窗口（Qt::FramelessWindowHint），包含：
 *   - 自定义 TitleBar（52px）：NIUMA 标题 + 最小化/最大化/关闭按钮
 *   - 侧边栏 QListWidget（220px，8 项导航）
 *   - QStackedWidget（8 个页面占位，含页标题）
 *   - QStatusBar（36px）：数据库状态 / 任务数 / 工作目录
 *
 * 窗口阴影通过 Windows API DwmExtendFrameIntoClientArea 实现。
 * 全局 QSS 从 :/styles/neutral_modern.qss 加载（qApp->setStyleSheet）。
 *
 * 依赖：Qt5 Widgets、ui/widgets/TitleBar、ui/widgets/SidebarDelegate
 * 文档：PRD 14.1、qt_implementation_guide.md 3.1、coding.md 7.1
 */
#pragma once

#include <QMainWindow>
#include <QVector>

class TitleBar;
class QListWidget;
class QListWidgetItem;
class QStackedWidget;
class QLabel;
class TaskListView;
class TaskDetailView;
class TaskEditDialog;
class ReferenceView;
class AchievementView;
class SettingsView;
class HomeView;
class StatsView;
class PeriodicTaskView;
class RecycleBinView;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void refreshStatusBar();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
#endif

private:
    void setupUI();
    void setupStyle();
    void setupWindowShadow();
    void centerOnScreen();
    void setupTaskManagementPage();
    void setupReferencePage();
    void setupAchievementPage();
    void setupSettingsPage();
    void setupHomePage();
    void setupStatsPage();
    void setupPeriodicTaskPage();
    void setupRecycleBinPage();

    void onMinimizeClicked();
    void onMaximizeClicked();
    void onCloseClicked();
    void onCreateTaskClicked();
    void onTaskClicked(int taskId);
    void onEditRequested(int taskId);
    void onCreateReferenceClicked();
    void onReferenceClicked(int refId);
    void onExportAchievements(int year);
    void onSidebarRowChanged(int row);
    void onCheckNewDay();
    void refreshAllViews();
    void onCreatePeriodicTaskClicked();
    void onEditPeriodicTask(int id);
    void onCreatePeriodicTaskFromTemplate(int templateId);
    void onIgnorePeriodicReminder(int templateId);
    void onWorkDirChangeRequested();
    void onBackupRequested();
    void onRestoreRequested();
    void onScanImportRequested();
    void onInitCompleted();
    void onRecycleBinChanged();

    void updateSidebarSpacer();
    void updateNavigationButtons();

    TitleBar *titleBar_;
    QListWidget *sidebar_;
    QListWidgetItem *sidebarSpacerItem_ = nullptr;
    QStackedWidget *stackedWidget_;
    QLabel *dbStatusLabel_;
    QLabel *taskCountLabel_;
    QLabel *workDirLabel_;
    TaskListView *taskListView_;
    ReferenceView *referenceView_;
    AchievementView *achievementView_;
    SettingsView *settingsView_;
    HomeView *homeView_;
    StatsView *statsView_;
    PeriodicTaskView *periodicTaskView_;
    RecycleBinView *recycleBinView_;
    QVector<int> navigationRows_;
    int navigationPosition_ = -1;
    bool navigatingHistory_ = false;
};
