#include "controller/print/Printer.h"

#include "printer/Printer.h"

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>

#include <iostream>

namespace controller::print {
namespace fs = std::filesystem;

namespace {

std::wstring Utf8ToWide(const std::string& text) {
    if (text.empty()) {
        return L"";
    }

    const int required = ::MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        text.c_str(),
        static_cast<int>(text.size()),
        nullptr,
        0
    );

    if (required <= 0) {
        std::wstring fallback;
        fallback.reserve(text.size());
        for (unsigned char c : text) {
            fallback.push_back(static_cast<wchar_t>(c));
        }
        return fallback;
    }

    std::wstring out(static_cast<size_t>(required), L'\0');
    ::MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        text.c_str(),
        static_cast<int>(text.size()),
        out.data(),
        required
    );
    return out;
}

std::string ToLowerCopy(std::string s) {
    for (char& ch : s) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return s;
}

bool IsBmpFile(const fs::directory_entry& entry) {
    if (!entry.is_regular_file()) {
        return false;
    }
    return ToLowerCopy(entry.path().extension().string()) == ".bmp";
}

// Hỗ trợ tên dạng: 12-d.bmp hoặc 12-i.bmp
bool ExtractPageNumber(const fs::path& filePath, int& pageOut) {
    const std::string stem = filePath.stem().string(); // ví dụ: "12-d"
    const std::size_t dashPos = stem.find('-');

    if (dashPos == std::string::npos || dashPos == 0) {
        return false;
    }

    const std::string pageStr = stem.substr(0, dashPos);
    const std::string suffix  = stem.substr(dashPos + 1);

    if (suffix != "d" && suffix != "i") {
        return false;
    }

    for (char c : pageStr) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }

    try {
        pageOut = std::stoi(pageStr);
        return pageOut > 0;
    } catch (...) {
        return false;
    }
}

int CountPagesInTempDir(const std::string& tempDir) {
    int maxPage = 0;

    try {
        const fs::path dirPath = fs::u8path(tempDir);

        if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) {
            return 0;
        }

        for (const auto& entry : fs::directory_iterator(dirPath)) {
            if (!IsBmpFile(entry)) {
                continue;
            }

            int page = 0;
            if (!ExtractPageNumber(entry.path(), page)) {
                continue;
            }

            if (page > maxPage) {
                maxPage = page;
            }
        }
    } catch (...) {
        return 0;
    }

    return maxPage;
}

std::string BuildPagePath(const std::string& tempDir, int page, char suffix) {
    fs::path p = fs::u8path(tempDir);
    p /= std::to_string(page) + "-" + suffix + ".bmp";
    return p.string();
}

bool FileExists(const std::string& path) {
    try {
        return fs::exists(fs::u8path(path));
    } catch (...) {
        return false;
    }
}

// Ưu tiên suffix đầu tiên, nếu không có thì fallback suffix thứ hai
std::string ResolvePagePath(
    const std::string& tempDir,
    int page,
    char preferSuffix,
    char fallbackSuffix
) {
    const std::string preferPath   = BuildPagePath(tempDir, page, preferSuffix);
    const std::string fallbackPath = BuildPagePath(tempDir, page, fallbackSuffix);

    if (FileExists(preferPath)) {
        return preferPath;
    }
    if (FileExists(fallbackPath)) {
        return fallbackPath;
    }
    return {};
}

} // namespace

void Printer::Init(
    config::ConfigData& cfg,
    const std::string& tempDir,
    std::atomic<bool>& cancelFlag,
    std::atomic<bool>& pauseFlag,
    ui::PrintWindow& win
) {
    std::lock_guard<std::mutex> lock(m_state.mtx);

    if (m_state.running) {
        return;
    }

    m_state.cfg = cfg;
    m_state.tempDir = tempDir;
    m_state.cancelFlag = &cancelFlag;
    m_state.pauseFlag = &pauseFlag;
    m_state.win = &win;

    cancelFlag.store(false, std::memory_order_relaxed);
    pauseFlag.store(false, std::memory_order_relaxed);

    m_state.running = true;
    m_state.worker = std::thread(&Printer::Run, this);
}

void Printer::Run() {
    auto* const cancelFlag = m_state.cancelFlag;
    auto* const pauseFlag   = m_state.pauseFlag;
    auto* const win         = m_state.win;

    if (!cancelFlag || !pauseFlag || !win) {
        return;
    }

    auto IsCancelled = [&]() -> bool {
        return cancelFlag->load(std::memory_order_relaxed);
    };

    auto IsPaused = [&]() -> bool {
        return pauseFlag->load(std::memory_order_relaxed);
    };

    auto RestoreNormalUI = [&]() {
        win->SetPrintProcessColor("green");
        win->SetAllowPause(true);
        win->SetAllowContinue(false);
        win->SetNotification(L"");
    };

    auto SetPausedUI = [&](const std::wstring& note) {
        win->SetPrintProcessColor("yellow");
        win->SetAllowPause(false);
        win->SetAllowContinue(true);

        if (!note.empty()) {
            win->SetNotification(note);
        }
    };

    auto WaitIfPaused = [&](const std::wstring& note = L"Printing paused. Press Continue to resume.") -> bool {
        if (!IsPaused()) {
            return !IsCancelled();
        }

        SetPausedUI(note);

        while (IsPaused()) {
            if (IsCancelled()) {
                return false;
            }
            ::Sleep(30);
        }

        if (IsCancelled()) {
            return false;
        }

        RestoreNormalUI();
        return true;
    };

    auto HandlePrintError = [&](const std::string& error, int page, int totalPages) -> bool {
        if (error.empty()) {
            return true;
        }

        win->SetPrintProcessColor("red");

        const std::wstring message =
            L"Printing error at page " + std::to_wstring(page) +
            L"/" + std::to_wstring(totalPages) +
            L":\n\n" + Utf8ToWide(error) +
            L"\n\nOK = Skip this page\nCancel = Stop printing";

        const int ret = ::MessageBoxW(
            nullptr,
            message.c_str(),
            L"Print error",
            MB_OKCANCEL | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND
        );

        if (ret == IDOK) {
            win->SetPrintProcessColor("green");
            return true; // skip page
        }

        cancelFlag->store(true, std::memory_order_relaxed);
        return false;
    };

    auto PrintSimplexOnePage = [&](int page, int totalPages, char preferSuffix, char fallbackSuffix) -> bool {
        if (!WaitIfPaused()) {
            return false;
        }

        const std::string path = ResolvePagePath(m_state.tempDir, page, preferSuffix, fallbackSuffix);
        std::string error;

        if (path.empty()) {
            error = "Missing bitmap file for page " + std::to_string(page);
        } else {
            printer::PrintSimplex(path, error);
        }

        if (!HandlePrintError(error, page, totalPages)) {
            return false;
        }

        win->SetPrintProcess(totalPages, page);
        return !IsCancelled();
    };

    auto PrintDuplexPair = [&](int firstPage, int secondPage, int totalPages) -> bool {
        if (!WaitIfPaused()) {
            return false;
        }

        const std::string frontPath = ResolvePagePath(m_state.tempDir, firstPage, 'd', 'i');
        const std::string backPath  = ResolvePagePath(m_state.tempDir, secondPage, 'i', 'd');

        std::string error;

        if (frontPath.empty()) {
            error = "Missing bitmap file for page " + std::to_string(firstPage);
        } else if (backPath.empty()) {
            error = "Missing bitmap file for page " + std::to_string(secondPage);
        } else {
            printer::PrintDuplex(frontPath, backPath, error);
        }

        if (!HandlePrintError(error, secondPage, totalPages)) {
            return false;
        }

        win->SetPrintProcess(totalPages, secondPage);
        return !IsCancelled();
    };

    auto WaitForManualFlip = [&](const std::wstring& note) -> bool {
        win->SetPrintProcessColor("yellow");
        win->SetAllowPause(false);
        win->SetAllowContinue(true);
        win->SetNotification(note);

        pauseFlag->store(true, std::memory_order_relaxed);

        while (IsPaused()) {
            if (IsCancelled()) {
                return false;
            }
            ::Sleep(30);
        }

        if (IsCancelled()) {
            return false;
        }

        RestoreNormalUI();
        return true;
    };

    try {
        printer::Init(m_state.cfg);

        const int pages = CountPagesInTempDir(m_state.tempDir);

        win->SetPrintProcessLabel(L"Printing...");
        win->SetPrintProcessColor("green");
        win->SetAllowPause(true);
        win->SetAllowContinue(false);
        win->SetNotification(L"");
        win->SetPrintProcess(pages, 0);

        if (pages <= 0) {
            win->SetPrintProcessColor("red");
            win->SetNotification(L"No bitmap pages found in the temp folder.");
            return;
        }

        const std::string mode = m_state.cfg.printMode;

        if (mode == "Simplex") {
            for (int page = 1; page <= pages; ++page) {
                if (IsCancelled()) {
                    return;
                }

                if (!PrintSimplexOnePage(page, pages, 'd', 'i')) {
                    return;
                }
            }
        }
        else if (mode == "Duplex") {
            for (int page = 1; page <= pages; page += 2) {
                if (IsCancelled()) {
                    return;
                }

                if (!PrintDuplexPair(page, page + 1, pages)) {
                    return;
                }
            }
        }
        else if (mode == "Manual Duplex (Flip On Long Edge)") {
            for (int page = 1; page < pages; page += 2) {
                if (IsCancelled()) {
                    return;
                }

                if (!PrintSimplexOnePage(page, pages, 'd', 'i')) {
                    return;
                }
            }

            if (!WaitForManualFlip(L"Flip the paper on the long edge.")) {
                return;
            }

            for (int page = 2; page <= pages; page += 2) {
                if (IsCancelled()) {
                    return;
                }

                if (!PrintSimplexOnePage(page, pages, 'i', 'd')) {
                    return;
                }
            }
        }
        else if (mode == "Manual Duplex (Flip On Short Edge)") {
            for (int page = 1; page < pages; page += 2) {
                if (IsCancelled()) {
                    return;
                }

                if (!PrintSimplexOnePage(page, pages, 'd', 'i')) {
                    return;
                }
            }

            if (!WaitForManualFlip(L"Flip the paper on the short edge.")) {
                return;
            }

            for (int page = pages; page >= 2; page -= 2) {
                if (IsCancelled()) {
                    return;
                }

                if (!PrintSimplexOnePage(page, pages, 'i', 'd')) {
                    return;
                }
            }
        }
        else {
            // Fallback an toàn: in simplex
            for (int page = 1; page <= pages; ++page) {
                if (IsCancelled()) {
                    return;
                }

                if (!PrintSimplexOnePage(page, pages, 'd', 'i')) {
                    return;
                }
            }
        }

        if (!IsCancelled()) {
            win->SetPrintProcessColor("green");
            win->SetNotification(L"Printing finished.");
        }
    }
    catch (...) {
        win->SetPrintProcessColor("red");
        win->SetNotification(L"Unexpected printing error.");
        cancelFlag->store(true, std::memory_order_relaxed);
    }
}

void Printer::Destroy() {
    std::thread workerToJoin;

    {
        std::lock_guard<std::mutex> lock(m_state.mtx);

        if (m_state.cancelFlag) {
            m_state.cancelFlag->store(true, std::memory_order_relaxed);
        }
        if (m_state.pauseFlag) {
            m_state.pauseFlag->store(false, std::memory_order_relaxed);
        }

        if (m_state.worker.joinable()) {
            workerToJoin = std::move(m_state.worker);
        }
    }

    if (workerToJoin.joinable()) {
        workerToJoin.join();
    }

    printer::ShutDown();

    {
        std::lock_guard<std::mutex> lock(m_state.mtx);

        m_state.running = false;
        m_state.cfg = config::ConfigData{};
        m_state.tempDir.clear();
        m_state.cancelFlag = nullptr;
        m_state.pauseFlag = nullptr;
        m_state.win = nullptr;
    }
}

} // namespace controller::print