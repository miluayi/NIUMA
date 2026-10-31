/**
 * @file StatsView.cpp
 * @brief 统计页面实现
 *
 * 依赖：ui/StatsView.h、application/stats/StatsUseCase、
 *       ui/widgets/TrendChart、ui/widgets/HeatmapWidget、ui/Spacing
 * 文档：Phase 6 统计模块、qt_implementation_guide.md 3.6、coding.md 第七章
 */
#include "StatsView.h"

#include <QComboBox>
#include <QDate>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QProgressBar>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QVBoxLayout>

#include "application/stats/StatsUseCase.h"
#include "ui/Spacing.h"
#include "ui/widgets/HeatmapWidget.h"
#include "ui/widgets/TrendChart.h"

// ============================================================================
// 构造
// ============================================================================

StatsView::StatsView(QWidget *parent)
    : QWidget(parent)
    , statTotalLabel_(nullptr)
    , statCompletedLabel_(nullptr)
    , statInProgressLabel_(nullptr)
    , statRateLabel_(nullptr)
    , categoryLayout_(nullptr)
    , sourceLayout_(nullptr)
    , trendChart_(nullptr)
    , trendTitleLabel_(nullptr)
    , trendYearCombo_(nullptr)
    , heatmapWidget_(nullptr)
    , heatmapDimensionCombo_(nullptr)
    , heatmapYearCombo_(nullptr)
    , tabWidget_(nullptr)
{
    setupUI();
    refresh();
}

// ============================================================================
// UI 构建
// ============================================================================

void StatsView::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(Spacing::XXL, Spacing::XL, Spacing::XXL, Spacing::XL);
    mainLayout->setSpacing(Spacing::LG);

    // === 页头 ===
    mainLayout->addWidget(buildPageHeader());

    // === 标签页 ===
    tabWidget_ = new QTabWidget(this);
    tabWidget_->setObjectName("statsTabs");
    tabWidget_->setStyleSheet(
        "QTabWidget::pane {"
        "  border:none;"
        "  background:transparent;"
        "}"
        "QTabBar::tab {"
        "  background:transparent;"
        "  color:#0F172A;"
        "  padding:8px 20px;"
        "  border:none;"
        "  border-bottom:2px solid transparent;"
        "  font-size:15px;"
        "  margin-right:4px;"
        "}"
        "QTabBar::tab:selected {"
        "  color:#2463EB;"
        "  border-bottom:2px solid #2463EB;"
        "  font-weight:700;"
        "}"
        "QTabBar::tab:hover:!selected {"
        "  color:#2463EB;"
        "}"
    );
    tabWidget_->addTab(buildStatsTab(), QStringLiteral("\u4EFB\u52A1\u7EDF\u8BA1"));   // 任务统计
    tabWidget_->addTab(buildTrendTab(), QStringLiteral("\u8D8B\u52BF\u56FE"));        // 趋势图
    tabWidget_->addTab(buildHeatmapTab(), QStringLiteral("\u70ED\u529B\u56FE"));      // 热力图

    mainLayout->addWidget(tabWidget_, 1);
}

QFrame* StatsView::buildPageHeader()
{
    QFrame *header = new QFrame(this);
    header->setObjectName("PageHeader");

    QVBoxLayout *headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(Spacing::XS);

    QLabel *titleLabel = new QLabel(QStringLiteral("\u7EDF\u8BA1"), header);  // 统计
    QFont titleFont(QStringLiteral("MiSans"), 22, QFont::Normal);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color:#0F172A;");
    headerLayout->addWidget(titleLabel);

    QLabel *subtitleLabel = new QLabel(
        QStringLiteral("\u67E5\u770B\u4EFB\u52A1\u7EDF\u8BA1\u3001\u6708\u5EA6\u8D8B\u52BF\u4E0E\u5DE5\u4F5C\u5BC6\u5EA6\u70ED\u529B\u56FE"),
        header);  // 查看任务统计、月度趋势与工作密度热力图
    QFont subtitleFont(QStringLiteral("MiSans"), 14);
    subtitleLabel->setFont(subtitleFont);
    subtitleLabel->setStyleSheet("color:#0F172A;");
    headerLayout->addWidget(subtitleLabel);

    return header;
}

QWidget* StatsView::buildStatsTab()
{
    QWidget *tab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Spacing::LG);

    // === 汇总统计（4 个小卡片）===
    QHBoxLayout *statsRow = new QHBoxLayout();
    statsRow->setSpacing(Spacing::MD);

    // Lambda 构建单个统计卡片
    auto buildStatCard = [this](const QString &title, const QString &colorHex,
                                const QString &objectName, QLabel **valueLabel) -> QFrame* {
        QFrame *card = new QFrame(this);
        card->setObjectName("StatCard");
        card->setStyleSheet(
            "QFrame#StatCard {"
            "  background:#FFFFFF;"
            "  border:1px solid #DCE3EF;"
            "  border-radius:12px;"
            "}"
        );
        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(Spacing::LG, Spacing::LG, Spacing::LG, Spacing::LG);
        cardLayout->setSpacing(Spacing::XS);

        *valueLabel = new QLabel(QStringLiteral("0"), card);
        (*valueLabel)->setObjectName(objectName);
        (*valueLabel)->setStyleSheet(
            QString("font-size:28px; font-weight:700; color:%1; border:none; background:transparent;").arg(colorHex));
        cardLayout->addWidget(*valueLabel);

        QLabel *titleLabel = new QLabel(title, card);
        titleLabel->setStyleSheet("font-size:14px; color:#0F172A; border:none; background:transparent;");
        cardLayout->addWidget(titleLabel);

        return card;
    };

    statsRow->addWidget(buildStatCard(QStringLiteral("\u4EFB\u52A1\u603B\u6570"), "#2463EB", QStringLiteral("statsTotal"), &statTotalLabel_));
    statsRow->addWidget(buildStatCard(QStringLiteral("\u5DF2\u5B8C\u6210"), "#16A34A", QStringLiteral("statsCompleted"), &statCompletedLabel_));
    statsRow->addWidget(buildStatCard(QStringLiteral("\u8FDB\u884C\u4E2D"), "#EAB308", QStringLiteral("statsInProgress"), &statInProgressLabel_));
    statsRow->addWidget(buildStatCard(QStringLiteral("\u5B8C\u6210\u7387"), "#8B5CF6", QStringLiteral("statsCompletionRate"), &statRateLabel_));

    layout->addLayout(statsRow);

    // === 分类分布 + 来源分布（左右并排）===
    QHBoxLayout *distRow = new QHBoxLayout();
    distRow->setSpacing(Spacing::LG);

    QVBoxLayout *categoryBodyLayout = nullptr;
    QFrame *categoryCard = createCard(QStringLiteral("\u5206\u7C7B\u5206\u5E03"), QStringLiteral("statsCategoryCard"), &categoryBodyLayout);
    categoryLayout_ = categoryBodyLayout;
    distRow->addWidget(categoryCard, 1);

    QVBoxLayout *sourceBodyLayout = nullptr;
    QFrame *sourceCard = createCard(QStringLiteral("\u6765\u6E90\u5206\u5E03"), QStringLiteral("statsSourceCard"), &sourceBodyLayout);
    sourceLayout_ = sourceBodyLayout;
    distRow->addWidget(sourceCard, 1);

    layout->addLayout(distRow);
    layout->addStretch();

    return tab;
}

QWidget* StatsView::buildTrendTab()
{
    QWidget *tab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Spacing::LG);

    QVBoxLayout *bodyLayout = nullptr;
    QFrame *card = createCard(QString(), QStringLiteral("statsTrendCard"), &bodyLayout);

    // 标题行：标题 + 年份选择
    QHBoxLayout *headerRow = new QHBoxLayout();
    headerRow->setSpacing(Spacing::MD);

    trendTitleLabel_ = new QLabel(card);
    trendTitleLabel_->setObjectName("statsTrendTitle");
    trendTitleLabel_->setStyleSheet("font-size:16px; font-weight:700; color:#0F172A; border:none; background:transparent;");
    headerRow->addWidget(trendTitleLabel_);
    headerRow->addStretch();

    // 年份选择
    trendYearCombo_ = new QComboBox(card);
    trendYearCombo_->setObjectName("statsTrendYearCombo");
    trendYearCombo_->setFixedWidth(90);
    int currentYear = QDate::currentDate().year();
    for (int y = 2020; y <= currentYear + 1; ++y) {
        trendYearCombo_->addItem(QString::number(y), y);
    }
    trendYearCombo_->setCurrentText(QString::number(currentYear));
    trendYearCombo_->setStyleSheet(
        "QComboBox {"
        "  padding:5px 10px;"
        "  border:1px solid #E2E8F0;"
        "  border-radius:6px;"
        "  font-size:13px;"
        "  background:#FFFFFF;"
        "  color:#0F172A;"
        "}"
    );
    headerRow->addWidget(trendYearCombo_);

    bodyLayout->insertLayout(0, headerRow);

    // TrendChart 控件
    trendChart_ = new TrendChart(card);
    trendChart_->setObjectName("statsTrendChart");
    bodyLayout->addWidget(trendChart_);

    layout->addWidget(card);
    layout->addStretch();

    // 信号连接
    connect(trendYearCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StatsView::onTrendYearChanged);

    return tab;
}

QWidget* StatsView::buildHeatmapTab()
{
    QWidget *tab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Spacing::LG);

    QVBoxLayout *bodyLayout = nullptr;
    QFrame *card = createCard(QString(), QStringLiteral("statsHeatmapCard"), &bodyLayout);

    // 标题行：标题 + 维度选择 + 年份选择
    QHBoxLayout *headerRow = new QHBoxLayout();
    headerRow->setSpacing(Spacing::MD);

    QLabel *title = new QLabel(QStringLiteral("\u5DE5\u4F5C\u5BC6\u5EA6\u70ED\u529B\u56FE"), card);  // 工作密度热力图
    title->setStyleSheet("font-size:16px; font-weight:700; color:#0F172A; border:none; background:transparent;");
    headerRow->addWidget(title);
    headerRow->addStretch();

    // 维度选择
    heatmapDimensionCombo_ = new QComboBox(card);
    heatmapDimensionCombo_->setObjectName("statsHeatmapDimensionCombo");
    heatmapDimensionCombo_->setFixedWidth(130);
    heatmapDimensionCombo_->addItem(QStringLiteral("\u6309\u521B\u5EFA\u65E5\u671F"), "create_time");  // 按创建日期
    heatmapDimensionCombo_->addItem(QStringLiteral("\u6309\u5B8C\u6210\u65E5\u671F"), "finish_time");  // 按完成日期
    heatmapDimensionCombo_->setStyleSheet(
        "QComboBox {"
        "  padding:5px 10px;"
        "  border:1px solid #E2E8F0;"
        "  border-radius:6px;"
        "  font-size:13px;"
        "  background:#FFFFFF;"
        "  color:#0F172A;"
        "}"
    );
    headerRow->addWidget(heatmapDimensionCombo_);

    // 年份选择
    heatmapYearCombo_ = new QComboBox(card);
    heatmapYearCombo_->setObjectName("statsHeatmapYearCombo");
    heatmapYearCombo_->setFixedWidth(104);
    int currentYear = QDate::currentDate().year();
    for (int y = 2020; y <= currentYear + 1; ++y) {
        heatmapYearCombo_->addItem(QString::number(y), y);
    }
    heatmapYearCombo_->setCurrentText(QString::number(currentYear));
    heatmapYearCombo_->setStyleSheet(heatmapDimensionCombo_->styleSheet());
    headerRow->addWidget(heatmapYearCombo_);

    bodyLayout->insertLayout(0, headerRow);

    // HeatmapWidget 控件
    heatmapWidget_ = new HeatmapWidget(card);
    heatmapWidget_->setObjectName("statsHeatmapWidget");
    bodyLayout->addWidget(heatmapWidget_, 1);

    layout->addWidget(card, 1);

    // 信号连接
    connect(heatmapDimensionCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StatsView::onHeatmapDimensionChanged);
    connect(heatmapYearCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StatsView::onHeatmapDimensionChanged);

    return tab;
}

QFrame* StatsView::createCard(const QString &title, const QString &objectName, QVBoxLayout **bodyLayout)
{
    QFrame *card = new QFrame(this);
    card->setObjectName(objectName);
    card->setStyleSheet(
        "QFrame#" + objectName + " {"
        "  background:#FFFFFF;"
        "  border:1px solid #DCE3EF;"
        "  border-radius:12px;"
        "}"
    );

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(Spacing::LG, Spacing::LG, Spacing::LG, Spacing::LG);
    cardLayout->setSpacing(Spacing::MD);

    if (!title.isEmpty()) {
        QLabel *titleLabel = new QLabel(title, card);
        titleLabel->setStyleSheet("font-size:16px; font-weight:700; color:#0F172A; border:none; background:transparent;");
        cardLayout->addWidget(titleLabel);
    }

    *bodyLayout = new QVBoxLayout();
    (*bodyLayout)->setSpacing(Spacing::SM);
    cardLayout->addLayout(*bodyLayout);

    return card;
}

// ============================================================================
// 数据加载
// ============================================================================

void StatsView::refresh()
{
    loadStats();
    // 同步趋势图与热力图年份到当前年
    int currentYear = QDate::currentDate().year();
    if (trendYearCombo_) {
        int idx = trendYearCombo_->findData(currentYear);
        if (idx >= 0) {
            QSignalBlocker blocker(*trendYearCombo_);
            trendYearCombo_->setCurrentIndex(idx);
        }
    }
    if (heatmapYearCombo_) {
        int idx = heatmapYearCombo_->findData(currentYear);
        if (idx >= 0) {
            QSignalBlocker blocker(*heatmapYearCombo_);
            heatmapYearCombo_->setCurrentIndex(idx);
        }
    }
    loadTrendChart(currentYear);
    loadHeatmap(currentYear);
}

void StatsView::loadStats()
{
    // 全量统计：使用极宽日期范围覆盖所有数据
    QDate start(2000, 1, 1);
    QDate end(2099, 12, 31);

    const StatsUseCase useCase;

    // 汇总统计
    TaskStats stats = useCase.summary(start, end);
    statTotalLabel_->setText(QString::number(stats.totalCount));
    statCompletedLabel_->setText(QString::number(stats.completedCount));
    statInProgressLabel_->setText(QString::number(stats.inProgressCount));
    statRateLabel_->setText(QStringLiteral("%1%").arg(QString::number(stats.completionRate, 'f', 1)));

    // 分类分布
    QList<CategoryStat> categories = useCase.categories(start, end);
    loadCategoryDistribution(categories);

    // 来源分布
    QList<SourceStat> sources = useCase.sources(start, end);
    loadSourceDistribution(sources);
}

void StatsView::loadCategoryDistribution(const QList<CategoryStat> &stats)
{
    clearLayout(categoryLayout_);

    if (stats.isEmpty()) {
        QLabel *empty = new QLabel(QStringLiteral("\u6682\u65E0\u6570\u636E"));  // 暂无数据
        empty->setObjectName("statsCategoryEmpty");
        empty->setStyleSheet("color:#0F172A; font-size:14px; border:none; background:transparent;");
        categoryLayout_->addWidget(empty);
        return;
    }

    // 求最大值用于计算百分比
    int maxCount = 0;
    for (const CategoryStat &s : stats) {
        if (s.count > maxCount) {
            maxCount = s.count;
        }
    }

    for (const CategoryStat &s : stats) {
        QVBoxLayout *itemLayout = new QVBoxLayout();
        itemLayout->setSpacing(4);

        // 标签行：分类名 + 数量
        QHBoxLayout *labelRow = new QHBoxLayout();
        QLabel *nameLabel = new QLabel(s.category.isEmpty() ? QStringLiteral("\u672A\u5206\u7C7B") : s.category);
        nameLabel->setStyleSheet("font-size:14px; color:#0F172A; border:none; background:transparent;");
        labelRow->addWidget(nameLabel);
        labelRow->addStretch();
        QLabel *countLabel = new QLabel(QStringLiteral("%1 \u9879").arg(s.count));  // N 项
        countLabel->setStyleSheet("font-size:14px; color:#0F172A; border:none; background:transparent;");
        labelRow->addWidget(countLabel);
        itemLayout->addLayout(labelRow);

        // 进度条
        QProgressBar *bar = new QProgressBar();
        bar->setRange(0, 100);
        bar->setFixedHeight(6);
        bar->setTextVisible(false);
        int percent = (maxCount > 0) ? static_cast<int>(static_cast<double>(s.count) / maxCount * 100) : 0;
        bar->setValue(percent);
        QString color = categoryColorHex(s.category);
        bar->setStyleSheet(
            QString(
                "QProgressBar {"
                "  background:#F1F5F9;"
                "  border:none;"
                "  border-radius:3px;"
                "}"
                "QProgressBar::chunk {"
                "  background:%1;"
                "  border-radius:3px;"
                "}"
            ).arg(color)
        );
        itemLayout->addWidget(bar);

        categoryLayout_->addLayout(itemLayout);
    }
}

void StatsView::loadSourceDistribution(const QList<SourceStat> &stats)
{
    clearLayout(sourceLayout_);

    if (stats.isEmpty()) {
        QLabel *empty = new QLabel(QStringLiteral("\u6682\u65E0\u6570\u636E"));  // 暂无数据
        empty->setObjectName("statsSourceEmpty");
        empty->setStyleSheet("color:#0F172A; font-size:14px; border:none; background:transparent;");
        sourceLayout_->addWidget(empty);
        return;
    }

    for (int i = 0; i < stats.size(); ++i) {
        const SourceStat &s = stats[i];
        QFrame *row = new QFrame();
        QString borderStyle = (i < stats.size() - 1) ? "border-bottom:1px solid #F1F5F9;" : "border-bottom:none;";
        row->setStyleSheet(
            QString("QFrame { %1 border-top:none; border-left:none; border-right:none; background:transparent; }").arg(borderStyle)
        );

        QHBoxLayout *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 4, 0, 4);
        rowLayout->setSpacing(0);

        QLabel *nameLabel = new QLabel(s.source.isEmpty() ? QStringLiteral("\u672A\u77E5\u6765\u6E90") : s.source);
        nameLabel->setStyleSheet("font-size:14px; color:#0F172A; border:none; background:transparent;");
        rowLayout->addWidget(nameLabel);
        rowLayout->addStretch();
        QLabel *countLabel = new QLabel(QStringLiteral("%1 \u9879").arg(s.count));  // N 项
        countLabel->setStyleSheet("font-size:14px; color:#0F172A; border:none; background:transparent;");
        rowLayout->addWidget(countLabel);

        sourceLayout_->addWidget(row);
    }
}

void StatsView::loadTrendChart(int year)
{
    QList<MonthlyTrend> trend = StatsUseCase().monthlyTrend(year);
    trendChart_->setData(trend);
    if (trendTitleLabel_) {
        trendTitleLabel_->setText(QStringLiteral("%1\u5E74\u6708\u5EA6\u4EFB\u52A1\u8D8B\u52BF").arg(year));  // "YYYY年月度任务趋势"
    }
}

void StatsView::loadHeatmap(int year)
{
    QString dimension = "create_time";
    if (heatmapDimensionCombo_ && heatmapDimensionCombo_->currentIndex() == 1) {
        dimension = "finish_time";
    }
    QList<HeatmapData> data = StatsUseCase().heatmap(year, dimension);
    heatmapWidget_->setData(data, year);
}

// ============================================================================
// 事件处理
// ============================================================================

void StatsView::onHeatmapDimensionChanged()
{
    int year = QDate::currentDate().year();
    if (heatmapYearCombo_) {
        year = heatmapYearCombo_->currentData().toInt();
    }
    loadHeatmap(year);
}

void StatsView::onTrendYearChanged()
{
    if (!trendYearCombo_) {
        return;
    }
    int year = trendYearCombo_->currentData().toInt();
    loadTrendChart(year);
}

// ============================================================================
// 工具方法
// ============================================================================

void StatsView::clearLayout(QLayout *layout)
{
    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        if (item->layout()) {
            clearLayout(item->layout());
        }
        delete item;
    }
}

QString StatsView::categoryColorHex(const QString &category) const
{
    if (category == QStringLiteral("\u50AC")) return "#F59E0B";  // 催
    if (category == QStringLiteral("\u67E5")) return "#2463EB";  // 查
    if (category == QStringLiteral("\u62A5")) return "#60A5FA";  // 报
    if (category == QStringLiteral("\u529E")) return "#8B5CF6";  // 办
    return "#2463EB";  // 默认
}
