#include "PrintingWindow.h"

#define ID_BTN_PAUSE     1001
#define ID_BTN_CONTINUE 1002
#define ID_BTN_CANCEL   1003

#define MARGIN  8
#define BTN_H   22
#define BTN_GAP 6
#define SECTION_GAP 10



// ================= CREATE =================

bool PrintingWindow::CreateWindowInstance() {
    return Create(
        L"PrintingWindowClass",
        L"Printing...",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, // có taskbar
        0,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        280, 
        202
    );
}

// ================= PUBLIC =================

void PrintingWindow::SetResourceProcess(int t, int c) {
    m_resTarget = t;
    m_resCurrent = c;
    InvalidateRect(GetHwnd(), nullptr, FALSE);
}

void PrintingWindow::SetResourceProcessLabel(const std::wstring& s) {
    m_resLabel = s;
    InvalidateRect(GetHwnd(), nullptr, FALSE);
}

void PrintingWindow::SetResourceProcessColor(const std::string& c) {
    m_resColor = ParseColor(c);
    InvalidateRect(GetHwnd(), nullptr, FALSE);
}

void PrintingWindow::SetPrintProcess(int t, int c) {
    m_printTarget = t;
    m_printCurrent = c;
    InvalidateRect(GetHwnd(), nullptr, FALSE);
}

void PrintingWindow::SetPrintProcessLabel(const std::wstring& s) {
    m_printLabel = s;
    InvalidateRect(GetHwnd(), nullptr, FALSE);
}

void PrintingWindow::SetPrintProcessColor(const std::string& c) {
    m_printColor = ParseColor(c);
    InvalidateRect(GetHwnd(), nullptr, FALSE);
}

void PrintingWindow::SetAllowPause(bool allow) {
    m_allowPause = allow;
    if (m_btnPause) EnableWindow(m_btnPause, allow);
}

void PrintingWindow::SetAllowContinue(bool allow) {
    m_allowContinue = allow;
    if (m_btnContinue) EnableWindow(m_btnContinue, allow);
}

void PrintingWindow::SetNotification(const std::wstring& s) {
    if (m_notification == s) return;
    m_notification = s;
    InvalidateRect(GetHwnd(), nullptr, FALSE);
}

void PrintingWindow::OnPause(std::function<void()> cb) { m_cbPause = cb; }
void PrintingWindow::OnContinue(std::function<void()> cb) { m_cbContinue = cb; }
void PrintingWindow::OnCancel(std::function<void()> cb) { m_cbCancel = cb; }

// ================= INTERNAL =================

COLORREF PrintingWindow::ParseColor(const std::string& c) {
    if (c == "red") return RGB(200,0,0);
    if (c == "green") return RGB(0,200,0);
    if (c == "yellow") return RGB(200,200,0);
    return RGB(200,200,200);
}

void PrintingWindow::OnCreate() {
    // ===== STYLE FIX (rất quan trọng cho flicker) =====
    SetWindowLong(GetHwnd(), GWL_STYLE,
        GetWindowLong(GetHwnd(), GWL_STYLE) | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

    RECT rc;
    GetClientRect(GetHwnd(), &rc);

    int btnW = (rc.right - MARGIN * 2 - BTN_GAP) / 2;

    // ===== FONT =====
    m_font = CreateFont(
        -12, 0, 0, 0,
        FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH,
        L"Segoe UI"
    );
    m_fontSmall = CreateFont(
        -11, 0, 0, 0,   // nhỏ hơn -12
        FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH,
        L"Segoe UI"
    );

    // ===== BUTTONS =====
    m_btnPause = CreateWindowEx(0, L"BUTTON", L"Pause",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        MARGIN,
        rc.bottom - MARGIN - BTN_GAP - BTN_H * 2,
        btnW,
        BTN_H,
        GetHwnd(),
        (HMENU)ID_BTN_PAUSE,
        GetModuleHandle(nullptr),
        nullptr);

    m_btnContinue = CreateWindowEx(0, L"BUTTON", L"Continue",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        MARGIN + BTN_GAP + btnW,
        rc.bottom - MARGIN - BTN_GAP - BTN_H * 2,
        btnW,
        BTN_H,
        GetHwnd(),
        (HMENU)ID_BTN_CONTINUE,
        GetModuleHandle(nullptr),
        nullptr);

    m_btnCancel = CreateWindowEx(0, L"BUTTON", L"Cancel",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        MARGIN,
        rc.bottom - MARGIN - BTN_H,
        rc.right - MARGIN * 2,
        BTN_H,
        GetHwnd(),
        (HMENU)ID_BTN_CANCEL,
        GetModuleHandle(nullptr),
        nullptr);

    // apply font cho control (để đồng bộ UI)
    SendMessage(m_btnPause, WM_SETFONT, (WPARAM)m_font, TRUE);
    SendMessage(m_btnContinue, WM_SETFONT, (WPARAM)m_font, TRUE);
    SendMessage(m_btnCancel, WM_SETFONT, (WPARAM)m_font, TRUE);
    SendMessage(m_lblNotification, WM_SETFONT, (WPARAM)m_font, TRUE);

    // ===== APPLY STATE =====
    EnableWindow(m_btnPause, m_allowPause);
    EnableWindow(m_btnContinue, m_allowContinue);
}

void PrintingWindow::OnCommand(WPARAM wParam) {
    switch (LOWORD(wParam)) {
    case ID_BTN_PAUSE:
        if (m_allowPause && m_cbPause) m_cbPause();
        break;
    case ID_BTN_CONTINUE:
        if (m_allowContinue && m_cbContinue) m_cbContinue();
        break;
    case ID_BTN_CANCEL:
        if (m_cbCancel) m_cbCancel();
        break;
    }
}

// ================= RENDER =================
void PrintingWindow::OnPaint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(GetHwnd(), &ps);

    RECT rc;
    GetClientRect(GetHwnd(), &rc);

    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, bmp);

    // ===== BG =====
    HBRUSH bg = CreateSolidBrush(RGB(245, 245, 245));
    FillRect(memDC, &rc, bg);
    DeleteObject(bg);

    SetBkMode(memDC, TRANSPARENT);
    HFONT oldFont = (HFONT)SelectObject(memDC, m_font);


    // ===== PANEL (viền bao) =====
    RECT panel = {
        MARGIN,
        MARGIN,
        rc.right - MARGIN,
        rc.bottom - MARGIN - BTN_H * 2 - BTN_GAP - SECTION_GAP
    };

    // nền panel (nhẹ hơn bg)
    HBRUSH panelBrush = CreateSolidBrush(RGB(252, 252, 252));
    FillRect(memDC, &panel, panelBrush);
    DeleteObject(panelBrush);

    // viền panel
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
    HPEN oldPen = (HPEN)SelectObject(memDC, pen);
    Rectangle(memDC, panel.left, panel.top, panel.right, panel.bottom);
    SelectObject(memDC, oldPen);
    DeleteObject(pen);

    // ===== LAYOUT =====
    int x = panel.left + 10;
    int y = panel.top + 10;
    int barH = 8;
    int barW = panel.right - panel.left - 20;

    int labelToBar = 18;   // 🔥 tăng khoảng cách
    int sectionGap = 15;   // khoảng cách giữa 2 block
    int descGap = 15;      // 🔥 giảm khoảng cách xuống description

    SIZE sz;

    // ================= RESOURCE =================

    TextOut(memDC, x, y, m_resLabel.c_str(), m_resLabel.length());

    {
        HFONT oldFont2 = (HFONT)SelectObject(memDC, m_fontSmall);

        std::wstring rtxt = std::to_wstring(m_resCurrent) + L"/" + std::to_wstring(m_resTarget);
        GetTextExtentPoint32(memDC, rtxt.c_str(), rtxt.length(), &sz);

        TextOut(memDC, panel.right - 10 - sz.cx, y + 2, rtxt.c_str(), rtxt.length());

        SelectObject(memDC, oldFont2);        
    }


    // progress background (không viền)
    y += labelToBar;

    RECT bgBar = { x, y, x + barW, y + barH };
    HBRUSH barBg = CreateSolidBrush(RGB(220, 220, 220)); // nền tối nhẹ
    FillRect(memDC, &bgBar, barBg);
    DeleteObject(barBg);

    // fill
    int fill = (m_resTarget > 0) ? barW * m_resCurrent / m_resTarget : 0;
    HBRUSH fillBr = CreateSolidBrush(m_resColor);
    RECT fillRect = { x, y, x + fill, y + barH };
    FillRect(memDC, &fillRect, fillBr);
    DeleteObject(fillBr);

    // ================= PRINT =================

    y += sectionGap;

    TextOut(memDC, x, y, m_printLabel.c_str(), m_printLabel.length());

    {
        HFONT oldFont2 = (HFONT)SelectObject(memDC, m_fontSmall);

        std::wstring ptxt = std::to_wstring(m_printCurrent) + L"/" + std::to_wstring(m_printTarget);
        GetTextExtentPoint32(memDC, ptxt.c_str(), ptxt.length(), &sz);

        TextOut(memDC, panel.right - 10 - sz.cx, y + 2, ptxt.c_str(), ptxt.length());

        SelectObject(memDC, oldFont2);        
    }


    y += labelToBar;

    RECT bgBar2 = { x, y, x + barW, y + barH };
    barBg = CreateSolidBrush(RGB(220, 220, 220));
    FillRect(memDC, &bgBar2, barBg);
    DeleteObject(barBg);

    fill = (m_printTarget > 0) ? barW * m_printCurrent / m_printTarget : 0;
    fillBr = CreateSolidBrush(m_printColor);
    RECT fillRect2 = { x, y, x + fill, y + barH };
    FillRect(memDC, &fillRect2, fillBr);
    DeleteObject(fillBr);

    // ================= DESCRIPTION =================

    y += descGap;

    TextOut(memDC, x, y,
        m_notification.c_str(),
        m_notification.length());

    // ===== BLIT =====
    BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);

    // cleanup
    SelectObject(memDC, oldFont);
    SelectObject(memDC, oldBmp);
    DeleteObject(bmp);
    DeleteDC(memDC);

    EndPaint(GetHwnd(), &ps);
}

// ================= MSG =================
LRESULT PrintingWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

    case WM_CREATE:
        OnCreate();
        return 0;

    case WM_COMMAND:
        OnCommand(wParam);
        return 0;

    case WM_PAINT:
        OnPaint();
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        if (m_font) DeleteObject(m_font);
        if (m_fontSmall) DeleteObject(m_fontSmall);
        PostQuitMessage(0);
        return 0;

    // ===== OWNER DRAW BUTTON =====
    case WM_DRAWITEM:
    {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;

        if (dis->CtlType == ODT_BUTTON)
        {
            HDC dc = dis->hDC;
            RECT r = dis->rcItem;

            bool pressed  = (dis->itemState & ODS_SELECTED) != 0;
            bool disabled = (dis->itemState & ODS_DISABLED) != 0;

            // ⚠️ hover phải tự detect (fix ở dưới)
            bool hovered =
                (GetCapture() == NULL) && // tránh khi drag
                (GetForegroundWindow() == GetHwnd());

            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(GetHwnd(), &pt);

            if (!PtInRect(&r, pt))
                hovered = false;

            // ===== COLORS =====
            COLORREF border = RGB(120,120,120);
            COLORREF bg = RGB(255,255,255);

            if (hovered) bg = RGB(235,235,235);
            if (pressed) bg = RGB(220,220,220);
            if (disabled) bg = RGB(245,245,245);

            // ===== BG =====
            HBRUSH br = CreateSolidBrush(bg);
            FillRect(dc, &r, br);
            DeleteObject(br);

            // ===== BORDER =====
            HPEN pen = CreatePen(PS_SOLID, 1, border);
            HPEN oldPen = (HPEN)SelectObject(dc, pen);
            HBRUSH oldBrush = (HBRUSH)SelectObject(dc, GetStockObject(NULL_BRUSH));

            RoundRect(dc, r.left, r.top, r.right, r.bottom, 6, 6);

            SelectObject(dc, oldBrush);
            SelectObject(dc, oldPen);
            DeleteObject(pen);

            // ===== TEXT =====
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, disabled ? RGB(150,150,150) : RGB(40,40,40));

            HFONT oldFont = (HFONT)SelectObject(dc, m_font);

            wchar_t text[64];
            GetWindowText(dis->hwndItem, text, 64);

            DrawText(dc, text, -1, &r,
                DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            SelectObject(dc, oldFont);

            return TRUE;
        }
        break;
    }
    }

    return BaseWindow::HandleMessage(msg, wParam, lParam);
}