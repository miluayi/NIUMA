#pragma once

#include <QPainter>
#include <QWidget>

// An input-transparent overlay keeps the edge visible over all shell children.
class WindowOutline : public QWidget {
public:
    explicit WindowOutline(QWidget *parent) : QWidget(parent) {
        setObjectName("windowOutline");
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setGeometry(parent->rect());
        show();
        raise();
    }
protected:
    void paintEvent(QPaintEvent *) override {
        if (window()->property("appUiNativeCorners").toBool()) return;
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(Qt::NoBrush);
        const qreal radius = 0; // Older Windows uses its native square outline.
        painter.setPen(QColor("#BBC3D0"));
        painter.drawRoundedRect(QRectF(rect()).adjusted(.5, .5, -.5, -.5), radius, radius);
        painter.setPen(QColor(60, 75, 100, 20));
        painter.drawRoundedRect(QRectF(rect()).adjusted(1.5, 1.5, -1.5, -1.5), radius, radius);
    }
};
