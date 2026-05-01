#pragma once

#include "config/ConfigModel.h"

namespace controller {

class PrintController {
public:
    explicit PrintController(const config::ConfigData& cfg);

    int Run();

private:
    config::ConfigData m_cfg;
};

} // namespace controller