#include "controller/print/Printer.h"

#include "printer/Printer.h"

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>

#include <fstream>
#include <cstdint>

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

#pragma pack(push, 1)
struct BmpFileHeader {
    std::uint16_t bfType = 0x4D42; // "BM"
    std::uint32_t bfSize = 0;
    std::uint16_t bfReserved1 = 0;
    std::uint16_t bfReserved2 = 0;
    std::uint32_t bfOffBits = 0;
};
#pragma pack(pop)

constexpr const char* kEmptyBmpName = "empty.bmp";

bool IsDuplexLikeMode(const std::string& mode) {
    return mode == "Duplex" ||
           mode == "Manual Duplex (Flip On Long Edge)" ||
           mode == "Manual Duplex (Flip On Short Edge)";
}

bool NeedPadBlankPage(const std::string& mode, int pages) {
    return pages > 1 && IsDuplexLikeMode(mode) && (pages % 2 == 1);
}

std::string EmptyBmpPath(const std::string& tempDir) {
    fs::path p = fs::u8path(tempDir);
    p /= kEmptyBmpName;
    return p.string();
}

bool CreateEmptyBmpFile(const std::string& tempDir, std::string& outPath, std::string& error) {
    const fs::path filePath = fs::u8path(EmptyBmpPath(tempDir));
    outPath = filePath.string();

    try {
        if (fs::exists(filePath)) {
            return true;
        }

        if (!fs::exists(filePath.parent_path())) {
            fs::create_directories(filePath.parent_path());
        }
    } catch (...) {
        error = "Failed to prepare folder for empty.bmp";
        return false;
    }

    // 1x1 white BMP, đủ hợp lệ để printer::BuildPageImage scale lên
    BmpFileHeader fileHeader{};
    BITMAPINFOHEADER infoHeader{};

    fileHeader.bfOffBits = static_cast<std::uint32_t>(sizeof(BmpFileHeader) + sizeof(BITMAPINFOHEADER));
    fileHeader.bfSize = fileHeader.bfOffBits + 4; // 1 pixel BGRA

    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = 1;
    infoHeader.biHeight = 1; // bottom-up
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 32;
    infoHeader.biCompression = BI_RGB;
    infoHeader.biSizeImage = 4;

    const std::uint32_t whitePixel = 0xFFFFFFFFu;

    std::ofstream out(filePath, std::ios::binary);
    if (!out) {
        error = "Failed to create empty.bmp";
        return false;
    }

    out.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
    out.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));
    out.write(reinterpret_cast<const char*>(&whitePixel), sizeof(whitePixel));

    if (!out) {
        error = "Failed to write empty.bmp";
        return false;
    }

    return true;
}

std::string ResolvePagePathOrEmpty(
    const std::string& tempDir,
    int page,
    int actualPages,
    char preferSuffix,
    char fallbackSuffix,
    const std::string& emptyBmpPath
) {
    if (page > actualPages) {
        return emptyBmpPath;
    }
    return ResolvePagePath(tempDir, page, preferSuffix, fallbackSuffix);
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
    auto* const pauseFlag  = m_state.pauseFlag;
    auto* const win        = m_state.win;

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
            return true;
        }

        cancelFlag->store(true, std::memory_order_relaxed);
        return false;
    };

    auto CreateEmptyBmpFile = [&](const std::string& tempDir, std::string& outPath, std::string& error) -> bool {
        fs::path filePath = fs::u8path(tempDir);
        filePath /= "empty.bmp";
        outPath = filePath.string();

        try {
            if (fs::exists(filePath)) {
                return true;
            }

            if (!fs::exists(filePath.parent_path())) {
                fs::create_directories(filePath.parent_path());
            }
        } catch (...) {
            error = "Failed to prepare folder for empty.bmp";
            return false;
        }

#pragma pack(push, 1)
        struct BmpFileHeader {
            WORD  bfType;
            DWORD bfSize;
            WORD  bfReserved1;
            WORD  bfReserved2;
            DWORD bfOffBits;
        };

        struct BmpInfoHeader {
            DWORD biSize;
            LONG  biWidth;
            LONG  biHeight;
            WORD  biPlanes;
            WORD  biBitCount;
            DWORD biCompression;
            DWORD biSizeImage;
            LONG  biXPelsPerMeter;
            LONG  biYPelsPerMeter;
            DWORD biClrUsed;
            DWORD biClrImportant;
        };
#pragma pack(pop)

        BmpFileHeader fh{};
        BmpInfoHeader ih{};

        fh.bfType = 0x4D42; // "BM"
        fh.bfOffBits = sizeof(BmpFileHeader) + sizeof(BmpInfoHeader);
        fh.bfSize = fh.bfOffBits + 4; // 1 pixel BGRA

        ih.biSize = sizeof(BmpInfoHeader);
        ih.biWidth = 1;
        ih.biHeight = 1; // bottom-up
        ih.biPlanes = 1;
        ih.biBitCount = 32;
        ih.biCompression = BI_RGB;
        ih.biSizeImage = 4;

        const DWORD whitePixel = 0xFFFFFFFFu;

        HANDLE hFile = ::CreateFileW(
            filePath.c_str(),
            GENERIC_WRITE,
            0,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );

        if (hFile == INVALID_HANDLE_VALUE) {
            error = "Failed to create empty.bmp";
            return false;
        }

        bool ok = true;
        DWORD written = 0;

        ok = ok && ::WriteFile(hFile, &fh, sizeof(fh), &written, nullptr) && written == sizeof(fh);
        ok = ok && ::WriteFile(hFile, &ih, sizeof(ih), &written, nullptr) && written == sizeof(ih);
        ok = ok && ::WriteFile(hFile, &whitePixel, sizeof(whitePixel), &written, nullptr) && written == sizeof(whitePixel);

        ::CloseHandle(hFile);

        if (!ok) {
            ::DeleteFileW(filePath.c_str());
            error = "Failed to write empty.bmp";
            return false;
        }

        return true;
    };

    try {
        printer::Init(m_state.cfg);

        const int pages = CountPagesInTempDir(m_state.tempDir);
        const std::string mode = m_state.cfg.printMode;

        const bool isDuplexLike =
            mode == "Duplex" ||
            mode == "Manual Duplex (Flip On Long Edge)" ||
            mode == "Manual Duplex (Flip On Short Edge)";

        const bool padBlankPage = (pages > 1) && isDuplexLike && ((pages % 2) == 1);
        const int jobPages = padBlankPage ? (pages + 1) : pages;

        std::string emptyBmpPath;
        if (padBlankPage) {
            std::string createError;
            if (!CreateEmptyBmpFile(m_state.tempDir, emptyBmpPath, createError)) {
                win->SetPrintProcessColor("red");
                win->SetNotification(Utf8ToWide(createError));
                return;
            }
        }

        auto ResolvePagePathOrEmpty = [&](int page, char preferSuffix, char fallbackSuffix) -> std::string {
            if (page > pages) {
                return emptyBmpPath;
            }
            return ResolvePagePath(m_state.tempDir, page, preferSuffix, fallbackSuffix);
        };


        auto PrintSimplexOnePage = [&](int page, int totalPages, int completedPages, char preferSuffix, char fallbackSuffix) -> bool {
            if (!WaitIfPaused()) {
                return false;
            }

            const std::string path = ResolvePagePathOrEmpty(page, preferSuffix, fallbackSuffix);
            std::string error;

            if (path.empty()) {
                error = "Missing bitmap file for page " + std::to_string(page);
            } else {
                printer::PrintSimplex(path, error);
            }

            if (!HandlePrintError(error, page, totalPages)) {
                return false;
            }

            win->SetPrintProcess(totalPages, completedPages);
            return !IsCancelled();
        };

        auto PrintDuplexPair = [&](int firstPage, int secondPage, int totalPages, int completedPages) -> bool {
            if (!WaitIfPaused()) {
                return false;
            }

            const std::string frontPath = ResolvePagePathOrEmpty(firstPage, 'd', 'i');
            const std::string backPath  = ResolvePagePathOrEmpty(secondPage, 'i', 'd');

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

            win->SetPrintProcess(totalPages, completedPages);
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

        win->SetPrintProcessLabel(L"Printing...");
        win->SetPrintProcessColor("green");
        win->SetAllowPause(true);
        win->SetAllowContinue(false);
        win->SetNotification(L"");
        win->SetPrintProcess(jobPages, 0);


        if (pages <= 0) {
            win->SetPrintProcessColor("red");
            win->SetNotification(L"No bitmap pages found in the temp folder.");
            return;
        }

        if (mode == "Simplex") {
            int printedCount = 0;
            for (int page = 1; page <= pages; ++page) {
                if (IsCancelled()) {
                    return;
                }

                if (!PrintSimplexOnePage(page, pages, ++printedCount, 'd', 'i')) {
                    return;
                }
            }
        }
        else if (mode == "Duplex") {
            int printedCount = 0;

            for (int page = 1; page <= jobPages; page += 2) {
                if (IsCancelled()) {
                    return;
                }

                if (!PrintDuplexPair(page, page + 1, jobPages, printedCount + 2)) {
                    return;
                }

                printedCount += 2;
            }
        }
        else if (mode == "Manual Duplex (Flip On Long Edge)") {
            int printedCount = 0;

            for (int page = 1; page <= jobPages; page += 2) {
                if (IsCancelled()) {
                    return;
                }

                if (!PrintSimplexOnePage(page, jobPages, ++printedCount, 'd', 'i')) {
                    return;
                }
            }

            if (!WaitForManualFlip(L"Flip the paper on the long edge.")) {
                return;
            }

            for (int page = 2; page <= jobPages; page += 2) {
                if (IsCancelled()) {
                    return;
                }

                if (!PrintSimplexOnePage(page, jobPages, ++printedCount, 'i', 'd')) {
                    return;
                }
            }
        }
        else if (mode == "Manual Duplex (Flip On Short Edge)") {
            int printedCount = 0;

            for (int page = 1; page <= jobPages; page += 2) {
                if (IsCancelled()) {
                    return;
                }

                if (!PrintSimplexOnePage(page, jobPages, ++printedCount, 'd', 'i')) {
                    return;
                }
            }

            if (!WaitForManualFlip(L"Flip the paper on the short edge.")) {
                return;
            }

            for (int page = jobPages; page >= 2; page -= 2) {
                if (IsCancelled()) {
                    return;
                }

                if (!PrintSimplexOnePage(page, jobPages, ++printedCount, 'i', 'd')) {
                    return;
                }
            }
        }
        else {
            int printedCount = 0;
            for (int page = 1; page <= pages; ++page) {
                if (IsCancelled()) {
                    return;
                }

                if (!PrintSimplexOnePage(page, pages, ++printedCount, 'd', 'i')) {
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