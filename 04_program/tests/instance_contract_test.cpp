#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QProcess>
#include <QProcessEnvironment>
#include <QThread>
#include <functional>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <algorithm>
#include <windows.h>
#include <dwmapi.h>
#include "windows_mouse_input.h"

static bool waitFor(const std::function<bool()> &check)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 15000) {
        if (check()) return true;
        QThread::msleep(50);
    }
    return false;
}

struct Search { DWORD pid; HWND window = nullptr; };
static BOOL CALLBACK findWindow(HWND window, LPARAM param)
{
    auto *search = reinterpret_cast<Search *>(param);
    DWORD pid = 0;
    GetWindowThreadProcessId(window, &pid);
    wchar_t title[32] = {};
    GetWindowTextW(window, title, 32);
    wchar_t className[32] = {};
    GetClassNameW(window, className, 32);
    // Fresh isolated Data shows the initial work-directory chooser. Cancel it
    // so startup creates its documented fallback inside this test's Data.
    if (pid == search->pid && wcscmp(className, L"#32770") == 0)
        PostMessageW(window, WM_CLOSE, 0, 0);
    if (pid == search->pid && IsWindowVisible(window) && wcscmp(title, L"NIUMA") == 0) {
        search->window = window;
        return FALSE;
    }
    return TRUE;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if ((argc != 2 && argc != 3) || qEnvironmentVariableIsEmpty("NIUMA_TEST_ROOT")) return 1;
    if (argc == 2 && QFileInfo(QString::fromLocal8Bit(argv[1])).fileName() != "Niuma.exe") return 17;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("NIUMA_DATA_DIR", QDir(qEnvironmentVariable("NIUMA_TEST_ROOT")).filePath("Data"));
    QProcess primary;
    primary.setProcessEnvironment(env);
    primary.start(QString::fromLocal8Bit(argv[1]), QStringList());
    if (!primary.waitForStarted()) return 2;
    Search search{static_cast<DWORD>(primary.processId())};
    if (!waitFor([&]() { EnumWindows(findWindow, reinterpret_cast<LPARAM>(&search)); return search.window; })) return 3;
    // Verify the OS mouse path; calling QPushButton::click bypasses native overlays.
    if (argc == 3 && QString::fromLocal8Bit(argv[2]) == "--input-probe") {
        ShowWindow(search.window, SW_RESTORE);
        SetWindowPos(search.window,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
        SetForegroundWindow(search.window);
        QThread::msleep(700);
        RECT client;
        GetClientRect(search.window,&client);
        POINT point{client.right-115,23};
        ClientToScreen(search.window,&point);
        const HWND hit=WindowFromPoint(point);
        RECT hitBounds, mainBounds;
        GetWindowRect(hit,&hitBounds);
        GetWindowRect(search.window,&mainBounds);
        QFile report(QDir(qEnvironmentVariable("NIUMA_TEST_ROOT")).filePath("native-input.txt"));
        if (!report.open(QIODevice::WriteOnly|QIODevice::Text)) return 18;
        QTextStream out(&report);
        out << "main=" << quintptr(search.window) << " hit=" << quintptr(hit)
            << " point=" << point.x << ',' << point.y << " client=" << client.right << ',' << client.bottom
            << " hitCode=" << SendMessageW(search.window,WM_NCHITTEST,0,MAKELPARAM(point.x,point.y))
            << " mainEnabled=" << IsWindowEnabled(search.window)
            << " hitRect=" << hitBounds.left << ',' << hitBounds.top << ',' << hitBounds.right << ',' << hitBounds.bottom
            << " mainRect=" << mainBounds.left << ',' << mainBounds.top << ',' << mainBounds.right << ',' << mainBounds.bottom << '\n';
        out.flush();
        DWORD hitPid=0;
        GetWindowThreadProcessId(hit,&hitPid);
        if (hitPid != search.pid) return 19;
        POINT previous;
        GetCursorPos(&previous);
        if (!sendPhysicalClick(point)) return 21;
        const bool minimized=waitFor([&]() { return IsIconic(search.window); });
        SetCursorPos(previous.x,previous.y);
        out << "mouseMinimized=" << minimized << '\n';
        report.close();
        primary.kill();
        primary.waitForFinished();
        return minimized ? 0 : 20;
    }
    if (argc == 3 && QString::fromLocal8Bit(argv[2]) == "--benchmark") {
        QVector<double> samples;
        for (int index = 0; index < 100; ++index) {
            QElapsedTimer elapsed;
            elapsed.start();
            SetWindowPos(search.window, nullptr, 0, 0, 1200+(index%30)*6, 720+(index%30)*4,
                         SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            SendMessageW(search.window, WM_NULL, 0, 0);
            if (index >= 10) samples.append(elapsed.nsecsElapsed()/1000000.0);
            QThread::msleep(2);
        }
        std::sort(samples.begin(), samples.end());
        double sum = 0;
        for (double sample : samples) sum += sample;
        QFile report(QDir(qEnvironmentVariable("NIUMA_TEST_ROOT")).filePath("resize-benchmark.txt"));
        if (!report.open(QIODevice::WriteOnly | QIODevice::Text)) return 16;
        QTextStream(&report) << "samples=" << samples.size() << " mean_ms=" << sum/samples.size()
                            << " p95_ms=" << samples.at(samples.size()*95/100)
                            << " max_ms=" << samples.last() << '\n';
        primary.kill();
        primary.waitForFinished();
        return 0;
    }
    // SC_MINIMIZE is the native system command used by the taskbar.
    const LONG_PTR style = GetWindowLongPtrW(search.window, GWL_STYLE);
    const bool canMinimize = (style & WS_MINIMIZEBOX) && (style & WS_SYSMENU);
    if (!(style & WS_THICKFRAME)) return 13;
    const auto checkEdges = [&]() {
        RECT r;
        GetWindowRect(search.window, &r);
        struct Edge { int x, y, hit; };
        const Edge points[] = {{r.left+2,r.top+2,HTTOPLEFT}, {r.right-3,r.top+2,HTTOPRIGHT},
            {r.left+2,r.bottom-3,HTBOTTOMLEFT}, {r.right-3,r.bottom-3,HTBOTTOMRIGHT},
            {r.left+2,(r.top+r.bottom)/2,HTLEFT}, {r.right-3,(r.top+r.bottom)/2,HTRIGHT},
            {(r.left+r.right)/2,r.top+2,HTTOP}, {(r.left+r.right)/2,r.bottom-3,HTBOTTOM}};
        for (const auto &point : points)
            if (SendMessageW(search.window, WM_NCHITTEST, 0, MAKELPARAM(point.x,point.y)) != point.hit) return false;
        HRGN region = CreateRectRgn(0,0,0,0);
        const int kind = GetWindowRgn(search.window, region);
        DeleteObject(region);
        return kind == ERROR; // No pixel mask: DWM owns the antialiased outline.
    };
    if (!checkEdges()) return 14;
    RECT before;
    GetWindowRect(search.window, &before);
    SetWindowPos(search.window, nullptr, 0, 0, 1100, 720, SWP_NOMOVE | SWP_NOZORDER);
    if (!checkEdges()) return 15;
    SendMessageW(search.window, WM_SYSCOMMAND, SC_MINIMIZE, 0);
    if (!waitFor([&]() { return IsIconic(search.window); })) return 4;
    for (int index = 0; index < 3; ++index) {
        QProcess secondary;
        secondary.setProcessEnvironment(env);
        secondary.start(QString::fromLocal8Bit(argv[1]), QStringList());
        if (!secondary.waitForStarted() || !secondary.waitForFinished(15000)
            || secondary.exitCode() != 0) return 5;
        if (!waitFor([&]() { return !IsIconic(search.window) && GetForegroundWindow() == search.window; })) return 6;
        if (primary.state() != QProcess::Running) return 7;
        if (index == 0) ShowWindow(search.window, SW_MAXIMIZE);
        if (index > 0 && !IsZoomed(search.window)) return 8;
    }
    if (!canMinimize) return 9;
    primary.kill();
    primary.waitForFinished();
    // A killed process must not leave a stale single-instance lock.
    primary.start(QString::fromLocal8Bit(argv[1]), QStringList());
    if (!primary.waitForStarted()) return 10;
    search = {static_cast<DWORD>(primary.processId())};
    if (!waitFor([&]() { EnumWindows(findWindow, reinterpret_cast<LPARAM>(&search)); return search.window; })) return 11;
    SendMessageW(search.window, WM_CLOSE, 0, 0);
    if (!primary.waitForFinished(15000) || primary.exitCode() != 0) return 12;
    return 0;
}
