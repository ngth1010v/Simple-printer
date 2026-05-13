#pragma once

#include "config/ConfigModel.h"
#include "ui/PrintWindow.h"

#include <atomic>
#include <string>
#include <mutex>
#include <thread>

namespace controller::print {

class Printer {
public:
    void Init(
        config::ConfigData& cfg,
        const std::wstring& tempDir,
        std::atomic<bool>& cancelFlag,
        std::atomic<bool>& pauseFlag,
        ui::PrintWindow& win
    );

    void Run();
    void Destroy();

private:
    struct State {
        std::mutex mtx;
        std::thread worker;
        bool running = false;

        config::ConfigData cfg;
        std::wstring tempDir;

        std::atomic<bool>* cancelFlag = nullptr;
        std::atomic<bool>* pauseFlag  = nullptr;
        ui::PrintWindow* win          = nullptr;
    };

    State m_state;
};

} // namespace controller::print