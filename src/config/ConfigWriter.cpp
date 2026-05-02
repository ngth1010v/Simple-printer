#include "config/ConfigWriter.h"
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
    WritePrivateProfileStringA("Basic",     NULL, NULL, path.c_str());
    WritePrivateProfileStringA("Advance",   NULL, NULL, path.c_str());
    WritePrivateProfileStringA("Margin",    NULL, NULL, path.c_str());
    WritePrivateProfileStringA("Files",     NULL, NULL, path.c_str());

    
    // ===== Basic =====
    WriteString("Basic", "printer", data.printer, path);

    // ===== Advance =====
    WriteString("Advance", "printMode",   data.printMode,   path);
    WriteString("Advance", "paper",       data.paper,       path);
    WriteString("Advance", "scale",       data.scale,       path);
    WriteString("Advance", "orientation", data.orientation, path);
    WriteString("Advance", "collate",     data.collate,     path);


    // ===== Files =====
    if (!data.files.empty()) {
        WriteInt("Files", "count", (int)data.files.size(), path);

        for (int i = 0; i < (int)data.files.size(); ++i) {
            char key[64];

            wsprintfA(key, "path%d", i);
            WriteString("Files", key, data.files[i].path, path);

            wsprintfA(key, "fromRange%d", i);
            WriteInt("Files", key, data.files[i].fromRange, path);

            wsprintfA(key, "toRange%d", i);
            WriteInt("Files", key, data.files[i].toRange, path);
        }
    }
    else {
        // nếu muốn sạch file cũ (optional):
        WritePrivateProfileStringA("Files", NULL, NULL, path.c_str());
    }
}

}