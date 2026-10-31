#pragma once

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

// A session-wide kernel object: survives path changes, disappears even on a crash.
namespace SingleInstance {
inline const wchar_t *windowProperty() { return L"NIUMA.PrimaryWindow.3"; }
inline UINT activationMessage() { return RegisterWindowMessageW(L"NIUMA.ActivatePrimaryWindow.3"); }

class Guard {
public:
    Guard() : handle(CreateMutexW(nullptr, FALSE, L"Local\\NIUMA.SingleInstance.3")),
              existing(handle && GetLastError() == ERROR_ALREADY_EXISTS) {}
    ~Guard() { if (handle) CloseHandle(handle); }
    Guard(const Guard &) = delete;
    Guard &operator=(const Guard &) = delete;
    bool valid() const { return handle != nullptr; }
    bool isSecondary() const { return existing; }

    bool activateExisting() const
    {
        // The first process may still be initializing before it publishes its window.
        for (int attempt = 0; attempt < 600; ++attempt) {
            HWND window = nullptr;
            EnumWindows([](HWND candidate, LPARAM data) -> BOOL {
                if (!GetPropW(candidate, windowProperty())) return TRUE;
                *reinterpret_cast<HWND *>(data) = candidate;
                return FALSE;
            }, reinterpret_cast<LPARAM>(&window));
            if (window) {
                DWORD pid = 0;
                GetWindowThreadProcessId(window, &pid);
                AllowSetForegroundWindow(pid);
                return SendNotifyMessageW(window, activationMessage(), 0, 0);
            }
            Sleep(50);
        }
        return false;
    }
private:
    HANDLE handle;
    bool existing;
};
}
#endif
