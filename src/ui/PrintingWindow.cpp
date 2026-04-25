#include "PrintingWindow.h"

#define ID_BTN_STOP 1001

bool PrintingWindow::CreateWindowInstance() {
    return Create(
        L"PrintingWindowClass",
        L"Printing...",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        0,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        400,
        200
    );
}

void PrintingWindow::SetStatus(const std::wstring& text) {
    m_statusText = text;

    if (m_labelStatus) {
        SetWindowText(m_labelStatus, m_statusText.c_str());
    }
}

void PrintingWindow::OnCreate() {
    // label
    m_labelStatus = CreateWindowEx(
        0,
        L"STATIC",
        m_statusText.c_str(),
        WS_CHILD | WS_VISIBLE,
        20, 20, 340, 30,
        GetHwnd(),
        nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );

    // stop button
    m_btnStop = CreateWindowEx(
        0,
        L"BUTTON",
        L"Stop",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        150, 80, 80, 30,
        GetHwnd(),
        (HMENU)ID_BTN_STOP,
        GetModuleHandle(nullptr),
        nullptr
    );
}

void PrintingWindow::OnCommand(WPARAM wParam) {
    int id = LOWORD(wParam);

    if (id == ID_BTN_STOP) {
        // TODO: hook vào PrintWorker sau
        SetStatus(L"Stopping...");
    }
}

void PrintingWindow::OnPaint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(GetHwnd(), &ps);

    // optional custom draw

    EndPaint(GetHwnd(), &ps);
}

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

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return BaseWindow::HandleMessage(msg, wParam, lParam);
}