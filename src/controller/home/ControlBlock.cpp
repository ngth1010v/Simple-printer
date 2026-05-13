#include "controller/home/ControlBlock.h"

#include "controller/PrintController.h"
#include "ui/HomeWindow.h"
#include "config/ConfigWriter.h"

#include <windows.h>

using namespace controller::home;

ControlBlockController::ControlBlockController(HomeWindow& win, config::ConfigData& cfg)
    : m_win(win), m_cfg(cfg) {
}

void ControlBlockController::Bind()
{
    m_win.m_control.OnCancel([&]() {
        config::Write(m_cfg);
        DestroyWindow(m_win.GetHwnd());
    });

    m_win.m_control.OnPrint([win = &m_win, cfg = &m_cfg]() {
        config::Write(*cfg);

        std::wstring cmd = L"SimplePrinter.exe --print";

        STARTUPINFOW si{};
        si.cb = sizeof(si);

        PROCESS_INFORMATION pi{};

        BOOL ok = CreateProcessW(
            nullptr,
            cmd.data(),     // command line
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            nullptr,
            &si,
            &pi
        );

        if (ok) {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }

        DestroyWindow(win->GetHwnd());
    });
}
