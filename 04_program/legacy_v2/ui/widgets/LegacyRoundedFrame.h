#pragma once
#include <QApplication>
#include <QEvent>
#include <QPainter>
#include <QPainterPath>
#include <QStyle>
#include <QStyleOption>
#include <QTimer>
#include <QWidget>
#include <array>

// Paint only eight narrow edge pieces. No full-window effect or input overlay.
// Clearing the four outside corners preserves antialiasing in the layered HWND.
class LegacyFrameEdge final : public QWidget {
public:
    explicit LegacyFrameEdge(QWidget *parent) : QWidget(parent) {
        setObjectName(QStringLiteral("legacyFrameEdge"));
        setAttribute(Qt::WA_DontCreateNativeAncestors);
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setFocusPolicy(Qt::NoFocus);
    }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const bool square=parentWidget()->isMaximized() || parentWidget()->isFullScreen();
        QPainterPath outline;
        outline.addRoundedRect(QRectF(parentWidget()->rect()).adjusted(.5,.5,-.5,-.5),
                               square ? 0 : 12,square ? 0 : 12);
        QPainterPath outside;
        outside.addRect(QRectF(parentWidget()->rect()));
        painter.translate(-pos());
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.fillPath(outside.subtracted(outline),Qt::transparent);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.setPen(QPen(QColor(QStringLiteral("#BBC3D0")),1));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(outline);
    }
};

class LegacyRoundedFrame final : public QObject {
public:
    static void install(QWidget *window) {
        if (!window || window->property("legacyFrameInstalled").toBool()) return;
        window->setProperty("legacyFrameInstalled",true);
        window->installEventFilter(new LegacyRoundedFrame(window));
    }
protected:
    bool eventFilter(QObject *,QEvent *event) override {
        if (event->type()==QEvent::Paint) {
            // Translucent windows disable Qt's automatic opaque background.
            // Fill the normal surface before children; only the corners are clear.
            QPainter painter(window_);
            painter.fillRect(window_->rect(),window_->palette().window());
            QStyleOption option;
            option.initFrom(window_);
            window_->style()->drawPrimitive(QStyle::PE_Widget,&option,&painter,window_);
        }
        if (event->type()==QEvent::Show || event->type()==QEvent::Resize
            || event->type()==QEvent::WindowStateChange) arrange();
        if (event->type()==QEvent::ChildAdded && !pending_) {
            pending_=true;
            QTimer::singleShot(0,this,[this] { pending_=false; arrange(); });
        }
        return false;
    }
private:
    explicit LegacyRoundedFrame(QWidget *window) : QObject(window),window_(window) {}
    void arrange() {
        if (!window_->isVisible()) return;
        for (auto *&edge : edges_) if (!edge) edge=new LegacyFrameEdge(window_);
        const int w=window_->width(),h=window_->height();
        const int c=qMin(16,qMin(w,h)/2);
        const std::array<QRect,8> rects={QRect(0,0,c,c),QRect(w-c,0,c,c),
            QRect(0,h-c,c,c),QRect(w-c,h-c,c,c),QRect(c,0,w-2*c,1),
            QRect(c,h-1,w-2*c,1),QRect(0,c,1,h-2*c),QRect(w-1,c,1,h-2*c)};
        for (size_t index=0; index<edges_.size(); ++index) {
            edges_[index]->setGeometry(rects[index]);
            edges_[index]->show();
            edges_[index]->raise();
            edges_[index]->update();
        }
    }
    QWidget *window_;
    std::array<LegacyFrameEdge *,8> edges_{};
    bool pending_=false;
};
