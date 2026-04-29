#include "ui/home/BasicConfigSection.h"
#include "ui/home/HomeComponentMap.h"

#include <commctrl.h>

#define ID_BTN_PRINTER  2001
#define ID_EDIT_COPIES   2002
#define ID_PRINTER_MENU_BASE 3000

using namespace ui::home;

namespace {

    COLORREF InputTextColor(BOOL disabled) {
        return disabled ? Style::INPUT_TEXT_DISABLED : Style::INPUT_TEXT;
    }

    COLORREF InputBgColor(BOOL disabled, BOOL hot, BOOL focused) {
        if (disabled) return Style::INPUT_BG_DISABLED;
        if (focused)  return Style::INPUT_BG_FOCUS;
        if (hot)      return Style::INPUT_BG_HOVER;
        return Style::INPUT_BG;
    }

    COLORREF InputBorderColor(BOOL disabled, BOOL hot, BOOL focused) {
        if (disabled) return Style::INPUT_BORDER_DISABLED;
        if (focused)  return Style::INPUT_BORDER_FOCUS;
        if (hot)      return Style::INPUT_BORDER_HOVER;
        return Style::INPUT_BORDER;
    }

    void DrawRoundedFrame(
        HDC hdc,
        const RECT& rc,
        COLORREF borderColor,
        int radius
    ) {
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        HPEN pen = CreatePen(PS_SOLID, 1, borderColor);
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);

        RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, radius, radius);

        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(pen);
    }

    RECT GetControlRectInParent(HWND parent, HWND child) {
        RECT rc{};
        GetWindowRect(child, &rc);
        MapWindowPoints(nullptr, parent, reinterpret_cast<POINT*>(&rc), 2);
        return rc;
    }

    void DrawInputChrome(
        HDC hdc,
        HWND parent,
        HWND hwnd,
        bool focused
    ) {
        RECT rc = GetControlRectInParent(parent, hwnd);

        rc.left   -= ui::home::Layout::INPUT_BORDER_OFFSET;
        rc.top    -= ui::home::Layout::INPUT_BORDER_OFFSET;
        rc.right  += ui::home::Layout::INPUT_BORDER_OFFSET;
        rc.bottom += ui::home::Layout::INPUT_BORDER_OFFSET;

        const BOOL disabled = !IsWindowEnabled(hwnd);
        const BOOL hot = FALSE;
        const BOOL isFocused = focused ? TRUE : FALSE;

        DrawRoundedFrame(
            hdc,
            rc,
            InputBorderColor(disabled, hot, isFocused),
            Style::INPUT_RADIUS
        );
    }
}

BasicConfigSection::BasicConfigSection() {}

std::wstring BasicConfigSection::ToWide(const std::string& s) const {
    if (s.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (len <= 0) {
        return std::wstring(s.begin(), s.end());
    }

    std::wstring ws(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, ws.data(), len);
    return ws;
}

std::string BasicConfigSection::ToNarrow(const std::wstring& ws) const {
    if (ws.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) {
        return std::string(ws.begin(), ws.end());
    }

    std::string s(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, s.data(), len, nullptr, nullptr);
    return s;
}

void BasicConfigSection::Create(HWND parent, HFONT font) {
    m_parent = parent;
    m_font = font;

    using namespace Layout::BasicSection;

    m_btnPrinter = CreateWindowEx(
        0,
        L"BUTTON",
        L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        X,
        Y,
        INPUT_W - 2 * ui::home::Layout::INPUT_BORDER_OFFSET,
        ui::home::Layout::INPUT_H - 2 * ui::home::Layout::INPUT_BORDER_OFFSET,
        parent,
        (HMENU)ID_BTN_PRINTER,
        GetModuleHandle(nullptr),
        nullptr
    );

    m_editCopies = CreateWindowEx(
        0,
        L"EDIT",
        L"1",
        WS_CHILD | WS_VISIBLE | ES_NUMBER | WS_TABSTOP,
        X,
        Y,
        INPUT_W - 2 * ui::home::Layout::INPUT_BORDER_OFFSET,
        ui::home::Layout::INPUT_H - 2 * ui::home::Layout::INPUT_BORDER_OFFSET,
        parent,
        (HMENU)ID_EDIT_COPIES,
        GetModuleHandle(nullptr),
        nullptr
    );

    SendMessage(m_btnPrinter, WM_SETFONT, (WPARAM)m_font, TRUE);
    SendMessage(m_editCopies, WM_SETFONT, (WPARAM)m_font, TRUE);

    LONG_PTR exStyle = GetWindowLongPtr(m_editCopies, GWL_EXSTYLE);
    exStyle &= ~WS_EX_CLIENTEDGE;
    SetWindowLongPtr(m_editCopies, GWL_EXSTYLE, exStyle);
    SetWindowPos(
        m_editCopies,
        nullptr,
        0, 0, 0, 0,
        SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE
    );
}

void BasicConfigSection::Resize(int parentWidth) {
    (void)parentWidth;

    using namespace Layout::BasicSection;

    MoveWindow(
        m_btnPrinter,
        X + PRINTER_INPUT_X + ui::home::Layout::INPUT_BORDER_OFFSET,
        Y + PRINTER_INPUT_Y + ui::home::Layout::INPUT_BORDER_OFFSET,
        INPUT_W - 2 * ui::home::Layout::INPUT_BORDER_OFFSET,
        ui::home::Layout::INPUT_H - 2 * ui::home::Layout::INPUT_BORDER_OFFSET,
        TRUE
    );
    MoveWindow(
        m_editCopies,
        X + COPIES_INPUT_X + ui::home::Layout::INPUT_BORDER_OFFSET,
        Y + COPIES_INPUT_Y + ui::home::Layout::INPUT_BORDER_OFFSET,
        INPUT_W - 2 * ui::home::Layout::INPUT_BORDER_OFFSET,
        ui::home::Layout::INPUT_H - 2 * ui::home::Layout::INPUT_BORDER_OFFSET,
        TRUE
    );
}

void BasicConfigSection::SetPrinterOptions(const std::vector<std::string>& options) {
    m_printerOptions.clear();
    m_printerOptions.reserve(options.size());

    for (const auto& s : options) {
        m_printerOptions.push_back(ToWide(s));
    }

    if (!m_printerOptions.empty()) {
        m_printerSelection = 0;
    } else {
        m_printerSelection = -1;
    }

    InvalidateRect(m_btnPrinter, nullptr, TRUE);
}

void BasicConfigSection::SetPrinterValue(const std::string& value) {
    std::wstring wv = ToWide(value);

    for (size_t i = 0; i < m_printerOptions.size(); ++i) {
        if (m_printerOptions[i] == wv) {
            m_printerSelection = (int)i;
            InvalidateRect(m_btnPrinter, nullptr, TRUE);
            return;
        }
    }
}

void BasicConfigSection::OnPrinterChange(std::function<void(const std::string&)> cb) {
    m_cbPrinterChange = cb;
}

void BasicConfigSection::SetCopiesValue(const std::string& value) {
    std::wstring ws = ToWide(value);
    SetWindowText(m_editCopies, ws.c_str());
}

void BasicConfigSection::OnCopiesChange(std::function<void(const std::string&)> cb) {
    m_cbCopiesChange = cb;
}

void BasicConfigSection::HandleCommand(WPARAM wParam) {
    int id = LOWORD(wParam);
    int code = HIWORD(wParam);

    if (id == ID_BTN_PRINTER && code == BN_CLICKED) {
        ShowPrinterPopup();
        return;
    }

    if (id == ID_EDIT_COPIES && code == EN_CHANGE) {
        if (m_cbCopiesChange) {
            m_cbCopiesChange(GetEditText(m_editCopies));
        }
        return;
    }
}

void BasicConfigSection::ShowPrinterPopup() {
    if (m_printerOptions.empty()) {
        return;
    }

    HMENU menu = CreatePopupMenu();
    if (!menu) {
        return;
    }

    for (size_t i = 0; i < m_printerOptions.size(); ++i) {
        UINT id = ID_PRINTER_MENU_BASE + (UINT)i;
        AppendMenuW(menu, MF_STRING, id, m_printerOptions[i].c_str());
    }

    RECT rc{};
    GetWindowRect(m_btnPrinter, &rc);

    UINT flags = TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY;
    int cmd = TrackPopupMenu(menu, flags, rc.left, rc.bottom, 0, m_parent, nullptr);

    DestroyMenu(menu);

    if (cmd >= ID_PRINTER_MENU_BASE &&
        cmd < ID_PRINTER_MENU_BASE + (int)m_printerOptions.size()) {
        m_printerSelection = cmd - ID_PRINTER_MENU_BASE;
        InvalidateRect(m_btnPrinter, nullptr, TRUE);

        if (m_cbPrinterChange) {
            m_cbPrinterChange(ToNarrow(m_printerOptions[m_printerSelection]));
        }
    }
}

void BasicConfigSection::HandleDrawItem(const DRAWITEMSTRUCT* dis) {
    if (!dis) return;

    if (dis->CtlType == ODT_BUTTON && dis->CtlID == ID_BTN_PRINTER) {
        HDC hdc = dis->hDC;
        RECT rc = dis->rcItem;

        const bool pressed = (dis->itemState & ODS_SELECTED) != 0;
        const bool disabled = (dis->itemState & ODS_DISABLED) != 0;
        const bool focused = (dis->itemState & ODS_FOCUS) != 0;
        const bool hot = (dis->itemState & ODS_HOTLIGHT) != 0;

        COLORREF bg = InputBgColor(disabled, hot, pressed);
        COLORREF textColor = InputTextColor(disabled);
        COLORREF borderColor = InputBorderColor(disabled, hot, focused);

        HBRUSH brush = CreateSolidBrush(bg);
        FillRect(hdc, &rc, brush);
        DeleteObject(brush);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, textColor);

        HFONT oldFont = (HFONT)SelectObject(hdc, m_font);

        std::wstring text;
        if (m_printerSelection >= 0 && m_printerSelection < (int)m_printerOptions.size()) {
            text = m_printerOptions[m_printerSelection];
        }

        RECT textRc = rc;
        textRc.left += 8;
        textRc.right -= 24;

        DrawTextW(
            hdc,
            text.c_str(),
            -1,
            &textRc,
            DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS
        );

        int midX = rc.right - 12;
        int midY = (rc.top + rc.bottom) / 2 - 1;

        POINT tri[3] = {
            { midX - 4, midY - 1 },
            { midX + 4, midY - 1 },
            { midX,     midY + 3 }
        };

        HBRUSH arrowBrush = CreateSolidBrush(textColor);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, arrowBrush);
        HPEN oldPen = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
        Polygon(hdc, tri, 3);
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(arrowBrush);

        SelectObject(hdc, oldFont);
        return;
    }
}

std::string BasicConfigSection::GetSelectedPrinterText() const {
    if (m_printerSelection < 0 || m_printerSelection >= (int)m_printerOptions.size()) {
        return "";
    }
    return ToNarrow(m_printerOptions[m_printerSelection]);
}

std::string BasicConfigSection::GetEditText(HWND hwnd) {
    wchar_t buf[64]{};
    GetWindowText(hwnd, buf, 64);

    std::wstring ws(buf);
    return ToNarrow(ws);
}

void BasicConfigSection::OnPaint(HDC hdc) {
    using namespace Layout::BasicSection;

    RECT sec = { X, Y, X + W, Y + H };

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

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, Style::LABEL);

    HFONT oldFont = (HFONT)SelectObject(hdc, m_font);

    RECT rcPrinter = {
        X + PRINTER_LABEL_X,
        Y + PRINTER_LABEL_Y,
        X + W,
        Y + PRINTER_LABEL_Y + ui::home::Layout::LABEL_H
    };
    DrawText(hdc, L"Printer", -1, &rcPrinter, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT rcCopies = {
        X + COPIES_LABEL_X,
        Y + COPIES_LABEL_Y,
        X + W,
        Y + COPIES_LABEL_Y + ui::home::Layout::LABEL_H
    };
    DrawText(hdc, L"Copies", -1, &rcCopies, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    bool editFocused = (GetFocus() == m_editCopies);

    DrawInputChrome(hdc, m_parent, m_btnPrinter, (GetFocus() == m_btnPrinter));
    DrawInputChrome(hdc, m_parent, m_editCopies, editFocused);

    SelectObject(hdc, oldFont);
}