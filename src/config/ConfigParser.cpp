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
    data.margin[0] = ReadInt("Margin", "top",    data.margin[0], path);
    data.margin[1] = ReadInt("Margin", "right",  data.margin[1], path);
    data.margin[2] = ReadInt("Margin", "bottom", data.margin[2], path);
    data.margin[3] = ReadInt("Margin", "left",   data.margin[3], path);

    return data;
}

}