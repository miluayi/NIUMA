/**
 * @file RecycleBinView.h
 * @brief 回收站页面（T8.1）
 *
 * 布局：
 *   QVBoxLayout
 *   ├── page-header（标题"回收站" + 副标题"已删除任务保留 30 天，过期自动清理"）
 *   ├── 批量操作工具栏（恢复所选 / 彻底删除所选 / 清空回收站 / 刷新 / 全选）
 *   ├── QTableWidget 表格（7 列：复选框/编号/任务名称/删除时间/剩余天数/原状态/操作）
 *   └── 分页（PaginationWidget，50 条/页）
 *
 * 信号：
 *   - tasksRestored(int count)：恢复完成
 *   - tasksDeleted(int count)：彻底删除完成
 *
 * 依赖：Qt5 Widgets、application/recyclebin/RecycleBinUseCase、ui/widgets/PaginationWidget
 * 文档：task.md T8.1、PRD 11.1、qt_guide 3.3
 */
#pragma once

#include <QList>
#include <QSet>
#include <QWidget>

#include "models/Task.h"

class QTableWidget;
class QCheckBox;
class QPushButton;
class QLabel;
class QTableWidgetItem;
class QVBoxLayout;
class PaginationWidget;

class RecycleBinView : public QWidget {
    Q_OBJECT

public:
    explicit RecycleBinView(QWidget *parent = nullptr);

    /// 刷新数据（重新加载已删除任务列表）
    void refresh();

signals:
    /// 恢复完成
    void tasksRestored(int count);
    /// 彻底删除完成
    void tasksDeleted(int count);

private slots:
    void onSelectAllToggled(bool checked);
    void onRestoreSelectedClicked();
    void onDeleteSelectedClicked();
    void onEmptyRecycleBinClicked();
    void onRefreshClicked();
    void onPageChanged(int page);

private:
    QTableWidget *table_;
    QCheckBox *selectAllCheckbox_;
    QPushButton *restoreSelectedBtn_;
    QPushButton *deleteSelectedBtn_;
    QPushButton *emptyBtn_;
    QPushButton *refreshBtn_;
    QLabel *emptyLabel_;
    PaginationWidget *pagination_;

    /// 当前已删除任务列表
    QList<Task> allTasks_;
    /// 选中的任务 ID 集合（跨页保持）
    QSet<int> selectedTaskIds_;

    /// 保留天数（用于计算剩余天数）
    static const int kRetentionDays = 30;
    /// 每页条数
    static const int kPageSize = 50;

    // ---- UI 构建 ----
    void setupUI();
    void setupToolbar(QVBoxLayout *mainLayout);
    void setupTable(QVBoxLayout *mainLayout);

    // ---- 数据加载 ----
    void loadData();
    void populateTable();
    void updateEmptyState(int total);

    // ---- 辅助 ----
    int currentPage() const;
    void clearSelection();
    QList<int> getSelectedTaskIds() const;
    void restoreTask(int taskId);
    void permanentDeleteTask(int taskId);

    /// 计算剩余天数（retentionDays - daysSinceDelete）
    static int calcRemainingDays(const Task &task);
    /// 根据剩余天数返回显示文本
    static QString remainingDaysText(int remaining);
    /// 根据剩余天数返回颜色
    static QColor remainingDaysColor(int remaining);
    /// 是否加粗显示
    static bool remainingDaysBold(int remaining);
};
