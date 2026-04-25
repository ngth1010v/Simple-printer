#include "BaseWindow.h"

BaseWindow::BaseWindow() : m_hwnd(nullptr) {}
BaseWindow::~BaseWindow() {}

HWND BaseWindow::GetHwnd() const {
    return m_hwnd;
}

bool BaseWindow::Create(
    LPCWSTR className,
    LPCWSTR windowName,
    DWORD style,
    DWORD exStyle,
    int x,
    int y,
    int width,
    int height,
    HWND parent,
    HMENU menu
) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = BaseWindow::WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = className;

    RegisterClass(&wc);

    m_hwnd = CreateWindowEx(
        exStyle,
        className,
        windowName,
        style,
        x, y, width, height,
        parent,
        menu,
        GetModuleHandle(nullptr),
        this // truyền this vào
    );

    return (m_hwnd != nullptr);
}

// static
LRESULT CALLBACK BaseWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    BaseWindow* pThis = nullptr;

    if (msg == WM_NCCREATE) {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<BaseWindow*>(cs->lpCreateParams);

        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        pThis->m_hwnd = hwnd;
    }
    else {
        pThis = reinterpret_cast<BaseWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->HandleMessage(msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT BaseWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(m_hwnd, msg, wParam, lParam);
}