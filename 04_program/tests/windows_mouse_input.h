#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

// Submit position and both button transitions as one OS input batch, so
// unrelated desktop input cannot separate moving the pointer from clicking.
inline bool sendPhysicalClick(const POINT &point)
{
    INPUT events[3] = {};
    for (auto &event : events) {
        event.type=INPUT_MOUSE;
        event.mi.dx=MulDiv(point.x-GetSystemMetrics(SM_XVIRTUALSCREEN),65535,GetSystemMetrics(SM_CXVIRTUALSCREEN)-1);
        event.mi.dy=MulDiv(point.y-GetSystemMetrics(SM_YVIRTUALSCREEN),65535,GetSystemMetrics(SM_CYVIRTUALSCREEN)-1);
        event.mi.dwFlags=MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_VIRTUALDESK|MOUSEEVENTF_MOVE;
    }
    events[1].mi.dwFlags|=MOUSEEVENTF_LEFTDOWN;
    events[2].mi.dwFlags|=MOUSEEVENTF_LEFTUP;
    return SendInput(3,events,sizeof(INPUT))==3;
}
