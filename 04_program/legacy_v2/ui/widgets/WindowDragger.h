/**
 * @file WindowDragger.h
 * @brief 无边框弹窗标题栏拖拽工具
 *
 * 用法：在弹窗的标题栏 widget 上安装此事件过滤器即可实现拖拽移动弹窗。
 *   headerFrame->installEventFilter(new WindowDragger(this));
 *
 * 原理：拦截 mousePressEvent / mouseMoveEvent，计算全局位移并调用 window()->move()。
 *
 * 依赖：Qt5 Widgets
 */
#pragma once

#include <QEvent>
#include <QMouseEvent>
#include <QObject>
#include <QPoint>
#include <QWidget>

class WindowDragger : public QObject {
public:
    explicit WindowDragger(QObject *parent = nullptr)
        : QObject(parent), dragging_(false) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        auto *widget = qobject_cast<QWidget*>(watched);
        if (!widget) return QObject::eventFilter(watched, event);

        if (event->type() == QEvent::MouseButtonPress) {
            auto *me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                QWidget *win = widget->window();
                if (win) {
                    dragging_ = true;
                    dragPos_ = me->globalPos() - win->frameGeometry().topLeft();
                    event->accept();
                    return true;
                }
            }
        } else if (event->type() == QEvent::MouseMove) {
            auto *me = static_cast<QMouseEvent*>(event);
            if (dragging_ && (me->buttons() & Qt::LeftButton)) {
                QWidget *win = widget->window();
                if (win) {
                    win->move(me->globalPos() - dragPos_);
                    event->accept();
                    return true;
                }
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            if (dragging_) {
                dragging_ = false;
                event->accept();
                return true;
            }
        }
        return QObject::eventFilter(watched, event);
    }

private:
    bool dragging_;
    QPoint dragPos_;
};
