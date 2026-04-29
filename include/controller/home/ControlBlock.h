#pragma once

#include "config/ConfigModel.h"
class HomeWindow;

namespace controller {
namespace home {

class ControlBlockController {
public:
    ControlBlockController(HomeWindow& win, config::ConfigData& cfg);

    void Bind();

private:
    HomeWindow& m_win;
    config::ConfigData& m_cfg;
};

}
}
