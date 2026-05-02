#include "printer/WinSpooler.h"

#include <windows.h>
#include <winspool.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace printer {
namespace {

struct PrinterHandle {
    HANDLE handle = nullptr;

    ~PrinterHandle() {
        if (handle != nullptr) {
            ClosePrinter(handle);
        }
    }
};

struct DevModeDeleter {
    void operator()(DEVMODEW* p) const noexcept {
        if (p != nullptr) {
            HeapFree(GetProcessHeap(), 0, p);
        }
    }
};

using DevModePtr = std::unique_ptr<DEVMODEW, DevModeDeleter>;

std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) {
        return {};
    }

    const int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (sizeNeeded <= 0) {
        return {};
    }

    std::wstring out(static_cast<std::size_t>(sizeNeeded - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), sizeNeeded);
    return out;
}

std::string WideToUtf8(const std::wstring& s) {
    if (s.empty()) {
        return {};
    }

    const int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (sizeNeeded <= 0) {
        return {};
    }

    std::string out(static_cast<std::size_t>(sizeNeeded - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, out.data(), sizeNeeded, nullptr, nullptr);
    return out;
}

std::string NormalizeKey(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char ch : s) {
        if (std::isalnum(ch)) {
            out.push_back(static_cast<char>(std::tolower(ch)));
        }
    }
    return out;
}

std::string AliasForPaperId(WORD paperId) {
    switch (paperId) {
    case DMPAPER_LETTER: return "letter";
    case DMPAPER_LEGAL: return "legal";
    case DMPAPER_A3: return "a3";
    case DMPAPER_A4: return "a4";
    case DMPAPER_A5: return "a5";
    case DMPAPER_B4: return "b4";
    case DMPAPER_B5: return "b5";
    case DMPAPER_EXECUTIVE: return "executive";
    case DMPAPER_TABLOID: return "tabloid";
    case DMPAPER_LEDGER: return "ledger";
    case DMPAPER_STATEMENT: return "statement";
    case DMPAPER_FOLIO: return "folio";
    case DMPAPER_QUARTO: return "quarto";
    case DMPAPER_10X14: return "10x14";
    default: return {};
    }
}

struct PaperEntry {
    WORD id = 0;
    std::wstring name;
};

std::vector<PaperEntry> QuerySupportedPapers(const std::wstring& printerName) {
    std::vector<PaperEntry> result;

    const int count = DeviceCapabilitiesW(
        printerName.c_str(),
        nullptr,
        DC_PAPERS,
        nullptr,
        nullptr);

    if (count <= 0) {
        return result;
    }

    std::vector<WORD> paperIds(static_cast<std::size_t>(count));
    std::vector<wchar_t> paperNames(static_cast<std::size_t>(count) * 64u, L'\0');

    DeviceCapabilitiesW(
        printerName.c_str(),
        nullptr,
        DC_PAPERS,
        reinterpret_cast<LPWSTR>(paperIds.data()),
        nullptr);

    DeviceCapabilitiesW(
        printerName.c_str(),
        nullptr,
        DC_PAPERNAMES,
        paperNames.data(),
        nullptr);

    result.reserve(static_cast<std::size_t>(count));

    for (int i = 0; i < count; ++i) {
        const wchar_t* namePtr = &paperNames[static_cast<std::size_t>(i) * 64u];
        result.push_back(PaperEntry{paperIds[static_cast<std::size_t>(i)], std::wstring(namePtr)});
    }

    return result;
}

bool ResolvePaperId(const std::wstring& printerName, const std::string& paper, WORD& paperId, std::string& error) {
    const std::string normalizedInput = NormalizeKey(paper);
    const auto supported = QuerySupportedPapers(printerName);

    if (supported.empty()) {
        error = "Failed to query supported papers for printer.";
        return false;
    }

    for (const auto& entry : supported) {
        const std::string exactName = NormalizeKey(WideToUtf8(entry.name));
        if (!exactName.empty() && exactName == normalizedInput) {
            paperId = entry.id;
            return true;
        }

        const std::string alias = NormalizeKey(AliasForPaperId(entry.id));
        if (!alias.empty() && alias == normalizedInput) {
            paperId = entry.id;
            return true;
        }
    }

    error = "Paper not supported by printer: " + paper;
    return false;
}

bool CreateValidatedDevMode(
    const std::wstring& printerName,
    WORD paperId,
    bool duplex,
    DevModePtr& outDm,
    std::string& error) {
    PrinterHandle printerHandle;
    if (!OpenPrinterW(const_cast<LPWSTR>(printerName.c_str()), &printerHandle.handle, nullptr)) {
        error = "OpenPrinterW failed.";
        return false;
    }

    const LONG needed = DocumentPropertiesW(nullptr, printerHandle.handle, const_cast<LPWSTR>(printerName.c_str()), nullptr, nullptr, 0);
    if (needed <= 0) {
        error = "DocumentPropertiesW size query failed.";
        return false;
    }

    DevModePtr dm(static_cast<DEVMODEW*>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, static_cast<SIZE_T>(needed))));
    if (!dm) {
        error = "HeapAlloc failed for DEVMODE.";
        return false;
    }

    const LONG getDefault = DocumentPropertiesW(nullptr, printerHandle.handle, const_cast<LPWSTR>(printerName.c_str()), dm.get(), nullptr, DM_OUT_BUFFER);
    if (getDefault != IDOK) {
        error = "DocumentPropertiesW default query failed.";
        return false;
    }

    dm->dmFields |= DM_ORIENTATION | DM_PAPERSIZE;
    dm->dmOrientation = DMORIENT_PORTRAIT;
    dm->dmPaperSize = paperId;

    if (duplex) {
        dm->dmFields |= DM_DUPLEX;
        dm->dmDuplex = DMDUP_VERTICAL;
    }

    const LONG validate = DocumentPropertiesW(
        nullptr,
        printerHandle.handle,
        const_cast<LPWSTR>(printerName.c_str()),
        dm.get(),
        dm.get(),
        DM_IN_BUFFER | DM_OUT_BUFFER);

    if (validate != IDOK) {
        error = "Printer rejected the selected paper/orientation/duplex settings.";
        return false;
    }

    outDm = std::move(dm);
    return true;
}

bool DrawPageToPrinter(HDC hdc, const BmpImage& page, std::string& error) {
    if (page.width <= 0 || page.height <= 0 || page.pixels.empty()) {
        error = "Empty rendered page.";
        return false;
    }

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = page.width;
    bmi.bmiHeader.biHeight = -page.height; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    const int drawn = StretchDIBits(
        hdc,
        0,
        0,
        page.width,
        page.height,
        0,
        0,
        page.width,
        page.height,
        page.pixels.data(),
        &bmi,
        DIB_RGB_COLORS,
        SRCCOPY);

    if (drawn == GDI_ERROR) {
        error = "StretchDIBits failed.";
        return false;
    }

    return true;
}

} // namespace

bool PrinterSupportsDuplex(const std::string& printerName, std::string& error) {
    const std::wstring wPrinter = Utf8ToWide(printerName);
    if (wPrinter.empty()) {
        error = "Failed to convert printer name.";
        return false;
    }

    const int duplexCap = DeviceCapabilitiesW(wPrinter.c_str(), nullptr, DC_DUPLEX, nullptr, nullptr);
    if (duplexCap <= 0) {
        error = "Printer does not support duplex printing.";
        return false;
    }

    return true;
}

bool OpenPrinterDc(
    const std::string& printerName,
    const std::string& paper,
    bool duplex,
    HDC& hdc,
    PrinterPageInfo& info,
    std::string& error) {
    hdc = nullptr;
    info = {};

    const std::wstring wPrinter = Utf8ToWide(printerName);
    if (wPrinter.empty()) {
        error = "Failed to convert printer name.";
        return false;
    }

    WORD paperId = 0;
    if (!ResolvePaperId(wPrinter, paper, paperId, error)) {
        return false;
    }

    if (duplex) {
        std::string duplexError;
        if (!PrinterSupportsDuplex(printerName, duplexError)) {
            error = duplexError;
            return false;
        }
    }

    DevModePtr dm;
    if (!CreateValidatedDevMode(wPrinter, paperId, duplex, dm, error)) {
        return false;
    }

    hdc = CreateDCW(L"WINSPOOL", wPrinter.c_str(), nullptr, dm.get());
    if (hdc == nullptr) {
        error = "CreateDCW failed for printer.";
        return false;
    }

    info.horzRes = GetDeviceCaps(hdc, HORZRES);
    info.vertRes = GetDeviceCaps(hdc, VERTRES);
    info.physicalOffsetX = GetDeviceCaps(hdc, PHYSICALOFFSETX);
    info.physicalOffsetY = GetDeviceCaps(hdc, PHYSICALOFFSETY);
    info.dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
    info.dpiY = GetDeviceCaps(hdc, LOGPIXELSY);

    if (info.horzRes <= 0 || info.vertRes <= 0) {
        DeleteDC(hdc);
        hdc = nullptr;
        error = "Failed to read printer page size.";
        return false;
    }

    return true;
}

bool PrintBitmapPages(
    HDC hdc,
    const std::vector<BmpImage>& pages,
    std::string& error) {
    if (hdc == nullptr) {
        error = "Invalid printer DC.";
        return false;
    }

    if (pages.empty()) {
        error = "No pages to print.";
        return false;
    }

    DOCINFOW di{};
    di.cbSize = sizeof(di);
    di.lpszDocName = L"Bmp Print Job";

    const int started = StartDocW(hdc, &di);
    if (started <= 0) {
        error = "StartDocW failed.";
        return false;
    }

    bool jobOk = true;

    for (const auto& page : pages) {
        if (StartPage(hdc) <= 0) {
            error = "StartPage failed.";
            jobOk = false;
            break;
        }

        if (!DrawPageToPrinter(hdc, page, error)) {
            jobOk = false;
            EndPage(hdc);
            break;
        }

        if (EndPage(hdc) <= 0) {
            error = "EndPage failed.";
            jobOk = false;
            break;
        }
    }

    if (!jobOk) {
        AbortDoc(hdc);
        return false;
    }

    if (EndDoc(hdc) <= 0) {
        error = "EndDoc failed.";
        return false;
    }

    return true;
}

} // namespace printer
