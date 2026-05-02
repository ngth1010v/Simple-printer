#include "controller/home/BasicConfigSection.h"

#include "platform/WinUtils.h"
#include "ui/HomeWindow.h"

#include <cctype>
#include <string>
#include <vector>

namespace {

static void BasicPrinterUiInit(HomeWindow& win, config::ConfigData& cfg) {
    auto printers = platform::GetPrinters();
    win.m_basic.SetPrinterOptions(printers);

    bool match = false;
    for (const auto& p : printers) {
        if (p == cfg.printer) {
            match = true;
            break;
        }
    }

    if (!match) {
        if (!printers.empty()) {
            cfg.printer = printers.back();
        }
        else {
            cfg.printer = "No printer found";
        }
    }

    win.m_basic.SetPrinterValue(cfg.printer);
}

static void BasicPrinterUiUpdate(HomeWindow& win, config::ConfigData& cfg, const std::string& v) {
    cfg.printer = v;

    // ===== UPDATE PAPER WHEN PRINTER CHANGE =====
    auto papers = platform::GetSupportedPapers(v);

    if (!papers.empty()) {
        win.m_adv.SetPaperOptions(papers);

        bool match = false;
        for (auto& p : papers) {
            if (p == cfg.paper) {
                match = true;
                break;
            }
        }

        if (!match) {
            cfg.paper = papers[0];
        }

        win.m_adv.SetPaperValue(cfg.paper);
    }
}

static void BasicCopiesUiInit(HomeWindow& win, config::ConfigData& cfg) {
    win.m_basic.SetCopiesValue(std::to_string(cfg.copies));
}

static void BasicCopiesUiUpdate(HomeWindow& win, config::ConfigData& cfg, const std::string& v) {
    bool valid = true;
    if (v.empty()) valid = false;

    for (char c : v) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            valid = false;
            break;
        }
    }

    if (!valid) {
        win.m_basic.SetCopiesValue(std::to_string(cfg.copies));
        return;
    }

    cfg.copies = std::stoi(v);
}

}

using namespace controller::home;

BasicConfigSectionController::BasicConfigSectionController(HomeWindow& win, config::ConfigData& cfg)
    : m_win(win), m_cfg(cfg) {
}

void BasicConfigSectionController::Init()
{
    BasicPrinterUiInit(m_win, m_cfg);
    BasicCopiesUiInit(m_win, m_cfg);
}

void BasicConfigSectionController::Bind()
{
    m_win.m_basic.OnPrinterChange([&](const std::string& v) {
        BasicPrinterUiUpdate(m_win, m_cfg, v);
    });

    m_win.m_basic.OnCopiesChange([&](const std::string& v) {
        BasicCopiesUiUpdate(m_win, m_cfg, v);
    });
}
