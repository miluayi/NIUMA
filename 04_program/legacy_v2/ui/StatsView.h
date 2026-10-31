/**
 * @file StatsView.h
 * @brief 统计页面（QWidget 子类，MainWindow stacked widget index 5）
 *
 * 布局：
 *   QVBoxLayout
 *   ├── page-header（标题"统计" + 副标题）
 *   └── QTabWidget（3 个标签页）
 *       ├── 任务统计：全量汇总卡片 + 分类分布 + 来源分布
 *       ├── 趋势图：年份切换 + TrendChart
 *       └── 热力图：维度/年份切换 + HeatmapWidget
 *
 * 信号：无（统计页面纯展示，不发射交互信号）
 *
 * 依赖：Qt5 Widgets、application/stats/StatsUseCase、
 *       ui/widgets/TrendChart、ui/widgets/HeatmapWidget
 * 文档：Phase 6 统计模块、qt_implementation_guide.md 3.6、coding.md 第七章
 */
#pragma once

#include <QWidget>
#include <QDate>
#include <QList>
#include "models/Statistics.h"

class QLabel;
class QTabWidget;
class QComboBox;
class QVBoxLayout;
class QFrame;
class TrendChart;
class HeatmapWidget;

class StatsView : public QWidget {
    Q_OBJECT

public:
    explicit StatsView(QWidget *parent = nullptr);

    /// 重新加载全部统计数据
    void refresh();

private slots:
    void onHeatmapDimensionChanged();
    void onTrendYearChanged();

private:
    // ---- 汇总统计 ----
    QLabel *statTotalLabel_;
    QLabel *statCompletedLabel_;
    QLabel *statInProgressLabel_;
    QLabel *statRateLabel_;

    // ---- 分类分布 ----
    QVBoxLayout *categoryLayout_;

    // ---- 来源分布 ----
    QVBoxLayout *sourceLayout_;

    // ---- 趋势图 ----
    TrendChart *trendChart_;
    QLabel *trendTitleLabel_;
    QComboBox *trendYearCombo_;

    // ---- 热力图 ----
    HeatmapWidget *heatmapWidget_;
    QComboBox *heatmapDimensionCombo_;
    QComboBox *heatmapYearCombo_;

    // ---- 标签页 ----
    QTabWidget *tabWidget_;

    // ---- UI 构建 ----
    void setupUI();
    QFrame* buildPageHeader();
    QWidget* buildStatsTab();
    QWidget* buildTrendTab();
    QWidget* buildHeatmapTab();
    QFrame* createCard(const QString &title, const QString &objectName, QVBoxLayout **bodyLayout);

    // ---- 数据加载 ----
    void loadStats();
    void loadCategoryDistribution(const QList<CategoryStat> &stats);
    void loadSourceDistribution(const QList<SourceStat> &stats);
    void loadTrendChart(int year);
    void loadHeatmap(int year);

    // ---- 工具 ----
    void clearLayout(QLayout *layout);

    QString categoryColorHex(const QString &category) const;
};
