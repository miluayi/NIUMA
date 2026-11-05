#pragma once
#include <QAbstractItemView>
#include <QCompleter>
#include <QEvent>
#include <QFrame>
#include <QLayout>
#include <QPainter>
#include <QPointer>
#include "NativePopupFrame.h"

// One alpha surface owns the border and shadow. Neither the native popup frame
// nor the list's rectangular viewport may paint over its rounded corners.
class ComboPopupSurface final : public QObject {
public:
    static void install(QWidget *popup, QAbstractItemView *view, QCompleter *completer=nullptr) {
        if (!popup || popup->property("appUiPopupSurface").toBool()) return;
        popup->setProperty("appUiPopupSurface",true);
        popup->setAttribute(Qt::WA_DontCreateNativeAncestors);
        popup->setWindowFlags(popup->windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
        popup->setAttribute(Qt::WA_TranslucentBackground);
        popup->setAutoFillBackground(false);
        popup->setObjectName(QStringLiteral("appUiComboPopup"));
        const bool completion=popup==view;
        popup->setStyleSheet(completion ? QStringLiteral(
            "QAbstractItemView#appUiComboPopup { background:transparent; border:none;"
            " margin:8px; padding:5px; border-radius:0; outline:none; }") : QStringLiteral(
            "QFrame#appUiComboPopup { background:transparent; border:none; }"
            "QFrame#appUiComboPopup QAbstractItemView { background:#FFFFFF; border:none;"
            " border-radius:0; padding:0; outline:none; }"));
        if (!completion) {
            if (auto *frame=qobject_cast<QFrame *>(popup)) frame->setFrameShape(QFrame::NoFrame);
            popup->setContentsMargins(13,13,13,13);
            if (popup->layout()) popup->layout()->setContentsMargins(0,0,0,0);
        }
        // The viewport is safely inside the arc, so selected rows and scrollbars
        // cannot turn the outside corners opaque (including on Windows 7).
        view->viewport()->setAutoFillBackground(true);
        QPalette palette=view->viewport()->palette();
        palette.setColor(QPalette::Window,Qt::white);
        palette.setColor(QPalette::Base,Qt::white);
        view->viewport()->setPalette(palette);
#ifdef Q_OS_WIN
        WindowsChrome::prepare(popup);
#endif
        auto *surface=new ComboPopupSurface(popup,completer);
        popup->installEventFilter(surface);
        if (completer) {
            // QCompleter's Qt5 height formula assumes a six-pixel frame. Reserve
            // our margins too, so even a single suggestion stays fully visible.
            auto *model=view->model();
            connect(model,&QAbstractItemModel::modelReset,surface,[surface] { surface->updateCompletionHeight(); });
            connect(model,&QAbstractItemModel::rowsInserted,surface,[surface] { surface->updateCompletionHeight(); });
            connect(model,&QAbstractItemModel::rowsRemoved,surface,[surface] { surface->updateCompletionHeight(); });
            connect(model,&QAbstractItemModel::layoutChanged,surface,[surface] { surface->updateCompletionHeight(); });
            surface->updateCompletionHeight();
        }
    }
protected:
    bool eventFilter(QObject *, QEvent *event) override {
        if (event->type()==QEvent::FontChange || event->type()==QEvent::StyleChange)
            updateCompletionHeight();
        if (event->type()==QEvent::Show || event->type()==QEvent::WinIdChange)
            NativePopupFrame::apply(popup_);
        if (event->type()!=QEvent::Paint) return false;
        QPainter painter(popup_);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillRect(popup_->rect(),Qt::transparent);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        const QRectF body=QRectF(popup_->rect()).adjusted(8.5,8.5,-8.5,-8.5);
        // Nested low-alpha rounded shapes form a soft shadow entirely inside
        // the layered window, independent of Aero/DWM and system menu shadows.
        painter.setPen(Qt::NoPen);
        for (int spread=7; spread>=1; --spread) {
            painter.setBrush(QColor(30,45,65,spread<4 ? 7 : 3));
            painter.drawRoundedRect(body.translated(0,1).adjusted(-spread,-spread,spread,spread),
                                    8+spread,8+spread);
        }
        painter.setPen(QPen(QColor("#DCE3EF"),1));
        painter.setBrush(Qt::white);
        painter.drawRoundedRect(body,8,8);
        return true; // Suppress QFrame / platform menu panel painting.
    }
private:
    explicit ComboPopupSurface(QWidget *popup,QCompleter *completer)
        : QObject(popup),popup_(popup),completer_(completer) {}
    void updateCompletionHeight() {
        if (!completer_) return;
        auto *view=static_cast<QAbstractItemView *>(popup_);
        if (!view->model()) return;
        const int rows=qMin(completer_->maxVisibleItems(),view->model()->rowCount());
        popup_->setMinimumHeight(qMax(0,view->sizeHintForRow(0))*rows+26);
    }
    QWidget *popup_;
    QPointer<QCompleter> completer_;
};
