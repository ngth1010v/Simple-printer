#include "config/ConfigParser.h"

#include <windows.h>

namespace config {

static std::string GetConfigPath()
{
    return "config.ini";
}

static bool FileExists(const std::string& path)
{
    DWORD attr = GetFileAttributesA(path.c_str());

    return (
        attr != INVALID_FILE_ATTRIBUTES &&
        !(attr & FILE_ATTRIBUTE_DIRECTORY)
    );
}

static std::wstring Utf8ToWString(const std::string& str)
{
    if (str.empty()) {
        return L"";
    }

    int size = MultiByteToWideChar(
        CP_UTF8,
        0,
        str.c_str(),
        -1,
        nullptr,
        0
    );

    if (size <= 0) {
        return L"";
    }

    std::wstring result(size - 1, L'\0');

    MultiByteToWideChar(
        CP_UTF8,
        0,
        str.c_str(),
        -1,
        &result[0],
        size
    );

    return result;
}

static std::string ReadString(
    const char* section,
    const char* key,
    const std::string& def,
    const std::string& path)
{
    char buffer[512] = {};

    GetPrivateProfileStringA(
        section,
        key,
        def.c_str(),
        buffer,
        sizeof(buffer),
        path.c_str()
    );

    return std::string(buffer);
}

static int ReadInt(
    const char* section,
    const char* key,
    int def,
    const std::string& path)
{
    return GetPrivateProfileIntA(
        section,
        key,
        def,
        path.c_str()
    );
}

ConfigData Parse(bool keepFiles)
{
    ConfigData data;

    std::string configPath = GetConfigPath();

    if (!FileExists(configPath)) {
        return data;
    }

    // ===== Basic =====
    data.printer = ReadString(
        "Basic",
        "printer",
        data.printer,
        configPath
    );

    data.copies = ReadInt(
        "Basic",
        "copies",
        data.copies,
        configPath
    );

    // ===== Advance =====
    data.printMode = ReadString(
        "Advance",
        "printMode",
        data.printMode,
        configPath
    );

    data.paper = ReadString(
        "Advance",
        "paper",
        data.paper,
        configPath
    );

    data.scale = ReadString(
        "Advance",
        "scale",
        data.scale,
        configPath
    );

    data.orientation = ReadString(
        "Advance",
        "orientation",
        data.orientation,
        configPath
    );

    data.skipBlankPage = ReadString(
        "Advance",
        "skipBlankPage",
        data.skipBlankPage,
        configPath
    );

    // ===== Files =====
    if (keepFiles) {
        int count = ReadInt(
            "Files",
            "count",
            0,
            configPath
        );

        data.files.clear();

        for (int i = 0; i < count; ++i) {
            char key[64];

            // path{i}
            wsprintfA(key, "path%d", i);

            std::string filePathUtf8 = ReadString(
                "Files",
                key,
                "",
                configPath
            );

            if (filePathUtf8.empty()) {
                continue;
            }

            // fromRange{i}
            wsprintfA(key, "fromRange%d", i);

            int from = ReadInt(
                "Files",
                key,
                0,
                configPath
            );

            // toRange{i}
            wsprintfA(key, "toRange%d", i);

            int to = ReadInt(
                "Files",
                key,
                0,
                configPath
            );

            FileData f;
            f.path = Utf8ToWString(filePathUtf8);
            f.fromRange = from;
            f.toRange = to;

            data.files.push_back(f);
        }
    }

    return data;
}

}