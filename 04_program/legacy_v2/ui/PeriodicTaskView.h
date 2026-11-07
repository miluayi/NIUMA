/**
 * @file PeriodicTaskView.h
 * @brief 周期任务页面（Phase 7）
 *
 * 布局：
 *   QVBoxLayout
 *   ├── page-header（标题"周期任务" + 副标题 + "＋ 新建模板"按钮）
 *   ├── 表格区（QFrame#Card 包裹 QTableWidget，7 列）
 *   │     列：名称 / 分类 / 周期 / 来源 / 提醒时间 / 启用 / 操作
 *   └── 空状态占位（无模板时显示）
 *
 * 信号：
 *   - createPeriodicTaskClicked()：点击"新建模板"按钮
 *   - editPeriodicTask(int id)：点击行内"编辑"按钮
 *
 * 依赖：Qt5 Widgets、application/periodic/PeriodicTaskUseCase、models/PeriodicTask、
 *       ui/ColorPalette、ui/Spacing、ui/Radius
 * 文档：Phase 7、PRD 9.3、qt_implementation_guide.md 3.3、coding.md 第七章
 */
#pragma once

#include <QList>
#include <QWidget>

#include "models/PeriodicTask.h"

class QTableWidget;
class QTableWidgetItem;
class QLabel;
class QPushButton;

class PeriodicTaskView : public QWidget {
    Q_OBJECT

public:
    explicit PeriodicTaskView(QWidget *parent = nullptr);

    // 刷新数据（重新加载模板列表）
    void refresh();

signals:
    void createPeriodicTaskClicked();
    void editPeriodicTask(int id);

private slots:
    void onCreateClicked();
    void onEditClicked();
    void onDeleteClicked();
    void onEnabledToggled(bool checked);

private:
    QTableWidget *table_;
    QLabel *emptyLabel_;

    QList<PeriodicTask> templates_;  // 当前缓存数据

    // 列枚举
    enum Column {
        ColName = 0,         // 名称
        ColCategory,         // 分类
        ColPeriod,           // 周期
        ColSource,           // 来源
        ColTag,              // 标签
        ColReminderTime,     // 提醒时间
        ColEnabled,          // 启用
        ColActions,          // 操作
        ColumnCount
    };

    void setupUI();
    void loadData();
    void fillTable();
    void updateEmptyState();
    void setColumnWidths();

    // 工具：周期类型 + 数值 → 中文显示
    static QString periodDisplay(const PeriodicTask &t);

    // 取得某行对应的模板 ID（从 CellWidget 的 property 获取）
    int templateIdAtRow(int row) const;
};
