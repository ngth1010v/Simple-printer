#pragma once

#include "config/ConfigModel.h"
#include <functional>

class HomeWindow;

namespace controller {
namespace home {

class AdvangeConfigSectionController {
public:
    using ChangeCallback = std::function<void()>;

    AdvangeConfigSectionController(HomeWindow& win, config::ConfigData& cfg);

    void Init(ChangeCallback cb);
    void Bind();

private:
    HomeWindow& m_win;
    config::ConfigData& m_cfg;

    ChangeCallback m_cb;
    void Fire(const ChangeCallback& cb);
};

}
}