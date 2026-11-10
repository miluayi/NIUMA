/**
 * @file AchievementView.h
 * @brief 年度成果页面（T4.3）
 *
 * 布局：
 *   QVBoxLayout
 *   ├── page-header（标题"年度成果" + 副标题 + 年份选择 QComboBox + "导出总结"按钮）
 *   ├── stats-card（4 项统计：成果总数 / 累计金额 / 重点成果 / 成果率）
 *   ├── 表格区（QFrame#Card 包裹 QTableView，含操作列）
 *   └── 分页（PaginationWidget，50 条/页）
 *
 * 信号：
 *   - exportRequested(int year)：点击"导出总结"按钮时发射，MainWindow 处理 WordExport
 *
 * 依赖：Qt5 Widgets、application/achievement/AchievementUseCase、
 *       ui/widgets/PaginationWidget、utils/AmountUtil
 * 文档：task.md T4.3、qt_implementation_guide.md 3.4、coding.md 第七章
 */
#pragma once

#include <QList>
#include <QWidget>

#include "application/achievement/AchievementUseCase.h"

class QTableView;
class QComboBox;
class QPushButton;
class QLabel;
class QVBoxLayout;
class QStandardItemModel;
class PaginationWidget;

class AchievementView : public QWidget {
    Q_OBJECT

public:
    explicit AchievementView(QWidget *parent = nullptr);

    // 刷新数据（重新加载当前年份）
    void refresh();

signals:
    void exportRequested(int year);

private slots:
    void onYearChanged(int index);
    void onExportClicked();
    void onPageChanged(int page);

private:
    // 头部控件
    QComboBox *yearCombo_;
    QPushButton *exportBtn_;

    // 统计标签
    QLabel *statTotalLabel_;
    QLabel *statAmountLabel_;
    QLabel *statKeyLabel_;
    QLabel *statRateLabel_;

    // 表格
    QTableView *tableView_;
    QStandardItemModel *model_;

    // 分页
    PaginationWidget *pagination_;

    // 状态
    int currentYear_;
    int currentPage_;
    QList<AchievementListItem> allAchievements_;  // 当前年份全部成果缓存（分页切片用）

    // 列枚举
    enum Column {
        ColTaskNo = 0,     // 编号
        ColWorkContent,    // 工作内容
        ColWorkObject,     // 工作对象
        ColResultCount,    // 成果数量
        ColResultAmount,   // 成果金额
        ColCreateTime,     // 完成时间
        ColTag,            // 标记
        ColActions,        // 操作
        ColumnCount
    };

    void setupUI();
    void setupHeader(QVBoxLayout *mainLayout);
    void setupStatsCard(QVBoxLayout *mainLayout);
    void setupTable(QVBoxLayout *mainLayout);
    void loadYearOptions();
    void loadData();
    void fillTablePage();
    void setRowActions(int row, const AchievementListItem &item);
    void updateStats(int totalCount, qint64 totalAmount, int keyCount, int completedCount);
    void setColumnWidths();
};
