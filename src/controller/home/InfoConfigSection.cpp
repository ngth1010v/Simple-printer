#include "controller/home/InfoConfigSection.h"
#include "ui/HomeWindow.h"

using namespace controller::home;

InfoConfigSectionController::InfoConfigSectionController(HomeWindow& win, config::ConfigData& cfg)
    : m_win(win), m_cfg(cfg) {
}

void InfoConfigSectionController::Init()
{
    (void)m_cfg;

    // hard test data, giữ nguyên để bạn thay sau
    m_win.m_info.SetTotalFilesValue("1");
    m_win.m_info.SetPagesToPrintValue("1");
    m_win.m_info.SetSheetsRequiredValue("1");
}
