/**
 * @file SidebarDelegate.h
 * @brief 侧边栏自定义委托
 *
 * 绘制选中项左侧 3px 蓝色（#2463EB）指示条（QSS 无法实现，必须自绘）。
 * sizeHint 返回 QSize(220, 48)。
 *
 * 依赖：Qt5 Widgets
 * 文档：qt_implementation_guide.md 第二节、coding.md 第九节
 */
#pragma once

#include <QStyledItemDelegate>

class SidebarDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit SidebarDelegate(QObject *parent = nullptr);
    ~SidebarDelegate() override;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
};
