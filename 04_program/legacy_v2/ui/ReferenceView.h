/**
 * @file ReferenceView.h
 * @brief 参考资料页面（T4.8）
 *
 * 布局：
 *   QVBoxLayout
 *   ├── page-header（标题"参考资料" + 副标题 + "打开目录"按钮 + "＋ 新建资料"按钮）
 *   ├── filter-bar（QHBoxLayout，单行不换行：搜索/来源/标签/日期/搜索/重置）
 *   ├── 表格区（QFrame#Card 包裹 QTableView，5 列）
 *   └── 分页（PaginationWidget，50 条/页）
 *
 * 信号：
 *   - createReferenceClicked()：点击新建资料按钮
 *   - referenceClicked(int refId)：单击行
 *
 * 依赖：Qt5 Widgets、application/reference/ReferenceQueryUseCase、models/ReferenceMaterial、
 *       service/FileService、ui/widgets/PaginationWidget
 * 文档：task.md T4.8、qt_implementation_guide.md 3.3
 */
#pragma once

#include <QWidget>

#include "models/ReferenceFilter.h"
#include "models/ReferenceMaterial.h"


class QTableView;
class QLineEdit;
class QComboBox;
class QDateEdit;
class QPushButton;
class QLabel;
class QStandardItemModel;
class PaginationWidget;

class ReferenceView : public QWidget {
    Q_OBJECT

public:
    explicit ReferenceView(QWidget *parent = nullptr);

    void refresh();
    void showCreatedReference();

signals:
    void createReferenceClicked();
    void referenceClicked(int refId);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onSearchClicked();
    void onResetClicked();
    void onCreateReferenceClicked();
    void onOpenDirClicked();
    void onReferenceClicked(const QModelIndex &index);
    void onReferenceDoubleClicked(const QModelIndex &index);
    void onPageChanged(int page);
    void onFilterChanged();
    void onYearChanged();

private:
    // 筛选控件
    QLineEdit *searchEdit_;
    QComboBox *sourceCombo_;
    QComboBox *tagCombo_;
    QComboBox *yearCombo_;
    QDateEdit *startDateEdit_;
    QDateEdit *endDateEdit_;
    QPushButton *searchBtn_;
    QPushButton *resetBtn_;

    // 表格
    QTableView *tableView_;
    QStandardItemModel *model_;
    QLabel *emptyLabel_;

    // 分页
    PaginationWidget *pagination_;

    // 筛选状态
    ReferenceFilter filter_;
    bool dateRangeEdited_ = false;

    void setupUI();
    void setupFilterBar();
    void setupTable();
    void loadFilterOptions();
    void applyFilterToModel();
    void resetFilter();
    void setColumnWidths();
    void updateEmptyState(int total);
};
