/**
 * @file HeatmapWidget.h
 * @brief 工作密度热力图控件（月份 × 天数 网格）
 *
 * 12 行（1-12 月）× 变长列（每月实际天数，28-31），每个单元格按 (月, 日) 聚合任务数。
 * 5 级颜色：#F1F5F9(0) → #D1FAE5(1-2) → #6EE7B7(3-5) → #34D399(6-9) → #059669(10+)
 * 未经过的天（年份为当前年且日期晚于今天）不绘制，留白以表达"未激活"。
 * 悬停显示 tooltip（月份 + 日 + 数量）。
 *
 * 依赖：Qt5 Widgets（QPainter/QToolTip）、models/Statistics.h（HeatmapData 结构）
 * 文档：Phase 6 统计模块、qt_implementation_guide.md、coding.md 第七章
 */
#pragma once

#include <QWidget>
#include <QList>
#include <QRect>
#include <QDate>

#include "models/Statistics.h"

class HeatmapWidget : public QWidget {
    Q_OBJECT

public:
    explicit HeatmapWidget(QWidget *parent = nullptr);

    /// 设置热力图数据（按日期分组）+ 当前年份（用于判断未经过的天），触发重绘
    void setData(const QList<HeatmapData> &data, int year);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    /// 12×31 聚合网格：grid_[month][day]，month: 0-11，day: 0-30
    int grid_[12][31];
    /// 每月天数（1-based：daysInMonth_[0]=1月天数）
    int daysInMonth_[12];
    /// 数据对应的年份
    int year_;
    /// 今天日期（用于判断未来日不绘制）
    QDate today_;

    /// 单元格矩形与信息（用于 hover 检测）
    struct CellInfo {
        QRect rect;
        int month;     // 1-12
        int day;       // 1-31
        int count;
        bool active;   // 是否已激活（已过去的天）
    };
    QList<CellInfo> cells_;

    /// 根据数量返回颜色等级（0-4）
    int colorLevel(int count) const;
    /// 根据等级返回颜色
    QColor levelColor(int level) const;
};
