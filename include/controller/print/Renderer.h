#pragma once

#include <string>
#include <atomic>
#include <vector>

#include "config/ConfigModel.h"
#include "ui/PrintWindow.h"

// namespace ui {
// class PrintWindow;
// }

namespace controller::print {

class Renderer {
public:
    void Init(config::ConfigData& cfg,
              std::string tempDir,
              int dpi,
              std::atomic<bool>& cancelFlag,
              ui::PrintWindow& win);

    void Run();
    void Destroy();
    bool IsDone() const { return done_.load(std::memory_order_relaxed); }
    

private:
    std::string BuildTargetPath(const std::string& inputPath, int index) const;
    std::atomic<bool> done_ = false;

private:
    config::ConfigData* cfg_ = nullptr;
    std::string tempDir_;
    int dpi_ = 300;

    std::atomic<bool>* cancelFlag_ = nullptr;
    ui::PrintWindow* win_ = nullptr;

    std::atomic<int> successPages_ = 0;
    std::atomic<int> donePages_ = 0;
    int totalPages_ = 0;
};

} // namespace controller::print