#pragma once

#include "config/ConfigModel.h"
#include "ui/home/FileListView.h"

#include <windows.h>
#include <functional>
#include <string>
#include <vector>

class HomeWindow;

namespace controller {
namespace home {

class FileListViewController {
public:
    explicit FileListViewController(HomeWindow& win, config::ConfigData& cfg);
    ~FileListViewController();

    void Init(std::function<void()> onChanged = {});
    void Bind();

private:
    struct CountResultMessage {
        std::wstring path;
        int pages = 0;
        std::string error;
    };

private:
    HomeWindow& m_win;
    config::ConfigData& m_cfg;

    std::function<void()> m_onChanged;

    std::vector<ui::home::UiFileData> m_files;

    HWND m_notifyHwnd = nullptr;
    bool m_subclassInstalled = false;

private:
    static constexpr UINT WM_FILELIST_COUNT_DONE = WM_APP + 0x431;
    static constexpr UINT_PTR SUBCLASS_ID = 0x46494C31; // "FIL1"
    static constexpr UINT WM_FILELIST_NOTIFY_CHANGE = WM_APP + 0x432;

private:
    static LRESULT CALLBACK SubclassProc(
        HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam,
        UINT_PTR uIdSubclass,
        DWORD_PTR dwRefData
    );

    void InstallSubclass();
    void UninstallSubclass();

    void HandleCountDone(CountResultMessage* msg);

    void HandleChangeRange(const std::wstring& path, const std::string& fromRange, const std::string& toRange);
    void HandleMoveUp(const std::wstring& path);
    void HandleMoveDown(const std::wstring& path);
    void HandleRemove(const std::wstring& path);
    void HandleAdd(const std::wstring& pathFromUi);

    void StartCountForPath(const std::wstring& path);
    void RequestCountForItem(int index);

    void NotifyHomeChanged();

    int FindIndexByPath(const std::wstring& path) const;

    static bool EqualPathNoCase(const std::wstring& a, const std::wstring& b);
    static std::wstring ToLowerCopy(std::wstring s);
    static std::string TrimCopy(const std::string& s);
    static bool ParseIntStrict(const std::string& s, int& out);

    static std::wstring BaseNameFromPath(const std::wstring& path);
    static std::string StatusColor(bool loaded, int pages);

    static ui::home::UiFileData BuildUiFromCfg(const config::FileData& f);
    static void UpdateUiFromCfg(ui::home::UiFileData& ui, const config::FileData& f);
    static void UpdateCfgFromUiStringRange(config::FileData& f, const std::string& fromRange, const std::string& toRange);

    std::wstring ToWide(const std::string& s) const;
    std::string ToNarrow(const std::wstring& ws) const;

    void RefreshUi();
};

} // namespace home
} // namespace controller