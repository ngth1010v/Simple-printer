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
        config::Write("./config", m_cfg);
        DestroyWindow(m_win.GetHwnd());
    });

    m_win.m_control.OnPrint([&]() {
        config::Write("./config", m_cfg);

        // run print flow with current cfg
        PrintController printer(m_cfg);
        printer.Run();

        // close home window after print returns
        if (m_win.GetHwnd()) {
            DestroyWindow(m_win.GetHwnd());
        }
    });
}
