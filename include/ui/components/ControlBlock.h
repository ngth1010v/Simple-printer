#pragma once

#include <windows.h>
#include <functional>

namespace ui {
namespace home {

class ControlBlock {
public:
    ControlBlock();

    void Create(HWND parent, HFONT font);
    void Resize(int parentW, int parentH);

    void HandleCommand(WPARAM wParam);
    void HandleDrawItem(const DRAWITEMSTRUCT* dis);
    void OnPaint(HDC hdc);

    void OnPrint(std::function<void()> cb);
    void OnCancel(std::function<void()> cb);

private:
    HWND m_parent = nullptr;
    HFONT m_font = nullptr;

    HWND m_btnPrint = nullptr;
    HWND m_btnCancel = nullptr;

    std::function<void()> m_cbPrint;
    std::function<void()> m_cbCancel;
};

}
}