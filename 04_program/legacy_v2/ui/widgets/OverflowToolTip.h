#pragma once
#include <QAbstractItemView>
#include <QHelpEvent>
#include <QLabel>
#include <QStyledItemDelegate>
#include <QToolTip>
#include <QTableView>
#include <QListView>
#include <QTreeView>
#include "CopyTextLayout.h"

// Use the same text geometry as painting/selection, including custom delegates.
// Evaluate on hover so resizing, scrolling and refreshed data cannot leave a
// stale tooltip behind. No model tooltip roles or per-cell caches are needed.
namespace OverflowToolTip {
class StyleOption final : public QStyledItemDelegate {
public:
    void initialize(QStyleOptionViewItem &option,const QModelIndex &index) const {
        initStyleOption(&option,index);
    }
};
inline bool handle(QWidget *widget,QHelpEvent *event) {
    QString text;
    QRect rect;
    bool clipped=false;
    auto *view=qobject_cast<QAbstractItemView *>(widget->parentWidget());
    if (view && widget==view->viewport() && view->objectName()!=QStringLiteral("sidebar")
        && !view->window()->property("appUiPopupSurface").toBool()) {
        const QModelIndex index=view->indexAt(event->pos());
        if (index.isValid() && !view->indexWidget(index)) {
            QStyleOptionViewItem option;
            option.initFrom(view);
            option.widget=view;
            option.rect=view->visualRect(index);
            if ((qobject_cast<QTableView *>(view) && static_cast<QTableView *>(view)->wordWrap())
                || (qobject_cast<QListView *>(view) && static_cast<QListView *>(view)->wordWrap())
                || (qobject_cast<QTreeView *>(view) && static_cast<QTreeView *>(view)->wordWrap()))
                option.features|=QStyleOptionViewItem::WrapText;
            StyleOption().initialize(option,index);
            CopyTextLayout layout;
            if (auto *provider=dynamic_cast<const CopyTextProvider *>(view->itemDelegate(index)))
                layout=provider->copyTextLayout(option,index);
            else {
                rect=view->style()->subElementRect(QStyle::SE_ItemViewItemText,&option,view);
                const int margin=view->style()->pixelMetric(QStyle::PM_FocusFrameHMargin,nullptr,view)+1;
                rect.adjust(margin,0,-margin,0);
                // SE_ItemViewItemText's height is based on the primary Latin
                // font. Qt draws fallback Chinese glyphs into the whole cell.
                rect.setTop(option.rect.top());
                rect.setBottom(option.rect.bottom());
                layout={option.text,option.font,rect,option.displayAlignment,
                        bool(option.features & QStyleOptionViewItem::WrapText),0};
            }
            QTextDocument document;
            layout.prepare(document);
            text=layout.text;
            rect=option.rect.intersected(widget->rect());
            const QRectF visible=layout.rect.intersected(rect);
            const QRectF painted(layout.origin(document),document.size());
            clipped=!visible.contains(painted);
        }
    } else if (auto *label=qobject_cast<QLabel *>(widget)) {
        bool listCell=false;
        for (auto *ancestor=label->parentWidget();ancestor;ancestor=ancestor->parentWidget()) {
            const QString name=ancestor->objectName();
            if (name=="FocusItem" || name=="InProgressItem" || name=="PeriodicReminderItem") {
                listCell=true; break;
            }
        }
        if (!listCell) return false;
        QTextDocument document;
        document.setDefaultFont(label->font());
        document.setDocumentMargin(0);
        if (label->textFormat()==Qt::RichText) document.setHtml(label->text());
        else document.setPlainText(label->text());
        rect=label->contentsRect().adjusted(label->margin(),label->margin(),-label->margin(),-label->margin());
        const int indent=label->indent()<0 && label->frameWidth()>0
            ? label->fontMetrics().horizontalAdvance(QLatin1Char('x'))/2-label->margin() : label->indent();
        if (indent>0) {
            if (label->alignment() & Qt::AlignLeft) rect.adjust(indent,0,0,0);
            if (label->alignment() & Qt::AlignRight) rect.adjust(0,0,-indent,0);
        }
        for (auto *ancestor=label->parentWidget();ancestor;ancestor=ancestor->parentWidget()) {
            rect=rect.intersected(QRect(label->mapFrom(ancestor,ancestor->contentsRect().topLeft()),
                                       ancestor->contentsRect().size()));
            if (ancestor->isWindow()) break;
        }
        if (label->wordWrap()) document.setTextWidth(rect.width());
        text=document.toPlainText();
        clipped=document.size().width()>rect.width() || document.size().height()>rect.height();
    } else return false;
    if (clipped && !text.isEmpty())
        QToolTip::showText(event->globalPos(),QStringLiteral("<p style='white-space:pre-wrap; margin:0'>%1</p>")
            .arg(text.toHtmlEscaped()),widget,rect);
    else QToolTip::hideText();
    event->accept();
    return true; // Suppress unconditional model tooltips on fully visible cells.
}
}
