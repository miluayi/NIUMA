#pragma once
#include <QAbstractTextDocumentLayout>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionViewItem>
#include <QStyle>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextLayout>
#include <QtMath>

struct CopyTextLayout {
    QString text;
    QFont font;
    QRectF rect;
    Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignVCenter;
    bool wrap = false;
    int margin = 0;
    bool nativeAlignment = false;
    bool fractionalOrigin = false;
    void prepare(QTextDocument &doc) const {
        doc.setDefaultFont(font);
        doc.setDocumentMargin(margin);
        doc.setPlainText(text);
        if (wrap) doc.setTextWidth(rect.width());
    }
    QPointF origin(QTextDocument &doc) const {
        const QSizeF size=doc.size();
        if(nativeAlignment)
            return QStyle::alignedRect(Qt::LeftToRight,alignment,size.toSize(),rect.toRect()).topLeft();
        qreal x=rect.left(),y=rect.top();
        if (alignment & Qt::AlignHCenter) x+=(rect.width()-size.width())/2;
        if (alignment & Qt::AlignRight) x+=rect.width()-size.width();
        if (alignment & Qt::AlignVCenter) y+=(rect.height()-size.height())/2;
        return fractionalOrigin ? QPointF(x,y) : QPointF(qFloor(x),qFloor(y));
    }
    int hit(QPointF point) const {
        QTextDocument doc;
        prepare(doc);
        return qBound(0,doc.documentLayout()->hitTest(point-origin(doc),Qt::FuzzyHit),text.size());
    }
    void highlight(QPainter &painter,int start,int length) const {
        if (!length) return;
        QTextDocument doc;
        prepare(doc);
        const QPointF offset=origin(doc);
        QPainterPath clip;
        for (QTextBlock block=doc.begin();block.isValid();block=block.next()) {
            auto *layout=block.layout();
            for (int i=0;i<layout->lineCount();++i) {
                const QTextLine line=layout->lineAt(i);
                const int first=qMax(start-block.position(),line.textStart());
                const int last=qMin(start+length-block.position(),line.textStart()+line.textLength());
                if (last<=first) continue;
                const qreal a=line.cursorToX(first),b=line.cursorToX(last);
                clip.addRect(QRectF(qMin(a,b),line.y(),qAbs(b-a),line.height()).translated(layout->position()+offset));
            }
        }
        painter.save();
        painter.setClipRect(rect,Qt::IntersectClip);
        painter.setClipPath(clip,Qt::IntersectClip);
        painter.translate(offset);
        QAbstractTextDocumentLayout::PaintContext context;
        QAbstractTextDocumentLayout::Selection selection;
        selection.cursor=QTextCursor(&doc);
        selection.cursor.setPosition(start);
        selection.cursor.setPosition(start+length,QTextCursor::KeepAnchor);
        selection.format.setBackground(QColor("#2463EB"));
        selection.format.setForeground(Qt::white);
        context.selections.append(selection);
        doc.documentLayout()->draw(&painter,context);
        painter.restore();
    }
};
class CopyTextProvider {
public:
    virtual ~CopyTextProvider()=default;
    virtual CopyTextLayout copyTextLayout(const QStyleOptionViewItem &,const QModelIndex &) const=0;
};
