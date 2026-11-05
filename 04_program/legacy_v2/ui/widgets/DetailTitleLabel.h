#pragma once
#include <QLabel>
#include <QEvent>
#include <QFontMetrics>
#include <QVariant>

// Reserve fifteen full-width Chinese characters per line and three lines.
// Recompute before layout when the user's font size or the theme changes.
class DetailTitleLabel final : public QLabel {
public:
    explicit DetailTitleLabel(QWidget *parent=nullptr) : QLabel(parent) {
        setTextFormat(Qt::PlainText);
        setWordWrap(true);
        setIndent(0);
        setMargin(0);
        setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
        setProperty("appUiNoLabelReset",true);
        updateTextArea();
    }
protected:
    bool event(QEvent *event) override {
        const bool result=QLabel::event(event);
        if (event->type()==QEvent::FontChange || event->type()==QEvent::StyleChange
            || event->type()==QEvent::Polish) updateTextArea();
        return result;
    }
private:
    void updateTextArea() {
        const QFontMetrics metrics(font());
        setFixedSize(metrics.horizontalAdvance(QString(15,QChar(0x725B)))+12,
                     metrics.lineSpacing()*3+12);
    }
};
