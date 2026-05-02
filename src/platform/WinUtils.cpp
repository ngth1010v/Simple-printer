#include "platform/WinUtils.h"
#include <windows.h>
#include <winspool.h>
#include <vector>
#include <string>
#include <unordered_map>

namespace platform {

// ===== UTF16 -> UTF8 =====
static std::string WStringToString(const std::wstring& w)
{
    if (w.empty()) return {};

    int size = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string result(size, 0);

    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), result.data(), size, nullptr, nullptr);
    return result;
}

// ===== UTF8 -> UTF16 =====
static std::wstring StringToWString(const std::string& s)
{
    if (s.empty()) return {};

    int size = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring result(size, 0);

    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), result.data(), size);
    return result;
}

// ===== Paper map (giống Word ~90%) =====
static std::string MapPaperName(WORD paperId, const std::wstring& driverName)
{
    static const std::unordered_map<WORD, std::string> map = {
        {DMPAPER_A3, "A3"},
        {DMPAPER_A4, "A4"},
        {DMPAPER_A5, "A5"},
        {DMPAPER_A6, "A6"},
        {DMPAPER_LETTER, "Letter"},
        {DMPAPER_LEGAL, "Legal"},
        {DMPAPER_EXECUTIVE, "Executive"},
        {DMPAPER_B4, "B4"},
        {DMPAPER_B5, "B5"},
    };

    auto it = map.find(paperId);
    if (it != map.end())
        return it->second;

    // fallback driver name
    return WStringToString(driverName);
}

// ============================================================
// GetPrinters
// ============================================================
std::vector<std::string> GetPrinters()
{
    std::vector<std::string> result;

    DWORD needed = 0;
    DWORD returned = 0;

    EnumPrintersW(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS,
                  nullptr, 2,
                  nullptr, 0,
                  &needed, &returned);

    if (needed == 0)
        return result;

    std::vector<BYTE> buffer(needed);

    if (!EnumPrintersW(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS,
                       nullptr, 2,
                       buffer.data(), needed,
                       &needed, &returned))
        return result;

    PRINTER_INFO_2W* printers = reinterpret_cast<PRINTER_INFO_2W*>(buffer.data());

    for (DWORD i = 0; i < returned; ++i) {
        if (printers[i].pPrinterName) {
            result.push_back(WStringToString(printers[i].pPrinterName));
        }
    }

    return result;
}

// ============================================================
// GetSupportedPapers
// ============================================================
std::vector<std::string> GetSupportedPapers(const std::string& printerName)
{
    std::vector<std::string> result;

    std::wstring wPrinter = StringToWString(printerName);

    int count = DeviceCapabilitiesW(
        wPrinter.c_str(),
        nullptr,
        DC_PAPERS,
        nullptr,
        nullptr
    );

    if (count <= 0)
        return result;

    std::vector<WORD> paperIds(count);
    std::vector<wchar_t> paperNames(count * 64); // mỗi name tối đa 64 wchar

    DeviceCapabilitiesW(
        wPrinter.c_str(),
        nullptr,
        DC_PAPERS,
        reinterpret_cast<LPWSTR>(paperIds.data()),
        nullptr
    );

    DeviceCapabilitiesW(
        wPrinter.c_str(),
        nullptr,
        DC_PAPERNAMES,
        paperNames.data(),
        nullptr
    );

    for (int i = 0; i < count; ++i) {
        std::wstring name(&paperNames[i * 64]);
        result.push_back(MapPaperName(paperIds[i], name));
    }

    return result;
}

bool GetPrinterDPI(const std::string& printerName, int& dpiX, int& dpiY)
{
    dpiX = 0;
    dpiY = 0;

    std::wstring wPrinter = StringToWString(printerName);

    // Tạo DC của printer
    HDC hdc = CreateDCW(
        L"WINSPOOL",           // driver chuẩn cho printer
        wPrinter.c_str(),      // tên máy in
        nullptr,
        nullptr
    );

    if (!hdc)
        return false;

    // DPI thật
    dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
    dpiY = GetDeviceCaps(hdc, LOGPIXELSY);

    DeleteDC(hdc);

    return (dpiX > 0 && dpiY > 0);
}

}