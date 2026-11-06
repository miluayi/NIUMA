#pragma once
#include <QLabel>
#include <QApplication>
#include <QVariant>
#include <QPainter>
#include <QStyle>
#include "TaskItemDelegate.h"

// The task-list renderer is the single source for category colors, size,
// rounded shape, font and visible-glyph centering on every surface.
class CategoryBadgeLabel final : public QLabel {
public:
    explicit CategoryBadgeLabel(const QString &category,QWidget *parent=nullptr) : QLabel(parent) {
        setObjectName(QStringLiteral("appUiCategoryTag"));
        setProperty("appUiNoLabelReset",true);
        setStyleSheet("background:transparent; border:none; padding:0;");
        setTextFormat(Qt::PlainText);
        setIndent(0);
        setAlignment(Qt::AlignCenter);
        setTextInteractionFlags(Qt::TextSelectableByMouse);
        setFocusPolicy(Qt::ClickFocus);
        QFont font(QStringLiteral("Segoe UI"));
        font.setPixelSize(14);
        setFont(font);
        setCategory(category);
    }
    void setCategory(const QString &category) {
        setText(category);
        setProperty("categoryText",category);
        setFixedSize(TaskItemDelegate::categoryBadgeWidth(category),24);
    }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        TaskItemDelegate::paintCategoryBadge(&painter,rect(),text(),!selectedText().isEmpty());
    }
};

class CategoryBadgeDelegate final : public QStyledItemDelegate, public CopyTextProvider {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter *painter,const QStyleOptionViewItem &option,const QModelIndex &index) const override {
        QStyleOptionViewItem background=option;
        initStyleOption(&background,index);
        background.text.clear();
        const QStyle *style=option.widget ? option.widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem,&background,painter,option.widget);
        TaskItemDelegate::paintCategoryBadge(painter,option.rect,index.data().toString());
    }
    CopyTextLayout copyTextLayout(const QStyleOptionViewItem &option,const QModelIndex &index) const override {
        return TaskItemDelegate::categoryTextLayout(option.rect,index.data().toString(),
            option.widget ? option.widget->devicePixelRatioF() : 1);
    }
};
