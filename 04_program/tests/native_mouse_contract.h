#pragma once
#include <QElapsedTimer>
#include <QThread>
#include <cstdio>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "windows_mouse_input.h"
#endif

inline bool physicalClick(QWidget &window, QWidget *target, const QPoint &local)
{
#ifdef Q_OS_WIN
    window.raise();
    window.activateWindow();
    QApplication::processEvents();
    const HWND root=reinterpret_cast<HWND>(window.winId());
    // Keep only the test's own window above unrelated desktop activity.
    SetWindowPos(root,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
    SetForegroundWindow(root);
    QElapsedTimer activation;
    activation.start();
    while (activation.elapsed()<100) { QApplication::processEvents(); QThread::msleep(5); }
    const QPoint point=target->mapTo(&window,local);
    POINT native{qRound(point.x()*window.devicePixelRatioF()),qRound(point.y()*window.devicePixelRatioF())};
    if (!ClientToScreen(root,&native)) return false;
    DWORD pid=0;
    QElapsedTimer ready;
    ready.start();
    do {
        GetWindowThreadProcessId(WindowFromPoint(native),&pid);
        if (pid==GetCurrentProcessId()) break;
        QApplication::processEvents(); QThread::msleep(10);
    } while (ready.elapsed()<1500);
    if (pid != GetCurrentProcessId()) {
        fprintf(stderr,"Native input did not hit the test window\n");
        RECT bounds;
        GetWindowRect(root,&bounds);
        const QImage pixels=window.grab().toImage();
        fprintf(stderr,"own=%lu hit=%lu point=%ld,%ld rect=%ld,%ld,%ld,%ld visible=%d enabled=%d style=%llx ex=%llx alpha=%d\n",
                GetCurrentProcessId(),pid,native.x,native.y,bounds.left,bounds.top,bounds.right,bounds.bottom,
                IsWindowVisible(root),IsWindowEnabled(root),static_cast<unsigned long long>(GetWindowLongPtrW(root,GWL_STYLE)),
                static_cast<unsigned long long>(GetWindowLongPtrW(root,GWL_EXSTYLE)),qAlpha(pixels.pixel(qRound(point.x()*window.devicePixelRatioF()),qRound(point.y()*window.devicePixelRatioF()))));
        return false;
    }
    POINT previous;
    GetCursorPos(&previous);
    if (!sendPhysicalClick(native)) return false;
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed()<120) { QApplication::processEvents(); QThread::msleep(5); }
    SetCursorPos(previous.x,previous.y);
    return true;
#else
    Q_UNUSED(window) Q_UNUSED(target) Q_UNUSED(local)
    return true;
#endif
}

inline int verifyPhysicalNavigation(MainWindow &window, QListWidget *sidebar)
{
    for (int row : {1,3,8,0}) {
        const QPoint point=sidebar->visualItemRect(sidebar->item(row)).center();
        if (!physicalClick(window,sidebar->viewport(),point) || sidebar->currentRow()!=row) {
            fprintf(stderr,"Navigation click expected=%d actual=%d\n",row,sidebar->currentRow());
            return 297;
        }
    }
    auto *search=window.findChild<QLineEdit *>("shellSearch");
    if (!search || !physicalClick(window,search,search->rect().center()) || !search->hasFocus()) {
        QWidget *focus=QApplication::focusWidget();
        fprintf(stderr,"Search click focus=%s active=%d\n",focus ? focus->objectName().toUtf8().constData() : "null",window.isActiveWindow());
        return 298;
    }
    return 0;
}
