/**
 * @file TaskListView.cpp
 * @brief 任务管理页面实现
 *
 * 依赖：ui/TaskListView.h、models/TaskTableModel、ui/widgets/TaskItemDelegate、
 *       ui/widgets/PaginationWidget、application/task/TaskQueryUseCase
 * 文档：task.md T1.9、qt_implementation_guide.md 3.3、coding.md 第七章
 */
#include "TaskListView.h"
#include "widgets/RoundedCardBorder.h"
#include "widgets/ListFilterControls.h"
#include "widgets/YearPicker.h"

#include <QComboBox>
#include <QDateEdit>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSizePolicy>
#include <QTableView>
#include <QScrollBar>
#include <QShortcut>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWheelEvent>

#include "application/task/TaskQueryUseCase.h"
#include "application/task/TaskCommandUseCase.h"
#include "models/TaskTableModel.h"
#include "ui/ColorPalette.h"
#include "ui/Radius.h"
#include "ui/Spacing.h"
#include "ui/widgets/PaginationWidget.h"
#include "ui/widgets/StyledMessageBox.h"
#include "ui/widgets/TaskItemDelegate.h"

// ============================================================================
// 构造
// ============================================================================

TaskListView::TaskListView(QWidget *parent)
    : QWidget(parent)
    , searchEdit_(nullptr)
    , categoryCombo_(nullptr)
    , sourceCombo_(nullptr)
    , tagCombo_(nullptr)
    , statusCombo_(nullptr)
    , handlerCombo_(nullptr)
    , yearCombo_(nullptr)
    , startDateEdit_(nullptr)
    , endDateEdit_(nullptr)
    , searchBtn_(nullptr)
    , resetBtn_(nullptr)
    , tableView_(nullptr)
    , model_(nullptr)
    , delegate_(nullptr)
    , tableStateStack_(nullptr)
    , emptyStateLabel_(nullptr)
    , pagination_(nullptr)
{
    filter_.pageSize = 10;
    setupUI();
    filter_.year = QDate::currentDate().year();
    filter_.startDate = startDateEdit_->date();
    filter_.endDate = endDateEdit_->date();
}

// ============================================================================
// UI 构建
// ============================================================================

void TaskListView::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(Spacing::XXL, Spacing::XL, Spacing::XXL, Spacing::XL);
    mainLayout->setSpacing(Spacing::LG);

    // === page-header ===
    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(Spacing::MD);

    QVBoxLayout *titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(Spacing::XS);

    QLabel *titleLabel = new QLabel(QStringLiteral("\u4EFB\u52A1\u7BA1\u7406"), this);  // 任务管理
    QFont titleFont(QStringLiteral("Segoe UI"), 22, QFont::Normal);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color:#0F172A;");
    titleLayout->addWidget(titleLabel);

    QLabel *subtitleLabel = new QLabel(
        QStringLiteral("\u7BA1\u7406\u65E5\u5E38\u4EFB\u52A1\uFF0C"
                       "\u652F\u6301\u6309\u5206\u7C7B\u3001\u6765\u6E90\u3001"
                       "\u72B6\u6001\u3001\u529E\u7406\u4EBA\u7B49\u7EF4\u5EA6\u7B5B\u9009"),
        this);  // 管理日常任务，支持按分类、来源、状态、办理人等维度筛选
    QFont subtitleFont(QStringLiteral("Segoe UI"), 14);
    subtitleLabel->setFont(subtitleFont);
    subtitleLabel->setStyleSheet("color:#0F172A;");
    titleLayout->addWidget(subtitleLabel);

    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();

    // "＋ 新建任务" 按钮（全角加号 \uFF0B，非 emoji）
    QPushButton *createBtn = new QPushButton(
        QStringLiteral("\uFF0B \u65B0\u5EFA\u4EFB\u52A1"), this);  // ＋ 新建任务
    createBtn->setObjectName("btnPrimary");
    createBtn->setCursor(Qt::PointingHandCursor);
    createBtn->setStyleSheet(
        "QPushButton#btnPrimary {"
        "  background:#2463EB;"
        "  color:#FFFFFF;"
        "  border:1px solid #2463EB;"
        "  border-radius:8px;"
        "  padding:9px 18px;"
        "  font-size:15px;"
        "  font-weight:700;"
        "}"
        "QPushButton#btnPrimary:hover {"
        "  background:#1D4ED8;"
        "}"
    );
    connect(createBtn, &QPushButton::clicked, this, &TaskListView::onCreateTaskClicked);
    headerLayout->addWidget(createBtn);

    mainLayout->addLayout(headerLayout);

    // === 筛选栏 ===
    setupFilterBar();

    // === 表格区 ===
    setupTable();

    // === 分页 ===
    setupPagination();
}

void TaskListView::setupFilterBar()
{
    // 筛选栏拆分为两行
    QVBoxLayout *filterWrap = new QVBoxLayout();
    filterWrap->setContentsMargins(0, 0, 0, 0);
    filterWrap->setSpacing(Spacing::SM);

    // === 第一行：搜索框 + 分类 + 来源 + 标签 + 状态 + 办理人 ===
    QHBoxLayout *row1 = new QHBoxLayout();
    row1->setContentsMargins(0, 0, 0, 0);
    row1->setSpacing(Spacing::SM);

    // 搜索框（可拉伸，窗口放大时仅搜索框变宽）
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(
        QStringLiteral("搜索名称、编号或附件名..."));
    searchEdit_->setMinimumWidth(160);
    searchEdit_->setObjectName("taskSearchEdit");
    searchEdit_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    searchEdit_->setStyleSheet(
        "QLineEdit {"
        "  padding:8px 12px;"
        "  border:1px solid #E2E8F0;"
        "  border-radius:8px;"
        "  font-size:14px;"
        "  background:#FFFFFF;"
        "  color:#0F172A;"
        "}"
        "QLineEdit:focus {"
        "  border:2px solid #2463EB;"
        "  padding:7px 11px;"
        "}"
    );
    row1->addWidget(searchEdit_, 4);

    // 分类下拉（固定宽度）
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->setMinimumWidth(125);
    categoryCombo_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    categoryCombo_->setObjectName("taskCategoryCombo");
    categoryCombo_->setStyleSheet(
        "QComboBox {"
        "  padding:8px 12px;"
        "  border:1px solid #E2E8F0;"
        "  border-radius:8px;"
        "  font-size:14px;"
        "  background:#FFFFFF;"
        "  color:#0F172A;"
        "}"
        "QComboBox::drop-down {"
        "  border:none;"
        "  width:24px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  border:1px solid #E2E8F0;"
        "  border-radius:8px;"
        "  background:#FFFFFF;"
        "  selection-background-color:#F5F7FA;"
        "  selection-color:#2463EB;"
        "  outline:none;"
        "  padding:4px;"
        "}"
    );
    row1->addWidget(categoryCombo_, 3);

    // 来源下拉（固定宽度）
    sourceCombo_ = new QComboBox(this);
    sourceCombo_->setObjectName("taskSourceCombo");
    sourceCombo_->setMinimumWidth(145);
    sourceCombo_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    sourceCombo_->setStyleSheet(categoryCombo_->styleSheet());
    row1->addWidget(sourceCombo_, 3);

    // 事项标签下拉（固定宽度）
    tagCombo_ = new QComboBox(this);
    tagCombo_->setObjectName("taskTagCombo");
    tagCombo_->setMinimumWidth(135);
    tagCombo_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    tagCombo_->setStyleSheet(categoryCombo_->styleSheet());
    row1->addWidget(tagCombo_, 3);

    // 状态下拉（固定宽度）
    statusCombo_ = new QComboBox(this);
    statusCombo_->setMinimumWidth(140);
    statusCombo_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    statusCombo_->setObjectName("taskStatusCombo");
    statusCombo_->setStyleSheet(categoryCombo_->styleSheet());
    row1->addWidget(statusCombo_, 3);

    // 办理人下拉（固定宽度）
    handlerCombo_ = new QComboBox(this);
    handlerCombo_->setObjectName("taskHandlerCombo");
    handlerCombo_->setMinimumWidth(140);
    handlerCombo_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    handlerCombo_->setStyleSheet(categoryCombo_->styleSheet());
    row1->addWidget(handlerCombo_, 3);

    filterWrap->addLayout(row1);

    // === 第二行：时间选择器 + 搜索 + 重置 ===
    QHBoxLayout *row2 = new QHBoxLayout();
    row2->setContentsMargins(0, 0, 0, 0);
    row2->setSpacing(Spacing::SM);

    yearCombo_ = new YearPicker(this);
    yearCombo_->setObjectName("taskYearCombo");
    yearCombo_->setFixedWidth(156);
    ListFilterControls::reloadYears(yearCombo_, {});
    row2->addWidget(yearCombo_);

    // 开始日期
    startDateEdit_ = new QDateEdit(this);
    startDateEdit_->setCalendarPopup(true);
    startDateEdit_->setDisplayFormat("yyyy-MM-dd");
    startDateEdit_->setFixedWidth(130);
    startDateEdit_->setStyleSheet(
        "QDateEdit {"
        "  padding:8px 10px;"
        "  border:1px solid #E2E8F0;"
        "  border-radius:8px;"
        "  font-size:14px;"
        "  background:#FFFFFF;"
        "  color:#0F172A;"
        "}"
    );
    row2->addWidget(startDateEdit_);

    // "至" 字
    QLabel *toLabel = new QLabel(QStringLiteral("\u81F3"), this);  // 至
    toLabel->setStyleSheet("color:#0F172A; font-size:14px;");
    row2->addWidget(toLabel);

    // 结束日期
    endDateEdit_ = new QDateEdit(this);
    endDateEdit_->setCalendarPopup(true);
    endDateEdit_->setDisplayFormat("yyyy-MM-dd");
    endDateEdit_->setFixedWidth(130);
    endDateEdit_->setStyleSheet(startDateEdit_->styleSheet());
    row2->addWidget(endDateEdit_);

    // 搜索按钮（primary sm）
    searchBtn_ = new QPushButton(QStringLiteral("\u641C\u7D22"), this);  // 搜索
    searchBtn_->setObjectName("taskSearchButton");
    searchBtn_->setCursor(Qt::PointingHandCursor);
    searchBtn_->setStyleSheet(
        "QPushButton {"
        "  background:#2463EB;"
        "  color:#FFFFFF;"
        "  border:1px solid #2463EB;"
        "  border-radius:8px;"
        "  padding:8px 16px;"
        "  font-size:14px;"
        "  font-weight:700;"
        "}"
        "QPushButton:hover {"
        "  background:#1D4ED8;"
        "}"
    );
    connect(searchBtn_, &QPushButton::clicked, this, &TaskListView::onSearchClicked);
    row2->addWidget(searchBtn_);

    // 重置按钮（secondary sm）
    resetBtn_ = new QPushButton(QStringLiteral("\u91CD\u7F6E"), this);  // 重置
    resetBtn_->setObjectName("taskResetButton");
    resetBtn_->setCursor(Qt::PointingHandCursor);
    resetBtn_->setStyleSheet(
        "QPushButton {"
        "  background:#FFFFFF;"
        "  color:#475569;"
        "  border:1px solid #E2E8F0;"
        "  border-radius:8px;"
        "  padding:8px 16px;"
        "  font-size:14px;"
        "}"
        "QPushButton:hover {"
        "  background:#F5F7FA;"
        "  border-color:#CBD5E1;"
        "}"
    );
    connect(resetBtn_, &QPushButton::clicked, this, &TaskListView::onResetClicked);
    row2->addWidget(resetBtn_);

    // 第二行末尾拉伸，让控件左对齐
    row2->addStretch();

    filterWrap->addLayout(row2);

    ListFilterControls::syncYearRange(yearCombo_, startDateEdit_, endDateEdit_);
    connect(yearCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TaskListView::onYearChanged);
    for (QComboBox *combo : {categoryCombo_, sourceCombo_, tagCombo_, statusCombo_, handlerCombo_}) {
        connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TaskListView::onFilterChanged);
    }
    for (QDateEdit *edit : {startDateEdit_, endDateEdit_}) {
        connect(edit, &QDateEdit::dateChanged, this, [this]() { dateRangeEdited_ = true; });
    }

    // 将筛选栏加入主布局
    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(this->layout());
    if (mainLayout) {
        mainLayout->addLayout(filterWrap);
    }
}

void TaskListView::setupTable()
{
    // 表格卡片容器（QFrame#Card）
    QFrame *cardFrame = new QFrame(this);
    cardFrame->setObjectName("TaskListCard");
    cardFrame->setStyleSheet(
        "QFrame#TaskListCard {"
        "  background:#FFFFFF;"
        "  border:none;"
        "  border-radius:12px;"
        "}"
    );

    QVBoxLayout *cardLayout = new QVBoxLayout(cardFrame);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(0);

    // QTableView
    tableStateStack_ = new QStackedWidget(cardFrame);
    tableStateStack_->setObjectName("taskTableState");
    tableView_ = new QTableView(tableStateStack_);
    tableView_->setObjectName("taskTable");
    tableView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView_->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView_->setShowGrid(false);  // 无网格线
    tableView_->setAlternatingRowColors(false);
    tableView_->setMouseTracking(true);  // 启用 hover 检测
    tableView_->setStyleSheet(
        "QTableView {"
        "  border:none;"
        "  font-size:14px;"
        "  selection-background-color:#F5F7FA;"
        "  selection-color:#475569;"
        "  background:#FFFFFF;"
        "  outline:none;"
        "}"
        "QTableView::item {"
        "  padding:12px 14px;"
        "  border-bottom:1px solid #F1F5F9;"
        "  background:#FFFFFF;"
        "}"
        "QTableView::item:selected {"
        "  background:#F5F7FA;"
        "}"
        "QHeaderView::section {"
        "  background:#F5F8FC;"
        "  color:#000000;"
        "  font-weight:700;"
        "  font-size:14px;"
        "  padding:10px 7px;"
        "  border:none;"
        "  border-right:1px solid #DCE3EF;"
        "}"
        "QHeaderView {"
        "  background:#F5F8FC;"
        "}"
    );

    // 数据模型与委托
    model_ = new TaskTableModel(this);
    delegate_ = new TaskItemDelegate(this);
    tableView_->setModel(model_);
    tableView_->setItemDelegate(delegate_);

    auto *deleteShortcut = new QShortcut(QKeySequence::Delete, tableView_);
    deleteShortcut->setContext(Qt::WidgetShortcut);
    deleteShortcut->setObjectName(QStringLiteral("shortcutDeleteSelectedTask"));
    connect(deleteShortcut, &QShortcut::activated, this, &TaskListView::onDeleteSelectedTask);

    // 表头设置
    QHeaderView *header = tableView_->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Interactive);  // 列宽可拖拽
    header->setStretchLastSection(false);  // 不自动拉伸最后一列，右边界固定
    header->setHighlightSections(false);  // 点击表头不高亮
    header->setDefaultAlignment(Qt::AlignCenter);  // 标题居中对齐
    header->setMinimumSectionSize(48);  // 允许分类列和天数列按内容收窄

    // 隐藏行号
    tableView_->verticalHeader()->setVisible(false);
    tableView_->verticalHeader()->setDefaultSectionSize(48);  // 行高 48px
    tableView_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    tableView_->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    tableView_->verticalScrollBar()->setSingleStep(24);
    tableView_->horizontalScrollBar()->setSingleStep(24);
    tableView_->viewport()->installEventFilter(this);

    setColumnWidths();

    // 信号连接
    connect(tableView_, &QTableView::clicked, this, &TaskListView::onTaskClicked);
    connect(tableView_, &QTableView::doubleClicked, this, &TaskListView::onTaskDoubleClicked);

    emptyStateLabel_ = new QLabel(tableStateStack_);
    emptyStateLabel_->setObjectName("taskListEmptyState");
    emptyStateLabel_->setAlignment(Qt::AlignCenter);
    emptyStateLabel_->setStyleSheet("color:#64748B; font-size:14px;");
    tableStateStack_->addWidget(tableView_);
    tableStateStack_->addWidget(emptyStateLabel_);
    cardLayout->addWidget(tableStateStack_);

    // 分页控件
    pagination_ = new PaginationWidget(cardFrame);
    pagination_->setPageSize(10);
    connect(pagination_, &PaginationWidget::pageChanged, this, &TaskListView::onPageChanged);
    cardLayout->addWidget(pagination_);

    // 将表格卡片加入主布局（拉伸填充剩余空间，stretch=1）
    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(this->layout());
    if (mainLayout) {
        mainLayout->addWidget(cardFrame, 1);
    }
    new RoundedCardBorder(cardFrame);
}

void TaskListView::setupPagination()
{
    // 分页控件已在 setupTable 中创建并添加到 cardFrame
}

void TaskListView::setColumnWidths()
{
    // 默认列宽（10 列）；窄窗口保留横向滚动。
    // 设置最小列宽确保标题完整显示
    tableView_->setColumnWidth(TaskTableModel::ColTaskNo, 130);    // 编号
    tableView_->setColumnWidth(TaskTableModel::ColTitle, 240);     // 任务名称（可截断）
    tableView_->setColumnWidth(TaskTableModel::ColCategory, 48);   // 分类
    tableView_->setColumnWidth(TaskTableModel::ColSource, 110);    // 来源
    tableView_->setColumnWidth(TaskTableModel::ColTag, 100);       // 标签
    tableView_->setColumnWidth(TaskTableModel::ColHandler, 80);    // 办理人
    tableView_->setColumnWidth(TaskTableModel::ColStatus, 90);     // 状态
    tableView_->setColumnWidth(TaskTableModel::ColDeadline, 110);  // 截止
    tableView_->setColumnWidth(TaskTableModel::ColFinishTime, 110); // 完成日期
    tableView_->setColumnWidth(TaskTableModel::ColDays, 56);       // 天数

    // Interactive mode keeps Excel-style double-click auto sizing available.
}

// ============================================================================
// 筛选选项加载
// ============================================================================

void TaskListView::loadFilterOptions(const TaskListData &data)
{
    const QSignalBlocker categoryBlocker(categoryCombo_), sourceBlocker(sourceCombo_),
        tagBlocker(tagCombo_), statusBlocker(statusCombo_), handlerBlocker(handlerCombo_);
    const QString category = categoryCombo_->currentText();
    const QString status = statusCombo_->currentText();
    // 分类下拉：全部分类 / 催 / 查 / 报 / 办
    categoryCombo_->clear();
    categoryCombo_->addItem(QStringLiteral("\u5168\u90E8\u5206\u7C7B"));  // 全部分类
    categoryCombo_->addItem(QStringLiteral("\u50AC"));  // 催
    categoryCombo_->addItem(QStringLiteral("\u67E5"));  // 查
    categoryCombo_->addItem(QStringLiteral("\u62A5"));  // 报
    categoryCombo_->addItem(QStringLiteral("\u529E"));  // 办

    // 来源下拉：全部来源 + 从已存在任务中获取去重来源列表
    ListFilterControls::reloadOptions(sourceCombo_, QStringLiteral("全部来源"), data.sources);

    // 事项标签下拉：全部标签 + 从已存在任务中获取去重标签列表
    ListFilterControls::reloadOptions(tagCombo_, QStringLiteral("全部标签"), data.tags);

    // 状态下拉：全部状态 / 进行中 / 已完成 / 他人办理
    statusCombo_->clear();
    statusCombo_->addItem(QStringLiteral("\u5168\u90E8\u72B6\u6001"));  // 全部状态
    statusCombo_->addItem(QStringLiteral("\u8FDB\u884C\u4E2D"));  // 进行中
    statusCombo_->addItem(QStringLiteral("\u5DF2\u5B8C\u6210"));  // 已完成
    statusCombo_->addItem(QStringLiteral("\u4ED6\u4EBA\u529E\u7406"));  // 他人办理

    ListFilterControls::reloadOptions(handlerCombo_, QStringLiteral("全部办理人"), data.handlers);
    ListFilterControls::reloadYears(yearCombo_, data.years);
    if (!category.isEmpty()) categoryCombo_->setCurrentText(category);
    if (!status.isEmpty()) statusCombo_->setCurrentText(status);
}

// ============================================================================
// 刷新与数据加载
// ============================================================================

void TaskListView::refresh()
{
    const TaskListData data = TaskQueryUseCase().loadList(filter_);
    loadFilterOptions(data);
    applyListData(data);
}

void TaskListView::showCreatedTask()
{
    resetFilter();
}

TaskFilter TaskListView::currentFilter() const
{
    return filter_;
}

void TaskListView::applyFilterToModel()
{
    applyListData(TaskQueryUseCase().loadList(filter_));
}

void TaskListView::applyListData(const TaskListData &data)
{
    model_->setTasks(data.tasks);

    if (data.total == 0) {
        emptyStateLabel_->setText(filter_.isEmpty()
            ? QStringLiteral("暂无任务")
            : QStringLiteral("没有符合当前筛选条件的任务"));
        tableStateStack_->setCurrentWidget(emptyStateLabel_);
    } else {
        tableStateStack_->setCurrentWidget(tableView_);
    }

    pagination_->setTotal(data.total);
    pagination_->setPage(filter_.page);
    pagination_->updateDisplay();
}

// ============================================================================
// 筛选栏操作
// ============================================================================

void TaskListView::onFilterChanged()
{
    // 分类：index=0 为"全部分类"
    if (categoryCombo_->currentIndex() > 0) {
        filter_.category = categoryCombo_->currentText();
    } else {
        filter_.category.clear();
    }

    // 来源：index=0 为"全部来源"
    if (sourceCombo_->currentIndex() > 0) {
        filter_.source = sourceCombo_->currentText();
    } else {
        filter_.source.clear();
    }

    // 事项标签：index=0 为"全部标签"
    if (tagCombo_->currentIndex() > 0) {
        filter_.tag = tagCombo_->currentText();
    } else {
        filter_.tag.clear();
    }

    // 状态：index=0 为"全部状态"
    if (statusCombo_->currentIndex() > 0) {
        filter_.status = statusCombo_->currentText();
    } else {
        filter_.status.clear();
    }

    // 办理人：index=0 为"全部办理人"
    if (handlerCombo_->currentIndex() > 0) {
        filter_.handler = handlerCombo_->currentText();
    } else {
        filter_.handler.clear();
    }

    // 重置到第 1 页
    filter_.page = 1;

    applyFilterToModel();
}

void TaskListView::onSearchClicked()
{
    filter_.keyword = searchEdit_->text().trimmed();
    filter_.year = yearCombo_->currentData().toInt();
    const bool useRange = filter_.year > 0 || dateRangeEdited_;
    filter_.startDate = useRange ? startDateEdit_->date() : QDate();
    filter_.endDate = useRange ? endDateEdit_->date() : QDate();
    onFilterChanged();
}

void TaskListView::searchFor(const QString &keyword)
{
    searchEdit_->setText(keyword);
    onSearchClicked();
}

void TaskListView::onResetClicked()
{
    resetFilter();
}

void TaskListView::resetFilter()
{
    // 清空所有筛选控件
    searchEdit_->clear();
    for (QComboBox *combo : {categoryCombo_, sourceCombo_, tagCombo_, statusCombo_, handlerCombo_}) {
        const QSignalBlocker blocker(combo);
        combo->setCurrentIndex(0);
    }
    {
        const QSignalBlocker blocker(yearCombo_);
        yearCombo_->setCurrentIndex(yearCombo_->findData(QDate::currentDate().year()));
    }
    ListFilterControls::syncYearRange(yearCombo_, startDateEdit_, endDateEdit_);
    dateRangeEdited_ = false;

    // 重置 filter_
    filter_.reset();
    filter_.year = QDate::currentDate().year();
    filter_.startDate = startDateEdit_->date();
    filter_.endDate = endDateEdit_->date();
    filter_.pageSize = 10;
    filter_.page = 1;

    applyFilterToModel();
}

void TaskListView::onCreateTaskClicked()
{
    emit createTaskClicked();
}

// ============================================================================
// 表格交互
// ============================================================================

void TaskListView::onTaskClicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }
    int row = index.row();
    int taskId = model_->getTaskId(row);
    if (taskId > 0) {
        emit taskClicked(taskId);
    }
}

void TaskListView::onTaskDoubleClicked(const QModelIndex &index)
{
    // 双击同单击行为：发射 taskClicked 信号
    onTaskClicked(index);
}

void TaskListView::onDeleteSelectedTask()
{
    const QModelIndex index = tableView_->currentIndex();
    if (!index.isValid()) {
        return;
    }

    const int taskId = model_->getTaskId(index.row());
    if (taskId <= 0) {
        return;
    }

    const int answer = StyledMessageBox::question(
        this, QStringLiteral("确认删除"),
        QStringLiteral("确定要删除选中的任务吗？删除后可在回收站恢复。"),
        QStringLiteral("确认删除"), QStringLiteral("取消"));
    if (answer != StyledMessageBox::Yes) {
        return;
    }

    const TaskResult result = TaskCommandUseCase().deleteTask(taskId);
    if (!result.ok) {
        StyledMessageBox::warning(this, QStringLiteral("删除失败"),
                                  result.error.isEmpty()
                                      ? QStringLiteral("删除失败，请重试")
                                      : result.error);
        return;
    }

    refresh();
    emit taskDeleted(taskId);
}

void TaskListView::onPageChanged(int page)
{
    filter_.page = page;
    applyFilterToModel();
}

void TaskListView::onYearChanged()
{
    ListFilterControls::syncYearRange(yearCombo_, startDateEdit_, endDateEdit_);
    dateRangeEdited_ = false;
    filter_.year = yearCombo_->currentData().toInt();
    filter_.startDate = filter_.year > 0 ? startDateEdit_->date() : QDate();
    filter_.endDate = filter_.year > 0 ? endDateEdit_->date() : QDate();
    onFilterChanged();
}

bool TaskListView::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == tableView_->viewport() && event->type() == QEvent::Wheel) {
        auto *wheel = static_cast<QWheelEvent *>(event);
        QScrollBar *vertical = tableView_->verticalScrollBar();
        QScrollBar *horizontal = tableView_->horizontalScrollBar();
        if (vertical->maximum() == vertical->minimum()
            && horizontal->maximum() > horizontal->minimum()
            && !(wheel->modifiers() & (Qt::ControlModifier | Qt::AltModifier))) {
            const int delta = wheel->pixelDelta().y() ? wheel->pixelDelta().y()
                                                       : wheel->angleDelta().y();
            if (delta != 0) horizontal->setValue(horizontal->value() - delta);
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}
