#pragma once

#include <windows.h>

class BaseWindow {
public:
    BaseWindow();
    virtual ~BaseWindow();

    bool Create(
        LPCWSTR className,
        LPCWSTR windowName,
        DWORD style,
        DWORD exStyle = 0,
        int x = CW_USEDEFAULT,
        int y = CW_USEDEFAULT,
        int width = CW_USEDEFAULT,
        int height = CW_USEDEFAULT,
        HWND parent = nullptr,
        HMENU menu = nullptr
    );

    HWND GetHwnd() const;
    
protected:
    virtual LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    
private:
    HWND m_hwnd;

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};