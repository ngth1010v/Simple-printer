#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <functional>

namespace ui {
namespace home {

struct UiFileData {
    std::string path;
    std::string name;
    std::string statusColor;
    std::string fromRange;
    std::string toRange;
};

class FileListView {
public:
    FileListView();

    bool Create(HWND parent, HFONT font);
    void Resize(int parentWidth, int parentHeight);

    void Set(std::vector<UiFileData> files);

    void OnChangeRange(std::function<void(const std::string& path, const std::string& fromRange, const std::string& toRange)> cb);
    void OnMoveUp(std::function<void(const std::string& path)> cb);
    void OnMoveDown(std::function<void(const std::string& path)> cb);
    void OnRemove(std::function<void(const std::string& path)> cb);
    void OnAdd(std::function<void(const std::string& path)> cb);

    // Static part: section background / border / title.
    // Call this from HomeWindow::OnPaint(), like BasicConfigSection::OnPaint().
    void OnPaintStatic(HDC hdc, int parentWidth, int parentHeight);

    // Forward WM_COMMAND / WM_DRAWITEM from HomeWindow to here
    // for the file control buttons that are sibling children of HomeWindow.
    void HandleCommand(WPARAM wParam);
    void HandleDrawItem(const DRAWITEMSTRUCT* dis);

    HWND GetHWND() const { return m_hwnd; }

    void ClearSelectionPublic();
    bool IsPointInside(int x, int y) const;

private:
    bool m_isInternalUpdate = false;
    HWND m_parent = nullptr;
    HWND m_hwnd = nullptr;   // list child window only
    HFONT m_font = nullptr;

    std::vector<UiFileData> m_files;

    int m_scrollY = 0;
    int m_maxScrollY = 0;
    int m_selectedIndex = -1;

    // Buttons are NOT inside the scrolling list window.
    // They are sibling children of HomeWindow.
    HWND m_btnUp = nullptr;
    HWND m_btnDown = nullptr;
    HWND m_btnRemove = nullptr;
    HWND m_btnAdd = nullptr;

    // Selected row edit boxes are children of the list window.
    HWND m_editFrom = nullptr;
    HWND m_editTo = nullptr;

    std::string m_editOriginalFrom;
    std::string m_editOriginalTo;

    std::function<void(const std::string& path, const std::string& fromRange, const std::string& toRange)> m_cbChangeRange;
    std::function<void(const std::string& path)> m_cbMoveUp;
    std::function<void(const std::string& path)> m_cbMoveDown;
    std::function<void(const std::string& path)> m_cbRemove;
    std::function<void(const std::string& path)> m_cbAdd;

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

    
    void OnCreateInternal();
    void OnDestroyInternal();
    void OnSizeInternal();
    void OnPaintInternal(HDC hdc);
    void OnCommandInternal(WPARAM wParam, LPARAM lParam);
    void OnCtlColorEditInternal(HDC hdc, HWND hwnd);
    void OnLButtonDownInternal(int x, int y);
    void OnMouseWheelInternal(short delta);
    void OnVScrollInternal(WPARAM wParam);
    void OnSetFocusInternal();
    
    void UpdateLayout();
    void UpdateScrollBar();
    void UpdateButtonsState();
    void UpdateSelectedEditLayout();
    void EnsureSelectionVisible();
    void ClearSelection();
    void SelectRow(int index);
    void CreateSelectedEdits();
    void DestroySelectedEdits();
    void CommitSelectedEditsIfNeeded(bool forceCallback);
    void ScrollTo(int newScrollY);
    
    int  GetRowStep() const;
    int  GetListViewportHeight() const;
    
    int  HitTestRow(int x, int y) const;
    
    RECT GetClientRectSafe() const;
    RECT GetRowRect(int index) const;
    RECT GetLabelRect(int index) const;
    RECT GetWrapperRect(int index) const;
    RECT GetFromRect(int index) const;
    RECT GetToRect(int index) const;
    RECT GetDividerRect(int index) const;
    
    std::wstring ToWide(const std::string& s) const;
    std::string ToNarrow(const std::wstring& ws) const;
    std::string GetWindowTextUtf8(HWND hwnd) const;
    
    static std::string ToLowerCopy(std::string s);
    static bool StartsWithNoCase(const std::string& s, const char* prefix);
    static bool ParseRgbHex(const std::string& s, COLORREF& out);
    static COLORREF ParseColorString(const std::string& s, bool* ok = nullptr);
    static COLORREF BlendWithWhite(COLORREF c, int whitePercent);
    static void FillRectSolid(HDC hdc, const RECT& rc, COLORREF c);
    static void DrawRoundedFrame(HDC hdc, const RECT& rc, COLORREF borderColor, int radius);
    static void DrawCenteredText(HDC hdc, const RECT& rc, const std::wstring& text, COLORREF color, HFONT font);
    static void DrawLeftEllipsisText(HDC hdc, const RECT& rc, const std::wstring& text, COLORREF color, HFONT font);
    bool IsOurButton(HWND hwnd);

    void DrawRow(HDC hdc, int index, const RECT& clipRc);
    void DrawButton(HDC hdc, const DRAWITEMSTRUCT* dis, const wchar_t* text, COLORREF bg);
};

} // namespace home
} // namespace ui