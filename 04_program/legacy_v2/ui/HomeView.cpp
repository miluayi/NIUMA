/**
 * @file HomeView.cpp
 * @brief 首页实现
 *
 * 依赖：ui/HomeView.h、application/home/HomeDashboardUseCase、models/Task、ui/ColorPalette、ui/Spacing、ui/Radius、
 *       utils/DateUtil
 * 文档：task.md T5（今日专注）、PRD 5.1（首页布局）、qt_implementation_guide.md 3.2
 */
#include "HomeView.h"

#include <QDate>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPushButton>
#include <QToolTip>
#include <QVBoxLayout>

#include "application/home/HomeCommandUseCase.h"
#include "models/Task.h"
#include "ui/Radius.h"
#include "ui/Spacing.h"
#include "ui/widgets/PaginationWidget.h"
#include "ui/widgets/TaskItemDelegate.h"

namespace {
class HomeCategoryBadge final : public QFrame {
public:
    explicit HomeCategoryBadge(const QString &category)
        : category_(category)
    {
        setObjectName(QStringLiteral("appUiCategoryTag"));
        setProperty("categoryText", category);
        setFixedSize(TaskItemDelegate::categoryBadgeWidth(category), 24);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        TaskItemDelegate::paintCategoryBadge(&painter, rect(), category_);
    }

private:
    QString category_;
};
}

// ============================================================================
// 构造
// ============================================================================

HomeView::HomeView(QWidget *parent)
    : QWidget(parent)
    , subtitleLabel_(nullptr)
    , focusArea_(nullptr)
    , focusTitle_(nullptr)
    , focusListLayout_(nullptr)
    , focusEmptyLabel_(nullptr)
    , inProgressCountLabel_(nullptr)
    , inProgressListLayout_(nullptr)
    , inProgressPagination_(nullptr)
    , inProgressInfoLabel_(nullptr)
    , inProgressCurrentPage_(1)
    , periodicListLayout_(nullptr)
    , periodicEmptyLabel_(nullptr)
    , statTotalLabel_(nullptr)
    , statCompletedLabel_(nullptr)
    , statRateLabel_(nullptr)
    , statDiffLabel_(nullptr)
{
    setupUI();
}

// ============================================================================
// UI 构建
// ============================================================================

void HomeView::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(Spacing::XXL, Spacing::XL, Spacing::XXL, Spacing::XL);
    mainLayout->setSpacing(Spacing::LG);

    // === 页头 ===
    mainLayout->addWidget(buildPageHeader());

    // === 今日专注（全宽，渐变背景） ===
    focusArea_ = buildFocusArea();
    mainLayout->addWidget(focusArea_);

    // === 进行中任务（全宽，白卡片） ===
    mainLayout->addWidget(buildInProgressArea());

    // === 底部：周期任务提醒 + 本月概览（左右各半） ===
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(Spacing::LG);
    bottomLayout->addWidget(buildPeriodicArea(), 1);
    bottomLayout->addWidget(buildMonthlyStatsArea(), 1);
    mainLayout->addLayout(bottomLayout);

    mainLayout->addStretch();
}

QFrame* HomeView::buildPageHeader()
{
    QFrame *header = new QFrame(this);
    header->setObjectName("PageHeader");

    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(Spacing::MD);

    QVBoxLayout *titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(Spacing::XS);

    QLabel *titleLabel = new QLabel(QStringLiteral("\u9996\u9875"), header);  // 首页
    QFont titleFont(QStringLiteral("MiSans"), 22, QFont::Normal);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color:#0F172A;");
    titleLayout->addWidget(titleLabel);

    subtitleLabel_ = new QLabel(header);
    QFont subtitleFont(QStringLiteral("MiSans"), 14);
    subtitleLabel_->setFont(subtitleFont);
    subtitleLabel_->setStyleSheet("color:#0F172A;");
    titleLayout->addWidget(subtitleLabel_);
    updateDateSubtitle();

    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();

    // "＋ 新建任务" 按钮（全角加号，primary）
    QPushButton *createBtn = new QPushButton(
        QStringLiteral("\uFF0B \u65B0\u5EFA\u4EFB\u52A1"), header);  // ＋ 新建任务
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
    connect(createBtn, &QPushButton::clicked, this, &HomeView::onCreateTaskClicked);
    headerLayout->addWidget(createBtn);

    return header;
}

QFrame* HomeView::buildFocusArea()
{
    QFrame *area = new QFrame(this);
    area->setObjectName("FocusArea");
    area->setFixedHeight(230);
    area->setStyleSheet(
        "QFrame#FocusArea {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "                              stop:0 #EEF4FF, stop:1 #F5F8FF);"
        "  border:1px solid #D6E4FF;"
        "  border-radius:12px;"
        "}"
    );

    QVBoxLayout *layout = new QVBoxLayout(area);
    layout->setContentsMargins(Spacing::XL, Spacing::MD, Spacing::XL, Spacing::MD);
    layout->setSpacing(Spacing::MD);

    // 标题行
    focusTitle_ = new QLabel(QStringLiteral("\u4ECA\u65E5\u4E13\u6CE8  0/3"), area);
    focusTitle_->setObjectName("FocusTitle");
    QFont titleFont(QStringLiteral("MiSans"), 16, QFont::Normal);
    focusTitle_->setFont(titleFont);
    focusTitle_->setStyleSheet("color:#2463EB;");
    focusTitle_->setFixedHeight(26);
    layout->addWidget(focusTitle_);

    // 任务列表容器
    focusListLayout_ = new QVBoxLayout();
    focusListLayout_->setContentsMargins(0, 0, 0, 0);
    focusListLayout_->setSpacing(6);
    focusListLayout_->setAlignment(Qt::AlignTop);
    layout->addLayout(focusListLayout_);

    // 空状态提示
    focusEmptyLabel_ = new QLabel(
        QStringLiteral("\u4ECE\u4E0B\u65B9\u8FDB\u884C\u4E2D\u4EFB\u52A1\u6311\u9009\u4ECA\u5929\u8981\u4E13\u6CE8\u7684\u4E8B\u9879"),
        area);  // 从下方进行中任务挑选今天要专注的事项
    focusEmptyLabel_->setAlignment(Qt::AlignCenter);
    focusEmptyLabel_->setStyleSheet("color:#0F172A; font-size:15px; padding:20px;");
    focusEmptyLabel_->setVisible(false);
    layout->addWidget(focusEmptyLabel_);

    return area;
}

QFrame* HomeView::buildInProgressArea()
{
    QFrame *card = new QFrame(this);
    card->setObjectName("InProgressCard");
    card->setStyleSheet(
        "QFrame#InProgressCard {"
        "  background:#FFFFFF;"
        "  border:none;"
        "  border-radius:12px;"
        "}"
    );
    // 定稿：10 行任务（每行 48px）+ 列头/分页/footer，避免翻页时高度跳动。
    card->setFixedHeight(670);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(Spacing::XL, Spacing::MD, Spacing::XL, Spacing::MD);
    layout->setSpacing(0);

    // 卡片头
    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, Spacing::SM);
    headerLayout->setSpacing(Spacing::SM);

    QLabel *cardTitle = new QLabel(
        QStringLiteral("\u8FDB\u884C\u4E2D\u4EFB\u52A1"), card);  // 进行中任务
    QFont titleFont(QStringLiteral("MiSans"), 18, QFont::Normal);
    cardTitle->setFont(titleFont);
    cardTitle->setStyleSheet("color:#0F172A;");
    headerLayout->addWidget(cardTitle);

    inProgressCountLabel_ = new QLabel(card);
    inProgressCountLabel_->setStyleSheet("color:#0F172A; font-size:14px;");
    headerLayout->addWidget(inProgressCountLabel_);
    headerLayout->addStretch();
    layout->addLayout(headerLayout);

    QFrame *columnHeader = new QFrame(card);
    columnHeader->setObjectName("appUiColumnHeader");
    columnHeader->setFixedHeight(30);
    columnHeader->setStyleSheet("background:#F5F8FC; border-radius:6px;");
    QHBoxLayout *columnLayout = new QHBoxLayout(columnHeader);
    columnLayout->setContentsMargins(0, 0, 0, 0);
    columnLayout->setSpacing(0);
    auto addColumn = [columnHeader, columnLayout](const QString &text, int width, bool expanding) {
        QLabel *label = new QLabel(text, columnHeader);
        if (text == QStringLiteral("分类")) {
            label->setObjectName(QStringLiteral("HomeInProgressCategoryColumn"));
        }
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("color:#475569; font-size:13px; font-weight:700; border-right:1px solid #DCE3EF;");
        if (expanding) {
            columnLayout->addWidget(label, 1);
        } else {
            label->setFixedWidth(width);
            columnLayout->addWidget(label);
        }
    };
    addColumn(QStringLiteral("分类"), 48, false);
    addColumn(QStringLiteral("任务名称"), 0, true);
    addColumn(QStringLiteral("事项标签"), 120, false);
    addColumn(QStringLiteral("来源"), 104, false);
    addColumn(QStringLiteral("截止日期"), 102, false);
    layout->addWidget(columnHeader);

    // 任务列表容器（固定高度：10 行 × 48px = 480px，无论实际行数多少）
    QWidget *listContainer = new QWidget(card);
    listContainer->setObjectName("InProgressList");
    listContainer->setFixedHeight(480);
    listContainer->setStyleSheet("background:#FFFFFF;");
    inProgressListLayout_ = new QVBoxLayout(listContainer);
    inProgressListLayout_->setContentsMargins(0, 0, 0, 0);
    inProgressListLayout_->setSpacing(0);
    layout->addWidget(listContainer);

    // 分页控件（每页 10 行）— 隐藏内置信息标签，信息移到底部 footer 行
    inProgressPagination_ = new PaginationWidget(card);
    inProgressPagination_->setPageSize(10);
    inProgressPagination_->setInfoVisible(false);
    connect(inProgressPagination_, &PaginationWidget::pageChanged, this, &HomeView::onPageChanged);
    layout->addWidget(inProgressPagination_);

    // 底部分隔线 + 查看完整列表
    QFrame *separator = new QFrame(card);
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet("color:#F1F5F9; background:#F1F5F9; max-height:1px; border:none;");
    layout->addWidget(separator);

    QHBoxLayout *footerLayout = new QHBoxLayout();
    footerLayout->setContentsMargins(12, Spacing::SM, 12, 0);
    footerLayout->setSpacing(0);

    // 左侧：分页信息"显示 X-Y 条，共 Z 条"
    inProgressInfoLabel_ = new QLabel(card);
    inProgressInfoLabel_->setStyleSheet("color:#0F172A; font-size:14px; border:none; background:transparent;");
    footerLayout->addWidget(inProgressInfoLabel_);
    footerLayout->addStretch();

    QPushButton *viewAllBtn = new QPushButton(
        QStringLiteral("\u2192 \u67E5\u770B\u5B8C\u6574\u5217\u8868"), card);  // → 查看完整列表
    viewAllBtn->setCursor(Qt::PointingHandCursor);
    viewAllBtn->setStyleSheet(
        "QPushButton {"
        "  background:transparent;"
        "  color:#2463EB;"
        "  border:none;"
        "  font-size:14px;"
        "  padding:4px 8px;"
        "}"
        "QPushButton:hover {"
        "  color:#1D4ED8;"
        "  text-decoration:underline;"
        "}"
    );
    connect(viewAllBtn, &QPushButton::clicked, this, &HomeView::onViewAllTasksClicked);
    footerLayout->addWidget(viewAllBtn);
    layout->addLayout(footerLayout);

    return card;
}

QFrame* HomeView::buildPeriodicArea()
{
    QFrame *card = new QFrame(this);
    card->setObjectName("Card");
    card->setStyleSheet(
        "QFrame#Card {"
        "  background:#FFFFFF;"
        "  border:1px solid #DCE3EF;"
        "  border-radius:12px;"
        "}"
    );

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(Spacing::XL, Spacing::MD, Spacing::XL, Spacing::MD);
    layout->setSpacing(Spacing::SM);

    QLabel *cardTitle = new QLabel(
        QStringLiteral("\u5468\u671F\u4EFB\u52A1\u63D0\u9192"), card);  // 周期任务提醒
    QFont titleFont(QStringLiteral("MiSans"), 18, QFont::Normal);
    cardTitle->setFont(titleFont);
    cardTitle->setStyleSheet("color:#0F172A;");
    layout->addWidget(cardTitle);

    // 提醒列表容器
    periodicListLayout_ = new QVBoxLayout();
    periodicListLayout_->setContentsMargins(0, 0, 0, 0);
    periodicListLayout_->setSpacing(6);
    layout->addLayout(periodicListLayout_);

    // 空状态提示
    periodicEmptyLabel_ = new QLabel(
        QStringLiteral("\u6682\u65E0\u5468\u671F\u4EFB\u52A1\u63D0\u9192"), card);  // 暂无周期任务提醒
    periodicEmptyLabel_->setAlignment(Qt::AlignCenter);
    periodicEmptyLabel_->setStyleSheet("color:#0F172A; font-size:15px; padding:24px;");
    periodicEmptyLabel_->setVisible(false);
    layout->addWidget(periodicEmptyLabel_, 1);

    return card;
}

QFrame* HomeView::buildMonthlyStatsArea()
{
    QFrame *card = new QFrame(this);
    card->setObjectName("Card");
    card->setStyleSheet(
        "QFrame#Card {"
        "  background:#FFFFFF;"
        "  border:1px solid #DCE3EF;"
        "  border-radius:12px;"
        "}"
    );

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(Spacing::XL, Spacing::MD, Spacing::XL, Spacing::MD);
    layout->setSpacing(Spacing::SM);

    QLabel *cardTitle = new QLabel(
        QStringLiteral("\u672C\u6708\u6982\u89C8"), card);  // 本月概览
    QFont titleFont(QStringLiteral("MiSans"), 18, QFont::Normal);
    cardTitle->setFont(titleFont);
    cardTitle->setStyleSheet("color:#0F172A;");
    layout->addWidget(cardTitle);

    // 4 列统计网格
    QGridLayout *statsGrid = new QGridLayout();
    statsGrid->setContentsMargins(0, Spacing::SM, 0, 0);
    statsGrid->setHorizontalSpacing(Spacing::SM);
    statsGrid->setVerticalSpacing(Spacing::XS);

    auto makeStatCell = [this, card](QLabel *&valueLabel, const QString &labelText) {
        QFrame *cell = new QFrame(card);
        cell->setObjectName("appUiStatCell");
        cell->setStyleSheet("background:#F3F5F8; border:none; border-radius:10px;");
        QVBoxLayout *cellLayout = new QVBoxLayout(cell);
        cellLayout->setContentsMargins(0, Spacing::SM, 0, Spacing::SM);
        cellLayout->setSpacing(2);

        valueLabel = new QLabel(QStringLiteral("0"), cell);
        QFont valueFont(QStringLiteral("MiSans"), 24, QFont::Normal);
        valueLabel->setFont(valueFont);
        valueLabel->setAlignment(Qt::AlignCenter);
        valueLabel->setStyleSheet("color:#0F172A;");
        cellLayout->addWidget(valueLabel);

        QLabel *label = new QLabel(labelText, cell);
        QFont labelFont(QStringLiteral("MiSans"), 13);
        label->setFont(labelFont);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("color:#0F172A;");
        cellLayout->addWidget(label);
        return cell;
    };

    statsGrid->addWidget(makeStatCell(statTotalLabel_,
        QStringLiteral("\u4EFB\u52A1\u603B\u6570")), 0, 0);            // 任务总数
    statsGrid->addWidget(makeStatCell(statCompletedLabel_,
        QStringLiteral("\u5DF2\u5B8C\u6210")), 0, 1);                   // 已完成
    statsGrid->addWidget(makeStatCell(statRateLabel_,
        QStringLiteral("\u5B8C\u6210\u7387")), 0, 2);                   // 完成率
    statsGrid->addWidget(makeStatCell(statDiffLabel_,
        QStringLiteral("\u8F83\u4E0A\u6708")), 0, 3);                   // 较上月
    statTotalLabel_->setObjectName("StatTotal");
    statCompletedLabel_->setObjectName("StatCompleted");
    statRateLabel_->setObjectName("StatRate");
    statDiffLabel_->setObjectName("StatDiff");

    for (int c = 0; c < 4; ++c) {
        statsGrid->setColumnStretch(c, 1);
    }
    layout->addLayout(statsGrid);
    layout->addStretch();

    return card;
}

// ============================================================================
// 数据加载
// ============================================================================

void HomeView::refresh()
{
    dashboard_ = HomeDashboardUseCase().load(inProgressCurrentPage_);
    loadFocusTasks();
    loadInProgressTasks();
    loadPeriodicReminders();
    loadMonthlyStats();
    updateDateSubtitle();
}

void HomeView::updateDateSubtitle()
{
    if (!subtitleLabel_) {
        return;
    }
    subtitleLabel_->setText(formatHeaderDate());
}

void HomeView::loadFocusTasks()
{
    if (!focusListLayout_ || !focusEmptyLabel_) {
        return;
    }
    clearLayout(focusListLayout_);

    const QList<Task> &focusTasks = dashboard_.focusTasks;

    if (focusTitle_) {
        focusTitle_->setText(QStringLiteral("\u4ECA\u65E5\u4E13\u6CE8  %1/3").arg(focusTasks.size()));
    }

    if (focusTasks.isEmpty()) {
        focusEmptyLabel_->setVisible(true);
        return;
    }
    focusEmptyLabel_->setVisible(false);

    for (const Task &task : focusTasks) {
        // 行容器
        QFrame *row = new QFrame(focusArea_);
        row->setObjectName("FocusItem");
        row->setFixedHeight(48);
        row->setStyleSheet(
            "QFrame#FocusItem {"
            "  background:#FFFFFF;"
            "  border:1px solid #D6E4FF;"
            "  border-radius:8px;"
            "}"
            "QFrame#FocusItem:hover {"
            "  border-color:#A5C3FF;"
            "}"
        );
        row->setProperty("taskId", task.id);
        row->installEventFilter(this);
        row->setCursor(Qt::PointingHandCursor);

        QHBoxLayout *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(12, 8, 12, 8);
        rowLayout->setSpacing(0);

        auto *categoryCell = new QFrame(row);
        categoryCell->setFixedWidth(48);
        categoryCell->setStyleSheet(QStringLiteral("background:transparent; border:none;"));
        auto *categoryLayout = new QHBoxLayout(categoryCell);
        categoryLayout->setContentsMargins(0, 0, 0, 0);
        categoryLayout->addWidget(createCategoryTag(task.category), 0, Qt::AlignCenter);
        rowLayout->addWidget(categoryCell);
        rowLayout->addSpacing(10);

        // 任务名称 —— 不拉伸，五角星紧跟其后
        QLabel *nameLabel = createTaskNameLabel(task, false);
        nameLabel->setFont(QFont(QStringLiteral("MiSans"), 17));
        rowLayout->addWidget(nameLabel);
        rowLayout->addSpacing(10);

        // 五角星（已点亮，紧跟任务名称，点击移出）
        rowLayout->addWidget(createStarButton(true, task.id));

        // 弹簧：将后续元素推到右侧
        rowLayout->addStretch();

        QLabel *tagLabel = new QLabel(task.tag, row);
        tagLabel->setFixedWidth(120);
        tagLabel->setAlignment(Qt::AlignCenter);
        tagLabel->setStyleSheet("color:#0F172A; font-size:14px; background:transparent; border:none;");
        tagLabel->setProperty("appUiNoLabelReset", true);
        rowLayout->addWidget(tagLabel);

        QLabel *sourceLabel = new QLabel(task.source, row);
        sourceLabel->setFixedWidth(104);
        sourceLabel->setAlignment(Qt::AlignCenter);
        sourceLabel->setStyleSheet("color:#0F172A; font-size:14px; background:transparent; border:none;");
        sourceLabel->setProperty("appUiNoLabelReset", true);
        rowLayout->addWidget(sourceLabel);
        rowLayout->addWidget(createDeadlineLabel(task, row));

        focusListLayout_->addWidget(row);
    }
}

void HomeView::loadInProgressTasks()
{
    if (!inProgressListLayout_ || !inProgressCountLabel_) {
        return;
    }
    clearLayout(inProgressListLayout_);

    const QList<Task> &tasks = dashboard_.inProgressTasks;
    const int total = dashboard_.inProgressTotal;

    if (inProgressCountLabel_) {
        // 显示总数（不限 limit 的全量计数），与 HTML "共 N 项" 一致
        inProgressCountLabel_->setText(
            QStringLiteral("\u5171 %1 \u9879").arg(total));  // 共 N 项
    }

    // 同步分页控件
    if (inProgressPagination_) {
        inProgressPagination_->setPage(inProgressCurrentPage_);
        inProgressPagination_->setTotal(total);
        inProgressPagination_->updateDisplay();
    }
    // 同步底部信息标签
    if (inProgressInfoLabel_) {
        inProgressInfoLabel_->setText(inProgressPagination_ ? inProgressPagination_->infoText() : QString());
    }

    if (tasks.isEmpty()) {
        auto *emptyState = new QLabel(QStringLiteral("目前没有任务，请添加任务"));
        emptyState->setObjectName(QStringLiteral("InProgressEmptyState"));
        emptyState->setAlignment(Qt::AlignCenter);
        emptyState->setStyleSheet(QStringLiteral("color:#000000; background:transparent; font-size:14px;"));
        inProgressListLayout_->addWidget(emptyState, 1);
        return;
    }

    for (int i = 0; i < tasks.size(); ++i) {
        const Task &task = tasks.at(i);
        bool isLast = (i == tasks.size() - 1);

        QFrame *row = new QFrame(this);
        row->setObjectName("InProgressItem");
        // 固定行高，不足10条时不拉伸填充
        row->setFixedHeight(48);
        QString borderCss = isLast
            ? QStringLiteral("border:none;")
            : QStringLiteral("border-bottom:1px solid #F1F5F9;");
        row->setStyleSheet(
            "QFrame#InProgressItem {"
            "  background:#FFFFFF;"
            "  " + borderCss +
            "}"
            "QFrame#InProgressItem:hover {"
            "  background:#F5F7FA;"
            "}"
        );
        row->setProperty("taskId", task.id);
        row->installEventFilter(this);
        row->setCursor(Qt::PointingHandCursor);

        QHBoxLayout *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 8, 0, 8);
        rowLayout->setSpacing(0);

        // 分类列与任务管理保持相同宽度，分类 pill 在单元格内居中。
        auto *categoryCell = new QFrame(row);
        categoryCell->setObjectName(QStringLiteral("HomeInProgressCategoryCell"));
        categoryCell->setFixedWidth(48);
        categoryCell->setStyleSheet(QStringLiteral("background:transparent; border:none;"));
        auto *categoryCellLayout = new QHBoxLayout(categoryCell);
        categoryCellLayout->setContentsMargins(0, 0, 0, 0);
        categoryCellLayout->addWidget(createCategoryTag(task.category), 0, Qt::AlignCenter);
        rowLayout->addWidget(categoryCell);
        rowLayout->addSpacing(10);

        // 任务名称（逾期红色）—— 不拉伸，五角星紧跟其后
        bool overdue = task.isOverdue() && !task.isCompleted();
        QLabel *nameLabel = createTaskNameLabel(task, overdue);
        nameLabel->setFont(QFont(QStringLiteral("MiSans"), 15));
        rowLayout->addWidget(nameLabel);
        rowLayout->addSpacing(10);

        // 五角星（紧跟任务名称之后）
        rowLayout->addWidget(createStarButton(task.isTodayFocus, task.id));

        // 弹簧：将后续元素推到右侧
        rowLayout->addStretch();

        QLabel *tagLabel = new QLabel(task.tag, row);
        tagLabel->setFixedWidth(120);
        tagLabel->setAlignment(Qt::AlignCenter);
        tagLabel->setStyleSheet("color:#0F172A; font-size:14px; background:transparent; border:none;");
        tagLabel->setProperty("appUiNoLabelReset", true);
        rowLayout->addWidget(tagLabel);

        // 来源（右对齐，min-width 80px）
        QLabel *sourceLabel = new QLabel(task.source, row);
        sourceLabel->setFixedWidth(104);
        sourceLabel->setAlignment(Qt::AlignCenter);
        sourceLabel->setStyleSheet("color:#0F172A; font-size:14px; background:transparent; border:none;");
        sourceLabel->setProperty("appUiNoLabelReset", true);
        rowLayout->addWidget(sourceLabel);

        rowLayout->addWidget(createDeadlineLabel(task, row));

        inProgressListLayout_->addWidget(row);
    }

    // 不足10条时添加弹性空间，让空白在底部而非拉伸行高
    inProgressListLayout_->addStretch();
}

void HomeView::loadPeriodicReminders()
{
    if (!periodicListLayout_ || !periodicEmptyLabel_) {
        return;
    }
    clearLayout(periodicListLayout_);

    const QList<HomePeriodicReminder> &reminders = dashboard_.periodicReminders;

    if (reminders.isEmpty()) {
        periodicEmptyLabel_->setVisible(true);
        return;
    }
    periodicEmptyLabel_->setVisible(false);

    QDate today = QDate::currentDate();

    for (const HomePeriodicReminder &item : reminders) {
        QFrame *row = new QFrame(this);
        row->setObjectName("PeriodicReminderItem");
        QString borderColor = item.dueToday
            ? QStringLiteral("#DC2626")    // 今日到期：红色边框
            : QStringLiteral("#F59E0B");   // 未来7天：橙色边框
        row->setStyleSheet(
            "QFrame#PeriodicReminderItem {"
            "  background:#FFFFFF;"
            "  border:1px solid " + borderColor + ";"
            "  border-radius:8px;"
            "}"
        );

        QVBoxLayout *rowLayout = new QVBoxLayout(row);
        rowLayout->setContentsMargins(12, 8, 12, 8);
        rowLayout->setSpacing(4);

        // 第一行：任务名称 + 到期标签
        QHBoxLayout *topRow = new QHBoxLayout();
        topRow->setSpacing(8);

        QLabel *nameLabel = new QLabel(item.name, row);
        nameLabel->setStyleSheet("color:#0F172A; font-size:14px; font-weight:700; border:none; background:transparent;");
        topRow->addWidget(nameLabel);
        topRow->addStretch();

        // 到期标签
        QString dueText;
        QString dueColor;
        if (item.dueToday) {
            dueText = QStringLiteral("\u4ECA\u65E5\u5230\u671F");  // 今日到期
            dueColor = "#DC2626";
        } else {
            int daysLeft = today.daysTo(item.nextReminderDate);
            if (daysLeft == 1) {
                dueText = QStringLiteral("\u660E\u65E5\u5230\u671F");  // 明日到期
            } else {
                dueText = QStringLiteral("%1\u65E5\u540E\u5230\u671F").arg(daysLeft);  // N日后到期
            }
            dueColor = "#F59E0B";
        }
        QLabel *dueLabel = new QLabel(dueText, row);
        dueLabel->setStyleSheet(
            QString("color:%1; font-size:12px; font-weight:700; border:none; background:transparent;").arg(dueColor));
        topRow->addWidget(dueLabel);

        rowLayout->addLayout(topRow);

        // 第二行：创建任务 + 忽略本次 按钮
        QHBoxLayout *btnRow = new QHBoxLayout();
        btnRow->setSpacing(8);

        QPushButton *createBtn = new QPushButton(QStringLiteral("\u521B\u5EFA\u4EFB\u52A1"), row);  // 创建任务
        createBtn->setProperty("templateId", item.templateId);
        createBtn->setCursor(Qt::PointingHandCursor);
        createBtn->setStyleSheet(
            "QPushButton {"
            "  background:#2463EB;"
            "  color:#FFFFFF;"
            "  border:none;"
            "  border-radius:4px;"
            "  padding:4px 12px;"
            "  font-size:12px;"
            "  font-weight:700;"
            "}"
            "QPushButton:hover {"
            "  background:#1D4ED8;"
            "}"
        );
        connect(createBtn, &QPushButton::clicked, this, &HomeView::onCreatePeriodicTaskClicked);

        QPushButton *ignoreBtn = new QPushButton(QStringLiteral("\u5FFD\u7565\u672C\u6B21"), row);  // 忽略本次
        ignoreBtn->setProperty("templateId", item.templateId);
        ignoreBtn->setCursor(Qt::PointingHandCursor);
        ignoreBtn->setStyleSheet(
            "QPushButton {"
            "  background:#FFFFFF;"
            "  color:#64748B;"
            "  border:1px solid #CBD5E1;"
            "  border-radius:4px;"
            "  padding:4px 12px;"
            "  font-size:12px;"
            "}"
            "QPushButton:hover {"
            "  background:#F5F7FA;"
            "  color:#0F172A;"
            "}"
        );
        connect(ignoreBtn, &QPushButton::clicked, this, &HomeView::onIgnorePeriodicClicked);

        btnRow->addWidget(createBtn);
        btnRow->addWidget(ignoreBtn);
        btnRow->addStretch();
        rowLayout->addLayout(btnRow);

        periodicListLayout_->addWidget(row);
    }
}

void HomeView::loadMonthlyStats()
{
    if (!statTotalLabel_ || !statCompletedLabel_ ||
        !statRateLabel_ || !statDiffLabel_) {
        return;
    }

    const HomeMonthlyStats &stats = dashboard_.monthlyStats;

    statTotalLabel_->setText(QString::number(stats.total));
    statTotalLabel_->setStyleSheet("color:#0F172A; font-size:24px; font-weight:700;");

    statCompletedLabel_->setText(QString::number(stats.completed));
    statCompletedLabel_->setStyleSheet("color:#0F172A; font-size:24px; font-weight:700;");

    statRateLabel_->setText(QString::number(stats.completionRate) + QStringLiteral("%"));
    statRateLabel_->setStyleSheet("color:#0F172A; font-size:24px; font-weight:700;");

    // 较上月：正数绿色，负数红色，零默认
    if (stats.comparedToLastMonth > 0) {
        statDiffLabel_->setText(QStringLiteral("+%1").arg(stats.comparedToLastMonth));
        statDiffLabel_->setStyleSheet("color:#059669; font-size:24px; font-weight:700;");
    } else if (stats.comparedToLastMonth < 0) {
        statDiffLabel_->setText(QString::number(stats.comparedToLastMonth));  // 负号自带
        statDiffLabel_->setStyleSheet("color:#DC2626; font-size:24px; font-weight:700;");
    } else {
        statDiffLabel_->setText(QStringLiteral("0"));
        statDiffLabel_->setStyleSheet("color:#0F172A; font-size:24px; font-weight:700;");
    }
}

// ============================================================================
// 交互槽
// ============================================================================

void HomeView::onCreateTaskClicked()
{
    emit createTaskClicked();
}

void HomeView::onViewAllTasksClicked()
{
    emit viewAllTasksRequested();
}

void HomeView::onStarClicked()
{
    QPushButton *starBtn = qobject_cast<QPushButton*>(sender());
    if (!starBtn) {
        return;
    }
    int taskId = starBtn->property("taskId").toInt();
    if (taskId <= 0) {
        return;
    }
    bool currentlyFocused = starBtn->property("focused").toBool();

    HomeCommandResult result = HomeCommandUseCase().setFocus(taskId, !currentlyFocused);

    if (!result.ok) {
        // 达到上限或其它错误：在五角星上方显示提示
        QToolTip::showText(starBtn->mapToGlobal(QPoint(0, -30)),
                           QStringLiteral("\u4ECA\u65E5\u4E13\u6CE8\u5DF2\u8FBE\u4E0A\u9650\uFF083\u4E2A\uFF09"),
                           starBtn);  // 今日专注已达上限（3个）
        return;
    }

    dashboard_ = HomeDashboardUseCase().load(inProgressCurrentPage_);
    for (const Task &task : dashboard_.inProgressTasks) {
        if (task.id == taskId) {
            setStarButtonState(starBtn, task.isTodayFocus);
            break;
        }
    }
    loadFocusTasks();
}

void HomeView::onPageChanged(int page)
{
    if (page < 1) {
        return;
    }
    inProgressCurrentPage_ = page;
    refresh();
}

void HomeView::onCreatePeriodicTaskClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) {
        return;
    }
    int templateId = btn->property("templateId").toInt();
    if (templateId > 0) {
        emit createPeriodicTaskFromTemplate(templateId);
    }
}

void HomeView::onIgnorePeriodicClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) {
        return;
    }
    int templateId = btn->property("templateId").toInt();
    if (templateId > 0) {
        emit ignorePeriodicReminder(templateId);
    }
}

// ============================================================================
// 事件过滤：行级点击 → 发射 taskClicked
// ============================================================================

bool HomeView::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QFrame *row = qobject_cast<QFrame*>(watched);
        if (row) {
            QVariant tid = row->property("taskId");
            if (tid.isValid()) {
                int taskId = tid.toInt();
                if (taskId > 0) {
                    emit taskClicked(taskId);
                    return true;
                }
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

// ============================================================================
// 工具方法
// ============================================================================

void HomeView::clearLayout(QLayout *layout)
{
    if (!layout) {
        return;
    }
    QLayoutItem *item = nullptr;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (QWidget *w = item->widget()) {
            w->setParent(nullptr);
            delete w;
        } else if (QLayout *childLayout = item->layout()) {
            clearLayout(childLayout);
            delete childLayout;
        }
        delete item;
    }
}

QString HomeView::formatHeaderDate() const
{
    QDate today = QDate::currentDate();
    // "YYYY年M月D日 · 星期X"
    return QStringLiteral("%1\u5E74%2\u6708%3\u65E5 \u00B7 %4")
        .arg(today.year())
        .arg(today.month())
        .arg(today.day())
        .arg(weekdayChinese(today.dayOfWeek()));
}

QString HomeView::formatDeadlineText(const QDate &deadline) const
{
    // "截止 MM-dd"
    return QStringLiteral("\u622A\u6B62 %1")
        .arg(deadline.toString("MM-dd"));
}

QString HomeView::weekdayChinese(int weekday) const
{
    // Qt: 1=Monday ... 7=Sunday
    static const char *names[] = {
        "\u661F\u671F\u4E00",  // 星期一
        "\u661F\u671F\u4E8C",  // 星期二
        "\u661F\u671F\u4E09",  // 星期三
        "\u661F\u671F\u56DB",  // 星期四
        "\u661F\u671F\u4E94",  // 星期五
        "\u661F\u671F\u516D",  // 星期六
        "\u661F\u671F\u65E5"   // 星期日
    };
    if (weekday < 1 || weekday > 7) {
        return QString();
    }
    return QString::fromUtf8(names[weekday - 1]);
}

QPushButton* HomeView::createStarButton(bool focused, int taskId)
{
    QPushButton *btn = new QPushButton();
    btn->setCursor(Qt::PointingHandCursor);
    btn->setProperty("taskId", taskId);
    btn->setFixedSize(28, 28);
    setStarButtonState(btn, focused);
    connect(btn, &QPushButton::clicked, this, &HomeView::onStarClicked);
    return btn;
}

void HomeView::setStarButtonState(QPushButton *btn, bool focused) const
{
    if (!btn) {
        return;
    }
    btn->setProperty("focused", focused);
    btn->setText(focused
        ? QStringLiteral("\u2605")   // ★ 已点亮
        : QStringLiteral("\u2606")); // ☆ 未点亮
    QString color = focused ? QStringLiteral("#F59E0B") : QStringLiteral("#CBD5E1");
    btn->setStyleSheet(
        "QPushButton {"
        "  color:" + color + ";"
        "  font-size:18px;"
        "  border:none;"
        "  background:transparent;"
        "  padding:0;"
        "}"
        "QPushButton:hover {"
        "  color:" + (focused ? QStringLiteral("#D97706") : QStringLiteral("#F59E0B")) + ";"
        "}"
    );
    btn->setToolTip(focused
        ? QStringLiteral("\u79FB\u51FA\u4ECA\u65E5\u4E13\u6CE8")   // 移出今日专注
        : QStringLiteral("\u52A0\u5165\u4ECA\u65E5\u4E13\u6CE8")); // 加入今日专注
}

QFrame* HomeView::createCategoryTag(const QString &category)
{
    return new HomeCategoryBadge(category);
}

QLabel* HomeView::createTaskNameLabel(const Task &task, bool redIfOverdue)
{
    QLabel *label = new QLabel();
    label->setObjectName(QStringLiteral("appUiTaskTitle"));
    label->setTextFormat(Qt::RichText);
    label->setStyleSheet(QStringLiteral("background:transparent; border:none;"));
    label->setProperty("appUiNoLabelReset", true);

    QString baseColor = redIfOverdue ? QStringLiteral("#DC2626") : QStringLiteral("#0F172A");
    QString html;
    if (task.isKeyTask) {
        // 【重点】红色前缀 + 任务名
        html = QStringLiteral(
            "<span style=\"color:#FF0000; font-weight:400;\">\u3010\u91CD\u70B9\u3011</span> "
            "<span style=\"color:%1;\">%2</span>"
        ).arg(baseColor, task.title.toHtmlEscaped());
    } else {
        html = QStringLiteral(
            "<span style=\"color:%1;\">%2</span>"
        ).arg(baseColor, task.title.toHtmlEscaped());
    }
    label->setText(html);
    label->setTextInteractionFlags(Qt::NoTextInteraction);
    return label;
}

QLabel* HomeView::createDeadlineLabel(const Task &task, QWidget *parent)
{
    const bool overdue = task.status == QStringLiteral("进行中")
        && task.deadline.isValid() && task.deadline < QDate::currentDate();
    auto *label = new QLabel(overdue ? QStringLiteral("逾期")
        : task.deadline.isValid() ? formatDeadlineText(task.deadline) : QString(), parent);
    label->setObjectName(QStringLiteral("HomeDeadlineCell"));
    label->setFixedWidth(102);
    label->setAlignment(Qt::AlignCenter);
    const int daysLeft = QDate::currentDate().daysTo(task.deadline);
    const char *color = task.deadline.isValid() && daysLeft <= 1 ? "#FF0000"
        : task.deadline.isValid() && daysLeft <= 3 ? "#FAAD14" : "#0F172A";
    label->setStyleSheet(QStringLiteral(
        "color:%1; font-size:14px; background:transparent; border:none;").arg(color));
    label->setProperty("appUiNoLabelReset", true);
    return label;
}
