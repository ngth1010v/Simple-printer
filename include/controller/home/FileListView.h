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

    // Optional callback: HomeController can pass a no-arg lambda here.
    // Current existing code can still call Init() with no args.
    void Init(std::function<void()> onChanged = {});
    void Bind();

private:
    struct CountResultMessage {
        std::string path;
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

    void HandleChangeRange(const std::string& path, const std::string& fromRange, const std::string& toRange);
    void HandleMoveUp(const std::string& path);
    void HandleMoveDown(const std::string& path);
    void HandleRemove(const std::string& path);
    void HandleAdd(const std::string& pathFromUi);

    void StartCountForPath(const std::string& path);
    void RequestCountForItem(int index);

    void NotifyHomeChanged();

    int FindIndexByPath(const std::string& path) const;

    static bool EqualPathNoCase(const std::string& a, const std::string& b);
    static std::string ToLowerCopy(std::string s);
    static std::string TrimCopy(const std::string& s);
    static bool ParseIntStrict(const std::string& s, int& out);

    static std::string BaseNameFromPath(const std::string& path);
    static std::string StatusColor(bool loaded, int pages);

    static ui::home::UiFileData BuildUiFromCfg(const config::FileData& f);
    static void UpdateUiFromCfg(ui::home::UiFileData& ui, const config::FileData& f);
    static void UpdateCfgFromUiStringRange(config::FileData& f, const std::string& fromRange, const std::string& toRange);

    void RefreshUi();
};

} // namespace home
} // namespace controller