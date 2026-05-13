#include "controller/print/Counter.h"

#include <windows.h>
#include <algorithm>

#include "counter/Counter.h"

std::string WideToUtf8(const std::wstring& ws)
{
    if (ws.empty()) {
        return {};
    }

    int size = WideCharToMultiByte(
        CP_UTF8,
        0,
        ws.c_str(),
        -1,
        nullptr,
        0,
        nullptr,
        nullptr
    );

    if (size <= 0) {
        return {};
    }

    std::string result(static_cast<size_t>(size - 1), '\0');

    WideCharToMultiByte(
        CP_UTF8,
        0,
        ws.c_str(),
        -1,
        result.data(),
        size,
        nullptr,
        nullptr
    );

    return result;
}

namespace controller::print {

void Counter::Init(
    config::ConfigData& cfg,
    std::atomic<bool>& runFlag,
    std::atomic<bool>& cancelFlag
) {
    runFlag_    = &runFlag;
    cancelFlag_ = &cancelFlag;

    runFlag_->store(false, std::memory_order_relaxed);
    cancelFlag_->store(false, std::memory_order_relaxed);

    counter::Init();

    totalCount_ = static_cast<int>(cfg.files.size());
    finishedCount_.store(0, std::memory_order_relaxed);

    removeMask_.assign(cfg.files.size(), false);

    if (totalCount_ == 0) {
        runFlag_->store(true, std::memory_order_relaxed);
        return;
    }

    for (size_t i = 0; i < cfg.files.size(); ++i) {
        auto& file = cfg.files[i];

        counter::Count(file.path, [&, i](int pages, const std::string& error) {
            OnCountDone(cfg, i, pages, error);
        });
    }
}

void Counter::OnCountDone(
    config::ConfigData& cfg,
    size_t index,
    int pages,
    const std::string& error
) {
    if (cancelFlag_->load(std::memory_order_relaxed)) {
        return;
    }
 
    auto& file = cfg.files[index];

    if (!error.empty() || pages <= 0) {
        std::string msg = "Failed to read file:\n" + WideToUtf8(file.path) + "\n\nError: " + error;

        int res = ShowErrorBox(
            msg,
            "Count Error",
            MB_ICONERROR | MB_ABORTRETRYIGNORE
        );

        if (res == IDIGNORE) { // skip
            removeMask_[index] = true;
        } else { // cancel
            cancelFlag_->store(true, std::memory_order_relaxed);
            return;
        }
    } else {
        file.pages  = pages;
        file.loaded = true;

        if (file.fromRange == 0 && file.toRange == 0) {
            file.fromRange = 1;
            file.toRange   = pages;
        } else {
            bool valid =
                file.fromRange > 0 &&
                file.toRange   <= pages &&
                file.fromRange <= file.toRange;

            if (!valid) {
                std::string msg =
                    "Range mismatch:\n" + WideToUtf8(file.path) +
                    "\n\nFile may have changed.\n";

                int res = ShowErrorBox(
                    msg,
                    "Range Error",
                    MB_ICONWARNING | MB_ABORTRETRYIGNORE
                );

                if (res == IDIGNORE) { // skip
                    removeMask_[index] = true;
                } else if (res == IDRETRY) { // print all
                    file.fromRange = 1;
                    file.toRange   = pages;
                } else { // cancel
                    cancelFlag_->store(true, std::memory_order_relaxed);
                    return;
                }
            }
        }
    }

    int done = finishedCount_.fetch_add(1) + 1;

    if (done == totalCount_) {
        if (cancelFlag_->load(std::memory_order_relaxed)) {
            return;
        }

        // remove skipped files
        for (int i = static_cast<int>(cfg.files.size()) - 1; i >= 0; --i) {
            if (removeMask_[i]) {
                cfg.files.erase(cfg.files.begin() + i);
            }
        }

        runFlag_->store(true, std::memory_order_relaxed);
    }
}

int Counter::ShowErrorBox(const std::string& msg, const std::string& title, UINT type) {
    return MessageBoxA(
        nullptr,
        msg.c_str(),
        title.c_str(),
        type
    );
}

void Counter::Destroy() {
    counter::Shutdown();

    runFlag_    = nullptr;
    cancelFlag_ = nullptr;

    finishedCount_.store(0, std::memory_order_relaxed);
    totalCount_ = 0;
    removeMask_.clear();
}

} // namespace controller::print