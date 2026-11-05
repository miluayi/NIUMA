#pragma once
#include <QPlainTextEdit>
#include <QEvent>
#include <QFontMetrics>
#include <QTextDocument>
#include <QtMath>
#include <QKeyEvent>

class MultilineNameEdit final : public QPlainTextEdit {
public:
    explicit MultilineNameEdit(QWidget *parent=nullptr) : QPlainTextEdit(parent) {
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        updateTextArea();
    }
protected:
    void keyPressEvent(QKeyEvent *event) override {
        // Wrapping is visual; a task name remains a single logical line.
        // Let the dialog retain its existing Enter-to-save action.
        if (event->key()==Qt::Key_Return || event->key()==Qt::Key_Enter) {
            event->ignore(); return;
        }
        QPlainTextEdit::keyPressEvent(event);
    }
    bool event(QEvent *event) override {
        const bool result=QPlainTextEdit::event(event);
        if (event->type()==QEvent::FontChange || event->type()==QEvent::StyleChange
            || event->type()==QEvent::Polish) updateTextArea();
        return result;
    }
private:
    void updateTextArea() {
        const QFontMetrics metrics(font());
        const QMargins margins=contentsMargins();
        const int documentMargins=qCeil(2*document()->documentMargin())+2;
        const int textWidth=metrics.horizontalAdvance(QString(15,QChar(0x725B)))+documentMargins;
        // Segoe UI can have negative leading: lineSpacing alone reserves less
        // than QPlainTextEdit's actual ascent/descent and triggers a scrollbar.
        const int lineHeight=qMax(metrics.height(),metrics.lineSpacing());
        setFixedSize(textWidth+margins.left()+margins.right(),
                     lineHeight*3+margins.top()+margins.bottom()+documentMargins+12);
    }
};
