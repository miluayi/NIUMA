/**
 * @file RecycleBinView.cpp
 * @brief 回收站页面实现（T8.1）
 *
 * 依赖：ui/RecycleBinView.h、application/recyclebin/RecycleBinUseCase、
 *       ui/widgets/PaginationWidget、ui/ColorPalette.h、
 *       utils/Logger
 * 文档：task.md T8.1、PRD 11.1、qt_guide 3.3
 */

#include "RecycleBinView.h"
#include "widgets/RoundedCardBorder.h"

#include <QCheckBox>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "ColorPalette.h"
#include "Radius.h"
#include "Spacing.h"
#include "application/recyclebin/RecycleBinUseCase.h"
#include "widgets/StyledMessageBox.h"
#include "ui/widgets/PaginationWidget.h"
#include "utils/Logger.h"

// 列定义
namespace {
enum Column {
    ColCheckbox = 0,   // 复选框
    ColTaskNo = 1,     // 编号
    ColTitle = 2,      // 任务名称
    ColDeleteTime = 3, // 删除时间
    ColRemaining = 4,  // 剩余天数
    ColStatus = 5,     // 原状态
    ColAction = 6,     // 操作
    ColCount = 7
};
}

// ============================================================================
// 构造
// ============================================================================

RecycleBinView::RecycleBinView(QWidget *parent)
    : QWidget(parent)
    , table_(nullptr)
    , selectAllCheckbox_(nullptr)
    , restoreSelectedBtn_(nullptr)
    , deleteSelectedBtn_(nullptr)
    , emptyBtn_(nullptr)
    , refreshBtn_(nullptr)
    , emptyLabel_(nullptr)
    , pagination_(nullptr)
{
    setupUI();
    refresh();
}

// ============================================================================
// UI 构建
// ============================================================================

void RecycleBinView::setupUI()
{
    setObjectName("recycleBinView");
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(Spacing::XXL, Spacing::XL, Spacing::XXL, Spacing::XL);
    mainLayout->setSpacing(Spacing::LG);

    // === page-header ===
    QVBoxLayout *titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(Spacing::XS);

    QLabel *titleLabel = new QLabel(QStringLiteral("回收站"), this);
    titleLabel->setObjectName("recycleBinTitle");
    QFont titleFont(QStringLiteral("MiSans"), 22, QFont::Normal);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color:#0F172A;");
    titleLayout->addWidget(titleLabel);

    QLabel *subtitleLabel = new QLabel(
        QStringLiteral("已删除任务保留 30 天，过期自动清理"), this);
    subtitleLabel->setObjectName("recycleBinSubtitle");
    QFont subtitleFont(QStringLiteral("MiSans"), 14);
    subtitleLabel->setFont(subtitleFont);
    subtitleLabel->setStyleSheet("color:#0F172A;");
    titleLayout->addWidget(subtitleLabel);

    mainLayout->addLayout(titleLayout);

    // === 工具栏 ===
    setupToolbar(mainLayout);

    // === 表格区 ===
    setupTable(mainLayout);
}

void RecycleBinView::setupToolbar(QVBoxLayout *mainLayout)
{
    QHBoxLayout *toolbarLayout = new QHBoxLayout();
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(Spacing::SM);

    // 全选复选框
    selectAllCheckbox_ = new QCheckBox(QStringLiteral("全选"), this);
    selectAllCheckbox_->setObjectName("recycleBinSelectAll");
    selectAllCheckbox_->setStyleSheet(
        "QCheckBox {"
        "  font-size:14px;"
        "  color:#0F172A;"
        "  spacing:6px;"
        "}"
    );
    connect(selectAllCheckbox_, &QCheckBox::toggled, this, &RecycleBinView::onSelectAllToggled);
    toolbarLayout->addWidget(selectAllCheckbox_);

    // "恢复所选" 按钮（primary）
    restoreSelectedBtn_ = new QPushButton(QStringLiteral("恢复所选"), this);
    restoreSelectedBtn_->setObjectName("recycleBinRestoreSelected");
    restoreSelectedBtn_->setCursor(Qt::PointingHandCursor);
    restoreSelectedBtn_->setStyleSheet(
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
    connect(restoreSelectedBtn_, &QPushButton::clicked, this, &RecycleBinView::onRestoreSelectedClicked);
    toolbarLayout->addWidget(restoreSelectedBtn_);

    // "彻底删除所选" 按钮（danger）
    deleteSelectedBtn_ = new QPushButton(QStringLiteral("彻底删除所选"), this);
    deleteSelectedBtn_->setObjectName("recycleBinDeleteSelected");
    deleteSelectedBtn_->setCursor(Qt::PointingHandCursor);
    deleteSelectedBtn_->setStyleSheet(
        "QPushButton {"
        "  background:#DC2626;"
        "  color:#FFFFFF;"
        "  border:1px solid #DC2626;"
        "  border-radius:8px;"
        "  padding:8px 16px;"
        "  font-size:14px;"
        "  font-weight:700;"
        "}"
        "QPushButton:hover {"
        "  background:#B91C1C;"
        "}"
    );
    connect(deleteSelectedBtn_, &QPushButton::clicked, this, &RecycleBinView::onDeleteSelectedClicked);
    toolbarLayout->addWidget(deleteSelectedBtn_);

    // "清空回收站" 按钮（secondary，红色文字）
    emptyBtn_ = new QPushButton(QStringLiteral("清空回收站"), this);
    emptyBtn_->setObjectName("recycleBinEmpty");
    emptyBtn_->setCursor(Qt::PointingHandCursor);
    emptyBtn_->setStyleSheet(
        "QPushButton {"
        "  background:#FFFFFF;"
        "  color:#DC2626;"
        "  border:1px solid #E2E8F0;"
        "  border-radius:8px;"
        "  padding:8px 16px;"
        "  font-size:14px;"
        "}"
        "QPushButton:hover {"
        "  background:#FEF2F2;"
        "  border-color:#FCA5A5;"
        "}"
    );
    connect(emptyBtn_, &QPushButton::clicked, this, &RecycleBinView::onEmptyRecycleBinClicked);
    toolbarLayout->addWidget(emptyBtn_);

    toolbarLayout->addStretch();

    // 刷新按钮（右侧）
    refreshBtn_ = new QPushButton(QStringLiteral("刷新"), this);
    refreshBtn_->setObjectName("recycleBinRefresh");
    refreshBtn_->setCursor(Qt::PointingHandCursor);
    refreshBtn_->setStyleSheet(
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
    connect(refreshBtn_, &QPushButton::clicked, this, &RecycleBinView::onRefreshClicked);
    toolbarLayout->addWidget(refreshBtn_);

    mainLayout->addLayout(toolbarLayout);
}

void RecycleBinView::setupTable(QVBoxLayout *mainLayout)
{
    // 表格卡片容器（QFrame#Card）
    QFrame *cardFrame = new QFrame(this);
    cardFrame->setObjectName("recycleBinCard");
    cardFrame->setStyleSheet(
        "QFrame#recycleBinCard {"
        "  background:#FFFFFF;"
        "  border:none;"
        "  border-radius:12px;"
        "}"
    );

    QVBoxLayout *cardLayout = new QVBoxLayout(cardFrame);
    cardLayout->setContentsMargins(1, 1, 1, 1);
    cardLayout->setSpacing(0);

    // QTableWidget
    table_ = new QTableWidget(cardFrame);
    table_->setObjectName("recycleBinTable");
    table_->setColumnCount(ColCount);
    table_->setHorizontalHeaderLabels({
        QStringLiteral(""),         // 复选框
        QStringLiteral("编号"),     // 编号
        QStringLiteral("任务名称"), // 任务名称
        QStringLiteral("删除时间"), // 删除时间
        QStringLiteral("剩余天数"), // 剩余天数
        QStringLiteral("原状态"),   // 原状态
        QStringLiteral("操作")      // 操作
    });
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::NoSelection);
    table_->setShowGrid(false);
    table_->setAlternatingRowColors(false);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setFocusPolicy(Qt::NoFocus);
    table_->setStyleSheet(
        "QTableWidget {"
        "  border:none;"
        "  font-size:14px;"
        "  background:#FFFFFF;"
        "  outline:none;"
        "}"
        "QTableWidget::item {"
        "  padding:8px 12px;"
        "  border-bottom:1px solid #F1F5F9;"
        "  background:#FFFFFF;"
        "}"
        "QHeaderView::section {"
        "  background:#F5F7FA;"
        "  color:#0F172A;"
        "  font-weight:700;"
        "  font-size:13px;"
        "  padding:10px 12px;"
        "  border:none;"
        "  border-bottom:none;"
        "}"
        "QHeaderView {"
        "  background:#F5F7FA;"
        "}"
    );

    // 表头设置
    QHeaderView *header = table_->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setStretchLastSection(true);
    header->setHighlightSections(false);
    header->setDefaultAlignment(Qt::AlignCenter);
    table_->horizontalHeaderItem(ColTitle)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    table_->verticalHeader()->setVisible(false);
    table_->verticalHeader()->setDefaultSectionSize(52);

    // 列宽
    table_->setColumnWidth(ColCheckbox, 50);
    table_->setColumnWidth(ColTaskNo, 140);
    table_->setColumnWidth(ColTitle, 280);
    table_->setColumnWidth(ColDeleteTime, 150);
    table_->setColumnWidth(ColRemaining, 100);
    table_->setColumnWidth(ColStatus, 100);
    // ColAction 自动拉伸（stretchLastSection）

    cardLayout->addWidget(table_, 1);

    // 空状态标签
    emptyLabel_ = new QLabel(
        QStringLiteral("回收站是空的。删除的任务会出现在这里"), cardFrame);
    emptyLabel_->setObjectName("recycleBinEmptyState");
    emptyLabel_->setAlignment(Qt::AlignCenter);
    emptyLabel_->setStyleSheet(
        "QLabel {"
        "  color:#0F172A;"
        "  font-size:15px;"
        "  padding:60px;"
        "  background:#FFFFFF;"
        "  border:none;"
        "}"
    );
    emptyLabel_->hide();
    cardLayout->addWidget(emptyLabel_, 1);

    // 分页控件
    pagination_ = new PaginationWidget(cardFrame);
    pagination_->setObjectName("recycleBinPagination");
    pagination_->setPageSize(kPageSize);
    connect(pagination_, &PaginationWidget::pageChanged, this, &RecycleBinView::onPageChanged);
    cardLayout->addWidget(pagination_);

    mainLayout->addWidget(cardFrame, 1);
    new RoundedCardBorder(cardFrame);
}

// ============================================================================
// 数据加载
// ============================================================================

void RecycleBinView::loadData()
{
    allTasks_ = RecycleBinUseCase().listDeleted();
}

void RecycleBinView::populateTable()
{
    // 断开全选信号，避免 populate 时触发
    selectAllCheckbox_->blockSignals(true);
    selectAllCheckbox_->setChecked(false);
    selectAllCheckbox_->blockSignals(false);

    // 清空表格
    table_->setRowCount(0);

    int total = allTasks_.size();
    int page = currentPage();
    int startIndex = (page - 1) * kPageSize;
    int endIndex = qMin(startIndex + kPageSize, total);
    int rowCount = endIndex - startIndex;

    table_->setRowCount(rowCount);

    QFont bodyFont = table_->font();
    bodyFont.setPixelSize(14);
    bodyFont.setWeight(QFont::Normal);
    QFont monoFont(QStringLiteral("MiSans"));
    monoFont.setPixelSize(14);
    monoFont.setStyleHint(QFont::Monospace);

    for (int i = 0; i < rowCount; ++i) {
        const Task &task = allTasks_[startIndex + i];

        // ---- 列 0：复选框（居中） ----
        QWidget *checkboxWidget = new QWidget();
        QCheckBox *checkbox = new QCheckBox();
        checkbox->setChecked(selectedTaskIds_.contains(task.id));
        QHBoxLayout *checkboxLayout = new QHBoxLayout(checkboxWidget);
        checkboxLayout->addWidget(checkbox);
        checkboxLayout->setAlignment(Qt::AlignCenter);
        checkboxLayout->setContentsMargins(0, 0, 0, 0);
        checkbox->setStyleSheet(
            "QCheckBox {"
            "  spacing:0;"
            "}"
        );
        // 复选框状态变化
        connect(checkbox, &QCheckBox::toggled, this, [this, taskId = task.id](bool checked) {
            if (checked) {
                selectedTaskIds_.insert(taskId);
            } else {
                selectedTaskIds_.remove(taskId);
            }
        });
        table_->setCellWidget(i, ColCheckbox, checkboxWidget);

        // ---- 列 1：编号 ----
        QTableWidgetItem *taskNoItem = new QTableWidgetItem(task.taskNo);
        taskNoItem->setFont(monoFont);
        taskNoItem->setForeground(QColor("#0F172A"));
        taskNoItem->setTextAlignment(Qt::AlignCenter);
        table_->setItem(i, ColTaskNo, taskNoItem);

        // ---- 列 2：任务名称 ----
        QTableWidgetItem *titleItem = new QTableWidgetItem(task.title);
        titleItem->setFont(bodyFont);
        titleItem->setForeground(QColor("#0F172A"));
        titleItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        table_->setItem(i, ColTitle, titleItem);

        // ---- 列 3：删除时间（yyyy-MM-dd HH:mm） ----
        QString deleteTimeStr = task.deleteTime.isValid()
                                    ? task.deleteTime.toString("yyyy-MM-dd HH:mm")
                                    : QStringLiteral("-");
        QTableWidgetItem *deleteTimeItem = new QTableWidgetItem(deleteTimeStr);
        deleteTimeItem->setFont(bodyFont);
        deleteTimeItem->setForeground(QColor("#0F172A"));
        deleteTimeItem->setTextAlignment(Qt::AlignCenter);
        table_->setItem(i, ColDeleteTime, deleteTimeItem);

        // ---- 列 4：剩余天数 ----
        int remaining = calcRemainingDays(task);
        QString remainingText = remainingDaysText(remaining);
        QColor remainingColor = remainingDaysColor(remaining);
        bool remainingBold = remainingDaysBold(remaining);

        QTableWidgetItem *remainingItem = new QTableWidgetItem(remainingText);
        QFont remainingFont = bodyFont;
        if (remainingBold) {
            remainingFont.setWeight(QFont::Normal);
        }
        remainingItem->setFont(remainingFont);
        remainingItem->setForeground(remainingColor);
        remainingItem->setTextAlignment(Qt::AlignCenter);
        table_->setItem(i, ColRemaining, remainingItem);

        // ---- 列 5：原状态（tag 样式） ----
        QString originalStatus = task.originalStatus.isEmpty()
                                     ? QStringLiteral("进行中")
                                     : task.originalStatus;
        QTableWidgetItem *statusItem = new QTableWidgetItem(originalStatus);
        statusItem->setFont(bodyFont);
        statusItem->setTextAlignment(Qt::AlignCenter);

        // 根据原状态设置 tag 颜色
        if (originalStatus == QStringLiteral("已完成")) {
            statusItem->setBackground(QColor("#F0FDF4"));
            statusItem->setForeground(QColor("#16A34A"));
        } else if (originalStatus == QStringLiteral("他人办理")) {
            statusItem->setBackground(QColor("#FEFCE8"));
            statusItem->setForeground(QColor("#A16207"));
        } else {
            // 进行中
            statusItem->setBackground(QColor("#EFF6FF"));
            statusItem->setForeground(QColor("#2463EB"));
        }
        table_->setItem(i, ColStatus, statusItem);

        // ---- 列 6：操作（恢复 + 彻底删除按钮） ----
        QWidget *actionWidget = new QWidget();
        QHBoxLayout *actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setContentsMargins(4, 4, 4, 4);
        actionLayout->setSpacing(8);

        QPushButton *restoreBtn = new QPushButton(QStringLiteral("恢复"));
        restoreBtn->setObjectName(QStringLiteral("recycleBinRestore_%1").arg(task.id));
        restoreBtn->setCursor(Qt::PointingHandCursor);
        restoreBtn->setStyleSheet(
            "QPushButton {"
            "  background:#2463EB;"
            "  color:#FFFFFF;"
            "  border:none;"
            "  border-radius:6px;"
            "  padding:4px 12px;"
            "  font-size:13px;"
            "}"
            "QPushButton:hover {"
            "  background:#1D4ED8;"
            "}"
        );
        connect(restoreBtn, &QPushButton::clicked, this, [this, taskId = task.id]() {
            restoreTask(taskId);
        });
        actionLayout->addStretch();
        actionLayout->addWidget(restoreBtn);

        QPushButton *permDeleteBtn = new QPushButton(QStringLiteral("彻底删除"));
        permDeleteBtn->setObjectName(QStringLiteral("recycleBinPermanentDelete_%1").arg(task.id));
        permDeleteBtn->setCursor(Qt::PointingHandCursor);
        permDeleteBtn->setStyleSheet(
            "QPushButton {"
            "  background:#FFFFFF;"
            "  color:#DC2626;"
            "  border:1px solid #FCA5A5;"
            "  border-radius:6px;"
            "  padding:4px 12px;"
            "  font-size:13px;"
            "}"
            "QPushButton:hover {"
            "  background:#FEF2F2;"
            "  border-color:#DC2626;"
            "}"
        );
        connect(permDeleteBtn, &QPushButton::clicked, this, [this, taskId = task.id]() {
            permanentDeleteTask(taskId);
        });
        actionLayout->addWidget(permDeleteBtn);

        actionLayout->addStretch();
        table_->setCellWidget(i, ColAction, actionWidget);
    }

    // 更新分页控件
    pagination_->setTotal(total);
    pagination_->setPage(page);
    pagination_->updateDisplay();

    updateEmptyState(total);
}

void RecycleBinView::updateEmptyState(int total)
{
    if (total == 0) {
        table_->hide();
        emptyLabel_->show();
        pagination_->hide();
        selectAllCheckbox_->setEnabled(false);
        restoreSelectedBtn_->setEnabled(false);
        deleteSelectedBtn_->setEnabled(false);
        emptyBtn_->setEnabled(false);
    } else {
        table_->show();
        emptyLabel_->hide();
        pagination_->show();
        selectAllCheckbox_->setEnabled(true);
        restoreSelectedBtn_->setEnabled(true);
        deleteSelectedBtn_->setEnabled(true);
        emptyBtn_->setEnabled(true);
    }
}

void RecycleBinView::refresh()
{
    loadData();
    populateTable();
}

int RecycleBinView::currentPage() const
{
    return pagination_ ? pagination_->currentPage() : 1;
}

void RecycleBinView::clearSelection()
{
    selectedTaskIds_.clear();
}

QList<int> RecycleBinView::getSelectedTaskIds() const
{
    return selectedTaskIds_.values();
}

// ============================================================================
// 槽：全选 / 取消全选
// ============================================================================

void RecycleBinView::onSelectAllToggled(bool checked)
{
    int page = currentPage();
    int startIndex = (page - 1) * kPageSize;
    int endIndex = qMin(startIndex + kPageSize, allTasks_.size());

    for (int i = startIndex; i < endIndex; ++i) {
        int taskId = allTasks_[i].id;
        if (checked) {
            selectedTaskIds_.insert(taskId);
        } else {
            selectedTaskIds_.remove(taskId);
        }
    }

    // 更新当前页的复选框状态
    for (int row = 0; row < table_->rowCount(); ++row) {
        QWidget *w = table_->cellWidget(row, ColCheckbox);
        if (w) {
            QCheckBox *cb = w->findChild<QCheckBox *>();
            if (cb) {
                cb->blockSignals(true);
                cb->setChecked(checked);
                cb->blockSignals(false);
            }
        }
    }
}

// ============================================================================
// 槽：恢复所选
// ============================================================================

void RecycleBinView::onRestoreSelectedClicked()
{
    QList<int> ids = getSelectedTaskIds();
    if (ids.isEmpty()) {
        return;
    }

    const int count = RecycleBinUseCase().restoreTasks(ids);

    selectedTaskIds_.clear();
    refresh();

    if (count > 0) {
        emit tasksRestored(count);
    }
}

// ============================================================================
// 槽：彻底删除所选
// ============================================================================

void RecycleBinView::onDeleteSelectedClicked()
{
    QList<int> ids = getSelectedTaskIds();
    if (ids.isEmpty()) {
        return;
    }

    // 二次确认
    int reply = StyledMessageBox::question(
        this,
        QStringLiteral("确认彻底删除"),
        QStringLiteral("确定要彻底删除选中的 %1 个任务吗？\n此操作不可撤销。").arg(ids.size()));

    if (reply != StyledMessageBox::Yes) {
        return;
    }

    const int count = RecycleBinUseCase().permanentlyDeleteTasks(ids);

    selectedTaskIds_.clear();
    refresh();

    if (count > 0) {
        emit tasksDeleted(count);
    }
}

// ============================================================================
// 槽：清空回收站
// ============================================================================

void RecycleBinView::onEmptyRecycleBinClicked()
{
    if (allTasks_.isEmpty()) {
        return;
    }

    int reply = StyledMessageBox::question(
        this,
        QStringLiteral("确认清空回收站"),
        QStringLiteral("确定要清空回收站吗？\n所有 %1 个已删除任务将被永久删除，此操作不可撤销。").arg(allTasks_.size()));

    if (reply != StyledMessageBox::Yes) {
        return;
    }

    QList<int> taskIds;
    taskIds.reserve(allTasks_.size());
    for (const Task &task : allTasks_) {
        taskIds.append(task.id);
    }
    const int count = RecycleBinUseCase().permanentlyDeleteTasks(taskIds);

    selectedTaskIds_.clear();
    refresh();

    if (count > 0) {
        emit tasksDeleted(count);
    }
}

// ============================================================================
// 槽：刷新
// ============================================================================

void RecycleBinView::onRefreshClicked()
{
    refresh();
}

// ============================================================================
// 槽：分页
// ============================================================================

void RecycleBinView::onPageChanged(int /*page*/)
{
    // 分页变化时取消全选状态
    selectAllCheckbox_->blockSignals(true);
    selectAllCheckbox_->setChecked(false);
    selectAllCheckbox_->blockSignals(false);
    populateTable();
}

// ============================================================================
// 单行操作
// ============================================================================

void RecycleBinView::restoreTask(int taskId)
{
    TaskResult result = RecycleBinUseCase().restoreTask(taskId);
    if (result.ok) {
        selectedTaskIds_.remove(taskId);
        refresh();
        emit tasksRestored(1);
    } else {
        StyledMessageBox::warning(this, QStringLiteral("恢复失败"), result.error);
    }
}

void RecycleBinView::permanentDeleteTask(int taskId)
{
    int reply = StyledMessageBox::question(
        this,
        QStringLiteral("确认彻底删除"),
        QStringLiteral("确定要彻底删除此任务吗？\n此操作不可撤销。"));

    if (reply != StyledMessageBox::Yes) {
        return;
    }

    TaskResult result = RecycleBinUseCase().permanentlyDeleteTask(taskId);
    if (result.ok) {
        selectedTaskIds_.remove(taskId);
        refresh();
        emit tasksDeleted(1);
    } else {
        StyledMessageBox::warning(this, QStringLiteral("删除失败"), result.error);
    }
}

// ============================================================================
// 辅助：剩余天数计算
// ============================================================================

int RecycleBinView::calcRemainingDays(const Task &task)
{
    if (!task.deleteTime.isValid()) {
        return kRetentionDays;
    }
    int daysSinceDelete = task.deleteTime.date().daysTo(QDate::currentDate());
    return kRetentionDays - daysSinceDelete;
}

QString RecycleBinView::remainingDaysText(int remaining)
{
    if (remaining <= 0) {
        return QStringLiteral("今日清理");
    }
    return QStringLiteral("%1 天").arg(remaining);
}

QColor RecycleBinView::remainingDaysColor(int remaining)
{
    if (remaining <= 0) {
        return QColor("#DC2626");  // 红色
    } else if (remaining <= 2) {
        return QColor("#DC2626");  // 红色
    } else if (remaining <= 7) {
        return QColor("#EAB308");  // 橙色
    } else {
        return QColor("#94A3B8");  // 灰色
    }
}

bool RecycleBinView::remainingDaysBold(int remaining)
{
    return remaining <= 2;  // 1-2 天或 0/负数：加粗
}
