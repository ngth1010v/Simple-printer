#include "ui/home/AdvanceConfigSection.h"
#include "ui/home/HomeComponentMap.h"

#include <commctrl.h>

using namespace ui::home;

#define ID_BTN_PRINT_MODE     4101
#define ID_BTN_PAPER          4102
#define ID_BTN_SCALE          4103
#define ID_BTN_ORIENTATION    4104

#define ID_MENU_PRINT_MODE    5101
#define ID_MENU_PAPER         5201
#define ID_MENU_SCALE         5301
#define ID_MENU_ORIENTATION   5401

namespace {

COLORREF InputTextColor(BOOL disabled) {
    return disabled ? Style::INPUT_TEXT_DISABLED : Style::INPUT_TEXT;
}

COLORREF InputBgColor(BOOL disabled, BOOL hot, BOOL focused) {
    if (disabled) return Style::INPUT_BG_DISABLED;
    if (focused)  return Style::INPUT_BG_FOCUS;
    if (hot)      return Style::INPUT_BG_HOVER;
    return Style::INPUT_BG;
}

COLORREF InputBorderColor(BOOL disabled, BOOL hot, BOOL focused) {
    if (disabled) return Style::INPUT_BORDER_DISABLED;
    if (focused)  return Style::INPUT_BORDER_FOCUS;
    if (hot)      return Style::INPUT_BORDER_HOVER;
    return Style::INPUT_BORDER;
}

void DrawRoundedFrame(
    HDC hdc,
    const RECT& rc,
    COLORREF borderColor,
    int radius
) {
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    HPEN pen = CreatePen(PS_SOLID, 1, borderColor);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);

    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, radius, radius);

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

RECT GetControlRectInParent(HWND parent, HWND child) {
    RECT rc{};
    GetWindowRect(child, &rc);
    MapWindowPoints(nullptr, parent, reinterpret_cast<POINT*>(&rc), 2);
    return rc;
}

void DrawInputChrome(
    HDC hdc,
    HWND parent,
    HWND hwnd,
    bool focused
) {
    RECT rc = GetControlRectInParent(parent, hwnd);

    rc.left   -= Layout::INPUT_BORDER_OFFSET;
    rc.top    -= Layout::INPUT_BORDER_OFFSET;
    rc.right  += Layout::INPUT_BORDER_OFFSET;
    rc.bottom += Layout::INPUT_BORDER_OFFSET;

    const BOOL disabled = !IsWindowEnabled(hwnd);
    const BOOL hot = FALSE;
    const BOOL isFocused = focused ? TRUE : FALSE;

    DrawRoundedFrame(
        hdc,
        rc,
        InputBorderColor(disabled, hot, isFocused),
        Style::INPUT_RADIUS
    );
}

void DrawArrow(HDC hdc, const RECT& rc, COLORREF color) {
    int midX = rc.right - 12;
    int midY = (rc.top + rc.bottom) / 2 - 1;

    POINT tri[3] = {
        { midX - 4, midY - 1 },
        { midX + 4, midY - 1 },
        { midX,     midY + 3 }
    };

    HBRUSH arrowBrush = CreateSolidBrush(color);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, arrowBrush);
    HPEN oldPen = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
    Polygon(hdc, tri, 3);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(arrowBrush);
}

}

AdvanceConfigSection::AdvanceConfigSection() {}

std::wstring AdvanceConfigSection::ToWide(const std::string& s) const {
    if (s.empty()) return L"";

    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (len <= 0) {
        return std::wstring(s.begin(), s.end());
    }

    std::wstring ws(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, ws.data(), len);
    return ws;
}

std::string AdvanceConfigSection::ToNarrow(const std::wstring& ws) const {
    if (ws.empty()) return "";

    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) {
        return std::string(ws.begin(), ws.end());
    }

    std::string s(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, s.data(), len, nullptr, nullptr);
    return s;
}

void AdvanceConfigSection::Create(HWND parent, HFONT font) {
    m_parent = parent;
    m_font = font;

    using namespace Layout::AdvaceSection;

    auto createBtn = [&](int id, int x, int y) -> HWND {
        return CreateWindowEx(
            0,
            L"BUTTON",
            L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
            x + Layout::INPUT_BORDER_OFFSET,
            y + Layout::INPUT_BORDER_OFFSET,
            INPUT_W - 2 * Layout::INPUT_BORDER_OFFSET,
            Layout::INPUT_H - 2 * Layout::INPUT_BORDER_OFFSET,
            parent,
            (HMENU)id,
            GetModuleHandle(nullptr),
            nullptr
        );
    };

    m_btnPrintMode   = createBtn(ID_BTN_PRINT_MODE,   X + PRINT_MODE_INPUT_X,   Y + PRINT_MODE_INPUT_Y);
    m_btnPaper       = createBtn(ID_BTN_PAPER,        X + PAPER_INPUT_X,        Y + PAPER_INPUT_Y);
    m_btnScale       = createBtn(ID_BTN_SCALE,        X + SCALE_INPUT_X,        Y + SCALE_INPUT_Y);
    m_btnOrientation = createBtn(ID_BTN_ORIENTATION,  X + ORIENTATION_INPUT_X,  Y + ORIENTATION_INPUT_Y);

    HWND ctrls[] = {
        m_btnPrintMode,
        m_btnPaper,
        m_btnScale,
        m_btnOrientation,
    };

    for (HWND h : ctrls) {
        SendMessage(h, WM_SETFONT, (WPARAM)m_font, TRUE);
    }
}

void AdvanceConfigSection::Resize(int parentWidth) {
    (void)parentWidth;

    using namespace Layout::AdvaceSection;

    auto moveBtn = [&](HWND hwnd, int x, int y) {
        MoveWindow(
            hwnd,
            x + Layout::INPUT_BORDER_OFFSET,
            y + Layout::INPUT_BORDER_OFFSET,
            INPUT_W - 2 * Layout::INPUT_BORDER_OFFSET,
            Layout::INPUT_H - 2 * Layout::INPUT_BORDER_OFFSET,
            TRUE
        );
    };

    moveBtn(m_btnPrintMode,   X + PRINT_MODE_INPUT_X,  Y + PRINT_MODE_INPUT_Y);
    moveBtn(m_btnPaper,       X + PAPER_INPUT_X,       Y + PAPER_INPUT_Y);
    moveBtn(m_btnScale,       X + SCALE_INPUT_X,       Y + SCALE_INPUT_Y);
    moveBtn(m_btnOrientation, X + ORIENTATION_INPUT_X, Y + ORIENTATION_INPUT_Y);
}

static void SetOptionsImpl(
    std::vector<std::wstring>& dst,
    int& selection,
    const std::vector<std::string>& options,
    const std::wstring& currentValue = L""
) {
    dst.clear();
    dst.reserve(options.size());

    for (const auto& s : options) {
        std::wstring ws;
        if (!s.empty()) {
            int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
            if (len > 0) {
                ws.resize(len - 1);
                MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, ws.data(), len);
            } else {
                ws.assign(s.begin(), s.end());
            }
        }
        dst.push_back(ws);
    }

    selection = dst.empty() ? -1 : 0;

    if (!currentValue.empty()) {
        for (int i = 0; i < (int)dst.size(); ++i) {
            if (dst[i] == currentValue) {
                selection = i;
                break;
            }
        }
    }
}

void AdvanceConfigSection::SetPrintModeOptions(const std::vector<std::string>& options) {
    SetOptionsImpl(m_printModeOptions, m_selPrintMode, options);
    if (m_btnPrintMode) InvalidateRect(m_btnPrintMode, nullptr, TRUE);
}

void AdvanceConfigSection::SetPrintModeValue(const std::string& value) {
    std::wstring wv = ToWide(value);
    for (int i = 0; i < (int)m_printModeOptions.size(); ++i) {
        if (m_printModeOptions[i] == wv) {
            m_selPrintMode = i;
            if (m_btnPrintMode) InvalidateRect(m_btnPrintMode, nullptr, TRUE);
            return;
        }
    }
}

void AdvanceConfigSection::OnPrintModeChange(std::function<void(const std::string&)> cb) {
    m_cbPrintMode = cb;
}

void AdvanceConfigSection::SetPaperOptions(const std::vector<std::string>& options) {
    SetOptionsImpl(m_paperOptions, m_selPaper, options);
    if (m_btnPaper) InvalidateRect(m_btnPaper, nullptr, TRUE);
}

void AdvanceConfigSection::SetPaperValue(const std::string& value) {
    std::wstring wv = ToWide(value);
    for (int i = 0; i < (int)m_paperOptions.size(); ++i) {
        if (m_paperOptions[i] == wv) {
            m_selPaper = i;
            if (m_btnPaper) InvalidateRect(m_btnPaper, nullptr, TRUE);
            return;
        }
    }
}

void AdvanceConfigSection::OnPaperChange(std::function<void(const std::string&)> cb) {
    m_cbPaper = cb;
}

void AdvanceConfigSection::SetScaleOptions(const std::vector<std::string>& options) {
    SetOptionsImpl(m_scaleOptions, m_selScale, options);
    if (m_btnScale) InvalidateRect(m_btnScale, nullptr, TRUE);
}

void AdvanceConfigSection::SetScaleValue(const std::string& value) {
    std::wstring wv = ToWide(value);
    for (int i = 0; i < (int)m_scaleOptions.size(); ++i) {
        if (m_scaleOptions[i] == wv) {
            m_selScale = i;
            if (m_btnScale) InvalidateRect(m_btnScale, nullptr, TRUE);
            return;
        }
    }
}

void AdvanceConfigSection::OnScaleChange(std::function<void(const std::string&)> cb) {
    m_cbScale = cb;
}

void AdvanceConfigSection::SetOrientationOptions(const std::vector<std::string>& options) {
    SetOptionsImpl(m_orientationOptions, m_selOrientation, options);
    if (m_btnOrientation) InvalidateRect(m_btnOrientation, nullptr, TRUE);
}

void AdvanceConfigSection::SetOrientationValue(const std::string& value) {
    std::wstring wv = ToWide(value);
    for (int i = 0; i < (int)m_orientationOptions.size(); ++i) {
        if (m_orientationOptions[i] == wv) {
            m_selOrientation = i;
            if (m_btnOrientation) InvalidateRect(m_btnOrientation, nullptr, TRUE);
            return;
        }
    }
}

void AdvanceConfigSection::OnOrientationChange(std::function<void(const std::string&)> cb) {
    m_cbOrientation = cb;
}

void AdvanceConfigSection::SetPrintModeImage(const std::string& path) {
    if (m_printModeBmp) {
        DeleteObject(m_printModeBmp);
        m_printModeBmp = nullptr;
    }

    if (path.empty()) {
        if (m_parent) InvalidateRect(m_parent, nullptr, TRUE);
        return;
    }

    m_printModeBmp = (HBITMAP)LoadImageA(
        nullptr,
        path.c_str(),
        IMAGE_BITMAP,
        0,
        0,
        LR_LOADFROMFILE
    );

    if (m_parent) InvalidateRect(m_parent, nullptr, TRUE);
}

void AdvanceConfigSection::HandleCommand(WPARAM wParam) {
    int id = LOWORD(wParam);
    int code = HIWORD(wParam);

    if (code != BN_CLICKED) {
        return;
    }

    if (id == ID_BTN_PRINT_MODE) {
        if (m_printModeOptions.empty()) return;

        HMENU menu = CreatePopupMenu();
        if (!menu) return;

        for (size_t i = 0; i < m_printModeOptions.size(); ++i) {
            AppendMenuW(menu, MF_STRING, ID_MENU_PRINT_MODE + (UINT)i, m_printModeOptions[i].c_str());
        }

        RECT rc{};
        GetWindowRect(m_btnPrintMode, &rc);

        int cmd = TrackPopupMenu(
            menu,
            TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY,
            rc.left,
            rc.bottom,
            0,
            m_parent,
            nullptr
        );

        DestroyMenu(menu);

        if (cmd >= ID_MENU_PRINT_MODE &&
            cmd < ID_MENU_PRINT_MODE + (int)m_printModeOptions.size()) {
            m_selPrintMode = cmd - ID_MENU_PRINT_MODE;
            InvalidateRect(m_btnPrintMode, nullptr, TRUE);
            if (m_cbPrintMode) {
                m_cbPrintMode(ToNarrow(m_printModeOptions[m_selPrintMode]));
            }
        }
        return;
    }

    if (id == ID_BTN_PAPER) {
        if (m_paperOptions.empty()) return;

        HMENU menu = CreatePopupMenu();
        if (!menu) return;

        for (size_t i = 0; i < m_paperOptions.size(); ++i) {
            AppendMenuW(menu, MF_STRING, ID_MENU_PAPER + (UINT)i, m_paperOptions[i].c_str());
        }

        RECT rc{};
        GetWindowRect(m_btnPaper, &rc);

        int cmd = TrackPopupMenu(
            menu,
            TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY,
            rc.left,
            rc.bottom,
            0,
            m_parent,
            nullptr
        );

        DestroyMenu(menu);

        if (cmd >= ID_MENU_PAPER &&
            cmd < ID_MENU_PAPER + (int)m_paperOptions.size()) {
            m_selPaper = cmd - ID_MENU_PAPER;
            InvalidateRect(m_btnPaper, nullptr, TRUE);
            if (m_cbPaper) {
                m_cbPaper(ToNarrow(m_paperOptions[m_selPaper]));
            }
        }
        return;
    }

    if (id == ID_BTN_SCALE) {
        if (m_scaleOptions.empty()) return;

        HMENU menu = CreatePopupMenu();
        if (!menu) return;

        for (size_t i = 0; i < m_scaleOptions.size(); ++i) {
            AppendMenuW(menu, MF_STRING, ID_MENU_SCALE + (UINT)i, m_scaleOptions[i].c_str());
        }

        RECT rc{};
        GetWindowRect(m_btnScale, &rc);

        int cmd = TrackPopupMenu(
            menu,
            TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY,
            rc.left,
            rc.bottom,
            0,
            m_parent,
            nullptr
        );

        DestroyMenu(menu);

        if (cmd >= ID_MENU_SCALE &&
            cmd < ID_MENU_SCALE + (int)m_scaleOptions.size()) {
            m_selScale = cmd - ID_MENU_SCALE;
            InvalidateRect(m_btnScale, nullptr, TRUE);
            if (m_cbScale) {
                m_cbScale(ToNarrow(m_scaleOptions[m_selScale]));
            }
        }
        return;
    }

    if (id == ID_BTN_ORIENTATION) {
        if (m_orientationOptions.empty()) return;

        HMENU menu = CreatePopupMenu();
        if (!menu) return;

        for (size_t i = 0; i < m_orientationOptions.size(); ++i) {
            AppendMenuW(menu, MF_STRING, ID_MENU_ORIENTATION + (UINT)i, m_orientationOptions[i].c_str());
        }

        RECT rc{};
        GetWindowRect(m_btnOrientation, &rc);

        int cmd = TrackPopupMenu(
            menu,
            TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY,
            rc.left,
            rc.bottom,
            0,
            m_parent,
            nullptr
        );

        DestroyMenu(menu);

        if (cmd >= ID_MENU_ORIENTATION &&
            cmd < ID_MENU_ORIENTATION + (int)m_orientationOptions.size()) {
            m_selOrientation = cmd - ID_MENU_ORIENTATION;
            InvalidateRect(m_btnOrientation, nullptr, TRUE);
            if (m_cbOrientation) {
                m_cbOrientation(ToNarrow(m_orientationOptions[m_selOrientation]));
            }
        }
        return;
    }

}

void AdvanceConfigSection::HandleDrawItem(const DRAWITEMSTRUCT* dis) {
    if (!dis) return;
    if (dis->CtlType != ODT_BUTTON) return;

    HDC hdc = dis->hDC;
    HWND btn = dis->hwndItem;
    RECT rc = dis->rcItem;

    const bool pressed  = (dis->itemState & ODS_SELECTED) != 0;
    const bool disabled = (dis->itemState & ODS_DISABLED) != 0;
    const bool focused  = (dis->itemState & ODS_FOCUS) != 0;
    const bool hot      = (dis->itemState & ODS_HOTLIGHT) != 0;

    COLORREF bg = InputBgColor(disabled, hot, pressed);
    COLORREF textColor = InputTextColor(disabled);

    HBRUSH brush = CreateSolidBrush(bg);
    FillRect(hdc, &rc, brush);
    DeleteObject(brush);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, textColor);

    HFONT oldFont = (HFONT)SelectObject(hdc, m_font);

    std::wstring text;
    if (btn == m_btnPrintMode) {
        if (m_selPrintMode >= 0 && m_selPrintMode < (int)m_printModeOptions.size()) text = m_printModeOptions[m_selPrintMode];
    } else if (btn == m_btnPaper) {
        if (m_selPaper >= 0 && m_selPaper < (int)m_paperOptions.size()) text = m_paperOptions[m_selPaper];
    } else if (btn == m_btnScale) {
        if (m_selScale >= 0 && m_selScale < (int)m_scaleOptions.size()) text = m_scaleOptions[m_selScale];
    } else if (btn == m_btnOrientation) {
        if (m_selOrientation >= 0 && m_selOrientation < (int)m_orientationOptions.size()) text = m_orientationOptions[m_selOrientation];
    }

    RECT textRc = rc;
    textRc.left += 8;
    textRc.right -= 24;

    DrawTextW(
        hdc,
        text.c_str(),
        -1,
        &textRc,
        DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS
    );

    DrawArrow(hdc, rc, textColor);

    SelectObject(hdc, oldFont);
}

void AdvanceConfigSection::OnPaint(HDC hdc) {
    using namespace Layout::AdvaceSection;

    RECT sec = { X, Y, X + W, Y + H };

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

    auto drawLabel = [&](LPCWSTR text, int x, int y) {
        RECT r = {
            X + x,
            Y + y,
            X + W,
            Y + y + Layout::LABEL_H
        };
        DrawTextW(hdc, text, -1, &r, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    };

    drawLabel(L"Print Mode",  PRINT_MODE_LABEL_X,  PRINT_MODE_LABEL_Y);
    drawLabel(L"Paper",       PAPER_LABEL_X,       PAPER_LABEL_Y);
    drawLabel(L"Scale",       SCALE_LABEL_X,       SCALE_LABEL_Y);
    drawLabel(L"Orientation", ORIENTATION_LABEL_X, ORIENTATION_LABEL_Y);

    DrawInputChrome(hdc, m_parent, m_btnPrintMode,   (GetFocus() == m_btnPrintMode));
    DrawInputChrome(hdc, m_parent, m_btnPaper,       (GetFocus() == m_btnPaper));
    DrawInputChrome(hdc, m_parent, m_btnScale,       (GetFocus() == m_btnScale));
    DrawInputChrome(hdc, m_parent, m_btnOrientation, (GetFocus() == m_btnOrientation));

    if (m_printModeBmp) {
        HDC mem = CreateCompatibleDC(hdc);
        HBITMAP oldBmp = (HBITMAP)SelectObject(mem, m_printModeBmp);

        BITMAP bm{};
        GetObject(m_printModeBmp, sizeof(bm), &bm);

        RECT imgRc = {
            X + PRINT_MODE_IMAGE_X,
            Y + PRINT_MODE_IMAGE_Y,
            X + W - PRINT_MODE_IMAGE_X,
            Y + PRINT_MODE_IMAGE_Y + Layout::IMAGE_H
        };

        StretchBlt(
            hdc,
            imgRc.left,
            imgRc.top,
            imgRc.right - imgRc.left,
            imgRc.bottom - imgRc.top,
            mem,
            0, 0,
            bm.bmWidth,
            bm.bmHeight,
            SRCCOPY
        );

        // ===== draw image border =====
        RECT borderRc = {
            X + PRINT_MODE_IMAGE_X,
            Y + PRINT_MODE_IMAGE_Y,
            X + W - PRINT_MODE_IMAGE_X,
            Y + PRINT_MODE_IMAGE_Y + Layout::IMAGE_H
        };

        DrawRoundedFrame(
            hdc,
            borderRc,
            Style::INPUT_BORDER,
            Style::INPUT_RADIUS
        );

        SelectObject(mem, oldBmp);
        DeleteDC(mem);
    }

    SelectObject(hdc, oldFont);
}