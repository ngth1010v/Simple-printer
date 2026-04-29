#pragma once

#include "config/ConfigModel.h"
class HomeWindow;

namespace controller {
namespace home {

class InfoConfigSectionController {
public:
    InfoConfigSectionController(HomeWindow& win, config::ConfigData& cfg);

    void Init();

private:
    HomeWindow& m_win;
    config::ConfigData& m_cfg;
};

}
}
