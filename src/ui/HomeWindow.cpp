#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "ui/HomeWindow.h"
#include "ui/home/HomeComponentMap.h"

using namespace ui::home;

bool HomeWindow::CreateWindowInstance() {
    RECT rc = { 0, 0, Layout::MIN_WIDTH, Layout::MIN_HEIGHT };

    AdjustWindowRect(
        &rc,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_SIZEBOX,
        FALSE
    );

    return Create(
        L"HomeWindowClass",
        L"Simple Printer",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_SIZEBOX,
        0,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rc.right - rc.left,
        rc.bottom - rc.top
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
    m_adv.Create(GetHwnd(), m_font);
    m_margin.Create(GetHwnd(), m_font);

    // ===== INFO =====
    m_info.Create(GetHwnd(), m_font);

    // default
    m_info.SetTotalFilesValue("1");
    m_info.SetPagesToPrintValue("1");
    m_info.SetSheetsRequiredValue("1");

    // ===== CONTROL =====
    m_control.Create(GetHwnd(), m_font);

    m_control.OnPrint([]() {
        OutputDebugStringA("Print clicked\n");
    });

    m_control.OnCancel([]() {
        OutputDebugStringA("Cancel clicked\n");
    });

    // ===== FILE LIST =====
    m_files.Create(GetHwnd(), m_font);

    m_files.Set({
        {
            "C:/Docs/Test1.docx",
            "Test1.docx",
            "green",
            "1",
            "10"
        },
        {
            "C:/Docs/Test2.docx",
            "Test2.docx",
            "green",
            "1",
            "1"
        },
        {
            "C:/Docs/Test3.docx",
            "Test3.docx",
            "red",
            "5",
            "20"
        },
        {
            "C:/Docs/Test4.docx",
            "Test4.docx",
            "green",
            "10",
            "10"
        },
        {
            "C:/Docs/Test4.docx",
            "Test4.docx",
            "green",
            "10",
            "10"
        },
        {
            "C:/Docs/Test4.docx",
            "Test4.docx",
            "green",
            "10",
            "10"
        },
        {
            "C:/Docs/Test4.docx",
            "Test4.docx",
            "green",
            "10",
            "10"
        }
    });

    m_files.OnChangeRange([](const std::string& path, const std::string& fromRange, const std::string& toRange) {
        OutputDebugStringA(("ChangeRange: " + path + " | " + fromRange + " -> " + toRange + "\n").c_str());
    });

    m_files.OnMoveUp([](const std::string& path) {
        OutputDebugStringA(("MoveUp: " + path + "\n").c_str());
    });
    m_files.OnMoveDown([](const std::string& path) {
        OutputDebugStringA(("MoveDown: " + path + "\n").c_str());
    });
    m_files.OnRemove([](const std::string& path) {
        OutputDebugStringA(("Remove: " + path + "\n").c_str());
    });
    m_files.OnAdd([](const std::string& path) {
        OutputDebugStringA(("Add: " + path + "\n").c_str());
    });
}

void HomeWindow::OnCommand(WPARAM wParam) {
    m_basic.HandleCommand(wParam);
    m_adv.HandleCommand(wParam);
    m_margin.HandleCommand(wParam);
    m_control.HandleCommand(wParam);
}

void HomeWindow::OnSize() {
    RECT rc{};
    GetClientRect(GetHwnd(), &rc);

    m_basic.Resize(rc.right);
    m_adv.Resize(rc.right);
    m_margin.Resize(rc.right);
    m_info.Resize(rc.right);
    m_control.Resize(rc.right, rc.bottom);
    m_files.Resize(rc.right, rc.bottom);

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
    m_info.OnPaint(mem);
    m_control.OnPaint(mem);
    m_files.OnPaintStatic(mem, rc.right, rc.bottom);

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
        m_files.HandleCommand(wParam);
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

        if (dis->CtlID == 5001 || dis->CtlID == 5002) {
            m_control.HandleDrawItem(dis);
            return TRUE;
        }

        if (dis->CtlID >= 6201 && dis->CtlID <= 6204) {
            m_files.HandleDrawItem(dis);
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

        RECT rc = { 0, 0, Layout::MIN_WIDTH, Layout::MIN_HEIGHT };

        AdjustWindowRect(
            &rc,
            GetWindowLong(GetHwnd(), GWL_STYLE),
            FALSE
        );

        mmi->ptMinTrackSize.x = rc.right - rc.left;
        mmi->ptMinTrackSize.y = rc.bottom - rc.top;

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

    case WM_LBUTTONDOWN:
    {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);

        // Nếu click KHÔNG nằm trong FileListView
        if (!m_files.IsPointInside(x, y)) {
            m_files.ClearSelectionPublic();
        }

        break;
    }

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