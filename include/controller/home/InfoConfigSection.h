#pragma once

#include "config/ConfigModel.h"
#include <string>

class HomeWindow;

namespace controller {
namespace home {

class InfoConfigSectionController {
public:
    InfoConfigSectionController(HomeWindow& win, config::ConfigData& cfg);

    void Init();

    // 🔥 thêm hàm này để HomeController gọi lại
    void Reload();

private:
    HomeWindow& m_win;
    config::ConfigData& m_cfg;

private:
    int CalcTotalPagesInRanges() const;
};

}
}