#pragma once

#include "config/ConfigModel.h"

class HomeWindow;

namespace controller {
namespace home {

class AdvangeConfigSectionController {
public:
    AdvangeConfigSectionController(HomeWindow& win, config::ConfigData& cfg);

    void Init();
    void Bind();

private:
    HomeWindow& m_win;
    config::ConfigData& m_cfg;
};

}
}
