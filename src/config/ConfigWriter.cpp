#include "ConfigWriter.h"
#include <windows.h>

namespace config {

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

void Write(const std::string& path, const ConfigData& data)
{
    // ===== Basic =====
    WriteString("Basic", "printer", data.printer, path);

    // ===== Advance =====
    WriteString("Advance", "printMode",   data.printMode,   path);
    WriteString("Advance", "paper",       data.paper,       path);
    WriteString("Advance", "scale",       data.scale,       path);
    WriteString("Advance", "orientation", data.orientation, path);
    WriteString("Advance", "collate",     data.collate,     path);

    // ===== Margin =====
    WriteInt("Margin", "top",    data.margin[0], path);
    WriteInt("Margin", "right",  data.margin[1], path);
    WriteInt("Margin", "bottom", data.margin[2], path);
    WriteInt("Margin", "left",   data.margin[3], path);
}

}