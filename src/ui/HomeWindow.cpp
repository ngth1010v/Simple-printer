#include <windows.h>
#include <commctrl.h>
#include "HomeWindow.h"
#include "ui/components/HomeComponentMap.h"

using namespace ui::home;

bool HomeWindow::CreateWindowInstance() {
    return Create(
        L"HomeWindowClass",
        L"Simple Printer",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_SIZEBOX,
        0,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        Layout::MIN_WIDTH,
        Layout::MIN_HEIGHT
    );
}

void HomeWindow::OnCreate() {
    SetWindowLongPtr(
        GetHwnd(),
        GWL_STYLE,
        GetWindowLongPtr(GetHwnd(), GWL_STYLE) | WS_CLIPCHILDREN | WS_CLIPSIBLINGS
    );

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

    m_basic.Create(GetHwnd(), m_font);

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

    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
    HBITMAP oldBmp = (HBITMAP)SelectObject(mem, bmp);

    HBRUSH bg = CreateSolidBrush(Style::WINDOW_BG);
    FillRect(mem, &rc, bg);
    DeleteObject(bg);

    m_basic.OnPaint(mem);

    BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);

    SelectObject(mem, oldBmp);
    DeleteObject(bmp);
    DeleteDC(mem);

    EndPaint(GetHwnd(), &ps);
}

LRESULT HomeWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        OnCreate();
        return 0;

    case WM_COMMAND:
        OnCommand(wParam);
        return 0;

    case WM_DRAWITEM:
        m_basic.HandleDrawItem(reinterpret_cast<const DRAWITEMSTRUCT*>(lParam));
        return TRUE;

    case WM_SIZE:
        OnSize();
        return 0;

    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        mmi->ptMinTrackSize.x = Layout::MIN_WIDTH;
        mmi->ptMinTrackSize.y = Layout::MIN_HEIGHT;
        return 0;
    }

    case WM_ERASEBKGND:
    {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(GetHwnd(), &rc);

        HBRUSH br = CreateSolidBrush(Style::WINDOW_BG);
        FillRect(hdc, &rc, br);
        DeleteObject(br);
        return 1;
    }

    case WM_PAINT:
        OnPaint();
        return 0;

    case WM_DESTROY:
        if (m_font) {
            DeleteObject(m_font);
            m_font = nullptr;
        }
        PostQuitMessage(0);
        return 0;
    }

    return BaseWindow::HandleMessage(msg, wParam, lParam);
}