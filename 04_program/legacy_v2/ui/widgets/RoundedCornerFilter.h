/**
 * @file RoundedCornerFilter.h
 * @brief 无边框弹窗圆角遮罩事件过滤器
 *
 * 安装在无边框窗口上，在显示和尺寸变化时自动应用圆角遮罩。
 * 用法：dialog->installEventFilter(new RoundedCornerFilter(dialog));
 *
 * 依赖：Qt5 Widgets
 */
#pragma once

#include <QEvent>
#include <QObject>
#include <QPainterPath>
#include <QRegion>
#include <QWidget>
#include "NativePopupFrame.h"

class RoundedCornerFilter : public QObject {
public:
    explicit RoundedCornerFilter(int radius = 12, QObject *parent = nullptr)
        : QObject(parent), radius_(radius) {
        if (auto *widget = qobject_cast<QWidget *>(parent)) {
#ifdef Q_OS_WIN
            WindowsChrome::prepare(widget);
#else
            widget->setAttribute(Qt::WA_TranslucentBackground, false);
            widget->setAttribute(Qt::WA_StyledBackground, true);
#endif
        }
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
#ifdef Q_OS_WIN
        if (event->type() == QEvent::Show || event->type() == QEvent::WinIdChange)
            NativePopupFrame::apply(qobject_cast<QWidget *>(watched));
#else
        if (event->type() == QEvent::Resize || event->type() == QEvent::Show) {
            auto *widget = qobject_cast<QWidget*>(watched);
            if (widget) {
                widget->setAttribute(Qt::WA_TranslucentBackground, false);
                widget->setAttribute(Qt::WA_StyledBackground, true);
                QPainterPath path;
                path.addRoundedRect(QRectF(widget->rect()).adjusted(0.5, 0.5, -0.5, -0.5), radius_, radius_);
                widget->setMask(QRegion(path.toFillPolygon().toPolygon()));
            }
        }
#endif
        return QObject::eventFilter(watched, event);
    }

private:
    int radius_;
};
