#include "config/ConfigParser.h"
#include <windows.h>

namespace config {

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

ConfigData Parse(const std::string& path)
{
    ConfigData data;

    DWORD attr = GetFileAttributesA(path.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        return data;
    }

    // ===== Basic =====
    data.printer = ReadString("Basic", "printer", data.printer, path);

    // ===== Advance =====
    data.printMode   = ReadString("Advance", "printMode",   data.printMode,   path);
    data.paper       = ReadString("Advance", "paper",       data.paper,       path);
    data.scale       = ReadString("Advance", "scale",       data.scale,       path);
    data.orientation = ReadString("Advance", "orientation", data.orientation, path);
    data.collate     = ReadString("Advance", "collate",     data.collate,     path);

    // ===== Margin =====
    data.margin[0] = (float)ReadInt("Margin", "top",    (int)data.margin[0], path);
    data.margin[1] = (float)ReadInt("Margin", "right",  (int)data.margin[1], path);
    data.margin[2] = (float)ReadInt("Margin", "bottom", (int)data.margin[2], path);
    data.margin[3] = (float)ReadInt("Margin", "left",   (int)data.margin[3], path);

    // ===== Files =====
    int count = ReadInt("Files", "count", 0, path);
    data.files.clear();

    for (int i = 0; i < count; ++i) {
        char key[64];

        // path{i}
        wsprintfA(key, "path%d", i);
        std::string filePath = ReadString("Files", key, "", path);

        if (filePath.empty()) {
            continue; // skip invalid
        }

        // fromRange{i}
        wsprintfA(key, "fromRange%d", i);
        int from = ReadInt("Files", key, 0, path);

        // toRange{i}
        wsprintfA(key, "toRange%d", i);
        int to = ReadInt("Files", key, 0, path);

        FileData f;
        f.path = filePath;
        f.fromRange = from;
        f.toRange = to;

        data.files.push_back(f);
    }

    return data;
}

}