#include "platform/FilePicker.h"

#include <windows.h>
#include <shobjidl.h>  // IFileOpenDialog
#include <vector>
#include <string>

namespace platform {

//////////////////////////////////////////////////////
// UTF16 -> UTF8
//////////////////////////////////////////////////////
static std::string WStringToUtf8(const std::wstring& w)
{
    if (w.empty()) return {};

    int size = WideCharToMultiByte(
        CP_UTF8, 0,
        w.data(), (int)w.size(),
        nullptr, 0,
        nullptr, nullptr
    );

    std::string result(size, 0);

    WideCharToMultiByte(
        CP_UTF8, 0,
        w.data(), (int)w.size(),
        result.data(), size,
        nullptr, nullptr
    );

    return result;
}

//////////////////////////////////////////////////////
// Main
//////////////////////////////////////////////////////
std::vector<std::string> OpenFilePicker()
{
    std::vector<std::string> results;

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) return results;

    IFileOpenDialog* pDialog = nullptr;

    hr = CoCreateInstance(
        CLSID_FileOpenDialog,
        nullptr,
        CLSCTX_ALL,
        IID_PPV_ARGS(&pDialog)
    );

    if (FAILED(hr)) {
        CoUninitialize();
        return results;
    }

    // Options
    DWORD options;
    pDialog->GetOptions(&options);
    pDialog->SetOptions(options | FOS_ALLOWMULTISELECT | FOS_FILEMUSTEXIST);

    // Filter
    const COMDLG_FILTERSPEC filters[] = {
        { L"All Supported", L"*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.docx;*.pdf" },
        { L"Images",        L"*.png;*.jpg;*.jpeg;*.bmp;*.gif" },
        { L"Documents",     L"*.docx;*.pdf" }
    };

    pDialog->SetFileTypes(3, filters);
    pDialog->SetFileTypeIndex(1);

    // Show dialog
    hr = pDialog->Show(nullptr);

    if (SUCCEEDED(hr)) {
        IShellItemArray* pItems = nullptr;

        hr = pDialog->GetResults(&pItems);
        if (SUCCEEDED(hr)) {
            DWORD count = 0;
            pItems->GetCount(&count);

            for (DWORD i = 0; i < count; ++i) {
                IShellItem* pItem = nullptr;

                if (SUCCEEDED(pItems->GetItemAt(i, &pItem))) {
                    PWSTR path = nullptr;

                    if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
                        std::wstring wpath(path);
                        results.push_back(WStringToUtf8(wpath));
                        CoTaskMemFree(path);
                    }

                    pItem->Release();
                }
            }

            pItems->Release();
        }
    }

    pDialog->Release();
    CoUninitialize();

    return results;
}

}