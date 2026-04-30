#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <functional>



namespace ui {
namespace home {

LRESULT CALLBACK EditSubclassProc(
   HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR
);    

class MarginConfigSection {
public:
    MarginConfigSection();

    void Create(HWND parent, HFONT font);
    void Resize(int parentWidth);

    // ===== TOP =====
    void SetMarginTopValue(const std::string& value);
    void OnMarginTopChange(std::function<void(const std::string&)> cb);

    // ===== BOTTOM =====
    void SetMarginBottomValue(const std::string& value);
    void OnMarginBottomChange(std::function<void(const std::string&)> cb);

    // ===== LEFT =====
    void SetMarginLeftValue(const std::string& value);
    void OnMarginLeftChange(std::function<void(const std::string&)> cb);

    // ===== RIGHT =====
    void SetMarginRightValue(const std::string& value);
    void OnMarginRightChange(std::function<void(const std::string&)> cb);

    void HandleCommand(WPARAM wParam);
    void OnPaint(HDC hdc);

private:
    friend LRESULT CALLBACK EditSubclassProc(
        HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR
    );

    HWND m_parent = nullptr;
    HFONT m_font = nullptr;
    bool m_silentSet = false;

    HWND m_editTop = nullptr;
    HWND m_editBottom = nullptr;
    HWND m_editLeft = nullptr;
    HWND m_editRight = nullptr;

    std::function<void(const std::string&)> m_cbTop;
    std::function<void(const std::string&)> m_cbBottom;
    std::function<void(const std::string&)> m_cbLeft;
    std::function<void(const std::string&)> m_cbRight;

private:
    std::wstring ToWide(const std::string& s) const;
    std::string ToNarrow(const std::wstring& ws) const;
    std::string GetEditText(HWND hwnd);

    void RemoveEditBorder(HWND hwnd);
};

}
}