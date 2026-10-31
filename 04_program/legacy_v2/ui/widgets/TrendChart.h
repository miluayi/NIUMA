/**
 * @file TrendChart.h
 * @brief 月度任务趋势柱状图控件（QPainter 自绘）
 *
 * 12 根纵向柱子（1-12 月），高度按 count 比例，渐变色 #60A5FA→#2463EB，
 * 圆角顶部，柱上方显示数量，下方显示月份标签。固定高度 ~280px。
 *
 * 依赖：Qt5 Widgets（QPainter）、models/Statistics.h（MonthlyTrend 结构）
 * 文档：Phase 6 统计模块、qt_implementation_guide.md、coding.md 第七章
 */
#pragma once

#include <QWidget>
#include <QList>

#include "models/Statistics.h"

class TrendChart : public QWidget {
    Q_OBJECT

public:
    explicit TrendChart(QWidget *parent = nullptr);

    /// 设置月度趋势数据（12 条），触发重绘
    void setData(const QList<MonthlyTrend> &data);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QList<MonthlyTrend> data_;  // 12 条月度数据

    /// 月份标签（1-12 → "1月".."12月"）
    QString monthLabel(int month) const;
};
