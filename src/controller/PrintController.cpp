#include "controller/PrintController.h"
#include "controller/print/Counter.h"
#include "controller/print/Renderer.h"
#include "controller/print/Printer.h"
#include "platform/WinUtils.h"

#include "ui/PrintWindow.h"

#include <windows.h>

#include <atomic>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

#include <iostream>

namespace controller {
namespace fs = std::filesystem;

namespace {

bool ClearTempFolder(const fs::path& dir) {
    std::error_code ec;

    if (fs::exists(dir, ec)) {
        fs::remove_all(dir, ec);
        if (ec) {
            return false;
        }
    }

    fs::create_directories(dir, ec);
    return !ec;
}

} // namespace



PrintController::PrintController(const config::ConfigData& cfg): m_cfg(cfg) {}

int PrintController::Run() {

    //===================================================
    // Create window
    ui::PrintWindow win;
    if (!win.CreateWindowInstance()) {
        return 0;
    }
    ShowWindow(win.GetHwnd(), SW_SHOW);
    UpdateWindow(win.GetHwnd());



    //===================================================
    // Create temp dir
    const fs::path tempFolder = fs::current_path() / "_temp_simple_printer";
    {
        if (!ClearTempFolder(tempFolder)) {
            MessageBoxW(
                win.GetHwnd(),
                L"Cannot create or clear the temp folder \"_temp_simple_printer\".",
                L"Print error",
                MB_OK | MB_ICONERROR
            );
            return 0;
        }
    }



    //===================================================
    // Init flag
    std::atomic<bool> runFlag{false};
    std::atomic<bool> cancelFlag{false};
    std::atomic<bool> pauseFlag{false};
    bool printing = false;

    controller::print::Counter::Init(m_cfg, runFlag, cancelFlag);
    controller::print::Renderer renderer;
    controller::print::Printer printer;
    




    //===================================================
    // Create default value for win ui 
    {
        win.SetResourceProcessLabel(L"Rendering resources...");
        win.SetResourceProcessColor("green");
        win.SetResourceProcess(0, 0);

        win.SetPrintProcessLabel(L"Printing...");
        win.SetPrintProcessColor("green");
        win.SetPrintProcess(0, 0);

        win.SetNotification(L"Calculating pages...");
        win.SetAllowPause(false);
        win.SetAllowContinue(false);

        win.OnCancel([&]() {
            cancelFlag.store(true, std::memory_order_relaxed);
        });
        
        win.OnPause([&]() {
            pauseFlag.store(true, std::memory_order_relaxed);
        });
        
        win.OnContinue([&]() {
            pauseFlag.store(false, std::memory_order_relaxed);
        });
    }

    

    //===================================================
    // Message loop
    bool renderDoneHandled = false;
    MSG msg = {};

    while (true) {

        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                goto END_LOOP;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        //===================================================
        // CANCLE
        if (cancelFlag.load(std::memory_order_relaxed)) {
            PostQuitMessage(0);
        }

        //===================================================
        // DONE COUNTER
        if (runFlag.load(std::memory_order_relaxed) && !printing) {
            printing = true;

            win.SetNotification(L"Working on it...");
            win.SetAllowPause(true);
            controller::print::Counter::Destroy();

            int dpiX = 300;
            int dpiY = 300;
            if (!platform::GetPrinterDPI(m_cfg.printer, dpiX, dpiY)) {
                dpiX = 300;
                dpiY = 300;
            }

            renderer.Init(m_cfg, tempFolder.wstring(), std::max(dpiX, dpiY), cancelFlag, win);
            renderer.Run();
        }

        //===================================================
        // RENDER DONE
        if (printing && renderer.IsDone() && !renderDoneHandled) {
            renderDoneHandled = true;

            win.SetNotification(L"Start printing...");

            printer.Init(m_cfg, tempFolder.wstring(), cancelFlag, pauseFlag, win);
        }

        Sleep(1); // cực quan trọng: tránh ăn 100% CPU
    }

    END_LOOP:
    
    
    //===================================================
    // Clean up 
    controller::print::Counter::Destroy();
    renderer.Destroy();
    printer.Destroy();

    std::error_code ec;
    fs::remove_all(tempFolder, ec);

    return 0;
}

} // namespace controller