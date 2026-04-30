#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <functional>





namespace ui {
namespace home {

LRESULT CALLBACK EditSubclassProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam,
    UINT_PTR uIdSubclass,
    DWORD_PTR dwRefData
);    

class BasicConfigSection {
public:
    BasicConfigSection();

    void Create(HWND parent, HFONT font);
    void Resize(int parentWidth);

    void SetPrinterOptions(const std::vector<std::string>& options);
    void SetPrinterValue(const std::string& value);
    void OnPrinterChange(std::function<void(const std::string&)> cb);

    void SetCopiesValue(const std::string& value);
    void OnCopiesChange(std::function<void(const std::string&)> cb);

    void HandleCommand(WPARAM wParam);
    void HandleDrawItem(const DRAWITEMSTRUCT* dis);
    void OnPaint(HDC hdc);

    HWND GetPrinterHWND() const { return m_btnPrinter; }
    HWND GetCopiesHWND() const { return m_editCopies; }

private:

    friend LRESULT CALLBACK EditSubclassProc(
        HWND,
        UINT,
        WPARAM,
        LPARAM,
        UINT_PTR,
        DWORD_PTR
    );

    HWND m_parent = nullptr;
    HFONT m_font = nullptr;
    bool m_silentSet = false;

    // printer field is now a custom owner-draw button, not a native combobox
    HWND m_btnPrinter = nullptr;
    HWND m_editCopies = nullptr;

    std::vector<std::wstring> m_printerOptions;
    int m_printerSelection = -1;

    std::function<void(const std::string&)> m_cbPrinterChange;
    std::function<void(const std::string&)> m_cbCopiesChange;

private:
    void ShowPrinterPopup();
    std::wstring ToWide(const std::string& s) const;
    std::string ToNarrow(const std::wstring& ws) const;

    std::string GetSelectedPrinterText() const;
    std::string GetEditText(HWND hwnd);
};

}
}