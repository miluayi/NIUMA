/**
 * @file TaskListView.h
 * @brief 任务管理页面（QWidget 子类）
 *
 * 布局：
 *   QVBoxLayout
 *   ├── page-header（标题"任务管理" + 副标题 + "＋ 新建任务"按钮）
 *   ├── filter-bar（QHBoxLayout，单行不换行：搜索/分类/来源/状态/办理人/日期/搜索/重置）
 *   ├── 表格区（QFrame#Card 包裹 QTableView，10 列）
 *   └── 分页（PaginationWidget）
 *
 * 信号：
 *   - taskClicked(int taskId)：单击行
 *   - createTaskClicked()：点击新建任务按钮
 *
 * 依赖：Qt5 Widgets、models/TaskTableModel、models/TaskFilter、
 *       ui/widgets/TaskItemDelegate、ui/widgets/PaginationWidget
 * 文档：task.md T1.9、qt_implementation_guide.md 3.3、coding.md 第七章
 */
#pragma once

#include <QWidget>

#include "models/TaskFilter.h"

class QTableView;
class QLineEdit;
class QComboBox;
class QDateEdit;
class QPushButton;
class QLabel;
class QStackedWidget;
class TaskTableModel;
class TaskItemDelegate;
class PaginationWidget;
struct TaskListData;

class TaskListView : public QWidget {
    Q_OBJECT

public:
    explicit TaskListView(QWidget *parent = nullptr);

    void refresh();
    void showCreatedTask();
    TaskFilter currentFilter() const;
    void searchFor(const QString &keyword);

signals:
    void taskClicked(int taskId);
    void createTaskClicked();
    void taskDeleted(int taskId);

private slots:
    void onSearchClicked();
    void onResetClicked();
    void onCreateTaskClicked();
    void onTaskDoubleClicked(const QModelIndex &index);
    void onTaskClicked(const QModelIndex &index);
    void onDeleteSelectedTask();
    void onPageChanged(int page);
    void onFilterChanged();
    void onYearChanged();

private:
    // 筛选控件
    QLineEdit *searchEdit_;
    QComboBox *categoryCombo_;
    QComboBox *sourceCombo_;
    QComboBox *tagCombo_;
    QComboBox *statusCombo_;
    QComboBox *handlerCombo_;
    QComboBox *yearCombo_;
    QDateEdit *startDateEdit_;
    QDateEdit *endDateEdit_;
    QPushButton *searchBtn_;
    QPushButton *resetBtn_;

    // 表格
    QTableView *tableView_;
    TaskTableModel *model_;
    TaskItemDelegate *delegate_;
    QStackedWidget *tableStateStack_;
    QLabel *emptyStateLabel_;

    // 分页
    PaginationWidget *pagination_;

    // 筛选状态
    TaskFilter filter_;
    bool dateRangeEdited_ = false;

    void setupUI();
    void setupFilterBar();
    void setupTable();
    void setupPagination();
    void loadFilterOptions(const TaskListData &data);
    void applyFilterToModel();
    void applyListData(const TaskListData &data);
    void resetFilter();
    void setColumnWidths();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
};
