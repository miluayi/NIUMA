/**
 * @file SidebarDelegate.cpp
 * @brief 侧边栏自定义委托实现
 *
 * 先调用基类 paint 绘制 QSS 定义的背景/文字/图标，
 * 再在选中项左侧绘制 3px 宽、20px 高的蓝色（#2463EB）指示条。
 *
 * 依赖：ui/widgets/SidebarDelegate.h、ui/ColorPalette.h
 * 文档：qt_implementation_guide.md 第二节、coding.md 第九节
 */
#include "SidebarDelegate.h"

#include <QPainter>
#include <QStyle>

#include "ColorPalette.h"

SidebarDelegate::SidebarDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

SidebarDelegate::~SidebarDelegate() = default;

void SidebarDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    // Base class draws background, text, icon per QSS rules
    QStyledItemDelegate::paint(painter, option, index);

    // Draw 3px blue indicator bar on left side for selected items
    if (option.state & QStyle::State_Selected) {
        painter->save();
        painter->setPen(Qt::NoPen);
        painter->setBrush(ColorPalette::primary);
        QRect barRect(option.rect.left(), option.rect.center().y() - 10, 3, 20);
        painter->drawRect(barRect);
        painter->restore();
    }
}

QSize SidebarDelegate::sizeHint(const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    return QSize(220, 48);
}
