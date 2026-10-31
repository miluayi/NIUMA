#pragma once
#include <QApplication>
#include <QAbstractNativeEventFilter>
#include <QWidget>
#include <QPointer>
#include <QVariant>
#include "WindowsChrome.h"
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dwmapi.h>

namespace NativePopupFrame {
inline const wchar_t *propertyName() { return L"NIUMA.NativePopupFrame"; }
class Events final : public QObject, public QAbstractNativeEventFilter {
public:
    explicit Events(QObject *parent) : QObject(parent) {}
    bool nativeEventFilter(const QByteArray &, void *message, long *result) override {
        auto *msg = static_cast<MSG *>(message);
        if (!GetPropW(msg->hwnd, propertyName())) return false;
        if (auto *widget=QWidget::find(reinterpret_cast<WId>(msg->hwnd))) {
            if (WindowsChrome::suppressLegacyPainting(widget,msg->message,result)) return true;
            if (msg->message==0x02E0 || msg->message==WM_THEMECHANGED
                || msg->message==WM_DWMCOMPOSITIONCHANGED || msg->message==WM_SETTINGCHANGE)
                WindowsChrome::scheduleFrameSync(widget); // WM_DPICHANGED is absent on 7.
            if (msg->message==WM_NCCALCSIZE && widget->property("appUiQtCustomFrame").toBool()) return false;
        }
        if (msg->message == WM_NCCALCSIZE || msg->message == WM_NCHITTEST) {
            *result = msg->message == WM_NCHITTEST ? HTCLIENT : 0;
            return true;
        }
        return false;
    }
};

inline void apply(QWidget *widget)
{
    if (!widget || !widget->isWindow()
        || (widget->windowType() != Qt::Dialog && widget->windowType() != Qt::Popup)) return;
    // winId() must never promote the popup's ordinary widget ancestors/siblings.
    WindowsChrome::prepare(widget);
    // Show is delivered before Qt marks a widget visible. Only use an existing
    // handle: neither hidden preparation nor HWND teardown may create windows.
    if (!widget->internalWinId()
        || widget->property("nativePopupApplying").toBool()) return;
    widget->setProperty("nativePopupApplying", true);
    static QPointer<Events> events;
    if (!events) { events = new Events(qApp); qApp->installNativeEventFilter(events); }
    const HWND window = reinterpret_cast<HWND>(widget->internalWinId());
    const auto handle = static_cast<qulonglong>(reinterpret_cast<quintptr>(window));
    if (widget->property("nativePopupHandle").toULongLong() != handle) {
        SetPropW(window, propertyName(), reinterpret_cast<HANDLE>(1));
        WindowsChrome::configure(widget);
        widget->setProperty("nativePopupHandle", handle);
    }
    if (!widget->mask().isEmpty()) widget->clearMask();
    widget->setProperty("nativePopupApplying", false);
}
}
#else
namespace NativePopupFrame { inline void apply(QWidget *) {} }
#endif
