/**
 * @file HomeView.h
 * @brief 首页（QWidget 子类，MainWindow stacked widget index 0）
 *
 * 布局严格对齐 03_ui_design/index.html 第 998-1145 行首页原型：
 *   QVBoxLayout
 *   ├── page-header（标题"首页" + 副标题"YYYY年M月D日 · 星期X" + "＋ 新建任务"按钮）
 *   ├── 今日专注（QFrame#FocusArea，渐变背景，full width）
 *   ├── 进行中任务（QFrame#Card，full width）
 *   └── 底部 QHBoxLayout
 *       ├── 周期任务提醒（QFrame#Card，左半）
 *       └── 本月概览（QFrame#Card，右半）
 *
 * 信号：
 *   - taskClicked(int taskId)：点击任务行（除五角星外）
 *   - createTaskClicked()：点击"新建任务"按钮
 *   - viewAllTasksRequested()：点击"→ 查看完整列表"
 *   - createPeriodicTaskFromTemplate(int templateId)：点击周期提醒"创建任务"按钮
 *   - ignorePeriodicReminder(int templateId)：点击周期提醒"忽略本次"按钮
 *
 * 依赖：application/home/HomeDashboardUseCase、models/Task、ui/ColorPalette、ui/Spacing、ui/Radius、
 *       utils/DateUtil
 * 文档：task.md T5（今日专注）、PRD 5.1（首页布局）、qt_implementation_guide.md 3.2
 *
 * 关键约束：
 *  - 五角星仅出现在首页"进行中任务"卡片与"今日专注"区，使用 Unicode ☆/★（非 SVG）
 *  - 今日专注仅查询 status='进行中' 任务，上限 3 个
 *  - 【重点】前缀红色 #FA423E，仅在 isKeyTask=true 时显示
 *  - 截止日期文本：deadline < today 且 status != '已完成' 时显示为红色 #DC2626
 *  - 任务管理页（TaskListView）不显示五角星
 */
#pragma once

#include <QWidget>
#include <QList>

#include "application/home/HomeDashboardUseCase.h"

class QFrame;
class QLabel;
class QPushButton;
class QVBoxLayout;
class QEvent;
class PaginationWidget;

class HomeView : public QWidget {
    Q_OBJECT

public:
    explicit HomeView(QWidget *parent = nullptr);

    /// 重新加载全部 4 个区域
    void refresh();

signals:
    void taskClicked(int taskId);
    void createTaskClicked();
    void viewAllTasksRequested();
    void createPeriodicTaskFromTemplate(int templateId);
    void ignorePeriodicReminder(int templateId);

protected:
    /// 行级点击拦截：QFrame 行捕获 MouseButtonPress → 发射 taskClicked
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onCreateTaskClicked();
    void onViewAllTasksClicked();
    void onStarClicked();
    void onPageChanged(int page);
    void onCreatePeriodicTaskClicked();
    void onIgnorePeriodicClicked();

private:
    // ---- 页头 ----
    QLabel *subtitleLabel_;

    // ---- 今日专注区 ----
    QFrame *focusArea_;
    QLabel *focusTitle_;
    QVBoxLayout *focusListLayout_;
    QLabel *focusEmptyLabel_;

    // ---- 进行中任务区 ----
    QLabel *inProgressCountLabel_;
    QVBoxLayout *inProgressListLayout_;
    PaginationWidget *inProgressPagination_;
    QLabel *inProgressInfoLabel_;  // 底部"显示 X-Y 条，共 Z 条"
    int inProgressCurrentPage_ = 1;

    // ---- 周期任务提醒区 ----
    QVBoxLayout *periodicListLayout_;
    QLabel *periodicEmptyLabel_;

    // ---- 本月概览 ----
    QLabel *statTotalLabel_;
    QLabel *statCompletedLabel_;
    QLabel *statRateLabel_;
    QLabel *statDiffLabel_;
    HomeDashboard dashboard_;

    // ---- UI 构建 ----
    void setupUI();
    QFrame* buildPageHeader();
    QFrame* buildFocusArea();
    QFrame* buildInProgressArea();
    QFrame* buildPeriodicArea();
    QFrame* buildMonthlyStatsArea();

    // ---- 数据加载 ----
    void loadFocusTasks();
    void loadInProgressTasks();
    void loadMonthlyStats();
    void loadPeriodicReminders();
    void updateDateSubtitle();

    // ---- 工具 ----
    void clearLayout(QLayout *layout);
    QString formatHeaderDate() const;
    QString formatDeadlineText(const QDate &deadline) const;
    QString weekdayChinese(int weekday) const;

    QPushButton* createStarButton(bool focused, int taskId);
    void setStarButtonState(QPushButton *button, bool focused) const;
    QFrame* createCategoryTag(const QString &category);
    QLabel* createTaskNameLabel(const class Task &task, bool redIfOverdue);
    QLabel* createDeadlineLabel(const class Task &task, QWidget *parent);
};
