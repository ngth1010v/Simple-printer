#pragma once

#include "BaseWindow.h"
#include <string>

class PrintingWindow : public BaseWindow {
public:
    bool CreateWindowInstance();

    // gọi từ App / Worker để update
    void SetStatus(const std::wstring& text);

protected:
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) override;

private:
    HWND m_labelStatus = nullptr;
    HWND m_btnStop = nullptr;

    std::wstring m_statusText = L"Initializing...";

    void OnCreate();
    void OnCommand(WPARAM wParam);
    void OnPaint();
};