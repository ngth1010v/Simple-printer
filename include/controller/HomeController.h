#pragma once

#include "config/ConfigModel.h"

namespace controller {

class HomeController {
public:
    explicit HomeController(config::ConfigData& cfg);

    int Run();

private:
    config::ConfigData& m_cfg;
};
}
