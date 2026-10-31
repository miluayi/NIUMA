#pragma once

#include <QColor>
#include <QEvent>
#include <QPainter>
#include <QWidget>

// Paint the complete rounded outline above a table, whose viewport otherwise
// hides the one-pixel border at the corners of its parent card.
class RoundedCardBorder final : public QWidget {
public:
    explicit RoundedCardBorder(QWidget *card) : QWidget(card), card_(card)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setGeometry(card->rect());
        card->installEventFilter(this);
        show();
        raise();
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == card_ && (event->type() == QEvent::Resize
                                 || event->type() == QEvent::Show)) {
            setGeometry(card_->rect());
            raise();
        }
        return QWidget::eventFilter(watched, event);
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(QColor(QStringLiteral("#DCE3EF")), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 12, 12);
    }

private:
    QWidget *card_;
};
