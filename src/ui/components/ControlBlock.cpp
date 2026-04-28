#include "ControlBlock.h"
#include "HomeComponentMap.h"

#define ID_BTN_PRINT   5001
#define ID_BTN_CANCEL  5002

using namespace ui::home;

namespace {

    COLORREF InputTextColor(BOOL disabled) {
        return disabled ? Style::INPUT_TEXT_DISABLED : Style::INPUT_TEXT;
    }

    COLORREF InputBgColor(BOOL disabled, BOOL hot, BOOL pressed, bool isPrint) {
        if (disabled) return Style::INPUT_BG_DISABLED;

        if (isPrint) {
            return Style::PRINT_CONTROL_BG;
        }

        if (pressed) return Style::INPUT_BG_FOCUS;
        if (hot)     return Style::INPUT_BG_HOVER;
        return Style::INPUT_BG;
    }

    COLORREF InputBorderColor(BOOL disabled) {
        return disabled ? Style::INPUT_BORDER_DISABLED : Style::INPUT_BORDER;
    }

    void DrawRoundedFrame(HDC hdc, const RECT& rc, COLORREF borderColor, int radius) {
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        HPEN pen = CreatePen(PS_SOLID, 1, borderColor);
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);

        RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, radius, radius);

        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(pen);
    }

    RECT GetRect(HWND parent, HWND child) {
        RECT rc{};
        GetWindowRect(child, &rc);
        MapWindowPoints(nullptr, parent, (POINT*)&rc, 2);
        return rc;
    }

    void DrawChrome(HDC hdc, HWND parent, HWND hwnd) {
        RECT rc = GetRect(parent, hwnd);

        rc.left   -= Layout::INPUT_BORDER_OFFSET;
        rc.top    -= Layout::INPUT_BORDER_OFFSET;
        rc.right  += Layout::INPUT_BORDER_OFFSET;
        rc.bottom += Layout::INPUT_BORDER_OFFSET;

        DrawRoundedFrame(
            hdc,
            rc,
            Style::INPUT_BORDER,
            Style::INPUT_RADIUS
        );
    }
}

ControlBlock::ControlBlock() {}

void ControlBlock::Create(HWND parent, HFONT font) {
    m_parent = parent;
    m_font = font;

    m_btnPrint = CreateWindowEx(
        0, L"BUTTON", L"",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        0, 0, 0, 0,
        parent,
        (HMENU)ID_BTN_PRINT,
        GetModuleHandle(nullptr),
        nullptr
    );

    m_btnCancel = CreateWindowEx(
        0, L"BUTTON", L"",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        0, 0, 0, 0,
        parent,
        (HMENU)ID_BTN_CANCEL,
        GetModuleHandle(nullptr),
        nullptr
    );

    SendMessage(m_btnPrint, WM_SETFONT, (WPARAM)m_font, TRUE);
    SendMessage(m_btnCancel, WM_SETFONT, (WPARAM)m_font, TRUE);
}

void ControlBlock::Resize(int parentW, int parentH) {
    using namespace Layout::ControlBlock;

    MoveWindow(
        m_btnPrint,
        CALC_PRINT_INPUT_X(parentW) + Layout::INPUT_BORDER_OFFSET,
        CALC_PRINT_INPUT_Y(parentH) + Layout::INPUT_BORDER_OFFSET,
        INPUT_W                     - 2 * Layout::INPUT_BORDER_OFFSET,
        CONTROL_INPUT_H             - 2 * Layout::INPUT_BORDER_OFFSET,
        TRUE
    );

    MoveWindow(
        m_btnCancel,
        CALC_CANCEL_INPUT_X(parentW) + Layout::INPUT_BORDER_OFFSET,
        CALC_CANCEL_INPUT_Y(parentH) + Layout::INPUT_BORDER_OFFSET,
        INPUT_W                     - 2 * Layout::INPUT_BORDER_OFFSET,
        CONTROL_INPUT_H              - 2 * Layout::INPUT_BORDER_OFFSET,
        TRUE
    );
}

void ControlBlock::HandleCommand(WPARAM wParam) {
    int id = LOWORD(wParam);
    int code = HIWORD(wParam);

    if (code != BN_CLICKED) return;

    if (id == ID_BTN_PRINT) {
        if (m_cbPrint) m_cbPrint();
    }

    if (id == ID_BTN_CANCEL) {
        if (m_cbCancel) m_cbCancel();
    }
}

void ControlBlock::HandleDrawItem(const DRAWITEMSTRUCT* dis) {
    if (!dis) return;

    if (dis->CtlType != ODT_BUTTON) return;

    bool isPrint = (dis->CtlID == ID_BTN_PRINT);
    bool isCancel = (dis->CtlID == ID_BTN_CANCEL);

    if (!isPrint && !isCancel) return;

    HDC hdc = dis->hDC;
    RECT rc = dis->rcItem;

    const bool pressed = (dis->itemState & ODS_SELECTED) != 0;
    const bool disabled = (dis->itemState & ODS_DISABLED) != 0;
    const bool hot = (dis->itemState & ODS_HOTLIGHT) != 0;

    COLORREF bg = InputBgColor(disabled, hot, pressed, isPrint);
    COLORREF textColor = InputTextColor(disabled);

    HBRUSH brush = CreateSolidBrush(bg);
    FillRect(hdc, &rc, brush);
    DeleteObject(brush);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, textColor);

    HFONT oldFont = (HFONT)SelectObject(hdc, m_font);

    LPCWSTR text = isPrint ? L"Print" : L"Cancel";

    DrawTextW(
        hdc,
        text,
        -1,
        &rc,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE
    );

    SelectObject(hdc, oldFont);
}

void ControlBlock::OnPaint(HDC hdc) {
    DrawChrome(hdc, m_parent, m_btnPrint);
    DrawChrome(hdc, m_parent, m_btnCancel);
}

void ControlBlock::OnPrint(std::function<void()> cb) {
    m_cbPrint = cb;
}

void ControlBlock::OnCancel(std::function<void()> cb) {
    m_cbCancel = cb;
}