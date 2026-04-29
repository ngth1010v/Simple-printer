#include "App.h"
#include "CommandLine.h"
#include "PrintingWindow.h"
#include "HomeWindow.h"
#include "ConfigParser.h"
#include "WinPrinter.h"

#include <windows.h>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>

// ============================================================
// INTERNAL
// ============================================================

// ======================= BASIC =======================

// Printer
static void BasicPrinterUiInit(HomeWindow& win, config::ConfigData& cfg) {
    auto printers = platform::printer::GetPrinters();
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
    auto papers = platform::printer::GetSupportedPapers(v);

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

// Copies
static void BasicCopiesUiInit(HomeWindow& win, config::ConfigData& cfg) {
    win.m_basic.SetCopiesValue(std::to_string(cfg.copies));
}

static void BasicCopiesUiUpdate(HomeWindow& win, config::ConfigData& cfg, const std::string& v) {

    // Validate
    bool valid = true;
    if (v.empty()) valid = false;

    for (char c : v) {
        if (!std::isdigit(c)) {
            valid = false;
            break;
        }
    }

    if (!valid) {
        win.m_basic.SetCopiesValue(std::to_string(cfg.copies));
        return;
    }

    cfg.copies = std::stoi(v); // ✅ FIX BUG
}


// ======================= ADVANCE =======================

// PrintMode
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

    // update image
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

    for (int i = 0; i < (int)options.size(); i++) {
        if (options[i] == v) {
            win.m_adv.SetPrintModeImage(imgs[i]);
            break;
        }
    }
}

// Paper
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

// Scale
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

// Orientation
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

// Collate
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

// ======================= MARGIN =======================

static std::string Trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t");
    size_t end   = s.find_last_not_of(" \t");
    if (start == std::string::npos) return "";
    return s.substr(start, end - start + 1);
}

static std::string FloatToMarginString(float v) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << v;
    return oss.str() + "\"";
}

static bool ParseMargin(const std::string& input, float& out) {
    std::string s = Trim(input);

    if (s.empty()) return false;

    // remove "
    s.erase(std::remove(s.begin(), s.end(), '\"'), s.end());

    // validate: digits + 1 dot max
    int dotCount = 0;
    for (char c : s) {
        if (c == '.') {
            dotCount++;
            if (dotCount > 1) return false;
        }
        else if (!std::isdigit(c)) {
            return false;
        }
    }

    try {
        float v = std::stof(s);

        // check <= 2 decimal
        auto pos = s.find('.');
        if (pos != std::string::npos) {
            int decimals = (int)s.size() - pos - 1;
            if (decimals > 2) return false;
        }

        out = v;
        return true;
    }
    catch (...) {
        return false;
    }
}


// ===== INIT =====
static void MarginInit(HomeWindow& win, config::ConfigData& cfg) {
    win.m_margin.SetMarginTopValue   (FloatToMarginString(cfg.margin[0]));
    win.m_margin.SetMarginRightValue (FloatToMarginString(cfg.margin[1]));
    win.m_margin.SetMarginBottomValue(FloatToMarginString(cfg.margin[2]));
    win.m_margin.SetMarginLeftValue  (FloatToMarginString(cfg.margin[3]));
}


// ===== UPDATE HELPERS =====
static void MarginUpdateOne(
    std::function<void(const std::string&)> setter,
    float& cfgVal,
    const std::string& input
) {
    float parsed;
    if (!ParseMargin(input, parsed)) {
        setter(FloatToMarginString(cfgVal));
        return;
    }

    cfgVal = parsed;
    setter(FloatToMarginString(cfgVal));
}


// ============================================================

static int RunUI(const std::vector<std::string>& files, config::ConfigData& cfg)
{
    HomeWindow win;
    if (!win.CreateWindowInstance()) return 0;
    ShowWindow(win.GetHwnd(), SW_SHOW);

    // ================= INIT =================
    {
        // BASIC
        BasicPrinterUiInit(win, cfg);
        BasicCopiesUiInit(win, cfg);

        win.m_basic.OnPrinterChange([&](const std::string& v) {
            BasicPrinterUiUpdate(win, cfg, v);
        });

        win.m_basic.OnCopiesChange([&](const std::string& v) {
            BasicCopiesUiUpdate(win, cfg, v);
        });

        // ADVANCE
        AdvPrintModeInit(win, cfg);
        AdvPaperInit(win, cfg);
        AdvScaleInit(win, cfg);
        AdvOrientationInit(win, cfg);
        AdvCollateInit(win, cfg);

        win.m_adv.OnPrintModeChange([&](const std::string& v) {
            AdvPrintModeUpdate(win, cfg, v);
        });

        win.m_adv.OnPaperChange([&](const std::string& v) {
            AdvPaperUpdate(cfg, v);
        });

        win.m_adv.OnScaleChange([&](const std::string& v) {
            AdvScaleUpdate(cfg, v);
        });

        win.m_adv.OnOrientationChange([&](const std::string& v) {
            AdvOrientationUpdate(cfg, v);
        });

        win.m_adv.OnCollateChange([&](const std::string& v) {
            AdvCollateUpdate(cfg, v);
        });

        // MARGIN
        MarginInit(win, cfg);

        win.m_margin.OnMarginTopChange([&](const std::string& v) {
            MarginUpdateOne(
                [&](const std::string& s){ win.m_margin.SetMarginTopValue(s); },
                cfg.margin[0],
                v
            );
        });

        win.m_margin.OnMarginRightChange([&](const std::string& v) {
            MarginUpdateOne(
                [&](const std::string& s){ win.m_margin.SetMarginRightValue(s); },
                cfg.margin[1],
                v
            );
        });

        win.m_margin.OnMarginBottomChange([&](const std::string& v) {
            MarginUpdateOne(
                [&](const std::string& s){ win.m_margin.SetMarginBottomValue(s); },
                cfg.margin[2],
                v
            );
        });

        win.m_margin.OnMarginLeftChange([&](const std::string& v) {
            MarginUpdateOne(
                [&](const std::string& s){ win.m_margin.SetMarginLeftValue(s); },
                cfg.margin[3],
                v
            );
        });
    }

    // ================= LOOP =================
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}









// ============================================================

static int RunPrint(const std::vector<std::string>& files)
{
    if (files.empty()) {
        return 0;
    }

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

    win.SetAllowPause(true);
    win.SetAllowContinue(false);

    // ===== CALLBACK =====
    win.OnPause([&]() {
        win.SetNotification(L"Paused...");
        win.SetAllowPause(false);
        win.SetAllowContinue(true);
    });

    win.OnContinue([&]() {
        win.SetNotification(L"Continue...");
        win.SetAllowPause(true);
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

        if (msg.message == WM_TIMER && msg.wParam == 1) {

            if (resCurrent < 20) {
                resCurrent++;
                win.SetResourceProcess(20, resCurrent);
            }
            else if (printCurrent < 20) {
                printCurrent++;
                win.SetPrintProcess(20, printCurrent);
            }
            else {
                win.SetNotification(L"Done!");
                KillTimer(win.GetHwnd(), 1);
            }
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}



// ============================================================
// PUBLIC
// ============================================================
int App::Run()
{
    bool isPrint = CommandLine::HasFlag("print");
    const auto& files = CommandLine::GetArgs();

    auto cfg = config::Parse("./config.ini");

    // ===== UI MODE =====
    if (!isPrint) {
        return RunUI(files, cfg);
    }

    // ===== PRINT MODE =====
    if (files.empty()) {
        return 0;
    }

    auto printers = platform::printer::GetPrinters();

    bool printerMatch = false;

    for (const auto& p : printers) {
        if (p == cfg.printer) {
            printerMatch = true;
            break;
        }
    }

    if (printerMatch) {
        return RunPrint(files);
    }

    // ===== NOT MATCH =====
    int result = MessageBoxA(
        nullptr,
        "The printer in config was not found.\nDo you want to open the UI to select another printer?",
        "Printer not found",
        MB_YESNO | MB_ICONQUESTION
    );

    if (result == IDYES) {
        return RunUI(files, cfg);
    }

    return 0;
}