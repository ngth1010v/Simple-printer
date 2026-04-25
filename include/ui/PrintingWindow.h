#pragma once

#include "BaseWindow.h"
#include <string>
#include <functional>

class PrintingWindow : public BaseWindow {
public:
    bool CreateWindowInstance();

    void SetResourceProcess(int target, int current);
    void SetResourceProcessLabel(const std::wstring& content);
    void SetResourceProcessColor(const std::string& color);

    void SetPrintProcess(int target, int current);
    void SetPrintProcessLabel(const std::wstring& content);
    void SetPrintProcessColor(const std::string& color);

    void SetAllowStop(bool allow = true);
    void SetAllowContinue(bool allow = true);
    void SetNotification(const std::wstring& content = L"");

    void OnStop(std::function<void()> cb);
    void OnContinue(std::function<void()> cb);
    void OnCancel(std::function<void()> cb);

protected:
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) override;

private:
    HWND m_btnStop = nullptr;
    HWND m_btnContinue = nullptr;
    HWND m_btnCancel = nullptr;
    HWND m_lblNotification = nullptr;
    HFONT m_font = nullptr;

    int m_resTarget = 0;
    int m_resCurrent = 0;

    int m_printTarget = 0;
    int m_printCurrent = 0;

    std::wstring m_resLabel = L"Rendering resources...";
    std::wstring m_printLabel = L"Printing...";
    std::wstring m_notification = L"";

    COLORREF m_resColor = RGB(255,255,0);
    COLORREF m_printColor = RGB(0,255,0);

    bool m_allowStop = true;
    bool m_allowContinue = true;

    std::function<void()> m_cbStop;
    std::function<void()> m_cbContinue;
    std::function<void()> m_cbCancel;

private:
    void OnCreate();
    void OnCommand(WPARAM wParam);
    void OnPaint();

    COLORREF ParseColor(const std::string& color);
};