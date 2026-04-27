#pragma once

#include <windows.h>
#include <string>

namespace ui {
namespace home {

class InfoConfigSection {
public:
    InfoConfigSection();

    void Create(HWND parent, HFONT font);
    void Resize(int parentWidth);

    // ===== EXPORT =====
    void SetTotalFilesValue(const std::string& value);
    void SetPagesToPrintValue(const std::string& value);
    void SetSheetsRequiredValue(const std::string& value);

    void OnPaint(HDC hdc);

private:
    HWND m_parent = nullptr;
    HFONT m_font = nullptr;

    std::wstring m_totalFiles = L"1";
    std::wstring m_pagesToPrint = L"1";
    std::wstring m_sheetsRequired = L"1";

private:
    std::wstring ToWide(const std::string& s) const;
};

}
}