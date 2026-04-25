#include "App.h"
#include "CommandLine.h"
#include "PrintingWindow.h"

#include <windows.h>
#include <iostream>

int App::Run() {
    bool isPrint = CommandLine::HasFlag("print");
    const auto& files = CommandLine::GetArgs();

    if (files.empty()) {
        return 0;
    }

    std::cout<<isPrint<<'\n';
    if (isPrint) {
        PrintingWindow win;

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

        win.SetAllowStop(true);
        win.SetAllowContinue(false);

        // ===== CALLBACK =====
        win.OnStop([&]() {
            win.SetNotification(L"Stopped...");
            win.SetAllowStop(false);
            win.SetAllowContinue(true);
        });

        win.OnContinue([&]() {
            win.SetNotification(L"Continue...");
            win.SetAllowStop(true);
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

            if (msg.message == WM_TIMER) {
                if (msg.wParam == 1) {

                    // simulate resource loading
                    if (resCurrent < 20) {
                        resCurrent++;
                        win.SetResourceProcess(20, resCurrent);
                    }
                    else {
                        // simulate printing
                        if (printCurrent < 20) {
                            printCurrent++;
                            win.SetPrintProcess(20, printCurrent);
                        }
                        else {
                            win.SetNotification(L"Done!");
                            KillTimer(win.GetHwnd(), 1);
                        }
                    }
                }
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    else {
        // TODO: ReviewWindow
    }

    return 0;
}