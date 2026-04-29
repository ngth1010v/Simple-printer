#include "controller/HomeController.h"

#include "ui/HomeWindow.h"
#include "controller/PrintController.h"

#include "controller/home/BasicConfigSection.h"
#include "controller/home/AdvangeConfigSection.h"
#include "controller/home/MarginConfigSection.h"
#include "controller/home/InfoConfigSection.h"
#include "controller/home/ControlBlock.h"
#include "controller/home/FileListView.h"

#include <windows.h>

namespace controller {

HomeController::HomeController(config::ConfigData& cfg)
    : m_cfg(cfg) {
}

int HomeController::Run()
{
    HomeWindow win;
    if (!win.CreateWindowInstance()) return 0;
    ShowWindow(win.GetHwnd(), SW_SHOW);

    // ===== BASIC =====
    {
        controller::home::BasicConfigSectionController basic(win, m_cfg);
        basic.Init();
        basic.Bind();
    }

    // ===== ADVANCE =====
    {
        controller::home::AdvangeConfigSectionController adv(win, m_cfg);
        adv.Init();
        adv.Bind();
    }

    // ===== MARGIN =====
    {
        controller::home::MarginConfigSectionController margin(win, m_cfg);
        margin.Init();
        margin.Bind();
    }

    // ===== INFO =====
    {
        controller::home::InfoConfigSectionController info(win, m_cfg);
        info.Init();
    }

    // ===== FILE LIST =====
    {
        controller::home::FileListViewController files(win, m_cfg);
        files.Init();
        files.Bind();
    }

    // ===== CONTROL =====
    {
        controller::home::ControlBlockController control(win, m_cfg);
        control.Bind();
    }

    // ===== LOOP =====
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

}