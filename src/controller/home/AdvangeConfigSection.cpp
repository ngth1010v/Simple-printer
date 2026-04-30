#include "controller/home/AdvangeConfigSection.h"
#include "platform/WinPrinter.h"
#include "ui/HomeWindow.h"

#include <string>
#include <vector>

namespace {

static void AdvPrintModeInit(HomeWindow& win, config::ConfigData& cfg) {
    std::vector<std::string> options = {
        "Simplex",
        "Duplex",
        "Manual Duplex (Flip On Long Edge)",
        "Manual Duplex (Flip On Short Edge)"
    };

    std::vector<std::string> imgs = {
        "./assets/PrintMode/simplex.bmp",
        "./assets/PrintMode/duplex.bmp",
        "./assets/PrintMode/flip-long-edge.bmp",
        "./assets/PrintMode/flip-short-edge.bmp"
    };

    win.m_adv.SetPrintModeOptions(options);

    int matchId = 0;
    for (int i = 0; i < (int)options.size(); i++) {
        if (options[i] == cfg.printMode) {
            matchId = i;
            break;
        }
    }

    win.m_adv.SetPrintModeValue(options[matchId]);
    win.m_adv.SetPrintModeImage(imgs[matchId]);
}

static void AdvPrintModeUpdate(HomeWindow& win, config::ConfigData& cfg, const std::string& v) {
    cfg.printMode = v;

    std::vector<std::string> options = {
        "Simplex",
        "Duplex",
        "Manual Duplex (Flip On Long Edge)",
        "Manual Duplex (Flip On Short Edge)"
    };

    std::vector<std::string> imgs = {
        "../assets/PrintMode/simplex.bmp",
        "../assets/PrintMode/duplex.bmp",
        "../assets/PrintMode/flip-long-edge.bmp",
        "../assets/PrintMode/flip-short-edge.bmp"
    };

    for (int i = 0; i < (int)options.size(); i++) {
        if (options[i] == v) {
            win.m_adv.SetPrintModeImage(imgs[i]);
            break;
        }
    }
}

static void AdvPaperInit(HomeWindow& win, config::ConfigData& cfg) {
    auto papers = platform::printer::GetSupportedPapers(cfg.printer);

    if (papers.empty()) return;

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

static void AdvPaperUpdate(config::ConfigData& cfg, const std::string& v) {
    cfg.paper = v;
}

static void AdvScaleInit(HomeWindow& win, config::ConfigData& cfg) {
    std::vector<std::string> options = {
        "No Scale",
        "Fit to page (keep aspect ratio)",
        "Fill page (ignore aspect ratio)"
    };

    win.m_adv.SetScaleOptions(options);
    win.m_adv.SetScaleValue(cfg.scale);
}

static void AdvScaleUpdate(config::ConfigData& cfg, const std::string& v) {
    cfg.scale = v;
}

static void AdvOrientationInit(HomeWindow& win, config::ConfigData& cfg) {
    std::vector<std::string> options = {
        "Auto",
        "Alway Portrait",
        "Alway Landscape"
    };

    win.m_adv.SetOrientationOptions(options);
    win.m_adv.SetOrientationValue(cfg.orientation);
}

static void AdvOrientationUpdate(config::ConfigData& cfg, const std::string& v) {
    cfg.orientation = v;
}

static void AdvCollateInit(HomeWindow& win, config::ConfigData& cfg) {
    std::vector<std::string> options = {
        "Collated (1,2,3 | 1,2,3 | 1,2,3)",
        "Uncollated (1,1,1 | 2,2,2 | 3,3,3)"
    };

    win.m_adv.SetCollateOptions(options);
    win.m_adv.SetCollateValue(cfg.collate);
}

static void AdvCollateUpdate(config::ConfigData& cfg, const std::string& v) {
    cfg.collate = v;
}

}

using namespace controller::home;

AdvangeConfigSectionController::AdvangeConfigSectionController(HomeWindow& win, config::ConfigData& cfg)
    : m_win(win), m_cfg(cfg) {
}

void AdvangeConfigSectionController::Init()
{
    AdvPrintModeInit(m_win, m_cfg);
    AdvPaperInit(m_win, m_cfg);
    AdvScaleInit(m_win, m_cfg);
    AdvOrientationInit(m_win, m_cfg);
    AdvCollateInit(m_win, m_cfg);
}

void AdvangeConfigSectionController::Bind()
{
    m_win.m_adv.OnPrintModeChange([&](const std::string& v) {
        AdvPrintModeUpdate(m_win, m_cfg, v);
    });

    m_win.m_adv.OnPaperChange([&](const std::string& v) {
        AdvPaperUpdate(m_cfg, v);
    });

    m_win.m_adv.OnScaleChange([&](const std::string& v) {
        AdvScaleUpdate(m_cfg, v);
    });

    m_win.m_adv.OnOrientationChange([&](const std::string& v) {
        AdvOrientationUpdate(m_cfg, v);
    });

    m_win.m_adv.OnCollateChange([&](const std::string& v) {
        AdvCollateUpdate(m_cfg, v);
    });
}
