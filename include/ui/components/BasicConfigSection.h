#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <functional>

namespace ui {
namespace home {

class BasicConfigSection {
public:
    BasicConfigSection();

    void Create(HWND parent, HFONT font);

    void Resize(int parentWidth);

    // ===== API =====
    void SetPrinterOptions(const std::vector<std::string>& options);
    void SetPrinterValue(const std::string& value);
    void OnPrinterChange(std::function<void(const std::string&)> cb);

    void SetCopiesValue(const std::string& value);
    void OnCopiesChange(std::function<void(const std::string&)> cb);

    void HandleCommand(WPARAM wParam);
    void OnPaint(HDC hdc);
    void SetStyleApplier(std::function<void(HWND)> fn);

private:
    HWND m_parent = nullptr;
    std::function<void(HWND)> m_applyStyle;

    HWND m_lblPrinter = nullptr;
    HWND m_cbPrinter  = nullptr;

    HWND m_lblCopies  = nullptr;
    HWND m_editCopies = nullptr;

    HFONT m_font = nullptr;

    std::function<void(const std::string&)> m_cbPrinterChange;
    std::function<void(const std::string&)> m_cbCopiesChange;

private:
    std::string GetComboText(HWND hwnd);
    std::string GetEditText(HWND hwnd);
};

}
}
