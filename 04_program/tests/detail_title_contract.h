#pragma once
#include <QLabel>
#include <QTextLayout>
#include <QPlainTextEdit>
#include <QTextBlock>
#include <QScrollBar>

inline bool verifyThreeLineTitle(QLabel *label) {
    if (!label || !label->wordWrap() || label->text().size()!=45) return false;
    if (label->heightForWidth(label->width())>label->height()) {
        fprintf(stderr,"Title overflow: required=%d actual=%d width=%d indent=%d\n",label->heightForWidth(label->width()),
            label->height(),label->width(),label->indent());return false;
    }
    QTextLayout layout(label->text(),label->font());
    layout.beginLayout();
    int characters=0;
    qreal height=0;
    for (int row=0;row<3;++row) {
        QTextLine line=layout.createLine();
        if (!line.isValid()) return false;
        line.setLineWidth(label->contentsRect().width());
        if (line.textLength()!=15) return false;
        characters+=line.textLength();
        height+=line.height();
    }
    const bool fits=!layout.createLine().isValid() && characters==45
        && height<=label->contentsRect().height();
    layout.endLayout();
    return fits;
}

inline bool verifyThreeLineNameEdit(QPlainTextEdit *edit) {
    if (!edit || edit->toPlainText().size()!=45) return false;
    if (edit->verticalScrollBar()->maximum()!=0) {
        const auto *lines=edit->document()->firstBlock().layout();
        fprintf(stderr,"Name overflow: scroll=%d viewport=%dx%d size=%dx%d margins=%d,%d,%d,%d\n",
            edit->verticalScrollBar()->maximum(),edit->viewport()->width(),edit->viewport()->height(),edit->width(),edit->height(),
            edit->contentsMargins().left(),edit->contentsMargins().top(),edit->contentsMargins().right(),edit->contentsMargins().bottom());
        fprintf(stderr,"Name fonts: widget=%s %d doc=%s %d, lines=%d\n",qPrintable(edit->font().family()),edit->font().pixelSize(),
            qPrintable(edit->document()->defaultFont().family()),edit->document()->defaultFont().pixelSize(),lines->lineCount());
        for(int i=0;i<lines->lineCount();++i) fprintf(stderr,"line=%d length=%d width=%f height=%f\n",i,
            lines->lineAt(i).textLength(),lines->lineAt(i).width(),lines->lineAt(i).height());
        return false;
    }
    const QTextLayout *layout=edit->document()->firstBlock().layout();
    if (!layout || layout->lineCount()!=3) {
        fprintf(stderr,"Name layout: lines=%d viewport=%dx%d margins=%d,%d,%d,%d\n",layout ? layout->lineCount() : -1,
            edit->viewport()->width(),edit->viewport()->height(),edit->contentsMargins().left(),edit->contentsMargins().top(),
            edit->contentsMargins().right(),edit->contentsMargins().bottom());return false;
    }
    for (int row=0;row<3;++row) if (layout->lineAt(row).textLength()!=15) {
        fprintf(stderr,"Name line %d: %d characters viewport=%d\n",row,layout->lineAt(row).textLength(),edit->viewport()->width());return false;
    }
    return true;
}
