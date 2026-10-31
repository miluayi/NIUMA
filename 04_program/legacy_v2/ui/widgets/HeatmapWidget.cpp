/**
 * @file HeatmapWidget.cpp
 * @brief 工作密度热力图控件实现
 *
 * 绘制 12 行（月）× 变长列（每月实际天数）网格，5 级颜色映射，
 * 未经过的天（当前年的未来日期）不绘制以表达"未激活"。
 * 悬停 tooltip 显示"X月Y日: N 项任务"，右下角图例。
 *
 * 依赖：ui/widgets/HeatmapWidget.h
 * 文档：Phase 6 统计模块、qt_implementation_guide.md、coding.md 第七章
 */
#include "HeatmapWidget.h"

#include <QBrush>
#include <QFont>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QToolTip>

// ============================================================================
// 布局常量
// ============================================================================
namespace {
const int kLeftMargin   = 48;   // 月份标签宽度
const int kTopMargin    = 28;   // 顶部留白（日期刻度标签区）
const int kCellSize     = 22;   // 单元格边长（放大以填充页面）
const int kCellGap      = 3;    // 单元格间距
const int kRowGap       = 8;    // 行间距（月与月之间）
const int kBottomMargin = 40;   // 图例区域高度
const int kRightPadding = 16;   // 右侧留白
}

// ============================================================================
// 构造
// ============================================================================

HeatmapWidget::HeatmapWidget(QWidget *parent)
    : QWidget(parent)
    , year_(QDate::currentDate().year())
    , today_(QDate::currentDate())
{
    // 初始化网格与每月天数
    for (int m = 0; m < 12; ++m) {
        daysInMonth_[m] = QDate(year_, m + 1, 1).daysInMonth();
        for (int d = 0; d < 31; ++d) {
            grid_[m][d] = 0;
        }
    }

    setMouseTracking(true);  // 启用 hover 检测
    // 设置最小高度（基于默认单元格尺寸 22px，确保 12 个月完整显示）
    int minHeight = kTopMargin + 12 * (22 + 8) - 8 + kBottomMargin;
    setMinimumHeight(minHeight);
    // 设置 size policy 让控件可扩展，填充可用空间
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

// ============================================================================
// 数据设置
// ============================================================================

void HeatmapWidget::setData(const QList<HeatmapData> &data, int year)
{
    year_ = year;
    today_ = QDate::currentDate();

    // 重置每月天数与网格
    for (int m = 0; m < 12; ++m) {
        daysInMonth_[m] = QDate(year_, m + 1, 1).daysInMonth();
        for (int d = 0; d < 31; ++d) {
            grid_[m][d] = 0;
        }
    }

    // 聚合：按 (月份, 日) 累加 count
    for (const HeatmapData &item : data) {
        if (!item.date.isValid()) {
            continue;
        }
        int month = item.date.month();  // 1-12
        int day = item.date.day();      // 1-31
        if (month >= 1 && month <= 12 && day >= 1 && day <= 31) {
            grid_[month - 1][day - 1] += item.count;
        }
    }

    update();
}

// ============================================================================
// 尺寸提示
// ============================================================================

QSize HeatmapWidget::sizeHint() const
{
    // 默认尺寸：基于 22px 单元格
    int width = kLeftMargin + 31 * (22 + 3) - 3 + kRightPadding;
    int height = kTopMargin + 12 * (22 + 8) - 8 + kBottomMargin;
    return QSize(width, height);
}

QSize HeatmapWidget::minimumSizeHint() const
{
    return QSize(kLeftMargin + 28 * (14 + 3) - 3 + kRightPadding,
                 kTopMargin + 12 * (14 + 8) - 8 + kBottomMargin);
}

// ============================================================================
// 颜色等级
// ============================================================================

int HeatmapWidget::colorLevel(int count) const
{
    if (count == 0) return 0;
    if (count <= 2) return 1;
    if (count <= 5) return 2;
    if (count <= 9) return 3;
    return 4;
}

QColor HeatmapWidget::levelColor(int level) const
{
    switch (level) {
        case 0:  return QColor("#F1F5F9");
        case 1:  return QColor("#D1FAE5");
        case 2:  return QColor("#6EE7B7");
        case 3:  return QColor("#34D399");
        case 4:  return QColor("#059669");
        default: return QColor("#F1F5F9");
    }
}

// ============================================================================
// 绘制
// ============================================================================

void HeatmapWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 动态计算单元格大小：让 31 天的宽度填满控件可用宽度
    int availableWidth = width() - kLeftMargin - kRightPadding;
    // 单元格+间距 = availableWidth / 31；cellSize = (avail/31) - gap
    int cellGap = 3;
    int cellSize = (availableWidth - cellGap * 30) / 31;
    if (cellSize < 14) cellSize = 14;  // 最小单元格尺寸
    int rowGap = 8;

    QFont labelFont(QStringLiteral("Segoe UI"), 10);
    labelFont.setStyleName(QStringLiteral("Normal"));
    painter.setFont(labelFont);
    painter.setPen(QColor("#0F172A"));

    // --- 月份标签（左侧，1月..12月）---
    for (int m = 0; m < 12; ++m) {
        int y = kTopMargin + m * (cellSize + rowGap) + cellSize / 2;
        painter.drawText(QRect(0, y - 9, kLeftMargin - 6, 18),
                         Qt::AlignRight | Qt::AlignVCenter,
                         QStringLiteral("%1\u6708").arg(m + 1));  // N月
    }

    // --- 单元格 ---
    cells_.clear();
    painter.setPen(Qt::NoPen);

    // 第一遍：绘制周末背景（浅红粉色，与绿色热力图对比明显）
    for (int m = 0; m < 12; ++m) {
        int days = daysInMonth_[m];
        int y = kTopMargin + m * (cellSize + rowGap);

        for (int d = 0; d < days; ++d) {
            int day = d + 1;
            int x = kLeftMargin + d * (cellSize + cellGap);
            QDate cellDate(year_, m + 1, day);
            int dow = cellDate.dayOfWeek();  // 1=Mon..7=Sun
            if (dow == 6 || dow == 7) {
                // 周末背景：比单元格稍大，形成衬底效果
                QRect bgRect(x - 2, y - 2, cellSize + 4, cellSize + 4);
                painter.setBrush(QBrush(QColor("#FCD9D6")));  // 浅红粉色
                painter.setPen(Qt::NoPen);
                painter.drawRoundedRect(bgRect, 4, 4);
            }
        }
    }

    // 第二遍：绘制单元格
    for (int m = 0; m < 12; ++m) {
        int days = daysInMonth_[m];
        int y = kTopMargin + m * (cellSize + rowGap);

        for (int d = 0; d < days; ++d) {
            int day = d + 1;
            int x = kLeftMargin + d * (cellSize + cellGap);
            QRect rect(x, y, cellSize, cellSize);

            // 判断是否为未经过的天（仅当前年份的未来日期不绘制）
            QDate cellDate(year_, m + 1, day);
            bool isActive = true;
            if (year_ == today_.year() && cellDate > today_) {
                isActive = false;
            }

            if (isActive) {
                int count = grid_[m][d];
                int level = colorLevel(count);
                painter.setBrush(QBrush(levelColor(level)));
                painter.setPen(Qt::NoPen);
                painter.drawRoundedRect(rect, 3, 3);

                CellInfo info;
                info.rect = rect;
                info.month = m + 1;
                info.day = day;
                info.count = count;
                info.active = true;
                cells_.append(info);
            } else {
                // 未经过的天：绘制为浅色虚框，表达"未激活"
                painter.setBrush(QBrush(QColor("#FAFBFC")));
                painter.setPen(Qt::NoPen);
                painter.drawRoundedRect(rect, 3, 3);
                painter.setPen(QPen(QColor("#E2E8F0"), 1, Qt::DashLine));
                painter.setBrush(Qt::NoBrush);
                painter.drawRoundedRect(rect, 3, 3);
                painter.setPen(Qt::NoPen);

                CellInfo info;
                info.rect = rect;
                info.month = m + 1;
                info.day = day;
                info.count = 0;
                info.active = false;
                cells_.append(info);
            }
        }
    }

    // --- 图例（右下角：少 → 5 色块 → 多）---
    int legendY = height() - kBottomMargin + 16;
    int legendCellSize = 16;
    int legendGap = 4;

    painter.setPen(QColor("#0F172A"));
    QFont legendFont(QStringLiteral("Segoe UI"), 10);
    legendFont.setStyleName(QStringLiteral("Normal"));
    painter.setFont(legendFont);

    // "少" 文字
    int legendWidth = 20 + 5 * (legendCellSize + legendGap) - legendGap + 20;
    int legendStartX = width() - legendWidth - kRightPadding;
    painter.drawText(QRect(legendStartX, legendY, 20, 16), Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("\u5C11"));  // 少

    // 5 个色块
    painter.setPen(Qt::NoPen);
    int blockX = legendStartX + 20;
    for (int lv = 0; lv < 5; ++lv) {
        QRect blockRect(blockX, legendY + 1, legendCellSize, legendCellSize);
        painter.setBrush(QBrush(levelColor(lv)));
        painter.drawRoundedRect(blockRect, 3, 3);
        blockX += legendCellSize + legendGap;
    }

    // "多" 文字
    painter.setPen(QColor("#0F172A"));
    painter.drawText(QRect(blockX, legendY, 20, 16), Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("\u591A"));  // 多
}

// ============================================================================
// 鼠标悬停
// ============================================================================

void HeatmapWidget::mouseMoveEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();
    for (const CellInfo &info : cells_) {
        if (info.rect.contains(pos)) {
            QString tip;
            if (info.active) {
                // "X月Y日: N 项任务"
                tip = QStringLiteral("%1\u6708%2\u65E5: %3 \u9879\u4EFB\u52A1")
                          .arg(info.month)
                          .arg(info.day)
                          .arg(info.count);
            } else {
                // "X月Y日（未经过）"
                tip = QStringLiteral("%1\u6708%2\u65E5\uFF08\u672A\u7ECF\u8FC7\uFF09")
                          .arg(info.month)
                          .arg(info.day);
            }
            QToolTip::showText(event->globalPos(), tip, this);
            return;
        }
    }
    QToolTip::hideText();
    QWidget::mouseMoveEvent(event);
}
