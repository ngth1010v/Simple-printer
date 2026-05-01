#pragma once
#include <windows.h>
#include <atomic>
#include <vector>
#include <string>

#include "config/ConfigModel.h"

namespace controller::print {

class Counter {
public:
    static void Init(
        config::ConfigData& cfg,
        std::atomic<bool>& runFlag,
        std::atomic<bool>& cancelFlag
    );

    static void Destroy();

private:
    static void OnCountDone(
        config::ConfigData& cfg,
        size_t index,
        int pages,
        const std::string& error
    );

    static int ShowErrorBox(const std::string& msg, const std::string& title, UINT type);

private:
    static inline std::atomic<bool>* runFlag_    = nullptr;
    static inline std::atomic<bool>* cancelFlag_ = nullptr;

    static inline std::atomic<int>  finishedCount_ = 0;
    static inline int               totalCount_    = 0;

    static inline std::vector<bool> removeMask_;
};

} // namespace controller::print