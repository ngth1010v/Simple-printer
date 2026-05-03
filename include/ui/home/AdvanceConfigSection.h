#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <functional>

namespace ui {
namespace home {

class AdvanceConfigSection {
public:
    AdvanceConfigSection();

    void Create(HWND parent, HFONT font);
    void Resize(int parentWidth);

    // ===== Print Mode =====
    void SetPrintModeOptions(const std::vector<std::string>& options);
    void SetPrintModeValue(const std::string& value);
    void SetPrintModeImage(const std::string& path);
    void OnPrintModeChange(std::function<void(const std::string&)> cb);

    // ===== Paper =====
    void SetPaperOptions(const std::vector<std::string>& options);
    void SetPaperValue(const std::string& value);
    void OnPaperChange(std::function<void(const std::string&)> cb);

    // ===== Scale =====
    void SetScaleOptions(const std::vector<std::string>& options);
    void SetScaleValue(const std::string& value);
    void OnScaleChange(std::function<void(const std::string&)> cb);

    // ===== Orientation =====
    void SetOrientationOptions(const std::vector<std::string>& options);
    void SetOrientationValue(const std::string& value);
    void OnOrientationChange(std::function<void(const std::string&)> cb);

    void HandleCommand(WPARAM wParam);
    void HandleDrawItem(const DRAWITEMSTRUCT* dis);
    void OnPaint(HDC hdc);

private:
    HWND m_parent = nullptr;
    HFONT m_font = nullptr;

    HWND m_btnPrintMode = nullptr;
    HWND m_btnPaper = nullptr;
    HWND m_btnScale = nullptr;
    HWND m_btnOrientation = nullptr;

    HBITMAP m_printModeBmp = nullptr;

    std::vector<std::wstring> m_printModeOptions;
    std::vector<std::wstring> m_paperOptions;
    std::vector<std::wstring> m_scaleOptions;
    std::vector<std::wstring> m_orientationOptions;

    int m_selPrintMode = -1;
    int m_selPaper = -1;
    int m_selScale = -1;
    int m_selOrientation = -1;

    std::function<void(const std::string&)> m_cbPrintMode;
    std::function<void(const std::string&)> m_cbPaper;
    std::function<void(const std::string&)> m_cbScale;
    std::function<void(const std::string&)> m_cbOrientation;

private:
    void ShowPopup(HWND btn, const std::vector<std::wstring>& opts, int& sel, int baseId, std::function<void(const std::string&)> cb);

    std::wstring ToWide(const std::string& s) const;
    std::string ToNarrow(const std::wstring& ws) const;

    void SetOptions(std::vector<std::wstring>& dst, int& sel, const std::vector<std::string>& src);
    void SetValue(const std::vector<std::wstring>& opts, int& sel, const std::string& v);
};
}
}