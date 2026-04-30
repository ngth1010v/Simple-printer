#include "controller/HomeController.h"

#include "ui/HomeWindow.h"
#include "controller/PrintController.h"

#include "controller/home/BasicConfigSection.h"
#include "controller/home/AdvangeConfigSection.h"
#include "controller/home/MarginConfigSection.h"
#include "controller/home/InfoConfigSection.h"
#include "controller/home/ControlBlock.h"
#include "controller/home/FileListView.h"

#include "counter/Counter.h"

#include <windows.h>

//============================================================================
// MAIN
//============================================================================
namespace controller {

HomeController::HomeController(config::ConfigData& cfg)
    : m_cfg(cfg) {
}

int HomeController::Run()
{
    HomeWindow win;
    if (!win.CreateWindowInstance()) return 0;
    ShowWindow(win.GetHwnd(), SW_SHOW);
    counter::Init();



    // ===== BASIC =====
    controller::home::BasicConfigSectionController basic(win, m_cfg);
    basic.Init();
    basic.Bind();


    // ===== INFO =====
    controller::home::InfoConfigSectionController info(win, m_cfg);
    info.Init();
    
    
    // ===== ADVANCE =====
    controller::home::AdvangeConfigSectionController adv(win, m_cfg);
    adv.Init([&]() {
        info.Reload();
    });
    adv.Bind();


    // ===== MARGIN =====
    controller::home::MarginConfigSectionController margin(win, m_cfg);
    margin.Init();
    margin.Bind();


    // ===== FILE LIST =====
    controller::home::FileListViewController files(win, m_cfg);
    files.Init([&]() {
        info.Reload();
    });
    files.Bind();

    
    // ===== CONTROL =====
    controller::home::ControlBlockController control(win, m_cfg);
    control.Bind();


    // ===== LOOP =====
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    counter::Shutdown();
    return 0;
}

}