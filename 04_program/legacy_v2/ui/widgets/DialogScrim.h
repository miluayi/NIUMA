/**
 * @file DialogScrim.h
 * @brief 弹窗背景遮罩（dim scrim）
 *
 * 打开弹窗前在父窗口上覆盖半透明黑色遮罩，弹窗关闭后移除。
 * 效果类似 TRAE 客户端打开设置时背景变暗。
 *
 * 用法：
 *   DialogScrim scrim(parentWidget);
 *   scrim.show();
 *   dialog->exec();
 *   scrim.hide();  // 或随作用域自动析构
 *
 * 依赖：Qt5 Widgets
 */
#pragma once

#include <QWidget>

class DialogScrim {
public:
    explicit DialogScrim(QWidget *parent, int opacity = 120)
        : parent_(parent), scrim_(nullptr)
    {
        if (!parent_) return;
        scrim_ = new QWidget(parent_);
        // 半透明黑色背景，opacity 0-255
        int a = opacity;
        if (a < 0) a = 0;
        if (a > 255) a = 255;
        scrim_->setStyleSheet(
            QString("background: rgba(15, 23, 42, %1);").arg(a));
        scrim_->setGeometry(parent_->rect());
        scrim_->setAttribute(Qt::WA_TransparentForMouseEvents, false);
        scrim_->setVisible(false);
    }

    ~DialogScrim() {
        if (scrim_) {
            delete scrim_;
            scrim_ = nullptr;
        }
    }

    void show() {
        if (scrim_ && parent_) {
            scrim_->setGeometry(parent_->rect());
            scrim_->show();
            scrim_->raise();
        }
    }

    void hide() {
        if (scrim_) {
            scrim_->hide();
        }
    }

private:
    QWidget *parent_;
    QWidget *scrim_;
};
