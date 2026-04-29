#pragma once


#include "config/ConfigModel.h"

class HomeWindow;

namespace controller {
namespace home {

class BasicConfigSectionController {
public:
    BasicConfigSectionController(HomeWindow& win, config::ConfigData& cfg);

    void Init();
    void Bind();

private:
    HomeWindow& m_win;
    config::ConfigData& m_cfg;
};

}
}
