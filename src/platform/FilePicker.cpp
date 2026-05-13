// platform/FilePicker.cpp
#include "platform/FilePicker.h"

#include <windows.h>
#include <shobjidl.h>

#include <vector>
#include <string>

namespace platform {

std::vector<std::wstring> OpenFilePicker()
{
    std::vector<std::wstring> results;

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    if (FAILED(hr)) {
        return results;
    }

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

    DWORD options = 0;

    pDialog->GetOptions(&options);

    pDialog->SetOptions(
        options |
        FOS_ALLOWMULTISELECT |
        FOS_FILEMUSTEXIST
    );

    const COMDLG_FILTERSPEC filters[] = {
        { L"All Supported", L"*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.docx;*.pdf" },
        { L"Images",        L"*.png;*.jpg;*.jpeg;*.bmp;*.gif" },
        { L"Documents",     L"*.docx;*.pdf" }
    };

    pDialog->SetFileTypes(3, filters);
    pDialog->SetFileTypeIndex(1);

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
                        results.emplace_back(path);
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