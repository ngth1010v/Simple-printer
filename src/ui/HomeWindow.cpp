#include <windows.h>
#include <commctrl.h>
#include "HomeWindow.h"
#include "ui/components/HomeComponentMap.h"


static LRESULT CALLBACK GlobalInputProc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
    UINT_PTR, DWORD_PTR)
{
    switch (msg) {

    case WM_NCPAINT:
        return 0;

    case WM_PAINT:
    {
        LRESULT res = DefSubclassProc(hwnd, msg, wParam, lParam);

        HDC hdc = GetDC(hwnd);
        RECT rc;
        GetClientRect(hwnd, &rc);

        HPEN pen = CreatePen(PS_SOLID, 1, RGB(150,150,150));
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

        RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 6, 6);

        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(pen);

        ReleaseDC(hwnd, hdc);
        return res;
    }
    }

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

bool HomeWindow::CreateWindowInstance() {
    return Create(
        L"HomeWindowClass",
        L"Simple Printer",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_SIZEBOX,
        0,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        ui::home::Layout::MIN_WIDTH,
        ui::home::Layout::MIN_HEIGHT
    );
}

void HomeWindow::ApplyInputStyle(HWND hwnd) {
    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    SetWindowLong(hwnd, GWL_STYLE, style & ~WS_BORDER);

    SetWindowSubclass(hwnd, GlobalInputProc, 0, 0);
}

// ================= INTERNAL =================

void HomeWindow::OnCreate() {
    // ===== FIX FLICKER =====
    SetWindowLong(GetHwnd(), GWL_STYLE,
        GetWindowLong(GetHwnd(), GWL_STYLE) | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

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

    // ===== CREATE BASIC =====
    m_basic.Create(GetHwnd(), m_font);
    m_basic.SetStyleApplier([this](HWND h) {
        ApplyInputStyle(h);
    });

    // ===== TEST DATA =====
    m_basic.SetPrinterOptions({
        "Canon LBP 3018",
        "HP LaserJet",
        "Microsoft Print to PDF"
    });

    m_basic.OnPrinterChange([](const std::string& v) {
        OutputDebugStringA(("Printer: " + v + "\n").c_str());
    });

    m_basic.OnCopiesChange([](const std::string& v) {
        OutputDebugStringA(("Copies: " + v + "\n").c_str());
    });
}

void HomeWindow::OnCommand(WPARAM wParam) {
    m_basic.HandleCommand(wParam);
}

void HomeWindow::OnSize() {
    RECT rc;
    GetClientRect(GetHwnd(), &rc);

    m_basic.Resize(rc.right);
}

void HomeWindow::OnPaint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(GetHwnd(), &ps);

    RECT rc;
    GetClientRect(GetHwnd(), &rc);

    // double buffer
    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
    HBITMAP oldBmp = (HBITMAP)SelectObject(mem, bmp);

    // ===== WINDOW BG =====
    HBRUSH bg = CreateSolidBrush(RGB(240,240,240));
    FillRect(mem, &rc, bg);
    DeleteObject(bg);

    // ===== CALL SECTION =====
    m_basic.OnPaint(mem);

    // ===== BLIT =====
    BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);

    SelectObject(mem, oldBmp);
    DeleteObject(bmp);
    DeleteDC(mem);

    EndPaint(GetHwnd(), &ps);
}

// ================= MSG =================

LRESULT HomeWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

    case WM_CREATE:
        OnCreate();
        return 0;

    case WM_COMMAND:
        OnCommand(wParam);
        return 0;

    case WM_SIZE:
        OnSize();
        return 0;

    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        mmi->ptMinTrackSize.x = ui::home::Layout::MIN_WIDTH;
        mmi->ptMinTrackSize.y = ui::home::Layout::MIN_HEIGHT;
        return 0;
    }

    case WM_ERASEBKGND:
    {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(GetHwnd(), &rc);

        HBRUSH br = CreateSolidBrush(RGB(240,240,240));
        FillRect(hdc, &rc, br);
        DeleteObject(br);
        return 1;
    }

    case WM_PAINT:
        OnPaint();
        return 0;

    case WM_DESTROY:
        if (m_font) DeleteObject(m_font);
        PostQuitMessage(0);
        return 0;
    }

    return BaseWindow::HandleMessage(msg, wParam, lParam);
}

