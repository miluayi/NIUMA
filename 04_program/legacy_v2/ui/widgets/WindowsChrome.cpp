#include "WindowsChrome.h"
#ifdef Q_OS_WIN
#include <QMargins>
#include <QWindow>
#include <QTimer>
#include <qpa/qplatformnativeinterface.h>
Q_DECLARE_METATYPE(QMargins)

void WindowsChrome::syncQtFrameMargins(QWidget *widget)
{
    QWindow *window=widget->windowHandle();
    auto *native=QGuiApplication::platformNativeInterface();
    if (!window || !window->handle() || !native) return;
    const HWND hwnd=reinterpret_cast<HWND>(widget->internalWinId());
    if (!hwnd) return;
    const DWORD style=static_cast<DWORD>(GetWindowLongPtrW(hwnd,GWL_STYLE));
    const DWORD extended=static_cast<DWORD>(GetWindowLongPtrW(hwnd,GWL_EXSTYLE));
    RECT border={0,0,0,0};
    using GetDpi=UINT (WINAPI *)(HWND);
    using Adjust=BOOL (WINAPI *)(LPRECT,DWORD,BOOL,DWORD,UINT);
    const HMODULE user=GetModuleHandleW(L"user32.dll");
    const auto getDpi=resolve<GetDpi>(user,"GetDpiForWindow");
    const auto adjust=resolve<Adjust>(user,"AdjustWindowRectExForDpi");
    BOOL adjusted=FALSE;
    if (getDpi && adjust) adjusted=adjust(&border,style,FALSE,extended,getDpi(hwnd));
    if (!adjusted && !AdjustWindowRectEx(&border,style,FALSE,extended)) return;
    const QMargins custom(border.left,border.top,-border.right,-border.bottom);
    const QString name=QStringLiteral("WindowsCustomMargins");
    if (widget->property("appUiQtCustomFrame").toBool()
        && qvariant_cast<QMargins>(native->windowProperty(window->handle(),name))==custom) return;
    // Let Qt handle NCCALCSIZE with these margins so its cached frame and the
    // native client area agree (essential for UpdateLayeredWindow and DPI).
    widget->setProperty("appUiQtCustomFrame",true);
    RECT bounds;
    GetWindowRect(hwnd,&bounds);
    native->setWindowProperty(window->handle(),name,QVariant::fromValue(custom));
    SetWindowPos(hwnd,nullptr,bounds.left,bounds.top,bounds.right-bounds.left,bounds.bottom-bounds.top,
                 SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);
}

void WindowsChrome::scheduleFrameSync(QWidget *widget)
{
    if (widget->property("qtFrameSyncPending").toBool()) return;
    widget->setProperty("qtFrameSyncPending",true);
    QTimer::singleShot(0,widget,[widget] {
        widget->setProperty("qtFrameSyncPending",false);
        syncQtFrameMargins(widget);
    });
}
#endif
