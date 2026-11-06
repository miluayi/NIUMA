/**
 * @file PeriodicTaskView.cpp
 * @brief 周期任务页面实现（Phase 7）
 *
 * 实现：setupUI 搭建页面（page-header + 表格卡片 + 空状态） /
 *       loadData 通过 PeriodicTaskUseCase 加载全部模板 /
 *       fillTable 填充 QTableWidget，操作列内嵌编辑/删除按钮
 *
 * 依赖：ui/PeriodicTaskView.h、ui/ColorPalette.h、ui/Spacing.h、ui/Radius.h、
 *       application/periodic/PeriodicTaskUseCase、models/PeriodicTask、utils/Logger
 * 文档：Phase 7、PRD 9.3、qt_implementation_guide.md 3.3、coding.md 第七章
 */

#include "PeriodicTaskView.h"
#include "widgets/RoundedCardBorder.h"
#include "widgets/CategoryBadge.h"

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
#include <QVariant>

#include "ColorPalette.h"
#include "Radius.h"
#include "Spacing.h"
#include "application/periodic/PeriodicTaskUseCase.h"
#include "utils/Logger.h"
#include "widgets/StyledMessageBox.h"

// ============================================================================
// 构造
// ============================================================================

PeriodicTaskView::PeriodicTaskView(QWidget *parent)
    : QWidget(parent)
    , table_(nullptr)
    , emptyLabel_(nullptr)
{
    setupUI();
    loadData();
    updateEmptyState();
}

// ============================================================================
// UI 构建
// ============================================================================

void PeriodicTaskView::setupUI()
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

    QLabel *titleLabel = new QLabel(QStringLiteral("周期任务"), this);
    QFont titleFont(QStringLiteral("Segoe UI"), 22, QFont::Normal);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color:#0F172A;");
    titleLayout->addWidget(titleLabel);

    QLabel *subtitleLabel = new QLabel(
        QStringLiteral("管理周期性任务模板，自动提醒创建"), this);
    QFont subtitleFont(QStringLiteral("Segoe UI"), 14);
    subtitleLabel->setFont(subtitleFont);
    subtitleLabel->setStyleSheet("color:#0F172A;");
    titleLayout->addWidget(subtitleLabel);

    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();

    // "＋ 新建模板" 按钮
    QPushButton *createBtn = new QPushButton(QStringLiteral("＋ 新建模板"), this);
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
        "}");
    connect(createBtn, &QPushButton::clicked, this, &PeriodicTaskView::onCreateClicked);
    headerLayout->addWidget(createBtn);

    mainLayout->addLayout(headerLayout);

    // === 表格卡片 ===
    QFrame *cardFrame = new QFrame(this);
    cardFrame->setObjectName("periodicTaskListCard");
    cardFrame->setStyleSheet(
        "QFrame#periodicTaskListCard {"
        "  background:#FFFFFF;"
        "  border:none;"
        "  border-radius:12px;"
        "}");

    QVBoxLayout *cardLayout = new QVBoxLayout(cardFrame);
    cardLayout->setContentsMargins(1, 1, 1, 1);
    cardLayout->setSpacing(0);

    // QTableWidget
    table_ = new QTableWidget(cardFrame);
    table_->setObjectName("periodicTaskTable");
    table_->setItemDelegateForColumn(ColCategory,new CategoryBadgeDelegate(table_));
    table_->setColumnCount(ColumnCount);
    table_->setHorizontalHeaderLabels(QStringList()
                                      << QStringLiteral("名称")
                                      << QStringLiteral("分类")
                                      << QStringLiteral("周期")
                                      << QStringLiteral("来源")
                                      << QStringLiteral("提醒时间")
                                      << QStringLiteral("启用")
                                      << QStringLiteral("操作"));
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setShowGrid(false);
    table_->setAlternatingRowColors(false);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);  // 只读
    table_->setFocusPolicy(Qt::NoFocus);
    table_->setStyleSheet(
        "QTableWidget {"
        "  border:none;"
        "  font-size:14px;"
        "  selection-background-color:#EAF1FF;"
        "  selection-color:#172033;"
        "  background:#FFFFFF;"
        "  outline:none;"
        "}"
        "QTableWidget::item {"
        "  padding:7px;"
        "  border-bottom:1px solid #DCE3EF;"
        "  background:#FFFFFF;"
        "}"
        "QTableWidget::item:selected {"
        "  background:#EAF1FF;"
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
        "}");

    // 表头设置
    QHeaderView *header = table_->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setStretchLastSection(false);
    header->setSectionResizeMode(ColName, QHeaderView::Stretch);
    header->setSectionResizeMode(ColActions, QHeaderView::ResizeToContents);
    header->setHighlightSections(false);
    header->setDefaultAlignment(Qt::AlignCenter);

    table_->verticalHeader()->setVisible(false);
    table_->verticalHeader()->setDefaultSectionSize(52);

    setColumnWidths();

    cardLayout->addWidget(table_);

    // 空状态标签（叠加在卡片上，无数据时显示）
    emptyLabel_ = new QLabel(
        QStringLiteral("暂无周期任务模板。点击右上角'新建'按钮创建第一个模板"),
        cardFrame);
    emptyLabel_->setObjectName("periodicTaskEmptyState");
    emptyLabel_->setAlignment(Qt::AlignCenter);
    emptyLabel_->setStyleSheet(
        "color:#0F172A; font-size:14px; padding:60px;");
    cardLayout->addWidget(emptyLabel_);

    mainLayout->addWidget(cardFrame, 1);
    new RoundedCardBorder(cardFrame);
}

void PeriodicTaskView::setColumnWidths()
{
    if (!table_) {
        return;
    }
    table_->setColumnWidth(ColName, 220);
    table_->setColumnWidth(ColCategory, 70);
    table_->setColumnWidth(ColPeriod, 110);
    table_->setColumnWidth(ColSource, 140);
    table_->setColumnWidth(ColReminderTime, 100);
    table_->setColumnWidth(ColEnabled, 80);
    // 操作列自适应
}

// ============================================================================
// 周期类型显示
// ============================================================================

QString PeriodicTaskView::periodDisplay(const PeriodicTask &t)
{
    QString unit;
    QString suffix;
    if (t.periodType == QStringLiteral("daily")) {
        unit = QStringLiteral("每日");
        suffix = QStringLiteral("天");
    } else if (t.periodType == QStringLiteral("weekly")) {
        unit = QStringLiteral("每周");
        suffix = QStringLiteral("周");
    } else if (t.periodType == QStringLiteral("monthly")) {
        unit = QStringLiteral("每月");
        suffix = QStringLiteral("月");
    } else if (t.periodType == QStringLiteral("yearly")) {
        unit = QStringLiteral("每年");
        suffix = QStringLiteral("年");
    } else {
        unit = QStringLiteral("每日");
        suffix = QStringLiteral("天");
    }

    // periodValue > 1 时显示如 "每 2 周"，否则 "每周"
    if (t.periodValue > 1) {
        return QStringLiteral("每 %1 %2").arg(t.periodValue).arg(suffix);
    }
    return unit;
}

// ============================================================================
// 数据加载与刷新
// ============================================================================

void PeriodicTaskView::refresh()
{
    loadData();
    fillTable();
    updateEmptyState();
}

void PeriodicTaskView::loadData()
{
    templates_ = PeriodicTaskUseCase().listAll();
}

void PeriodicTaskView::fillTable()
{
    if (!table_) {
        return;
    }

    // 清空旧数据
    table_->setRowCount(0);
    table_->setRowCount(static_cast<int>(templates_.size()));

    for (int i = 0; i < templates_.size(); ++i) {
        const PeriodicTask &t = templates_.at(i);

        // 名称
        QTableWidgetItem *nameItem = new QTableWidgetItem(t.name);
        nameItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        nameItem->setData(Qt::UserRole, t.id);
        table_->setItem(i, ColName, nameItem);

        // 分类
        QTableWidgetItem *categoryItem = new QTableWidgetItem(t.category);
        categoryItem->setTextAlignment(Qt::AlignCenter);
        table_->setItem(i, ColCategory, categoryItem);

        // 周期
        QTableWidgetItem *periodItem = new QTableWidgetItem(periodDisplay(t));
        periodItem->setTextAlignment(Qt::AlignCenter);
        table_->setItem(i, ColPeriod, periodItem);

        // 来源
        QTableWidgetItem *sourceItem = new QTableWidgetItem(t.source);
        sourceItem->setTextAlignment(Qt::AlignCenter);
        table_->setItem(i, ColSource, sourceItem);

        // 提醒时间
        QTableWidgetItem *reminderItem = new QTableWidgetItem(
            t.reminderTime.isEmpty() ? QStringLiteral("—") : t.reminderTime);
        reminderItem->setTextAlignment(Qt::AlignCenter);
        table_->setItem(i, ColReminderTime, reminderItem);

        // 启用（复选框 CellWidget）
        QCheckBox *enabledCheck = new QCheckBox();
        enabledCheck->setChecked(t.isEnabled);
        enabledCheck->setProperty("templateId", t.id);
        enabledCheck->setStyleSheet(
            "QCheckBox { background:transparent; }"
            "QCheckBox::indicator { width:16px; height:16px; border-radius:3px; "
            "  border:1px solid #CBD5E1; background:#FFFFFF; }"
            "QCheckBox::indicator:checked { background:#2463EB; border-color:#2463EB; }");
        // 居中
        QWidget *checkContainer = new QWidget();
        QHBoxLayout *checkLayout = new QHBoxLayout(checkContainer);
        checkLayout->setContentsMargins(0, 0, 0, 0);
        checkLayout->setAlignment(Qt::AlignCenter);
        checkLayout->addWidget(enabledCheck);
        table_->setCellWidget(i, ColEnabled, checkContainer);
        connect(enabledCheck, &QCheckBox::toggled, this, &PeriodicTaskView::onEnabledToggled);

        // 操作列：编辑 + 删除按钮
        QWidget *actionsWidget = new QWidget();
        QHBoxLayout *actionsLayout = new QHBoxLayout(actionsWidget);
        actionsLayout->setContentsMargins(7, 0, 21, 0);
        actionsLayout->setSpacing(Spacing::SM);

        QPushButton *editBtn = new QPushButton(QStringLiteral("编辑"));
        editBtn->setProperty("templateId", t.id);
        editBtn->setCursor(Qt::PointingHandCursor);
        editBtn->setStyleSheet(
            "QPushButton {"
            "  background:#FFFFFF;"
            "  color:#2463EB;"
            "  border:1px solid #2463EB;"
            "  border-radius:4px;"
            "  padding:4px 10px;"
            "  font-size:12px;"
            "}"
            "QPushButton:hover {"
            "  background:#EFF6FF;"
            "}");

        QPushButton *deleteBtn = new QPushButton(QStringLiteral("删除"));
        deleteBtn->setProperty("templateId", t.id);
        deleteBtn->setCursor(Qt::PointingHandCursor);
        deleteBtn->setStyleSheet(
            "QPushButton {"
            "  background:#FFFFFF;"
            "  color:#DC2626;"
            "  border:1px solid #DC2626;"
            "  border-radius:4px;"
            "  padding:4px 10px;"
            "  font-size:12px;"
            "}"
            "QPushButton:hover {"
            "  background:#FEF2F2;"
            "}");

        actionsLayout->addStretch();
        actionsLayout->addWidget(editBtn);
        actionsLayout->addWidget(deleteBtn);
        actionsLayout->addStretch();
        actionsWidget->setMinimumWidth(editBtn->sizeHint().width()
                                       + deleteBtn->sizeHint().width()
                                       + actionsLayout->spacing() + 28);

        table_->setCellWidget(i, ColActions, actionsWidget);

        connect(editBtn, &QPushButton::clicked, this, &PeriodicTaskView::onEditClicked);
        connect(deleteBtn, &QPushButton::clicked, this, &PeriodicTaskView::onDeleteClicked);
    }
}

void PeriodicTaskView::updateEmptyState()
{
    if (!table_ || !emptyLabel_) {
        return;
    }
    bool empty = templates_.isEmpty();
    table_->setVisible(!empty);
    emptyLabel_->setVisible(empty);
}

// ============================================================================
// 槽：交互
// ============================================================================

void PeriodicTaskView::onCreateClicked()
{
    emit createPeriodicTaskClicked();
}

void PeriodicTaskView::onEditClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) {
        return;
    }
    int id = btn->property("templateId").toInt();
    if (id > 0) {
        emit editPeriodicTask(id);
    }
}

void PeriodicTaskView::onDeleteClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) {
        return;
    }
    int id = btn->property("templateId").toInt();
    if (id <= 0) {
        return;
    }

    // 二次确认
    auto ret = StyledMessageBox::question(
        this, QStringLiteral("确认删除"),
        QStringLiteral("确认删除此周期任务模板？删除后无法恢复。"));
    if (ret != StyledMessageBox::Yes) {
        return;
    }

    TaskResult result = PeriodicTaskUseCase().remove(id);
    if (!result.ok) {
        StyledMessageBox::warning(this, QStringLiteral("删除失败"), result.error);
        return;
    }

    Logger::instance().info(
        QStringLiteral("PeriodicTaskView 已删除模板: id=%1").arg(id));

    refresh();
}

void PeriodicTaskView::onEnabledToggled(bool checked)
{
    QCheckBox *box = qobject_cast<QCheckBox*>(sender());
    if (!box) {
        return;
    }
    int id = box->property("templateId").toInt();
    if (id <= 0) {
        return;
    }
    PeriodicTaskUseCase().setEnabled(id, checked);
    Logger::instance().info(
        QStringLiteral("PeriodicTaskView 切换启用状态: id=%1 enabled=%2").arg(id).arg(checked));
}

// ============================================================================
// 工具
// ============================================================================

int PeriodicTaskView::templateIdAtRow(int row) const
{
    if (!table_ || row < 0 || row >= table_->rowCount()) {
        return 0;
    }
    QTableWidgetItem *item = table_->item(row, ColName);
    if (!item) {
        return 0;
    }
    return item->data(Qt::UserRole).toInt();
}
