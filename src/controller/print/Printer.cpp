#include "controller/print/Printer.h"

#include "printer/Printer.h"

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <cwctype>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <fstream>
#include <cstdint>

#include <iostream>

constexpr double kWhiteLumaThreshold = 252.0;
constexpr double kWhiteRatioThreshold = 0.9995;

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

std::wstring ToLowerCopy(std::wstring s) {
    for (wchar_t& ch : s) {
        ch = static_cast<wchar_t>(std::towlower(ch));
    }
    return s;
}

bool IsBmpFile(const fs::directory_entry& entry) {
    if (!entry.is_regular_file()) {
        return false;
    }
    return ToLowerCopy(entry.path().extension().wstring()) == L".bmp";
}

// Hỗ trợ tên dạng: 12-d.bmp hoặc 12-i.bmp
bool ExtractPageNumber(const fs::path& filePath, int& pageOut) {
    const std::wstring stem = filePath.stem().wstring(); // ví dụ: L"12-d", L"12-D", L"12"
    const std::size_t dashPos = stem.find(L'-');

    std::wstring pageStr = stem;
    std::wstring suffix;

    if (dashPos != std::wstring::npos) {
        if (dashPos == 0) {
            return false;
        }

        pageStr = stem.substr(0, dashPos);
        suffix  = stem.substr(dashPos + 1);
        suffix  = ToLowerCopy(suffix);

        // Chỉ chấp nhận hậu tố d / i nếu có dấu '-'
        if (suffix != L"d" && suffix != L"i") {
            return false;
        }
    }

    if (pageStr.empty()) {
        return false;
    }

    for (wchar_t c : pageStr) {
        if (!std::iswdigit(c)) {
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

#pragma pack(push, 1)
struct BmpFileHeader {
    std::uint16_t bfType = 0x4D42; // "BM"
    std::uint32_t bfSize = 0;
    std::uint16_t bfReserved1 = 0;
    std::uint16_t bfReserved2 = 0;
    std::uint32_t bfOffBits = 0;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BmpInfoHeader {
    std::uint32_t biSize = 0;
    std::int32_t  biWidth = 0;
    std::int32_t  biHeight = 0;
    std::uint16_t biPlanes = 0;
    std::uint16_t biBitCount = 0;
    std::uint32_t biCompression = 0;
    std::uint32_t biSizeImage = 0;
    std::int32_t  biXPelsPerMeter = 0;
    std::int32_t  biYPelsPerMeter = 0;
    std::uint32_t biClrUsed = 0;
    std::uint32_t biClrImportant = 0;
};
#pragma pack(pop)

struct PageBmp {
    int page = 0;
    fs::path path;
};

std::vector<PageBmp> CollectPageBmpsInTempDir(const std::wstring& tempDir) {
    std::vector<PageBmp> out;

    try {
        const fs::path dirPath = fs::path(tempDir);
        if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) {
            return {};
        }

        for (const auto& entry : fs::directory_iterator(dirPath)) {
            if (!IsBmpFile(entry)) {
                continue;
            }

            int page = 0;
            if (ExtractPageNumber(entry.path(), page)) {
                out.push_back(PageBmp{ page, entry.path() });
            }
        }

        std::sort(out.begin(), out.end(),
            [](const PageBmp& a, const PageBmp& b) {
                return a.page < b.page;
            });

        out.erase(std::unique(out.begin(), out.end(),
            [](const PageBmp& a, const PageBmp& b) {
                return a.page == b.page;
            }), out.end());
    }
    catch (...) {
        return {};
    }

    return out;
}

bool IsBlankBmp(const std::wstring& bmpPath) {
    std::ifstream in(fs::path(bmpPath), std::ios::binary);
    if (!in) {
        return false; // không đọc được thì đừng coi là blank
    }

    BmpFileHeader fileHeader{};
    BmpInfoHeader infoHeader{};

    in.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    in.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));
    if (!in) {
        return false;
    }

    if (fileHeader.bfType != 0x4D42) { // "BM"
        return false;
    }

    if (infoHeader.biPlanes != 1 || infoHeader.biCompression != BI_RGB) {
        return false;
    }

    if (infoHeader.biBitCount != 24 && infoHeader.biBitCount != 32) {
        return false;
    }

    if (infoHeader.biWidth <= 0 || infoHeader.biHeight == 0) {
        return false;
    }

    const int width  = infoHeader.biWidth;
    const int height = (infoHeader.biHeight < 0) ? -infoHeader.biHeight : infoHeader.biHeight;

    const std::size_t bytesPerPixel = static_cast<std::size_t>(infoHeader.biBitCount / 8);
    const std::size_t rowBytes = ((static_cast<std::size_t>(width) * infoHeader.biBitCount + 31) / 32) * 4;

    std::vector<unsigned char> row(rowBytes);

    in.seekg(static_cast<std::streamoff>(fileHeader.bfOffBits), std::ios::beg);
    if (!in) {
        return false;
    }

    // Chỉ cần có đủ số pixel tối thiểu "không trắng" là coi như không blank.
    // Ngưỡng này an toàn hơn whiteRatio > 0.9995.
    constexpr double kDarkLumaThreshold = 245.0;
    constexpr std::size_t kMinDarkPixels = 32;
    constexpr double kDarkPixelRatio = 0.00002; // 0.002%

    const std::size_t totalPixels = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    const std::size_t neededDarkPixels = std::max(
        kMinDarkPixels,
        static_cast<std::size_t>(static_cast<double>(totalPixels) * kDarkPixelRatio)
    );

    std::size_t darkPixels = 0;

    for (int y = 0; y < height; ++y) {
        in.read(reinterpret_cast<char*>(row.data()), static_cast<std::streamsize>(row.size()));
        if (!in) {
            return false;
        }

        for (int x = 0; x < width; ++x) {
            const unsigned char* px = row.data() + static_cast<std::size_t>(x) * bytesPerPixel;

            const double b = px[0];
            const double g = px[1];
            const double r = px[2];
            const double luma = 0.299 * r + 0.587 * g + 0.114 * b;

            if (luma < kDarkLumaThreshold) {
                if (++darkPixels >= neededDarkPixels) {
                    return false; // có nội dung -> không blank
                }
            }
        }
    }

    return true; // quá ít pixel tối -> blank
}

std::wstring BuildPagePath(const std::wstring& tempDir, int page, char suffix) {
    fs::path p = fs::path(tempDir);
    p /= std::to_wstring(page) + L"-" + static_cast<wchar_t>(suffix) + L".bmp";
    return p.wstring();
}

bool FileExists(const std::wstring& path) {
    try {
        return fs::exists(fs::path(path));
    } catch (...) {
        return false;
    }
}

// Ưu tiên suffix đầu tiên, nếu không có thì fallback suffix thứ hai
std::wstring ResolvePagePath(
    const std::wstring& tempDir,
    int page,
    char preferSuffix,
    char fallbackSuffix
) {
    const std::wstring preferPath   = BuildPagePath(tempDir, page, preferSuffix);
    const std::wstring fallbackPath  = BuildPagePath(tempDir, page, fallbackSuffix);

    if (FileExists(preferPath)) {
        return preferPath;
    }
    if (FileExists(fallbackPath)) {
        return fallbackPath;
    }
    return L"";
}

std::vector<int> BuildPrintablePages(
    const std::wstring& tempDir,
    bool skipBlankPage
) {
    const std::vector<PageBmp> pages = CollectPageBmpsInTempDir(tempDir);

    std::vector<int> out;
    out.reserve(pages.size());

    if (!skipBlankPage) {
        for (const auto& item : pages) {
            out.push_back(item.page);
        }
        return out;
    }

    for (const auto& item : pages) {
        if (!IsBlankBmp(item.path.wstring())) {
            out.push_back(item.page);
        }
    }

    return out;
}

constexpr const char* kEmptyBmpName = "empty.bmp";

bool IsDuplexLikeMode(const std::string& mode) {
    return mode == "Duplex" ||
           mode == "Manual Duplex (Flip On Long Edge)" ||
           mode == "Manual Duplex (Flip On Short Edge)";
}

bool NeedPadBlankPage(const std::string& mode, int pages) {
    return pages > 1 && IsDuplexLikeMode(mode) && (pages % 2 == 1);
}

std::wstring EmptyBmpPath(const std::wstring& tempDir) {
    fs::path p = fs::path(tempDir);
    p /= kEmptyBmpName;
    return p.wstring();
}

bool CreateEmptyBmpFile(const std::wstring& tempDir, std::wstring& outPath, std::string& error) {
    const fs::path filePath = fs::path(EmptyBmpPath(tempDir));
    outPath = filePath.wstring();

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

} // namespace

void Printer::Init(
    config::ConfigData& cfg,
    const std::wstring& tempDir,
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

    try {
        printer::Init(m_state.cfg);

        const std::string mode = m_state.cfg.printMode;
        const int copies = std::max(1, m_state.cfg.copies);

        const bool skipBlankPage =
            m_state.cfg.skipBlankPage == "Yes (Actual sheets may be less than calculated)";

        const std::vector<int> printablePages =
            BuildPrintablePages(m_state.tempDir, skipBlankPage);

        if (printablePages.empty()) {
            win->SetPrintProcessColor("red");
            win->SetNotification(L"No printable pages found.");
            return;
        }

        const bool isDuplexLike =
            mode == "Duplex" ||
            mode == "Manual Duplex (Flip On Long Edge)" ||
            mode == "Manual Duplex (Flip On Short Edge)";

        std::vector<int> jobPages = printablePages;
        const bool padBlankPage =
            isDuplexLike && (jobPages.size() > 1) && ((jobPages.size() % 2) == 1);

        if (padBlankPage) {
            jobPages.push_back(0); // 0 = empty.bmp
        }

        const int pagesPerCopy  = static_cast<int>(jobPages.size());
        const int totalJobPages = pagesPerCopy * copies;

        std::wstring emptyBmpPath;
        if (padBlankPage) {
            std::string createError;
            if (!CreateEmptyBmpFile(m_state.tempDir, emptyBmpPath, createError)) {
                win->SetPrintProcessColor("red");
                win->SetNotification(Utf8ToWide(createError));
                return;
            }
        }

        auto ResolvePagePathOrEmpty = [&](int page, char preferSuffix, char fallbackSuffix) -> std::wstring {
            if (page <= 0) {
                return emptyBmpPath;
            }
            return ResolvePagePath(m_state.tempDir, page, preferSuffix, fallbackSuffix);
        };

        auto PrintSimplexOnePage = [&](int page, int totalPages, int completedPages, char preferSuffix, char fallbackSuffix) -> bool {
            if (!WaitIfPaused()) {
                return false;
            }

            const std::wstring path = ResolvePagePathOrEmpty(page, preferSuffix, fallbackSuffix);
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

            const std::wstring frontPath = ResolvePagePathOrEmpty(firstPage, 'd', 'i');
            const std::wstring backPath   = ResolvePagePathOrEmpty(secondPage, 'i', 'd');

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
        win->SetPrintProcess(totalJobPages, 0);

        if (mode == "Simplex") {
            int printedCount = 0;

            for (int copy = 0; copy < copies; ++copy) {
                for (int i = 0; i < static_cast<int>(printablePages.size()); ++i) {
                    const int page = printablePages[i];
                    if (IsCancelled()) {
                        return;
                    }

                    if (!PrintSimplexOnePage(page, totalJobPages, ++printedCount, 'd', 'i')) {
                        return;
                    }
                }
            }
        }
        else if (mode == "Duplex") {
            int printedCount = 0;

            for (int copy = 0; copy < copies; ++copy) {
                for (int i = 0; i < pagesPerCopy; i += 2) {
                    if (IsCancelled()) {
                        return;
                    }

                    if (!PrintDuplexPair(jobPages[i], jobPages[i + 1], totalJobPages, printedCount + 2)) {
                        return;
                    }

                    printedCount += 2;
                }
            }
        }
        else if (mode == "Manual Duplex (Flip On Long Edge)") {
            int printedCount = 0;

            // Phase 1: in toàn bộ trang lẻ của tất cả copies
            for (int copy = 0; copy < copies; ++copy) {
                for (int i = 0; i < pagesPerCopy; i += 2) {
                    const int page = jobPages[i];
                    if (IsCancelled()) {
                        return;
                    }

                    if (!PrintSimplexOnePage(page, totalJobPages, ++printedCount, 'd', 'i')) {
                        return;
                    }
                }
            }

            // Chỉ flip 1 lần duy nhất cho cả job
            if (pagesPerCopy > 1) {
                if (!WaitForManualFlip(L"Flip the paper on the long edge.")) {
                    return;
                }
            }

            // Phase 2: in toàn bộ trang chẵn của tất cả copies
            for (int copy = 0; copy < copies; ++copy) {
                for (int i = 1; i < pagesPerCopy; i += 2) {
                    const int page = jobPages[i];
                    if (IsCancelled()) {
                        return;
                    }

                    if (!PrintSimplexOnePage(page, totalJobPages, ++printedCount, 'i', 'd')) {
                        return;
                    }
                }
            }
        }
        else if (mode == "Manual Duplex (Flip On Short Edge)") {
            int printedCount = 0;

            // Phase 1: in toàn bộ trang lẻ của tất cả copies
            for (int copy = 0; copy < copies; ++copy) {
                for (int i = 0; i < pagesPerCopy; i += 2) {
                    const int page = jobPages[i];
                    if (IsCancelled()) {
                        return;
                    }

                    if (!PrintSimplexOnePage(page, totalJobPages, ++printedCount, 'd', 'i')) {
                        return;
                    }
                }
            }

            // Chỉ flip 1 lần duy nhất cho cả job
            if (pagesPerCopy > 1) {
                if (!WaitForManualFlip(L"Flip the paper on the short edge.")) {
                    return;
                }
            }

            // Phase 2: in toàn bộ trang chẵn của tất cả copies, nhưng đi ngược trong từng copy
            for (int copy = 0; copy < copies; ++copy) {
                for (int i = pagesPerCopy - 1; i >= 1; i -= 2) {
                    const int page = jobPages[i];
                    if (IsCancelled()) {
                        return;
                    }

                    if (!PrintSimplexOnePage(page, totalJobPages, ++printedCount, 'i', 'd')) {
                        return;
                    }
                }
            }
        }
        else {
            int printedCount = 0;

            for (int copy = 0; copy < copies; ++copy) {
                for (int i = 0; i < static_cast<int>(printablePages.size()); ++i) {
                    const int page = printablePages[i];
                    if (IsCancelled()) {
                        return;
                    }

                    if (!PrintSimplexOnePage(page, totalJobPages, ++printedCount, 'd', 'i')) {
                        return;
                    }
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