#include "controller/home/FileListView.h"

#include "ui/HomeWindow.h"
#include "platform/FilePicker.h"
#include "counter/Counter.h"

#include <CommCtrl.h>
#include <algorithm>
#include <cctype>
#include <cwctype>
#include <memory>
#include <utility>

namespace controller {
namespace home {

FileListViewController::FileListViewController(HomeWindow& win, config::ConfigData& cfg)
    : m_win(win), m_cfg(cfg) {
}

FileListViewController::~FileListViewController()
{
    UninstallSubclass();
}

void FileListViewController::Init(std::function<void()> onChanged)
{
    m_onChanged = std::move(onChanged);

    m_notifyHwnd = m_win.GetHwnd();
    InstallSubclass();

    m_files.clear();
    m_files.reserve(m_cfg.files.size());

    for (const auto& f : m_cfg.files) {
        m_files.push_back(BuildUiFromCfg(f));
    }

    m_win.m_files.Set(m_files);

    for (int i = 0; i < static_cast<int>(m_cfg.files.size()); ++i) {
        if (!m_cfg.files[i].loaded) {
            RequestCountForItem(i);
        }
    }
}

void FileListViewController::Bind()
{
    m_win.m_files.OnChangeRange([this](const std::wstring& path, const std::string& fromRange, const std::string& toRange) {
        HandleChangeRange(path, fromRange, toRange);
    });

    m_win.m_files.OnMoveUp([this](const std::wstring& path) {
        HandleMoveUp(path);
    });

    m_win.m_files.OnMoveDown([this](const std::wstring& path) {
        HandleMoveDown(path);
    });

    m_win.m_files.OnRemove([this](const std::wstring& path) {
        HandleRemove(path);
    });

    m_win.m_files.OnAdd([this](const std::wstring& path) {
        HandleAdd(path);
    });
}

void FileListViewController::InstallSubclass()
{
    if (m_subclassInstalled || !m_notifyHwnd) {
        return;
    }

    if (::SetWindowSubclass(m_notifyHwnd, &FileListViewController::SubclassProc, SUBCLASS_ID,
                            reinterpret_cast<DWORD_PTR>(this))) {
        m_subclassInstalled = true;
    }
}

void FileListViewController::UninstallSubclass()
{
    if (!m_subclassInstalled || !m_notifyHwnd) {
        return;
    }

    ::RemoveWindowSubclass(m_notifyHwnd, &FileListViewController::SubclassProc, SUBCLASS_ID);
    m_subclassInstalled = false;
}

LRESULT CALLBACK FileListViewController::SubclassProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam,
    UINT_PTR uIdSubclass,
    DWORD_PTR dwRefData)
{
    auto* self = reinterpret_cast<FileListViewController*>(dwRefData);
    if (!self) {
        return ::DefSubclassProc(hwnd, msg, wParam, lParam);
    }

    if (msg == WM_FILELIST_COUNT_DONE) {
        auto* payload = reinterpret_cast<CountResultMessage*>(lParam);
        self->HandleCountDone(payload);
        return 0;
    }

    if (msg == WM_NCDESTROY) {
        self->m_subclassInstalled = false;
        ::RemoveWindowSubclass(hwnd, &FileListViewController::SubclassProc, uIdSubclass);
        return ::DefSubclassProc(hwnd, msg, wParam, lParam);
    }

    return ::DefSubclassProc(hwnd, msg, wParam, lParam);
}

void FileListViewController::HandleCountDone(CountResultMessage* msg)
{
    std::unique_ptr<CountResultMessage> guard(msg);
    if (!msg) {
        return;
    }

    const int index = FindIndexByPath(msg->path);
    if (index < 0 || index >= static_cast<int>(m_cfg.files.size()) || index >= static_cast<int>(m_files.size())) {
        return;
    }

    auto& cfgFile = m_cfg.files[index];
    auto& uiFile = m_files[index];

    cfgFile.loaded = true;

    if (msg->pages > 0) {
        cfgFile.pages = msg->pages;
        cfgFile.fromRange = 1;
        cfgFile.toRange = msg->pages;
    } else {
        cfgFile.pages = 0;
        cfgFile.fromRange = 0;
        cfgFile.toRange = 0;
    }

    UpdateUiFromCfg(uiFile, cfgFile);

    m_win.m_files.Set(m_files);
    NotifyHomeChanged();
}

void FileListViewController::HandleChangeRange(
    const std::wstring& path,
    const std::string& fromRange,
    const std::string& toRange)
{
    const int index = FindIndexByPath(path);
    if (index < 0 || index >= (int)m_cfg.files.size()) {
        return;
    }

    auto& cfgFile = m_cfg.files[index];
    auto& uiFile  = m_files[index];

    int fromVal = 0;
    int toVal   = 0;

    const bool okFrom = ParseIntStrict(fromRange, fromVal);
    const bool okTo   = ParseIntStrict(toRange, toVal);

    bool valid = true;

    if (!okFrom || !okTo) {
        valid = false;
    }

    if (valid) {
        if (cfgFile.pages == 0) {
            if (!(fromVal == 0 && toVal == 0)) {
                valid = false;
            }
        } else {
            if (fromVal <= 0) valid = false;
            if (toVal <= 0) valid = false;
            if (fromVal > toVal) valid = false;
            if (toVal > cfgFile.pages) valid = false;
        }
    }

    if (valid) {
        cfgFile.fromRange = fromVal;
        cfgFile.toRange   = toVal;

        uiFile.fromRange = std::to_string(fromVal);
        uiFile.toRange   = std::to_string(toVal);

        m_win.m_files.Set(m_files);
        NotifyHomeChanged();
    } else {
        UpdateUiFromCfg(uiFile, cfgFile);
        m_win.m_files.Set(m_files);
    }
}

void FileListViewController::HandleMoveUp(const std::wstring& path)
{
    const int index = FindIndexByPath(path);
    if (index <= 0 || index >= static_cast<int>(m_files.size()) || index >= static_cast<int>(m_cfg.files.size())) {
        return;
    }

    std::swap(m_files[index], m_files[index - 1]);
    std::swap(m_cfg.files[index], m_cfg.files[index - 1]);

    m_win.m_files.Set(m_files);
    NotifyHomeChanged();
}

void FileListViewController::HandleMoveDown(const std::wstring& path)
{
    const int index = FindIndexByPath(path);
    if (index < 0 || index + 1 >= static_cast<int>(m_files.size()) || index + 1 >= static_cast<int>(m_cfg.files.size())) {
        return;
    }

    std::swap(m_files[index], m_files[index + 1]);
    std::swap(m_cfg.files[index], m_cfg.files[index + 1]);

    m_win.m_files.Set(m_files);
    NotifyHomeChanged();
}

void FileListViewController::HandleRemove(const std::wstring& path)
{
    const int index = FindIndexByPath(path);
    if (index < 0 || index >= static_cast<int>(m_files.size()) || index >= static_cast<int>(m_cfg.files.size())) {
        return;
    }

    m_files.erase(m_files.begin() + index);
    m_cfg.files.erase(m_cfg.files.begin() + index);

    m_win.m_files.Set(m_files);
    NotifyHomeChanged();
}

void FileListViewController::HandleAdd(const std::wstring& pathFromUi)
{
    std::vector<std::wstring> paths;

    if (!pathFromUi.empty()) {
        paths.push_back(pathFromUi);
    } else {
        const auto picked = platform::OpenFilePicker();
        paths.reserve(picked.size());
        for (const auto& p : picked) {
            paths.push_back(p);
        }
    }

    if (paths.empty()) {
        return;
    }

    bool addedAny = false;

    for (const auto& path : paths) {
        if (path.empty()) {
            continue;
        }

        if (FindIndexByPath(path) >= 0) {
            continue;
        }

        config::FileData cfgFile{};
        cfgFile.path = path;
        cfgFile.pages = 0;
        cfgFile.fromRange = 1;
        cfgFile.toRange = 0;
        cfgFile.loaded = false;

        m_cfg.files.push_back(cfgFile);
        m_files.push_back(BuildUiFromCfg(cfgFile));
        addedAny = true;
    }

    if (!addedAny) {
        return;
    }

    m_win.m_files.Set(m_files);
    NotifyHomeChanged();

    for (int i = 0; i < static_cast<int>(m_cfg.files.size()); ++i) {
        if (!m_cfg.files[i].loaded) {
            RequestCountForItem(i);
        }
    }
}

void FileListViewController::StartCountForPath(const std::wstring& path)
{

    counter::Count(path, [this, path](int pages, const std::string& error) {
        auto* payload = new CountResultMessage{};
        payload->path = path;
        payload->pages = pages;
        payload->error = error;

        if (!m_notifyHwnd || !::PostMessage(m_notifyHwnd, WM_FILELIST_COUNT_DONE, 0, reinterpret_cast<LPARAM>(payload))) {
            delete payload;
        }
    });
}

void FileListViewController::RequestCountForItem(int index)
{
    if (index < 0 || index >= static_cast<int>(m_cfg.files.size()) || index >= static_cast<int>(m_files.size())) {
        return;
    }

    if (m_cfg.files[index].loaded) {
        return;
    }

    StartCountForPath(m_cfg.files[index].path);
}

void FileListViewController::NotifyHomeChanged()
{
    if (!m_onChanged) return;

    if (GetCurrentThreadId() == GetWindowThreadProcessId(m_notifyHwnd, nullptr)) {
        m_onChanged();
    } else {
        auto cb = new std::function<void()>(m_onChanged);
        PostMessage(m_notifyHwnd, WM_APP + 999, 0, (LPARAM)cb);
    }
}

int FileListViewController::FindIndexByPath(const std::wstring& path) const
{
    for (int i = 0; i < static_cast<int>(m_files.size()); ++i) {
        if (EqualPathNoCase(m_files[i].path, path)) {
            return i;
        }
    }
    return -1;
}

bool FileListViewController::EqualPathNoCase(const std::wstring& a, const std::wstring& b)
{
    return ToLowerCopy(a) == ToLowerCopy(b);
}

std::wstring FileListViewController::ToLowerCopy(std::wstring s)
{
    for (wchar_t& c : s) {
        c = static_cast<wchar_t>(std::towlower(c));
    }
    return s;
}

std::string FileListViewController::TrimCopy(const std::string& s)
{
    size_t begin = 0;
    size_t end = s.size();

    while (begin < end && std::isspace(static_cast<unsigned char>(s[begin]))) {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }

    return s.substr(begin, end - begin);
}

bool FileListViewController::ParseIntStrict(const std::string& s, int& out)
{
    try {
        const std::string trimmed = TrimCopy(s);
        if (trimmed.empty()) {
            return false;
        }

        size_t idx = 0;
        const int value = std::stoi(trimmed, &idx, 10);
        if (idx != trimmed.size()) {
            return false;
        }

        out = value;
        return true;
    } catch (...) {
        return false;
    }
}

std::wstring FileListViewController::BaseNameFromPath(const std::wstring& path)
{
    const size_t slash = path.find_last_of(L"/\\");
    if (slash == std::wstring::npos) {
        return path;
    }
    return path.substr(slash + 1);
}

std::string FileListViewController::StatusColor(bool loaded, int pages)
{
    if (loaded && pages > 0) {
        return "green";
    }
    if (!loaded && pages == 0) {
        return "yellow";
    }
    if (loaded && pages == 0) {
        return "red";
    }
    return "yellow";
}

ui::home::UiFileData FileListViewController::BuildUiFromCfg(const config::FileData& f)
{
    ui::home::UiFileData ui;

    ui.path = f.path;

    {
        const std::wstring ws = BaseNameFromPath(f.path);

        int size = ::WideCharToMultiByte(
            CP_UTF8,
            0,
            ws.data(),
            static_cast<int>(ws.size()),
            nullptr,
            0,
            nullptr,
            nullptr
        );

        if (size > 0) {
            ui.name.resize(size);

            ::WideCharToMultiByte(
                CP_UTF8,
                0,
                ws.data(),
                static_cast<int>(ws.size()),
                ui.name.data(),
                size,
                nullptr,
                nullptr
            );
        }
    }

    ui.statusColor = StatusColor(f.loaded, f.pages);

    if (f.loaded && f.pages > 0) {
        const int fromValue = (f.fromRange > 0) ? f.fromRange : 1;
        const int toValue = (f.toRange > 0) ? f.toRange : f.pages;

        ui.fromRange = std::to_string(fromValue);
        ui.toRange = std::to_string(toValue);
    } else if (!f.loaded && f.pages == 0) {
        ui.fromRange = "1";
        ui.toRange = "0";
    } else if (f.pages > 0) {
        ui.fromRange = "1";
        ui.toRange = std::to_string(f.pages);
    } else {
        ui.fromRange = "0";
        ui.toRange = "0";
    }

    return ui;
}

void FileListViewController::UpdateUiFromCfg(ui::home::UiFileData& ui, const config::FileData& f)
{
    ui.path = f.path;

    {
        const std::wstring ws = BaseNameFromPath(f.path);

        int size = ::WideCharToMultiByte(
            CP_UTF8,
            0,
            ws.data(),
            static_cast<int>(ws.size()),
            nullptr,
            0,
            nullptr,
            nullptr
        );

        if (size > 0) {
            ui.name.resize(size);

            ::WideCharToMultiByte(
                CP_UTF8,
                0,
                ws.data(),
                static_cast<int>(ws.size()),
                ui.name.data(),
                size,
                nullptr,
                nullptr
            );
        } else {
            ui.name.clear();
        }
    }

    ui.statusColor = StatusColor(f.loaded, f.pages);

    if (f.loaded && f.pages > 0) {
        const int fromValue = (f.fromRange > 0) ? f.fromRange : 1;
        const int toValue = (f.toRange > 0) ? f.toRange : f.pages;

        ui.fromRange = std::to_string(fromValue);
        ui.toRange = std::to_string(toValue);
    } else if (!f.loaded && f.pages == 0) {
        ui.fromRange = "1";
        ui.toRange = "0";
    } else if (f.pages > 0) {
        ui.fromRange = "1";
        ui.toRange = std::to_string(f.pages);
    } else {
        ui.fromRange = "0";
        ui.toRange = "0";
    }
}

void FileListViewController::UpdateCfgFromUiStringRange(config::FileData& f, const std::string& fromRange, const std::string& toRange)
{
    int fromValue = 0;
    int toValue = 0;

    if (ParseIntStrict(fromRange, fromValue)) {
        f.fromRange = fromValue;
    }

    if (ParseIntStrict(toRange, toValue)) {
        f.toRange = toValue;
    }
}

std::wstring FileListViewController::ToWide(const std::string& s) const
{
    if (s.empty()) {
        return {};
    }

    int size = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (size <= 0) {
        size = ::MultiByteToWideChar(CP_ACP, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
        if (size <= 0) {
            return {};
        }

        std::wstring out(size, L'\0');
        ::MultiByteToWideChar(CP_ACP, 0, s.data(), static_cast<int>(s.size()), out.data(), size);
        return out;
    }

    std::wstring out(size, L'\0');
    ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.size()), out.data(), size);
    return out;
}

std::string FileListViewController::ToNarrow(const std::wstring& ws) const
{
    if (ws.empty()) {
        return {};
    }

    int size = ::WideCharToMultiByte(CP_UTF8, 0, ws.data(), static_cast<int>(ws.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
        return {};
    }

    std::string out(size, '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, ws.data(), static_cast<int>(ws.size()), out.data(), size, nullptr, nullptr);
    return out;
}

void FileListViewController::RefreshUi()
{
    m_win.m_files.Set(m_files);
}

} // namespace home
} // namespace controller