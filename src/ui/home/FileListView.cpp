#include "FileListView.h"
#include "HomeComponentMap.h"

#include <commctrl.h>
#include <windowsx.h>
#include <algorithm>
#include <cwchar>
#include <cstring>
#include <cctype>

using namespace ui::home;

namespace {

    // ===== INTERNAL IDS =====
    constexpr int ID_FLV_BTN_UP     = 6201;
    constexpr int ID_FLV_BTN_DOWN   = 6202;
    constexpr int ID_FLV_BTN_REMOVE = 6203;
    constexpr int ID_FLV_BTN_ADD    = 6204;

    constexpr int ID_FLV_EDIT_FROM  = 6301;
    constexpr int ID_FLV_EDIT_TO    = 6302;

    // Keep paint fast, but still preload a bit outside viewport.
    constexpr int LOAD_OUTSIDE_PX = 300;

    constexpr const wchar_t* FLV_CLASS_NAME = L"ui.home.FileListView";

    int ClampInt(int v, int lo, int hi) {
        return (v < lo) ? lo : (v > hi ? hi : v);
    }

    bool RectContains(const RECT& rc, int x, int y) {
        return x >= rc.left && x < rc.right && y >= rc.top && y < rc.bottom;
    }

    std::wstring WidenSimple(const std::string& s) {
        if (s.empty()) return L"";

        int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
        if (len > 0) {
            std::wstring ws(len - 1, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, ws.data(), len);
            return ws;
        }

        return std::wstring(s.begin(), s.end());
    }
}

FileListView::FileListView() {}

std::string FileListView::ToLowerCopy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) {
        return (char)std::tolower(ch);
    });
    return s;
}

bool FileListView::StartsWithNoCase(const std::string& s, const char* prefix) {
    const size_t plen = std::strlen(prefix);
    if (s.size() < plen) return false;

    for (size_t i = 0; i < plen; ++i) {
        if (std::tolower((unsigned char)s[i]) != std::tolower((unsigned char)prefix[i])) {
            return false;
        }
    }
    return true;
}

bool FileListView::ParseRgbHex(const std::string& s, COLORREF& out) {
    std::string t = s;
    if (!t.empty() && t[0] == '#') t.erase(t.begin());
    if (t.size() != 6) return false;

    auto hexVal = [](char ch, int& v) -> bool {
        if (ch >= '0' && ch <= '9') { v = ch - '0'; return true; }
        if (ch >= 'a' && ch <= 'f') { v = 10 + (ch - 'a'); return true; }
        if (ch >= 'A' && ch <= 'F') { v = 10 + (ch - 'A'); return true; }
        return false;
    };

    int r1 = 0, r2 = 0, g1 = 0, g2 = 0, b1 = 0, b2 = 0;
    if (!hexVal(t[0], r1) || !hexVal(t[1], r2) ||
        !hexVal(t[2], g1) || !hexVal(t[3], g2) ||
        !hexVal(t[4], b1) || !hexVal(t[5], b2)) {
        return false;
    }

    int r = (r1 << 4) | r2;
    int g = (g1 << 4) | g2;
    int b = (b1 << 4) | b2;

    out = RGB(r, g, b);
    return true;
}

COLORREF FileListView::ParseColorString(const std::string& s, bool* ok) {
    bool parsed = false;
    COLORREF out = Style::FILE_LIST_VIEW_GREEN_STATUS;

    std::string t = ToLowerCopy(s);

    if (t == "green") {
        out = Style::FILE_LIST_VIEW_GREEN_STATUS;
        parsed = true;
    } else if (t == "red") {
        out = Style::FILE_LIST_VIEW_RED_STATUS;
        parsed = true;
    } else if (t == "black") {
        out = RGB(0, 0, 0);
        parsed = true;
    } else if (t == "white") {
        out = RGB(255, 255, 255);
        parsed = true;
    } else if (ParseRgbHex(t, out)) {
        parsed = true;
    }

    if (ok) *ok = parsed;
    return out;
}

COLORREF FileListView::BlendWithWhite(COLORREF c, int whitePercent) {
    whitePercent = ClampInt(whitePercent, 0, 100);
    const int colorPercent = 100 - whitePercent;

    const int r = (GetRValue(c) * colorPercent + 255 * whitePercent) / 100;
    const int g = (GetGValue(c) * colorPercent + 255 * whitePercent) / 100;
    const int b = (GetBValue(c) * colorPercent + 255 * whitePercent) / 100;
    return RGB(r, g, b);
}

void FileListView::FillRectSolid(HDC hdc, const RECT& rc, COLORREF c) {
    if (rc.right <= rc.left || rc.bottom <= rc.top) return;
    HBRUSH br = CreateSolidBrush(c);
    FillRect(hdc, &rc, br);
    DeleteObject(br);
}

void FileListView::DrawRoundedFrame(HDC hdc, const RECT& rc, COLORREF borderColor, int radius) {
    HPEN pen = CreatePen(PS_SOLID, 1, borderColor);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

    if (radius <= 0) {
        Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
    } else {
        RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, radius, radius);
    }

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

void FileListView::DrawCenteredText(HDC hdc, const RECT& rc, const std::wstring& text, COLORREF color, HFONT font) {
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, color);
    HFONT oldFont = (HFONT)SelectObject(hdc, font);
    DrawTextW(hdc, text.c_str(), -1, const_cast<RECT*>(&rc), DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(hdc, oldFont);
}

void FileListView::DrawLeftEllipsisText(HDC hdc, const RECT& rc, const std::wstring& text, COLORREF color, HFONT font) {
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, color);
    HFONT oldFont = (HFONT)SelectObject(hdc, font);
    DrawTextW(hdc, text.c_str(), -1, const_cast<RECT*>(&rc), DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    SelectObject(hdc, oldFont);
}

RECT FileListView::GetClientRectSafe() const {
    RECT rc{};
    if (m_hwnd) {
        GetClientRect(m_hwnd, &rc);
    }
    return rc;
}

int FileListView::GetRowStep() const {
    return Layout::FilesSection::FileListView::ROW_H + Layout::FilesSection::FileListView::ROW_GAP;
}

int FileListView::GetListViewportHeight() const {
    RECT rc = GetClientRectSafe();
    return std::max((long)0, rc.bottom);
}

RECT FileListView::GetRowRect(int index) const {
    RECT rcClient = GetClientRectSafe();
    const int left = 0;
    const int right = std::max((long)left, rcClient.right);

    const int rowTop = GetRowStep() * index - m_scrollY;
    RECT rc{ left, rowTop, right, rowTop + Layout::FilesSection::FileListView::ROW_H };
    return rc;
}

RECT FileListView::GetLabelRect(int index) const {
    RECT row = GetRowRect(index);
    RECT rc = row;
    rc.left += Layout::FilesSection::FileListView::STATUS_W + Layout::FilesSection::FileListView::ROW_MARGIN;
    rc.top += Layout::FilesSection::FileListView::ROW_MARGIN;
    rc.right -= Layout::FilesSection::FileListView::ROW_MARGIN;
    rc.bottom = rc.top + Layout::LABEL_H;
    return rc;
}

RECT FileListView::GetWrapperRect(int index) const {
    RECT row = GetRowRect(index);
    RECT rc = row;
    rc.left += Layout::FilesSection::FileListView::STATUS_W + Layout::FilesSection::FileListView::ROW_MARGIN;
    rc.top += Layout::FilesSection::FileListView::ROW_MARGIN + Layout::LABEL_H + Layout::INPUT_GAP;
    rc.right -= Layout::FilesSection::FileListView::ROW_MARGIN;
    rc.bottom = rc.top + Layout::INPUT_H;
    return rc;
}

RECT FileListView::GetFromRect(int index) const {
    RECT wrapper = GetWrapperRect(index);
    const int halfW = (wrapper.right - wrapper.left - Layout::FilesSection::FileListView::INPUT_DIVER_W) / 2;
    RECT rc = wrapper;
    rc.right = rc.left + halfW;
    return rc;
}

RECT FileListView::GetDividerRect(int index) const {
    RECT wrapper = GetWrapperRect(index);
    const int totalW = wrapper.right - wrapper.left;
    const int divX = wrapper.left + (totalW - Layout::FilesSection::FileListView::INPUT_DIVER_W) / 2;
    RECT rc{
        divX,
        wrapper.top,
        divX + Layout::FilesSection::FileListView::INPUT_DIVER_W,
        wrapper.bottom
    };
    return rc;
}

RECT FileListView::GetToRect(int index) const {
    RECT wrapper = GetWrapperRect(index);
    const int halfW = (wrapper.right - wrapper.left - Layout::FilesSection::FileListView::INPUT_DIVER_W) / 2;
    RECT rc = wrapper;
    rc.left = rc.left + halfW + Layout::FilesSection::FileListView::INPUT_DIVER_W;
    return rc;
}

std::wstring FileListView::ToWide(const std::string& s) const {
    return WidenSimple(s);
}

std::string FileListView::ToNarrow(const std::wstring& ws) const {
    if (ws.empty()) return "";

    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len > 0) {
        std::string s(len - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, s.data(), len, nullptr, nullptr);
        return s;
    }

    return std::string(ws.begin(), ws.end());
}

std::string FileListView::GetWindowTextUtf8(HWND hwnd) const {
    wchar_t buf[512]{};
    GetWindowTextW(hwnd, buf, 512);
    return ToNarrow(buf);
}

int FileListView::HitTestRow(int x, int y) const {
    if (m_files.empty()) return -1;

    RECT rcClient = GetClientRectSafe();
    if (y < 0 || y >= rcClient.bottom) return -1;

    const int step = GetRowStep();
    const int index = (y + m_scrollY) / step;

    if (index < 0 || index >= (int)m_files.size()) return -1;

    const RECT row = GetRowRect(index);
    if (!RectContains(row, x, y)) return -1;

    return index;
}

void FileListView::UpdateButtonsState() {
    const BOOL hasSelection = (m_selectedIndex >= 0 && m_selectedIndex < (int)m_files.size()) ? TRUE : FALSE;

    if (m_btnUp)     EnableWindow(m_btnUp, hasSelection);
    if (m_btnDown)   EnableWindow(m_btnDown, hasSelection);
    if (m_btnRemove) EnableWindow(m_btnRemove, hasSelection);
    if (m_btnAdd)    EnableWindow(m_btnAdd, TRUE);
}

void FileListView::UpdateScrollBar() {
    if (!m_hwnd) return;

    RECT rc = GetClientRectSafe();
    const int viewportH = std::max((long)0, rc.bottom);

    const int rowCount = (int)m_files.size();
    const int contentH = std::max(0,
        rowCount * Layout::FilesSection::FileListView::ROW_H +
        std::max(0, rowCount - 1) * Layout::FilesSection::FileListView::ROW_GAP
    );

    m_maxScrollY = std::max(0, contentH - viewportH);
    m_scrollY = ClampInt(m_scrollY, 0, m_maxScrollY);

    SCROLLINFO si{};
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = std::max(0, contentH - 1);
    si.nPage = (UINT)std::max(0, viewportH);
    si.nPos = m_scrollY;
    SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);
    SetScrollPos(m_hwnd, SB_VERT, m_scrollY, TRUE);
}

void FileListView::ScrollTo(int newScrollY) {
    const int old = m_scrollY;
    m_scrollY = ClampInt(newScrollY, 0, m_maxScrollY);

    if (m_scrollY != old) {
        SetScrollPos(m_hwnd, SB_VERT, m_scrollY, TRUE);
        UpdateSelectedEditLayout();
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }
}

void FileListView::EnsureSelectionVisible() {
    if (m_selectedIndex < 0 || m_selectedIndex >= (int)m_files.size()) return;

    RECT rc = GetClientRectSafe();
    const int top = 0;
    const int bottom = rc.bottom;

    const int rowTop = m_selectedIndex * GetRowStep() - m_scrollY;
    const int rowBottom = rowTop + Layout::FilesSection::FileListView::ROW_H;

    if (rowTop < top) {
        ScrollTo(m_selectedIndex * GetRowStep());
    } else if (rowBottom > bottom) {
        ScrollTo((m_selectedIndex + 1) * GetRowStep() - (bottom - top));
    }
}

void FileListView::CreateSelectedEdits() {
    DestroySelectedEdits();

    if (m_selectedIndex < 0 || m_selectedIndex >= (int)m_files.size() || !m_hwnd) {
        return;
    }

    const FileData& data = m_files[m_selectedIndex];
    RECT fromRc = GetFromRect(m_selectedIndex);
    RECT toRc = GetToRect(m_selectedIndex);

    const int padX = 2;
    const int padY = 1;

    m_editFrom = CreateWindowExW(
        0,
        L"EDIT",
        WidenSimple(data.fromRange).c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_CENTER,
        fromRc.left + padX,
        fromRc.top + padY,
        std::max((long)0, (fromRc.right - fromRc.left) - 2 * padX),
        std::max((long)0, (fromRc.bottom - fromRc.top) - 2 * padY),
        m_hwnd,
        (HMENU)ID_FLV_EDIT_FROM,
        GetModuleHandle(nullptr),
        nullptr
    );

    m_editTo = CreateWindowExW(
        0,
        L"EDIT",
        WidenSimple(data.toRange).c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_CENTER,
        toRc.left + padX,
        toRc.top + padY,
        std::max((long)0, (toRc.right - toRc.left) - 2 * padX),
        std::max((long)0, (toRc.bottom - toRc.top) - 2 * padY),
        m_hwnd,
        (HMENU)ID_FLV_EDIT_TO,
        GetModuleHandle(nullptr),
        nullptr
    );

    if (m_editFrom) {
        SendMessageW(m_editFrom, WM_SETFONT, (WPARAM)m_font, TRUE);
        LONG_PTR ex = GetWindowLongPtrW(m_editFrom, GWL_EXSTYLE);
        ex &= ~WS_EX_CLIENTEDGE;
        SetWindowLongPtrW(m_editFrom, GWL_EXSTYLE, ex);
        SetWindowPos(m_editFrom, nullptr, 0, 0, 0, 0,
            SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowSubclass(m_editFrom, EditSubclassProc, 1, (DWORD_PTR)this);
    }

    if (m_editTo) {
        SendMessageW(m_editTo, WM_SETFONT, (WPARAM)m_font, TRUE);
        LONG_PTR ex = GetWindowLongPtrW(m_editTo, GWL_EXSTYLE);
        ex &= ~WS_EX_CLIENTEDGE;
        SetWindowLongPtrW(m_editTo, GWL_EXSTYLE, ex);
        SetWindowPos(m_editTo, nullptr, 0, 0, 0, 0,
            SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowSubclass(m_editTo, EditSubclassProc, 1, (DWORD_PTR)this);
    }

    UpdateSelectedEditLayout();
    UpdateButtonsState();

    if (m_editFrom) {
        SetFocus(m_editFrom);
    }
}

void FileListView::DestroySelectedEdits() {
    if (m_editFrom) {
        RemoveWindowSubclass(m_editFrom, EditSubclassProc, 1);
        DestroyWindow(m_editFrom);
        m_editFrom = nullptr;
    }

    if (m_editTo) {
        RemoveWindowSubclass(m_editTo, EditSubclassProc, 1);
        DestroyWindow(m_editTo);
        m_editTo = nullptr;
    }

    m_editOriginalFrom.clear();
    m_editOriginalTo.clear();
}

void FileListView::CommitSelectedEditsIfNeeded(bool forceCallback) {
    if (m_selectedIndex < 0 || m_selectedIndex >= (int)m_files.size()) return;
    if (!m_editFrom || !m_editTo) return;

    const std::string newFrom = GetWindowTextUtf8(m_editFrom);
    const std::string newTo = GetWindowTextUtf8(m_editTo);

    const bool changed = forceCallback ||
                         (newFrom != m_editOriginalFrom) ||
                         (newTo != m_editOriginalTo);

    if (!changed) {
        return;
    }

    if (m_cbChangeRange) {
        m_cbChangeRange(m_files[m_selectedIndex].path, newFrom, newTo);
    }

    m_editOriginalFrom = newFrom;
    m_editOriginalTo = newTo;
}

void FileListView::UpdateSelectedEditLayout() {
    if (m_selectedIndex < 0 || m_selectedIndex >= (int)m_files.size()) return;
    if (!m_editFrom || !m_editTo) return;

    RECT fromRc = GetFromRect(m_selectedIndex);
    RECT toRc = GetToRect(m_selectedIndex);

    const int padX = 2;
    const int padY = 1;

    MoveWindow(
        m_editFrom,
        fromRc.left + padX,
        fromRc.top + padY,
        std::max((long)0, (fromRc.right - fromRc.left) - 2 * padX),
        std::max((long)0, (fromRc.bottom - fromRc.top) - 2 * padY),
        TRUE
    );

    MoveWindow(
        m_editTo,
        toRc.left + padX,
        toRc.top + padY,
        std::max((long)0, (toRc.right - toRc.left) - 2 * padX),
        std::max((long)0, (toRc.bottom - toRc.top) - 2 * padY),
        TRUE
    );
}

void FileListView::ClearSelection() {
    if (m_selectedIndex < 0) {
        UpdateButtonsState();
        return;
    }

    CommitSelectedEditsIfNeeded(false);
    DestroySelectedEdits();
    m_selectedIndex = -1;
    UpdateButtonsState();
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void FileListView::SelectRow(int index) {
    if (index < 0 || index >= (int)m_files.size()) {
        ClearSelection();
        return;
    }

    if (m_selectedIndex == index && m_editFrom && m_editTo) {
        SetFocus(m_editFrom);
        return;
    }

    CommitSelectedEditsIfNeeded(false);
    DestroySelectedEdits();

    m_selectedIndex = index;
    CreateSelectedEdits();
    EnsureSelectionVisible();
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void FileListView::UpdateLayout() {
    if (!m_hwnd) return;

    RECT rc = GetClientRectSafe();
    const int clientW = rc.right;
    const int clientH = rc.bottom;
    (void)clientH;

    // Sibling buttons in HomeWindow area
    const int btnY = Layout::FilesSection::CALC_CONTROL_UP_INPUT_Y(0) + 0; // placeholder, overwritten below
    const int btnW = std::max(0, (clientW - 2 * Layout::SECTION_MARGIN - 3 * Layout::INPUT_GAP) / 4);

    // Buttons are positioned in parent (HomeWindow) coordinates.
    const int parentW = clientW; // only used for layout formula
    const int parentH = 0;       // not needed here but kept for readability

    (void)parentW;
    (void)parentH;

    // if (m_btnUp) {
    //     MoveWindow(
    //         m_btnUp,
    //         Layout::FilesSection::CALC_CONTROL_UP_INPUT_X(GetClientRectSafe().right),
    //         Layout::FilesSection::CALC_CONTROL_UP_INPUT_Y(GetClientRectSafe().bottom),
    //         btnW,
    //         Layout::INPUT_H,
    //         TRUE
    //     );
    // }
    // if (m_btnDown) {
    //     MoveWindow(
    //         m_btnDown,
    //         Layout::FilesSection::CALC_CONTROL_DOWN_INPUT_X(GetClientRectSafe().right),
    //         Layout::FilesSection::CALC_CONTROL_DOWN_INPUT_Y(GetClientRectSafe().bottom),
    //         btnW,
    //         Layout::INPUT_H,
    //         TRUE
    //     );
    // }
    // if (m_btnRemove) {
    //     MoveWindow(
    //         m_btnRemove,
    //         Layout::FilesSection::CALC_CONTROL_REMOVE_INPUT_X(GetClientRectSafe().right),
    //         Layout::FilesSection::CALC_CONTROL_REMOVE_INPUT_Y(GetClientRectSafe().bottom),
    //         btnW,
    //         Layout::INPUT_H,
    //         TRUE
    //     );
    // }
    // if (m_btnAdd) {
    //     MoveWindow(
    //         m_btnAdd,
    //         Layout::FilesSection::CALC_CONTROL_ADD_INPUT_X(GetClientRectSafe().right),
    //         Layout::FilesSection::CALC_CONTROL_ADD_INPUT_Y(GetClientRectSafe().bottom),
    //         btnW,
    //         Layout::INPUT_H,
    //         TRUE
    //     );
    // }

    UpdateSelectedEditLayout();
    UpdateScrollBar();
    // UpdateButtonsState();
}

void FileListView::OnSizeInternal() {
    UpdateLayout();
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void FileListView::OnSetFocusInternal() {
    if (m_selectedIndex >= 0 && m_editFrom) {
        SetFocus(m_editFrom);
    }
}

void FileListView::OnLButtonDownInternal(int x, int y) {
    const int hit = HitTestRow(x, y);
    if (hit >= 0) {
        SelectRow(hit);
        return;
    }

    ClearSelection();
    SetFocus(m_hwnd);
}

void FileListView::OnMouseWheelInternal(short delta) {
    const int step = GetRowStep();
    const int lines = (delta > 0) ? -3 : 3;
    ScrollTo(m_scrollY + lines * (step / 2));
}

void FileListView::OnVScrollInternal(WPARAM wParam) {
    switch (LOWORD(wParam)) {
    case SB_LINEUP:
        ScrollTo(m_scrollY - GetRowStep() / 2);
        break;
    case SB_LINEDOWN:
        ScrollTo(m_scrollY + GetRowStep() / 2);
        break;
    case SB_PAGEUP:
        ScrollTo(m_scrollY - GetListViewportHeight());
        break;
    case SB_PAGEDOWN:
        ScrollTo(m_scrollY + GetListViewportHeight());
        break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
    {
        SCROLLINFO si{};
        si.cbSize = sizeof(si);
        si.fMask = SIF_TRACKPOS;
        GetScrollInfo(m_hwnd, SB_VERT, &si);
        ScrollTo((int)si.nTrackPos);
        break;
    }
    case SB_TOP:
        ScrollTo(0);
        break;
    case SB_BOTTOM:
        ScrollTo(m_maxScrollY);
        break;
    default:
        break;
    }
}

void FileListView::OnCommandInternal(WPARAM wParam, LPARAM lParam) {
    const int id = LOWORD(wParam);
    const int code = HIWORD(wParam);
    HWND hwndFrom = (HWND)lParam;

    if ((id == ID_FLV_EDIT_FROM || id == ID_FLV_EDIT_TO) && hwndFrom) {
        if (code == EN_KILLFOCUS) {
            CommitSelectedEditsIfNeeded(false);
            return;
        }

        // Don't mutate data on EN_CHANGE.
        return;
    }
}

void FileListView::DrawButton(HDC hdc, const DRAWITEMSTRUCT* dis, const wchar_t* text, COLORREF bg) {
    RECT rc = dis->rcItem;

    const bool disabled = (dis->itemState & ODS_DISABLED) != 0;
    const bool selected = (dis->itemState & ODS_SELECTED) != 0;
    const bool hot = (dis->itemState & ODS_HOTLIGHT) != 0;
    const bool focused = (dis->itemState & ODS_FOCUS) != 0;

    COLORREF fill = bg;
    if (disabled) {
        fill = Style::INPUT_BG_DISABLED;
    } else if (selected) {
        fill = BlendWithWhite(bg, 15);
    } else if (hot) {
        fill = BlendWithWhite(bg, 8);
    }

    COLORREF border = Style::INPUT_BORDER;
    if (disabled) border = Style::INPUT_BORDER_DISABLED;
    else if (focused) border = Style::INPUT_BORDER_FOCUS;

    FillRectSolid(hdc, rc, fill);
    DrawRoundedFrame(hdc, rc, border, Style::INPUT_RADIUS);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, disabled ? Style::INPUT_TEXT_DISABLED : Style::INPUT_TEXT);
    HFONT oldFont = (HFONT)SelectObject(hdc, m_font);

    RECT textRc = rc;
    DrawTextW(hdc, text, -1, &textRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    SelectObject(hdc, oldFont);
}

void FileListView::OnPaintInternal(HDC hdc) {
    RECT rcClient = GetClientRectSafe();
    FillRectSolid(hdc, rcClient, Style::SECTION_BG);

    SetBkMode(hdc, TRANSPARENT);
    HFONT oldFont = (HFONT)SelectObject(hdc, m_font);

    // Only rows + scrollbar area are inside this child window.
    const int viewportH = rcClient.bottom;

    if (!m_files.empty()) {
        const int step = GetRowStep();
        int startIndex = std::max(0, (m_scrollY - LOAD_OUTSIDE_PX) / step);
        int endIndex = std::min((int)m_files.size() - 1, (m_scrollY + viewportH + LOAD_OUTSIDE_PX) / step);

        for (int i = startIndex; i <= endIndex; ++i) {
            DrawRow(hdc, i, rcClient);
        }
    }

    SelectObject(hdc, oldFont);
}

void FileListView::DrawRow(HDC hdc, int index, const RECT& clipRc) {
    if (index < 0 || index >= (int)m_files.size()) return;

    RECT row = GetRowRect(index);
    if (row.bottom < clipRc.top || row.top > clipRc.bottom) return;

    const FileData& data = m_files[index];

    bool parsed = false;
    COLORREF status = ParseColorString(data.statusColor, &parsed);
    COLORREF bg = Style::FILE_LIST_VIEW_GREEN_BG;

    std::string lc = ToLowerCopy(data.statusColor);
    if (lc.find("red") != std::string::npos) {
        status = Style::FILE_LIST_VIEW_RED_STATUS;
        bg = Style::FILE_LIST_VIEW_RED_BG;
    } else if (lc.find("green") != std::string::npos) {
        status = Style::FILE_LIST_VIEW_GREEN_STATUS;
        bg = Style::FILE_LIST_VIEW_GREEN_BG;
    } else if (parsed) {
        bg = BlendWithWhite(status, 85);
    } else {
        status = Style::FILE_LIST_VIEW_GREEN_STATUS;
        bg = Style::FILE_LIST_VIEW_GREEN_BG;
    }

    // Main row background.
    FillRectSolid(hdc, row, bg);

    // status strip
    RECT statusRc = row;
    statusRc.right = statusRc.left + Layout::FilesSection::FileListView::STATUS_W;
    FillRectSolid(hdc, statusRc, status);

    // Row selected border.
    if (index == m_selectedIndex) {
        DrawRoundedFrame(hdc, row, Style::FILE_LIST_VIEW_SELECTED_ROW_BORDER, 0);
    }

    // Name label
    RECT labelRc = GetLabelRect(index);
    DrawLeftEllipsisText(hdc, labelRc, WidenSimple(data.name), Style::LABEL, m_font);

    // From/To wrapper
    RECT wrapperRc = GetWrapperRect(index);
    FillRectSolid(hdc, wrapperRc, Style::INPUT_BG);
    DrawRoundedFrame(hdc, wrapperRc, Style::INPUT_BORDER, Style::INPUT_RADIUS);

    RECT fromRc = GetFromRect(index);
    RECT toRc = GetToRect(index);
    RECT divRc = GetDividerRect(index);

    DrawCenteredText(hdc, fromRc, WidenSimple(data.fromRange), Style::INPUT_TEXT, m_font);
    DrawCenteredText(hdc, toRc, WidenSimple(data.toRange), Style::INPUT_TEXT, m_font);
    DrawCenteredText(hdc, divRc, L"-", Style::INPUT_TEXT, m_font);
}

void FileListView::OnPaintStatic(HDC hdc, int parentWidth, int parentHeight) {
    const int x = Layout::FilesSection::X;
    const int y = Layout::FilesSection::Y;
    const int w = Layout::FilesSection::CALC_W(parentWidth);
    const int h = Layout::FilesSection::CALC_H(parentHeight);

    RECT sec{ x, y, x + std::max(0, w), y + std::max(0, h) };

    HBRUSH bg = CreateSolidBrush(Style::SECTION_BG);
    FillRect(hdc, &sec, bg);
    DeleteObject(bg);

    HPEN pen = CreatePen(PS_SOLID, 1, Style::SECTION_BORDER);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, sec.left, sec.top, sec.right, sec.bottom);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, Style::LABEL);
    HFONT oldFont = (HFONT)SelectObject(hdc, m_font);

    RECT titleRc{
        x + Layout::FilesSection::LABEL_X,
        y + Layout::FilesSection::LABEL_Y,
        x + w - Layout::SECTION_MARGIN,
        y + Layout::FilesSection::LABEL_Y + Layout::LABEL_H
    };
    DrawTextW(hdc, L"Selected Files", -1, &titleRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    SelectObject(hdc, oldFont);
}

void FileListView::Set(std::vector<FileData> files) {
    if (!m_hwnd) {
        m_files = std::move(files);
        return;
    }

    ClearSelection();
    m_files = std::move(files);
    m_scrollY = 0;
    UpdateScrollBar();
    UpdateLayout();
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void FileListView::OnChangeRange(std::function<void(const std::string& path, const std::string& fromRange, const std::string& toRange)> cb) {
    m_cbChangeRange = std::move(cb);
}

void FileListView::OnMoveUp(std::function<void(const std::string& path)> cb) {
    m_cbMoveUp = std::move(cb);
}

void FileListView::OnMoveDown(std::function<void(const std::string& path)> cb) {
    m_cbMoveDown = std::move(cb);
}

void FileListView::OnRemove(std::function<void(const std::string& path)> cb) {
    m_cbRemove = std::move(cb);
}

void FileListView::OnAdd(std::function<void(const std::string& path)> cb) {
    m_cbAdd = std::move(cb);
}

void FileListView::HandleCommand(WPARAM wParam) {
    const int id = LOWORD(wParam);
    const int code = HIWORD(wParam);

    if (code != BN_CLICKED) {
        return;
    }

    if (m_selectedIndex < 0 || m_selectedIndex >= (int)m_files.size()) {
        if (id == ID_FLV_BTN_ADD && m_cbAdd) {
            m_cbAdd("");
        }
        return;
    }

    const std::string& path = m_files[m_selectedIndex].path;

    if (id == ID_FLV_BTN_UP && m_cbMoveUp) {
        m_cbMoveUp(path);
        return;
    }

    if (id == ID_FLV_BTN_DOWN && m_cbMoveDown) {
        m_cbMoveDown(path);
        return;
    }

    if (id == ID_FLV_BTN_REMOVE && m_cbRemove) {
        m_cbRemove(path);
        return;
    }

    if (id == ID_FLV_BTN_ADD && m_cbAdd) {
        m_cbAdd(path);
        return;
    }
}

void FileListView::HandleDrawItem(const DRAWITEMSTRUCT* dis) {
    if (!dis || dis->CtlType != ODT_BUTTON) return;

    switch (dis->CtlID) {
    case ID_FLV_BTN_UP:
        DrawButton(dis->hDC, dis, L"↑", Style::INPUT_BG);
        return;
    case ID_FLV_BTN_DOWN:
        DrawButton(dis->hDC, dis, L"↓", Style::INPUT_BG);
        return;
    case ID_FLV_BTN_REMOVE:
        DrawButton(dis->hDC, dis, L"-", Style::INPUT_BG);
        return;
    case ID_FLV_BTN_ADD:
        DrawButton(dis->hDC, dis, L"+", Style::INPUT_BG);
        return;
    default:
        return;
    }
}

bool FileListView::Create(HWND parent, HFONT font) {
    m_parent = parent;
    m_font = font;

    HINSTANCE hInst = GetModuleHandle(nullptr);

    // Register child window class once.
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    if (!GetClassInfoExW(hInst, FLV_CLASS_NAME, &wc)) {
        WNDCLASSEXW reg{};
        reg.cbSize = sizeof(reg);
        reg.style = CS_HREDRAW | CS_VREDRAW;
        reg.lpfnWndProc = WndProc;
        reg.hInstance = hInst;
        reg.hCursor = LoadCursor(nullptr, IDC_ARROW);
        reg.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        reg.lpszClassName = FLV_CLASS_NAME;

        if (!RegisterClassExW(&reg)) {
            return false;
        }
    }

    // List window only.
    m_hwnd = CreateWindowExW(
        0,
        FLV_CLASS_NAME,
        L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        Layout::FilesSection::FileListView::X,
        Layout::FilesSection::FileListView::Y,
        10, 10,
        parent,
        nullptr,
        hInst,
        this
    );

    if (!m_hwnd) {
        return false;
    }

    // Buttons are sibling children of HomeWindow, not inside scrolling list.
    m_btnUp = CreateWindowExW(
        0, L"BUTTON", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        0, 0, 10, 10,
        parent,
        (HMENU)ID_FLV_BTN_UP,
        hInst,
        nullptr
    );

    m_btnDown = CreateWindowExW(
        0, L"BUTTON", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        0, 0, 10, 10,
        parent,
        (HMENU)ID_FLV_BTN_DOWN,
        hInst,
        nullptr
    );

    m_btnRemove = CreateWindowExW(
        0, L"BUTTON", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        0, 0, 10, 10,
        parent,
        (HMENU)ID_FLV_BTN_REMOVE,
        hInst,
        nullptr
    );

    m_btnAdd = CreateWindowExW(
        0, L"BUTTON", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        0, 0, 10, 10,
        parent,
        (HMENU)ID_FLV_BTN_ADD,
        hInst,
        nullptr
    );

    if (m_btnUp)     SendMessageW(m_btnUp, WM_SETFONT, (WPARAM)m_font, TRUE);
    if (m_btnDown)   SendMessageW(m_btnDown, WM_SETFONT, (WPARAM)m_font, TRUE);
    if (m_btnRemove) SendMessageW(m_btnRemove, WM_SETFONT, (WPARAM)m_font, TRUE);
    if (m_btnAdd)    SendMessageW(m_btnAdd, WM_SETFONT, (WPARAM)m_font, TRUE);

    UpdateLayout();
    UpdateScrollBar();
    UpdateButtonsState();
    return true;
}

void FileListView::Resize(int parentWidth, int parentHeight) {
    if (!m_hwnd) return;

    // List child window.
    const int listX = Layout::FilesSection::FileListView::X;
    const int listY = Layout::FilesSection::FileListView::Y;
    const int listW = Layout::FilesSection::FileListView::CALC_W(parentWidth);
    const int listH = Layout::FilesSection::FileListView::CALC_H(parentHeight);

    MoveWindow(m_hwnd, listX, listY, std::max(0, listW), std::max(0, listH), TRUE);

    // Buttons inside FilesSection, but outside scrolling area.
    const int btnW = std::max(0, Layout::FilesSection::CALC_CONTROL_INPUT_W(parentWidth));
    const int btnY = Layout::FilesSection::CALC_CONTROL_UP_INPUT_Y(parentHeight);

    if (m_btnUp) {
        MoveWindow(
            m_btnUp,
            Layout::FilesSection::X + Layout::FilesSection::CALC_CONTROL_UP_INPUT_X(parentWidth),
            Layout::FilesSection::Y + btnY,
            btnW,
            Layout::INPUT_H,
            TRUE
        );
    }

    if (m_btnDown) {
        MoveWindow(
            m_btnDown,
            Layout::FilesSection::X + Layout::FilesSection::CALC_CONTROL_DOWN_INPUT_X(parentWidth),
            Layout::FilesSection::Y + btnY,
            btnW,
            Layout::INPUT_H,
            TRUE
        );
    }

    if (m_btnRemove) {
        MoveWindow(
            m_btnRemove,
            Layout::FilesSection::X + Layout::FilesSection::CALC_CONTROL_REMOVE_INPUT_X(parentWidth),
            Layout::FilesSection::Y + btnY,
            btnW,
            Layout::INPUT_H,
            TRUE
        );
    }

    if (m_btnAdd) {
        MoveWindow(
            m_btnAdd,
            Layout::FilesSection::X + Layout::FilesSection::CALC_CONTROL_ADD_INPUT_X(parentWidth),
            Layout::FilesSection::Y + btnY,
            btnW,
            Layout::INPUT_H,
            TRUE
        );
    }

    UpdateLayout();
    UpdateScrollBar();
}

LRESULT CALLBACK FileListView::EditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    auto* self = reinterpret_cast<FileListView*>(dwRefData);
    if (!self) {
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }

    if (msg == WM_KEYDOWN && wParam == VK_RETURN) {
        self->CommitSelectedEditsIfNeeded(true);
        return 0;
    }

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void FileListView::OnCreateInternal() {
    UpdateScrollBar();
    UpdateLayout();
}

void FileListView::OnDestroyInternal() {
    DestroySelectedEdits();

    if (m_btnUp) {
        DestroyWindow(m_btnUp);
        m_btnUp = nullptr;
    }
    if (m_btnDown) {
        DestroyWindow(m_btnDown);
        m_btnDown = nullptr;
    }
    if (m_btnRemove) {
        DestroyWindow(m_btnRemove);
        m_btnRemove = nullptr;
    }
    if (m_btnAdd) {
        DestroyWindow(m_btnAdd);
        m_btnAdd = nullptr;
    }
}

void ui::home::FileListView::OnCtlColorEditInternal(HDC hdc, HWND hwnd) {
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, Style::INPUT_TEXT);
    SetBkColor(hdc, Style::INPUT_BG);
    (void)hwnd;
}

void FileListView::ClearSelectionPublic() {
    ClearSelection();
}

bool FileListView::IsPointInside(int x, int y) const {
    if (!m_hwnd) return false;

    RECT rc;
    GetWindowRect(m_hwnd, &rc);

    POINT pt{ x, y };
    return PtInRect(&rc, pt);
}

LRESULT CALLBACK FileListView::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    FileListView* self = reinterpret_cast<FileListView*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_NCCREATE:
    {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = reinterpret_cast<FileListView*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)self);
        if (self) {
            self->m_hwnd = hwnd;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    case WM_CREATE:
        if (self) self->OnCreateInternal();
        return 0;

    case WM_DESTROY:
        if (self) self->OnDestroyInternal();
        return 0;

    case WM_SIZE:
        if (self) self->OnSizeInternal();
        return 0;

    case WM_PAINT:
    {
        if (!self) break;
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        self->OnPaintInternal(hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_COMMAND:
        if (self) {
            self->OnCommandInternal(wParam, lParam);
        }
        return 0;

    case WM_CTLCOLOREDIT:
        if (self) {
            self->OnCtlColorEditInternal((HDC)wParam, (HWND)lParam);
            static HBRUSH s_br = CreateSolidBrush(Style::INPUT_BG);
            return (LRESULT)s_br;
        }
        break;

    case WM_LBUTTONDOWN:
        if (self) {
            self->OnLButtonDownInternal(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        }
        break;

    case WM_MOUSEWHEEL:
        if (self) {
            self->OnMouseWheelInternal(GET_WHEEL_DELTA_WPARAM(wParam));
            return 0;
        }
        break;

    case WM_VSCROLL:
        if (self) {
            self->OnVScrollInternal(wParam);
            return 0;
        }
        break;

    case WM_SETFOCUS:
        if (self) {
            self->OnSetFocusInternal();
            return 0;
        }
        break;

    case WM_ERASEBKGND:
        return 1;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}