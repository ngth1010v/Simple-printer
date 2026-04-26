#include "BasicConfigSection.h"
#include "HomeComponentMap.h"

#define ID_CB_PRINTER  2001
#define ID_EDIT_COPIES 2002

using namespace ui::home;

BasicConfigSection::BasicConfigSection() {}


void BasicConfigSection::Create(HWND parent, HFONT font) {
    m_parent = parent;
    m_font = font;

    using namespace Layout::BasicSection;

    // ===== LABEL PRINTER =====
    m_lblPrinter = CreateWindowEx(0, L"STATIC", L"Printer",
        WS_CHILD | WS_VISIBLE,
        X + PRINTER_LABEL_X,
        Y + PRINTER_LABEL_Y,
        W - 20,
        LABEL_H,
        parent, nullptr, GetModuleHandle(nullptr), nullptr);

    // ===== COMBO PRINTER =====
    m_cbPrinter = CreateWindowEx(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        X + PRINTER_INPUT_X,
        Y + PRINTER_INPUT_Y,
        W - 20,
        200,
        parent, (HMENU)ID_CB_PRINTER, GetModuleHandle(nullptr), nullptr);

    // ===== LABEL COPIES =====
    m_lblCopies = CreateWindowEx(0, L"STATIC", L"Copies",
        WS_CHILD | WS_VISIBLE,
        X + COPIES_LABEL_X,
        Y + COPIES_LABEL_Y,
        W - 20,
        LABEL_H,
        parent, nullptr, GetModuleHandle(nullptr), nullptr);

    // ===== EDIT COPIES =====
    m_editCopies = CreateWindowEx(0, L"EDIT", L"1",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
        X + COPIES_INPUT_X,
        Y + COPIES_INPUT_Y,
        W - 20,
        INPUT_H,
        parent, (HMENU)ID_EDIT_COPIES, GetModuleHandle(nullptr), nullptr);

    // ===== FONT =====
    SendMessage(m_lblPrinter, WM_SETFONT, (WPARAM)m_font, TRUE);
    SendMessage(m_cbPrinter,  WM_SETFONT, (WPARAM)m_font, TRUE);
    SendMessage(m_lblCopies,  WM_SETFONT, (WPARAM)m_font, TRUE);
    SendMessage(m_editCopies, WM_SETFONT, (WPARAM)m_font, TRUE);

    // ===== CREATE xong =====
    if (m_applyStyle) {
        m_applyStyle(m_cbPrinter);
        m_applyStyle(m_editCopies);
    }
}

void BasicConfigSection::Resize(int parentWidth) {
    using namespace Layout::BasicSection;

    int w = W;

    MoveWindow(m_lblPrinter, X + PRINTER_LABEL_X, Y + PRINTER_LABEL_Y, w - 20, LABEL_H, TRUE);
    MoveWindow(m_cbPrinter,  X + PRINTER_INPUT_X, Y + PRINTER_INPUT_Y, w - 20, 200, TRUE);

    MoveWindow(m_lblCopies,  X + COPIES_LABEL_X, Y + COPIES_LABEL_Y, w - 20, LABEL_H, TRUE);
    MoveWindow(m_editCopies, X + COPIES_INPUT_X, Y + COPIES_INPUT_Y, w - 20, INPUT_H, TRUE);
}

// ================= API =================

void BasicConfigSection::SetPrinterOptions(const std::vector<std::string>& options) {
    SendMessage(m_cbPrinter, CB_RESETCONTENT, 0, 0);

    for (const auto& s : options) {
        std::wstring ws(s.begin(), s.end());
        SendMessage(m_cbPrinter, CB_ADDSTRING, 0, (LPARAM)ws.c_str());
    }

    if (!options.empty()) {
        SendMessage(m_cbPrinter, CB_SETCURSEL, 0, 0);
    }
}

void BasicConfigSection::SetPrinterValue(const std::string& value) {
    std::wstring ws(value.begin(), value.end());
    int idx = SendMessage(m_cbPrinter, CB_FINDSTRINGEXACT, -1, (LPARAM)ws.c_str());
    if (idx != CB_ERR) {
        SendMessage(m_cbPrinter, CB_SETCURSEL, idx, 0);
    }
}

void BasicConfigSection::OnPrinterChange(std::function<void(const std::string&)> cb) {
    m_cbPrinterChange = cb;
}

void BasicConfigSection::SetCopiesValue(const std::string& value) {
    std::wstring ws(value.begin(), value.end());
    SetWindowText(m_editCopies, ws.c_str());
}

void BasicConfigSection::OnCopiesChange(std::function<void(const std::string&)> cb) {
    m_cbCopiesChange = cb;
}

// ================= INTERNAL =================

void BasicConfigSection::HandleCommand(WPARAM wParam) {
    int id = LOWORD(wParam);
    int code = HIWORD(wParam);

    if (id == ID_CB_PRINTER && code == CBN_SELCHANGE) {
        if (m_cbPrinterChange) {
            m_cbPrinterChange(GetComboText(m_cbPrinter));
        }
    }

    if (id == ID_EDIT_COPIES && code == EN_CHANGE) {
        if (m_cbCopiesChange) {
            m_cbCopiesChange(GetEditText(m_editCopies));
        }
    }
}

std::string BasicConfigSection::GetComboText(HWND hwnd) {
    int idx = SendMessage(hwnd, CB_GETCURSEL, 0, 0);
    if (idx == CB_ERR) return "";

    wchar_t buf[256];
    SendMessage(hwnd, CB_GETLBTEXT, idx, (LPARAM)buf);

    std::wstring ws(buf);
    return std::string(ws.begin(), ws.end());
}

std::string BasicConfigSection::GetEditText(HWND hwnd) {
    wchar_t buf[64];
    GetWindowText(hwnd, buf, 64);

    std::wstring ws(buf);
    return std::string(ws.begin(), ws.end());
}

void BasicConfigSection::OnPaint(HDC hdc) {
    using namespace Layout::BasicSection;

    RECT sec = {
        X,
        Y,
        X + W,
        Y + H
    };

    // ===== BG =====
    HBRUSH bg = CreateSolidBrush(RGB(255,255,255));
    FillRect(hdc, &sec, bg);
    DeleteObject(bg);

    // ===== BORDER =====
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(150,150,150));
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

    Rectangle(hdc, sec.left, sec.top, sec.right, sec.bottom);

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

void BasicConfigSection::SetStyleApplier(std::function<void(HWND)> fn) {
    m_applyStyle = fn;
}