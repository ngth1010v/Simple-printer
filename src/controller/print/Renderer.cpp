#include "controller/print/Renderer.h"

#include <windows.h>
#include <sstream>
#include <iostream>

#include "renderer/Renderer.h"

namespace controller::print {

void Renderer::Init(config::ConfigData& cfg,
                    std::string tempDir,
                    int dpi,
                    std::atomic<bool>& cancelFlag,
                    ui::PrintWindow& win) {
    cfg_ = &cfg;
    tempDir_ = std::move(tempDir);
    dpi_ = dpi;
    cancelFlag_ = &cancelFlag;
    win_ = &win;

    successPages_.store(0);
    donePages_.store(0);
    totalPages_ = 0;

    // count total pages
    for (const auto& f : cfg_->files) {

        int from = f.fromRange;
        int to   = f.toRange;

        if (from <= 0) from = 1;
        if (to <= 0 || to > f.pages) to = f.pages;

        if (to >= from) {
            totalPages_ += (to - from + 1);
        }
    }
}

void Renderer::Run() {
    if (!cfg_ || !win_ || !cancelFlag_) return;

    win_->SetResourceProcessLabel(L"Rendering resources...");  
    win_->SetResourceProcessColor("green");
    win_->SetResourceProcess(totalPages_, 0);

    renderer::Init(dpi_);

    int index = 1;

    for (const auto& f : cfg_->files) {
        if (!f.loaded) continue;

        int from = f.fromRange;
        int to   = f.toRange;

        if (from <= 0) from = 1;
        if (to <= 0 || to > f.pages) to = f.pages;

        for (int p = from; p <= to; ++p) {
            if (cancelFlag_->load(std::memory_order_relaxed)) return;

            std::string target = BuildTargetPath(index++);

            renderer::Render(
                f.path,
                target,
                p,
                [this](std::string error) {

                    if (cancelFlag_->load(std::memory_order_relaxed)) return;

                    if (!error.empty()) {
                        win_->SetResourceProcessColor("red");

                        std::wstring werr(error.begin(), error.end());

                        int res = MessageBoxW(
                            nullptr,
                            werr.c_str(),
                            L"Render Error",
                            MB_ICONERROR | MB_ABORTRETRYIGNORE);

                        if (res == IDABORT) {
                            cancelFlag_->store(true, std::memory_order_relaxed);
                            return;
                        }

                        // skip
                        win_->SetResourceProcessColor("green");

                        int done = ++donePages_;
                        int success = successPages_.load();

                        win_->SetResourceProcess(totalPages_, success);

                        if (done == totalPages_) {
                            win_->SetResourceProcessLabel(L"Rendering resources done.");
                        }
                        return;
                    }

                    // success
                    int success = ++successPages_;
                    int done = ++donePages_;

                    win_->SetResourceProcess(totalPages_, success);

                    if (done == totalPages_) {
                        win_->SetResourceProcessLabel(L"Rendering resources done.");
                    }
                });
        }
    }
}

std::string Renderer::BuildTargetPath(int index) const {
    std::ostringstream oss;
    oss << tempDir_ << "/" << index << ".bmp";
    return oss.str();
}

void Renderer::Destroy() {
    renderer::Shutdown();
}

} // namespace controller::print

