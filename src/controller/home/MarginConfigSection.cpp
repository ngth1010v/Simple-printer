#include "controller/home/MarginConfigSection.h"
#include "ui/HomeWindow.h"

#include <algorithm>
#include <cctype>
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>

namespace {

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

    s.erase(std::remove(s.begin(), s.end(), '\"'), s.end());

    int dotCount = 0;
    for (char c : s) {
        if (c == '.') {
            dotCount++;
            if (dotCount > 1) return false;
        }
        else if (!std::isdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }

    try {
        float v = std::stof(s);

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

static void MarginInit(HomeWindow& win, config::ConfigData& cfg) {
    win.m_margin.SetMarginTopValue   (FloatToMarginString(cfg.margin[0]));
    win.m_margin.SetMarginRightValue (FloatToMarginString(cfg.margin[1]));
    win.m_margin.SetMarginBottomValue(FloatToMarginString(cfg.margin[2]));
    win.m_margin.SetMarginLeftValue  (FloatToMarginString(cfg.margin[3]));
}

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

}

using namespace controller::home;

MarginConfigSectionController::MarginConfigSectionController(HomeWindow& win, config::ConfigData& cfg)
    : m_win(win), m_cfg(cfg) {
}

void MarginConfigSectionController::Init()
{
    MarginInit(m_win, m_cfg);
}

void MarginConfigSectionController::Bind()
{
    m_win.m_margin.OnMarginTopChange([&](const std::string& v) {
        MarginUpdateOne(
            [&](const std::string& s){ m_win.m_margin.SetMarginTopValue(s); },
            m_cfg.margin[0],
            v
        );
    });

    m_win.m_margin.OnMarginRightChange([&](const std::string& v) {
        MarginUpdateOne(
            [&](const std::string& s){ m_win.m_margin.SetMarginRightValue(s); },
            m_cfg.margin[1],
            v
        );
    });

    m_win.m_margin.OnMarginBottomChange([&](const std::string& v) {
        MarginUpdateOne(
            [&](const std::string& s){ m_win.m_margin.SetMarginBottomValue(s); },
            m_cfg.margin[2],
            v
        );
    });

    m_win.m_margin.OnMarginLeftChange([&](const std::string& v) {
        MarginUpdateOne(
            [&](const std::string& s){ m_win.m_margin.SetMarginLeftValue(s); },
            m_cfg.margin[3],
            v
        );
    });
}
