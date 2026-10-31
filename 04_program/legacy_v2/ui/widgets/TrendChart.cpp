/**
 * @file TrendChart.cpp
 * @brief 月度任务趋势柱状图控件实现
 *
 * 绘制 12 根渐变柱子（1-12 月），高度按 count/maxCount 比例，
 * 柱上方显示数量，下方显示月份标签，含水平网格线。
 *
 * 依赖：ui/widgets/TrendChart.h
 * 文档：Phase 6 统计模块、qt_implementation_guide.md、coding.md 第七章
 */
#include "TrendChart.h"

#include <QBrush>
#include <QFont>
#include <QLinearGradient>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>

// ============================================================================
// 布局常量
// ============================================================================
namespace {
const int kTopMargin    = 34;   // 顶部留白（数量标签）
const int kBottomMargin = 32;   // 底部留白（月份标签）
const int kLeftMargin   = 12;   // 左侧留白
const int kRightMargin  = 12;   // 右侧留白
const int kMaxBarHeight = 200;  // 最大柱高
const int kBarGap       = 10;   // 柱间距
const int kGridLines    = 4;    // 水平网格线数量
}

// ============================================================================
// 构造
// ============================================================================

TrendChart::TrendChart(QWidget *parent)
    : QWidget(parent)
{
    // 初始化 12 个月数据为 0
    for (int m = 1; m <= 12; ++m) {
        MonthlyTrend item;
        item.month = m;
        item.count = 0;
        data_.append(item);
    }

    setMinimumHeight(kTopMargin + kMaxBarHeight + kBottomMargin);
    setFixedHeight(kTopMargin + kMaxBarHeight + kBottomMargin);
}

// ============================================================================
// 数据设置
// ============================================================================

void TrendChart::setData(const QList<MonthlyTrend> &data)
{
    data_.clear();
    // 确保始终有 12 条数据
    for (int m = 1; m <= 12; ++m) {
        MonthlyTrend item;
        item.month = m;
        item.count = 0;
        data_.append(item);
    }
    // 填充传入的数据
    for (const MonthlyTrend &t : data) {
        if (t.month >= 1 && t.month <= 12) {
            data_[t.month - 1].count = t.count;
        }
    }
    update();
}

// ============================================================================
// 尺寸提示
// ============================================================================

QSize TrendChart::sizeHint() const
{
    return QSize(560, kTopMargin + kMaxBarHeight + kBottomMargin);
}

QSize TrendChart::minimumSizeHint() const
{
    return QSize(400, kTopMargin + kMaxBarHeight + kBottomMargin);
}

// ============================================================================
// 月份标签
// ============================================================================

QString TrendChart::monthLabel(int month) const
{
    // "1月".."12月"
    return QStringLiteral("%1\u6708").arg(month);
}

// ============================================================================
// 绘制
// ============================================================================

void TrendChart::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();

    // 计算柱子区域
    int chartLeft = kLeftMargin;
    int chartRight = w - kRightMargin;
    int chartTop = kTopMargin;
    int chartBottom = chartTop + kMaxBarHeight;
    int chartWidth = chartRight - chartLeft;

    // 计算每根柱子宽度
    int barWidth = (chartWidth - 11 * kBarGap) / 12;
    if (barWidth < 16) {
        barWidth = 16;
    }

    // 求最大值
    int maxCount = 0;
    for (const MonthlyTrend &t : data_) {
        if (t.count > maxCount) {
            maxCount = t.count;
        }
    }

    // --- 水平网格线 ---
    painter.setPen(QPen(QColor("#F1F5F9"), 1));
    for (int i = 0; i <= kGridLines; ++i) {
        int y = chartTop + kMaxBarHeight * i / kGridLines;
        painter.drawLine(chartLeft, y, chartRight, y);
    }

    // --- 基线 ---
    painter.setPen(QPen(QColor("#E2E8F0"), 1));
    painter.drawLine(chartLeft, chartBottom, chartRight, chartBottom);

    // --- 柱子 ---
    QFont countFont(QStringLiteral("Segoe UI"), 10);
    countFont.setWeight(QFont::Normal);
    countFont.setStyleName(QStringLiteral("Normal"));
    QFont labelFont(QStringLiteral("Segoe UI"), 10);
    labelFont.setStyleName(QStringLiteral("Normal"));

    for (int i = 0; i < 12; ++i) {
        int count = data_[i].count;
        int barX = chartLeft + i * (barWidth + kBarGap) + kBarGap / 2;
        int barHeight = 0;
        if (maxCount > 0) {
            barHeight = static_cast<int>(static_cast<double>(count) / maxCount * kMaxBarHeight);
        }
        int barY = chartBottom - barHeight;
        QRect barRect(barX, barY, barWidth, barHeight);

        // 渐变填充：上 #60A5FA → 下 #2463EB
        if (barHeight > 0) {
            QLinearGradient gradient(barX, barY, barX, chartBottom);
            gradient.setColorAt(0.0, QColor("#60A5FA"));
            gradient.setColorAt(1.0, QColor("#2463EB"));
            painter.setBrush(QBrush(gradient));
            painter.setPen(Qt::NoPen);
            // 圆角矩形（顶部圆角，底部直角）
            // barHeight 至少为 1 像素才可见
            int radius = qMin(4, barHeight / 2);
            painter.drawRoundedRect(barRect, radius, radius);

            // 如果底部圆角不想要，用 drawRect 覆盖底部
            if (barHeight > radius * 2) {
                painter.drawRect(barX, barY + radius, barWidth, barHeight - radius);
            }
        }

        // 数量标签（柱子上方）
        if (count > 0) {
            painter.setPen(QColor("#475569"));
            painter.setFont(countFont);
            QRect countRect(barX - 4, barY - 22, barWidth + 8, 18);
            painter.drawText(countRect, Qt::AlignCenter, QString::number(count));
        }

        // 月份标签（柱子下方）
        painter.setPen(QColor("#94A3B8"));
        painter.setFont(labelFont);
        QRect labelRect(barX - 4, chartBottom + 6, barWidth + 8, 20);
        painter.drawText(labelRect, Qt::AlignCenter, monthLabel(i + 1));
    }
}
