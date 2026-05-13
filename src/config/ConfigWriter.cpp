#include "config/ConfigWriter.h"

#include <windows.h>

namespace config {

static std::string GetConfigPath()
{
    char buffer[MAX_PATH];

    GetModuleFileNameA(nullptr, buffer, MAX_PATH);

    std::string path = buffer;

    size_t pos = path.find_last_of("\\/");
    if (pos != std::string::npos) {
        path = path.substr(0, pos + 1);
    }

    path += "config.ini";

    return path;
}

static std::string WStringToUtf8(const std::wstring& str)
{
    if (str.empty()) {
        return "";
    }

    int size = WideCharToMultiByte(
        CP_UTF8,
        0,
        str.c_str(),
        -1,
        nullptr,
        0,
        nullptr,
        nullptr
    );

    if (size <= 0) {
        return "";
    }

    std::string result(size - 1, '\0');

    WideCharToMultiByte(
        CP_UTF8,
        0,
        str.c_str(),
        -1,
        &result[0],
        size,
        nullptr,
        nullptr
    );

    return result;
}

static void WriteString(
    const char* section,
    const char* key,
    const std::string& value,
    const std::string& path)
{
    WritePrivateProfileStringA(
        section,
        key,
        value.c_str(),
        path.c_str()
    );
}

static void WriteInt(
    const char* section,
    const char* key,
    int value,
    const std::string& path)
{
    char buffer[64];

    wsprintfA(buffer, "%d", value);

    WritePrivateProfileStringA(
        section,
        key,
        buffer,
        path.c_str()
    );
}

void Write(const ConfigData& data)
{
    std::string path = GetConfigPath();

    // đảm bảo file tồn tại
    {
        HANDLE hFile = CreateFileA(
            path.c_str(),
            GENERIC_WRITE,
            FILE_SHARE_READ,
            nullptr,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );

        if (hFile != INVALID_HANDLE_VALUE) {
            CloseHandle(hFile);
        }
    }

    WritePrivateProfileStringA(
        "Basic",
        NULL,
        NULL,
        path.c_str()
    );

    WritePrivateProfileStringA(
        "Advance",
        NULL,
        NULL,
        path.c_str()
    );

    WritePrivateProfileStringA(
        "Margin",
        NULL,
        NULL,
        path.c_str()
    );

    WritePrivateProfileStringA(
        "Files",
        NULL,
        NULL,
        path.c_str()
    );

    // ===== Basic =====
    WriteString(
        "Basic",
        "printer",
        data.printer,
        path
    );

    WriteInt(
        "Basic",
        "copies",
        data.copies,
        path
    );

    // ===== Advance =====
    WriteString(
        "Advance",
        "printMode",
        data.printMode,
        path
    );

    WriteString(
        "Advance",
        "paper",
        data.paper,
        path
    );

    WriteString(
        "Advance",
        "scale",
        data.scale,
        path
    );

    WriteString(
        "Advance",
        "orientation",
        data.orientation,
        path
    );

    WriteString(
        "Advance",
        "skipBlankPage",
        data.skipBlankPage,
        path
    );

    // ===== Files =====
    if (!data.files.empty()) {
        WriteInt(
            "Files",
            "count",
            (int)data.files.size(),
            path
        );

        for (int i = 0; i < (int)data.files.size(); ++i) {
            char key[64];

            wsprintfA(key, "path%d", i);

            WriteString(
                "Files",
                key,
                WStringToUtf8(data.files[i].path),
                path
            );

            wsprintfA(key, "fromRange%d", i);

            WriteInt(
                "Files",
                key,
                data.files[i].fromRange,
                path
            );

            wsprintfA(key, "toRange%d", i);

            WriteInt(
                "Files",
                key,
                data.files[i].toRange,
                path
            );
        }
    }
    else {
        WritePrivateProfileStringA(
            "Files",
            NULL,
            NULL,
            path.c_str()
        );
    }

    WritePrivateProfileStringA(
        nullptr,
        nullptr,
        nullptr,
        path.c_str()
    );
}

}