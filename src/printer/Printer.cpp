// printer/Printer.cpp
#include "printer/Printer.h"

#include "printer/WinPrinter.h"
#include "printer/WinSpooler.h"

#include <mutex>
#include <utility>
#include <vector>

namespace printer {
namespace {

struct State {
    config::ConfigData cfg;
    bool initialized = false;
    std::mutex mtx;
};

State& GetState() {
    static State s;
    return s;
}

int PrintInternal(const std::vector<std::wstring>& paths, bool duplex, std::string& error) {
    State& state = GetState();

    config::ConfigData cfgCopy;
    {
        std::lock_guard<std::mutex> lock(state.mtx);
        if (!state.initialized) {
            error = "printer::Init() has not been called.";
            return 1;
        }
        cfgCopy = state.cfg;
    }

    HDC hdc = nullptr;
    PrinterPageInfo pageInfo{};

    if (!OpenPrinterDc(cfgCopy.printer, cfgCopy.paper, duplex, hdc, pageInfo, error)) {
        return 1;
    }

    std::vector<BmpImage> pages;
    pages.reserve(paths.size());

    for (const auto& path : paths) {
        BmpImage page;

        if (!BuildPageImage(
                path,
                cfgCopy.orientation,
                cfgCopy.scale,
                cfgCopy.margin,
                hdc,
                pageInfo.horzRes,
                pageInfo.vertRes,
                page,
                error)) {
            if (hdc != nullptr) {
                DeleteDC(hdc);
            }
            return 1;
        }

        pages.emplace_back(std::move(page));
    }

    const bool ok = PrintBitmapPages(hdc, pages, error);

    if (hdc != nullptr) {
        DeleteDC(hdc);
    }

    return ok ? 0 : 1;
}

} // namespace

void Init(config::ConfigData& cfg) {
    State& state = GetState();

    std::lock_guard<std::mutex> lock(state.mtx);

    state.cfg = cfg;
    state.initialized = true;
}

int PrintSimplex(std::wstring path, std::string& error) {
    return PrintInternal(
        std::vector<std::wstring>{std::move(path)},
        false,
        error
    );
}

int PrintDuplex(std::wstring frontPath, std::wstring backPath, std::string& error) {
    return PrintInternal(
        std::vector<std::wstring>{
            std::move(frontPath),
            std::move(backPath)
        },
        true,
        error
    );
}

void ShutDown() {
    State& state = GetState();

    std::lock_guard<std::mutex> lock(state.mtx);

    state.cfg = config::ConfigData{};
    state.initialized = false;
}

} // namespace printer