#include <windows.h>
#include <commctrl.h>
#include "HomeWindow.h"
#include "ui/components/HomeComponentMap.h"
#include "AdvanceConfigSection.h"
#include "MarginConfigSection.h"

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

    // ===== BASIC =====
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

    // ===== ADVANCE =====
    m_adv.Create(GetHwnd(), m_font);

    m_adv.SetPrintModeOptions({
        "Simplex",
        "Duplex",
        "Manual Duplex (Flip On Long Edge)",
        "Manual Duplex (Flip On Short Edge)"
    });
    m_adv.SetPrintModeValue("Manual Duplex (Flip On Short Edge)");
    m_adv.OnPrintModeChange([](const std::string& v) {
        OutputDebugStringA(("PrintMode: " + v + "\n").c_str());
    });

    m_adv.SetPrintModeImage("./assets/PrintMode/flip-short-edge.bmp");

    m_adv.SetPaperOptions({
        "A4",
        "A5",
        "Letter"
    });
    m_adv.SetPaperValue("A4");
    m_adv.OnPaperChange([](const std::string& v) {
        OutputDebugStringA(("Paper: " + v + "\n").c_str());
    });

    m_adv.SetScaleOptions({
        "No Scale",
        "Fit to page (keep aspect ratio)",
        "Fill page (ignore aspect ratio)"
    });
    m_adv.SetScaleValue("Fit to page (keep aspect ratio)");
    m_adv.OnScaleChange([](const std::string& v) {
        OutputDebugStringA(("Scale: " + v + "\n").c_str());
    });

    m_adv.SetOrientationOptions({
        "Auto",
        "Alway Portrait",
        "Alway Landscape"
    });
    m_adv.SetOrientationValue("Auto");
    m_adv.OnOrientationChange([](const std::string& v) {
        OutputDebugStringA(("Orientation: " + v + "\n").c_str());
    });

    m_adv.SetCollateOptions({
        "Collated (1,2,3 | 1,2,3 | 1,2,3)",
        "Uncollated (1,1,1 | 2,2,2 | 3,3,3)"
    });
    m_adv.SetCollateValue("Collated (1,2,3 | 1,2,3 | 1,2,3)");
    m_adv.OnCollateChange([](const std::string& v) {
        OutputDebugStringA(("Collate: " + v + "\n").c_str());
    });

    // ===== MARGIN =====
    m_margin.Create(GetHwnd(), m_font);

    m_margin.OnMarginTopChange([](const std::string& v) {
        OutputDebugStringA(("Top: " + v + "\n").c_str());
    });
    m_margin.OnMarginBottomChange([](const std::string& v) {
        OutputDebugStringA(("Bottom: " + v + "\n").c_str());
    });
    m_margin.OnMarginLeftChange([](const std::string& v) {
        OutputDebugStringA(("Left: " + v + "\n").c_str());
    });
    m_margin.OnMarginRightChange([](const std::string& v) {
        OutputDebugStringA(("Right: " + v + "\n").c_str());
    });
}

void HomeWindow::OnCommand(WPARAM wParam) {
    m_basic.HandleCommand(wParam);
    m_adv.HandleCommand(wParam);
    m_margin.HandleCommand(wParam);
}

void HomeWindow::OnSize() {
    RECT rc{};
    GetClientRect(GetHwnd(), &rc);

    m_basic.Resize(rc.right);
    m_adv.Resize(rc.right);
    m_margin.Resize(rc.right);

    InvalidateRect(GetHwnd(), nullptr, FALSE);
}

void HomeWindow::OnPaint() {
    PAINTSTRUCT ps{};
    HDC hdc = BeginPaint(GetHwnd(), &ps);

    RECT rc{};
    GetClientRect(GetHwnd(), &rc);

    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
    HBITMAP oldBmp = (HBITMAP)SelectObject(mem, bmp);

    HBRUSH bg = CreateSolidBrush(Style::WINDOW_BG);
    FillRect(mem, &rc, bg);
    DeleteObject(bg);

    // Vẽ tất cả section lên mem DC trước
    m_basic.OnPaint(mem);
    m_adv.OnPaint(mem);
    m_margin.OnPaint(mem);

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
        InvalidateRect(GetHwnd(), nullptr, FALSE);
        return 0;

    case WM_DRAWITEM:
    {
        const DRAWITEMSTRUCT* dis = reinterpret_cast<const DRAWITEMSTRUCT*>(lParam);

        if (dis->CtlID == 2001) {
            m_basic.HandleDrawItem(dis);
            return TRUE;
        }

        if (dis->CtlID >= 4101 && dis->CtlID <= 4105) {
            m_adv.HandleDrawItem(dis);
            return TRUE;
        }

        return FALSE;
    }

    case WM_CTLCOLOREDIT:
    {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, Style::INPUT_TEXT);

        static HBRUSH brush = CreateSolidBrush(Style::INPUT_BG);
        return (LRESULT)brush;
    }

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
        RECT rc{};
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