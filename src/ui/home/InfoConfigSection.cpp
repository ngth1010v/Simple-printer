#include "ui/home/InfoConfigSection.h"
#include "ui/home/HomeComponentMap.h"

using namespace ui::home;

namespace {

    void DrawRoundedFrame(HDC hdc, const RECT& rc, COLORREF borderColor, int radius) {
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        HPEN pen = CreatePen(PS_SOLID, 1, borderColor);
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);

        RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, radius, radius);

        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(pen);
    }

    void DrawFakeInput(
        HDC hdc,
        const RECT& rc,
        const std::wstring& text,
        HFONT font
    ) {
        // background
        HBRUSH bg = CreateSolidBrush(Style::INPUT_BG);
        FillRect(hdc, &rc, bg);
        DeleteObject(bg);

        // text
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, Style::INPUT_TEXT);

        HFONT oldFont = (HFONT)SelectObject(hdc, font);

        RECT textRc = rc;
        textRc.left += 6;
        textRc.right -= 6;

        DrawTextW(
            hdc,
            text.c_str(),
            -1,
            &textRc,
            DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS
        );

        SelectObject(hdc, oldFont);

        // border
        DrawRoundedFrame(hdc, rc, Style::INPUT_BORDER, Style::INPUT_RADIUS);
    }
}

InfoConfigSection::InfoConfigSection() {}

std::wstring InfoConfigSection::ToWide(const std::string& s) const {
    if (s.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);

    std::wstring ws(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, ws.data(), len);
    return ws;
}

void InfoConfigSection::Create(HWND parent, HFONT font) {
    m_parent = parent;
    m_font = font;
}

void InfoConfigSection::Resize(int parentWidth) {
    (void)parentWidth;
    // không cần làm gì vì không có HWND con
}

// ===== SET VALUE =====

void InfoConfigSection::SetTotalFilesValue(const std::string& value) {
    m_totalFiles = ToWide(value);
    InvalidateRect(m_parent, nullptr, TRUE);
}

void InfoConfigSection::SetPagesToPrintValue(const std::string& value) {
    m_pagesToPrint = ToWide(value);
    InvalidateRect(m_parent, nullptr, TRUE);
}

void InfoConfigSection::SetSheetsRequiredValue(const std::string& value) {
    m_sheetsRequired = ToWide(value);
    InvalidateRect(m_parent, nullptr, TRUE);
}

void InfoConfigSection::OnPaint(HDC hdc) {
    using namespace Layout::InfoSection;

    RECT rcParent{};
    GetClientRect(m_parent, &rcParent);
    int parentW = rcParent.right;

    int sectionW = CALC_W(parentW);
    int inputW   = CALC_INPUT_W(parentW);

    // ===== section background =====
    RECT sec = { X, Y, X + sectionW, Y + H };

    HBRUSH bg = CreateSolidBrush(Style::SECTION_BG);
    FillRect(hdc, &sec, bg);
    DeleteObject(bg);

    HPEN pen = CreatePen(PS_SOLID, 1, Style::SECTION_BORDER);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

    Rectangle(hdc, sec.left, sec.top, sec.right, sec.bottom);

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);

    // ===== label =====
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, Style::LABEL);

    HFONT oldFont = (HFONT)SelectObject(hdc, m_font);

    RECT l1 = { X + TOTAL_FILES_LABEL_X, Y + TOTAL_FILES_LABEL_Y, X + sectionW, Y + TOTAL_FILES_LABEL_Y + Layout::LABEL_H };
    DrawText(hdc, L"Total files", -1, &l1, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT l2 = { X + PAGES_TO_PRINT_LABEL_X, Y + PAGES_TO_PRINT_LABEL_Y, X + sectionW, Y + PAGES_TO_PRINT_LABEL_Y + Layout::LABEL_H };
    DrawText(hdc, L"Pages to print", -1, &l2, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT l3 = { X + SHEETS_REQUIRED_LABEL_X, Y + SHEETS_REQUIRED_LABEL_Y, X + sectionW, Y + SHEETS_REQUIRED_LABEL_Y + Layout::LABEL_H };
    DrawText(hdc, L"Sheets required", -1, &l3, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // ===== fake inputs =====

    RECT i1 = {
        X + TOTAL_FILES_INPUT_X,
        Y + TOTAL_FILES_INPUT_Y,
        X + TOTAL_FILES_INPUT_X + inputW,
        Y + TOTAL_FILES_INPUT_Y + Layout::INPUT_H
    };

    RECT i2 = {
        X + PAGES_TO_PRINT_INPUT_X,
        Y + PAGES_TO_PRINT_INPUT_Y,
        X + PAGES_TO_PRINT_INPUT_X + inputW,
        Y + PAGES_TO_PRINT_INPUT_Y + Layout::INPUT_H
    };

    RECT i3 = {
        X + SHEETS_REQUIRED_INPUT_X,
        Y + SHEETS_REQUIRED_INPUT_Y,
        X + SHEETS_REQUIRED_INPUT_X + inputW,
        Y + SHEETS_REQUIRED_INPUT_Y + Layout::INPUT_H
    };

    DrawFakeInput(hdc, i1, m_totalFiles, m_font);
    DrawFakeInput(hdc, i2, m_pagesToPrint, m_font);
    DrawFakeInput(hdc, i3, m_sheetsRequired, m_font);

    SelectObject(hdc, oldFont);
}