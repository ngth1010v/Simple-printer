#include "MarginConfigSection.h"
#include "HomeComponentMap.h"

#define ID_EDIT_TOP     5001
#define ID_EDIT_BOTTOM  5002
#define ID_EDIT_LEFT    5003
#define ID_EDIT_RIGHT   5004

using namespace ui::home;

namespace {

    RECT GetControlRectInParent(HWND parent, HWND child) {
        RECT rc{};
        GetWindowRect(child, &rc);
        MapWindowPoints(nullptr, parent, reinterpret_cast<POINT*>(&rc), 2);
        return rc;
    }

    void DrawRoundedFrame(HDC hdc, const RECT& rc, COLORREF color, int radius) {
        HPEN pen = CreatePen(PS_SOLID, 1, color);
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

        RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, radius, radius);

        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(pen);
    }

    void DrawInputChrome(HDC hdc, HWND parent, HWND hwnd, bool focused) {
        RECT rc = GetControlRectInParent(parent, hwnd);

        rc.left   -= Layout::INPUT_BORDER_OFFSET;
        rc.top    -= Layout::INPUT_BORDER_OFFSET;
        rc.right  += Layout::INPUT_BORDER_OFFSET;
        rc.bottom += Layout::INPUT_BORDER_OFFSET;

        DrawRoundedFrame(
            hdc,
            rc,
            focused ? Style::INPUT_BORDER_FOCUS : Style::INPUT_BORDER,
            Style::INPUT_RADIUS
        );
    }

    void DrawSectionRect(HDC hdc, const RECT& rc) {
        HBRUSH bg = CreateSolidBrush(Style::SECTION_BG);
        FillRect(hdc, &rc, bg);
        DeleteObject(bg);

        HPEN pen = CreatePen(PS_SOLID, 1, Style::SECTION_BORDER);
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

        Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(pen);
    }
}

MarginConfigSection::MarginConfigSection() {}

std::wstring MarginConfigSection::ToWide(const std::string& s) const {
    if (s.empty()) return L"";

    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (len <= 0) {
        return std::wstring(s.begin(), s.end());
    }

    std::wstring ws(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, ws.data(), len);
    return ws;
}

std::string MarginConfigSection::ToNarrow(const std::wstring& ws) const {
    if (ws.empty()) return "";

    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) {
        return std::string(ws.begin(), ws.end());
    }

    std::string s(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, s.data(), len, nullptr, nullptr);
    return s;
}

void MarginConfigSection::RemoveEditBorder(HWND hwnd) {
    LONG_PTR ex = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    ex &= ~WS_EX_CLIENTEDGE;
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, ex);

    SetWindowPos(
        hwnd,
        nullptr,
        0, 0, 0, 0,
        SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE
    );
}

void MarginConfigSection::Create(HWND parent, HFONT font) {
    m_parent = parent;
    m_font = font;

    auto createEdit = [&](int id) {
        return CreateWindowEx(
            0,
            L"EDIT",
            L"1\"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT,
            0, 0, 0, 0,
            parent,
            (HMENU)id,
            GetModuleHandle(nullptr),
            nullptr
        );
    };

    m_editTop = createEdit(ID_EDIT_TOP);
    m_editBottom = createEdit(ID_EDIT_BOTTOM);
    m_editLeft = createEdit(ID_EDIT_LEFT);
    m_editRight = createEdit(ID_EDIT_RIGHT);

    SendMessage(m_editTop, WM_SETFONT, (WPARAM)m_font, TRUE);
    SendMessage(m_editBottom, WM_SETFONT, (WPARAM)m_font, TRUE);
    SendMessage(m_editLeft, WM_SETFONT, (WPARAM)m_font, TRUE);
    SendMessage(m_editRight, WM_SETFONT, (WPARAM)m_font, TRUE);

    RemoveEditBorder(m_editTop);
    RemoveEditBorder(m_editBottom);
    RemoveEditBorder(m_editLeft);
    RemoveEditBorder(m_editRight);
}

void MarginConfigSection::Resize(int) {
    using namespace Layout::MarginSection;

    int w = MARGIN_INPUT_W;
    int h = Layout::INPUT_H - 2 * Layout::INPUT_BORDER_OFFSET;
    int editW = w - 2 * Layout::INPUT_BORDER_OFFSET;
    int editH = h;

    MoveWindow(
        m_editTop,
        X + MARGIN_TOP_INPUT_X + Layout::INPUT_BORDER_OFFSET,
        Y + MARGIN_TOP_INPUT_Y + Layout::INPUT_BORDER_OFFSET,
        editW,
        editH,
        TRUE
    );

    MoveWindow(
        m_editBottom,
        X + MARGIN_BOTTOM_INPUT_X + Layout::INPUT_BORDER_OFFSET,
        Y + MARGIN_BOTTOM_INPUT_Y + Layout::INPUT_BORDER_OFFSET,
        editW,
        editH,
        TRUE
    );

    MoveWindow(
        m_editLeft,
        X + MARGIN_LEFT_INPUT_X + Layout::INPUT_BORDER_OFFSET,
        Y + MARGIN_LEFT_INPUT_Y + Layout::INPUT_BORDER_OFFSET,
        editW,
        editH,
        TRUE
    );

    MoveWindow(
        m_editRight,
        X + MARGIN_RIGHT_INPUT_X + Layout::INPUT_BORDER_OFFSET,
        Y + MARGIN_RIGHT_INPUT_Y + Layout::INPUT_BORDER_OFFSET,
        editW,
        editH,
        TRUE
    );
}

void MarginConfigSection::SetMarginTopValue(const std::string& v) {
    if (m_editTop) SetWindowTextW(m_editTop, ToWide(v).c_str());
}

void MarginConfigSection::SetMarginBottomValue(const std::string& v) {
    if (m_editBottom) SetWindowTextW(m_editBottom, ToWide(v).c_str());
}

void MarginConfigSection::SetMarginLeftValue(const std::string& v) {
    if (m_editLeft) SetWindowTextW(m_editLeft, ToWide(v).c_str());
}

void MarginConfigSection::SetMarginRightValue(const std::string& v) {
    if (m_editRight) SetWindowTextW(m_editRight, ToWide(v).c_str());
}

void MarginConfigSection::OnMarginTopChange(std::function<void(const std::string&)> cb) {
    m_cbTop = std::move(cb);
}

void MarginConfigSection::OnMarginBottomChange(std::function<void(const std::string&)> cb) {
    m_cbBottom = std::move(cb);
}

void MarginConfigSection::OnMarginLeftChange(std::function<void(const std::string&)> cb) {
    m_cbLeft = std::move(cb);
}

void MarginConfigSection::OnMarginRightChange(std::function<void(const std::string&)> cb) {
    m_cbRight = std::move(cb);
}

std::string MarginConfigSection::GetEditText(HWND hwnd) {
    wchar_t buf[64]{};
    GetWindowTextW(hwnd, buf, 64);
    return ToNarrow(buf);
}

void MarginConfigSection::HandleCommand(WPARAM wParam) {
    int id = LOWORD(wParam);
    int code = HIWORD(wParam);

    if (code == EN_CHANGE) {
        if (id == ID_EDIT_TOP && m_cbTop) m_cbTop(GetEditText(m_editTop));
        if (id == ID_EDIT_BOTTOM && m_cbBottom) m_cbBottom(GetEditText(m_editBottom));
        if (id == ID_EDIT_LEFT && m_cbLeft) m_cbLeft(GetEditText(m_editLeft));
        if (id == ID_EDIT_RIGHT && m_cbRight) m_cbRight(GetEditText(m_editRight));

        if (m_parent) InvalidateRect(m_parent, nullptr, FALSE);
        return;
    }

    if (code == EN_SETFOCUS || code == EN_KILLFOCUS) {
        if (m_parent) InvalidateRect(m_parent, nullptr, FALSE);
        return;
    }
}

void MarginConfigSection::OnPaint(HDC hdc) {
    using namespace Layout::MarginSection;

    RECT sec = { X, Y, X + W, Y + H };
    DrawSectionRect(hdc, sec);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, Style::LABEL);

    HFONT oldFont = (HFONT)SelectObject(hdc, m_font);

    RECT rcTop = {
        X + MARGIN_TOP_LABEL_X,
        Y + MARGIN_TOP_LABEL_Y,
        X + MARGIN_TOP_LABEL_X + MARGIN_INPUT_W,
        Y + MARGIN_TOP_LABEL_Y + Layout::LABEL_H
    };
    DrawTextW(hdc, L"Margin Top", -1, &rcTop, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT rcBottom = {
        X + MARGIN_BOTTOM_LABEL_X,
        Y + MARGIN_BOTTOM_LABEL_Y,
        X + MARGIN_BOTTOM_LABEL_X + MARGIN_INPUT_W,
        Y + MARGIN_BOTTOM_LABEL_Y + Layout::LABEL_H
    };
    DrawTextW(hdc, L"Margin Bottom", -1, &rcBottom, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT rcLeft = {
        X + MARGIN_LEFT_LABEL_X,
        Y + MARGIN_LEFT_LABEL_Y,
        X + MARGIN_LEFT_LABEL_X + MARGIN_INPUT_W,
        Y + MARGIN_LEFT_LABEL_Y + Layout::LABEL_H
    };
    DrawTextW(hdc, L"Margin Left", -1, &rcLeft, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT rcRight = {
        X + MARGIN_RIGHT_LABEL_X,
        Y + MARGIN_RIGHT_LABEL_Y,
        X + MARGIN_RIGHT_LABEL_X + MARGIN_INPUT_W,
        Y + MARGIN_RIGHT_LABEL_Y + Layout::LABEL_H
    };
    DrawTextW(hdc, L"Margin Right", -1, &rcRight, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    DrawInputChrome(hdc, m_parent, m_editTop, GetFocus() == m_editTop);
    DrawInputChrome(hdc, m_parent, m_editBottom, GetFocus() == m_editBottom);
    DrawInputChrome(hdc, m_parent, m_editLeft, GetFocus() == m_editLeft);
    DrawInputChrome(hdc, m_parent, m_editRight, GetFocus() == m_editRight);

    SelectObject(hdc, oldFont);
}