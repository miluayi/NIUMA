#pragma once
#include <QApplication>
#include <QVariant>
#include <QWidget>
#include "LegacyRoundedFrame.h"
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dwmapi.h>
#include <cstring>

namespace WindowsChrome {
void syncQtFrameMargins(QWidget *widget);
void scheduleFrameSync(QWidget *widget);
template<class Function> inline Function resolve(HMODULE module,const char *name) {
    const FARPROC address=module ? GetProcAddress(module,name) : nullptr;
    Function function=nullptr;
    static_assert(sizeof(function)==sizeof(address),"Windows function pointer size");
    std::memcpy(&function,&address,sizeof(function));
    return function;
}
inline bool nativeCornersSupported() {
    using Version=LONG (WINAPI *)(OSVERSIONINFOW *);
    const auto version=resolve<Version>(GetModuleHandleW(L"ntdll.dll"),"RtlGetVersion");
    OSVERSIONINFOW info={};
    info.dwOSVersionInfoSize=sizeof(info);
    if (!version || version(&info)!=0 || info.dwMajorVersion<10 || info.dwBuildNumber<22000) return false;
    using Composition=HRESULT (WINAPI *)(BOOL *);
    const auto composition=resolve<Composition>(GetModuleHandleW(L"dwmapi.dll"),"DwmIsCompositionEnabled");
    BOOL enabled=FALSE;
    return composition && SUCCEEDED(composition(&enabled)) && enabled;
}
inline bool legacy() {
    // The application property is used only by contract-test processes.
    return qApp->property("niumaLegacyWindowChrome").toBool() || !nativeCornersSupported();
}
inline void prepare(QWidget *widget) {
    if (!widget) return;
    widget->setAttribute(Qt::WA_DontCreateNativeAncestors);
    const bool popupSurface=widget->property("appUiPopupSurface").toBool();
    const bool fallback=popupSurface || legacy();
    widget->setProperty("appUiLegacyCorners",fallback);
    widget->setAttribute(Qt::WA_TranslucentBackground,fallback);
    widget->setAttribute(Qt::WA_StyledBackground,true);
    if (fallback && !popupSurface) {
        if (!widget->internalWinId() && widget->windowType()==Qt::Popup)
            widget->setWindowFlag(Qt::FramelessWindowHint);
        LegacyRoundedFrame::install(widget);
    }
}
inline bool suppressLegacyPainting(QWidget *widget,UINT message,long *result) {
    if (!widget->property("appUiLegacyCorners").toBool()) return false;
    if (message==WM_NCPAINT || message==WM_NCACTIVATE) {
        *result=message==WM_NCACTIVATE ? TRUE : 0;
        return true;
    }
    return false;
}
inline void configure(QWidget *widget) {
    const HWND handle=reinterpret_cast<HWND>(widget->internalWinId());
    if (!handle) return;
    const bool fallback=widget->property("appUiLegacyCorners").toBool();
    const bool popupSurface=widget->property("appUiPopupSurface").toBool();
    LONG_PTR style=GetWindowLongPtrW(handle,GWL_STYLE);
    if (popupSurface) style &= ~(WS_CAPTION|WS_THICKFRAME|WS_BORDER|WS_DLGFRAME);
    else {
        style |= WS_THICKFRAME;
        style=fallback ? style & ~WS_CAPTION : style | WS_CAPTION;
    }
    SetWindowLongPtrW(handle,GWL_STYLE,style);
    const HMODULE dwm=GetModuleHandleW(L"dwmapi.dll");
    using Attribute=HRESULT (WINAPI *)(HWND,DWORD,LPCVOID,DWORD);
    using Extend=HRESULT (WINAPI *)(HWND,const MARGINS *);
    const auto attribute=resolve<Attribute>(dwm,"DwmSetWindowAttribute");
    const auto extend=resolve<Extend>(dwm,"DwmExtendFrameIntoClientArea");
    bool native=false;
    if (attribute) {
        const DWORD policy=fallback ? 1 : 2; // DISABLED / ENABLED
        const DWORD corner=fallback ? 1 : 2; // DONOTROUND / ROUND (11+)
        const COLORREF border=RGB(187,195,208);
        attribute(handle,2,&policy,sizeof(policy));
        const HRESULT rounded=attribute(handle,33,&corner,sizeof(corner));
        attribute(handle,34,&border,sizeof(border));
        native=!fallback && SUCCEEDED(rounded);
    }
    widget->setProperty("appUiNativeCorners",native);
    if (extend) {
        const int border=popupSurface ? 0 : 1;
        const MARGINS margins={border,border,border,border};
        extend(handle,&margins);
    }
    syncQtFrameMargins(widget);
    if (!widget->mask().isEmpty()) widget->clearMask();
    SetWindowPos(handle,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);
}
}
#endif
