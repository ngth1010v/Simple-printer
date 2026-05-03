#include "controller/home/InfoConfigSection.h"
#include "ui/HomeWindow.h"

#include <string>

using namespace controller::home;

InfoConfigSectionController::InfoConfigSectionController(HomeWindow& win, config::ConfigData& cfg)
    : m_win(win), m_cfg(cfg) {
}

void InfoConfigSectionController::Init()
{
    Reload(); // 🔥 init = load thật luôn
}

void InfoConfigSectionController::Reload()
{
    // ===== total files =====
    const int totalFiles = static_cast<int>(m_cfg.files.size());

    // ===== total pages in ranges =====
    const int totalRangePages = CalcTotalPagesInRanges();

    // ===== PagesToPrint =====
    std::string pagesToPrintStr = "0";

    if (m_cfg.printMode == "Simplex") {
        pagesToPrintStr = std::to_string(totalRangePages);
    } else {
        // duplex → phải chẵn
        if (totalRangePages % 2 == 0 || totalRangePages == 1) {
            pagesToPrintStr = std::to_string(totalRangePages);
        } else {
            pagesToPrintStr = std::to_string(totalRangePages) + " + 1";
        }
    }

    // ===== SheetsRequired =====
    int sheetsRequired = 0;

    if (m_cfg.printMode == "Simplex") {
        sheetsRequired = totalRangePages;
    } else {
        // ceil(total / 2)
        sheetsRequired = (totalRangePages + 1) / 2;
    }
    
    // ===== set UI =====
    m_win.m_info.SetTotalFilesValue(std::to_string(totalFiles));
    m_win.m_info.SetPagesToPrintValue(pagesToPrintStr);
    m_win.m_info.SetSheetsRequiredValue(std::to_string(sheetsRequired));
}

int InfoConfigSectionController::CalcTotalPagesInRanges() const
{
    int total = 0;

    for (const auto& f : m_cfg.files) {

        // chỉ tính file hợp lệ
        if (!f.loaded || f.pages <= 0) {
            continue;
        }

        if (f.fromRange <= 0 || f.toRange <= 0) {
            continue;
        }

        if (f.fromRange > f.toRange) {
            continue;
        }

        
        int count = f.toRange - f.fromRange + 1;
        if (count > 0) {
            total += count;
        }
    }

    return total;
}