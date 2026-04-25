#include "App.h"
#include "CommandLine.h"
#include "PrintingWindow.h"

#include <windows.h>

int App::Run() {
    bool isPrint = CommandLine::HasFlag("print");
    const auto& files = CommandLine::GetArgs();

    if (files.empty()) {
        return 0;
    }

    if (isPrint) {
        // tạo window
        PrintingWindow win;

        if (!win.CreateWindowInstance()) {
            return 0;
        }

        ShowWindow(win.GetHwnd(), SW_SHOW);

        // test update status
        win.SetStatus(L"Printing file 1/3...");

        // message loop
        MSG msg = {};
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    else {
        // sau này mở ReviewWindow
    }

    return 0;
}