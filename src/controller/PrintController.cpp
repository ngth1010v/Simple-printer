#include "controller/PrintController.h"
#include "ui/PrintWindow.h"

#include <windows.h>

namespace controller {


PrintController::PrintController(const config::ConfigData& cfg)
    : m_cfg(cfg) {
    (void)m_cfg;
}

int PrintController::Run()
{
    ui::PrintWindow win;

    if (!win.CreateWindowInstance()) {
        return 0;
    }

    ShowWindow(win.GetHwnd(), SW_SHOW);

    // ===== INIT UI =====
    win.SetResourceProcessLabel(L"Rendering resources...");
    win.SetResourceProcess(20, 0);
    win.SetResourceProcessColor("green");

    win.SetPrintProcessLabel(L"Printing...");
    win.SetPrintProcess(20, 0);
    win.SetPrintProcessColor("green");

    win.SetNotification(L"Wait for input...");

    win.SetAllowPause(true);
    win.SetAllowContinue(false);

    // ===== CALLBACK =====
    win.OnPause([&]() {
        win.SetNotification(L"Paused...");
        win.SetAllowPause(false);
        win.SetAllowContinue(true);
    });

    win.OnContinue([&]() {
        win.SetNotification(L"Continue...");
        win.SetAllowPause(true);
        win.SetAllowContinue(false);
    });

    win.OnCancel([&]() {
        PostQuitMessage(0);
    });

    // ===== SIMULATION TIMER =====
    SetTimer(win.GetHwnd(), 1, 100, nullptr);

    int resCurrent = 0;
    int printCurrent = 0;

    // ===== MESSAGE LOOP =====
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {

        if (msg.message == WM_TIMER && msg.wParam == 1) {

            if (resCurrent < 20) {
                resCurrent++;
                win.SetResourceProcess(20, resCurrent);
            }
            else if (printCurrent < 20) {
                printCurrent++;
                win.SetPrintProcess(20, printCurrent);
            }
            else {
                win.SetNotification(L"Done!");
                KillTimer(win.GetHwnd(), 1);
            }
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

}