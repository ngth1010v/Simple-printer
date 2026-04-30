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
    // ===== Basic =====
    WriteString("Basic", "printer", data.printer, path);

    // ===== Advance =====
    WriteString("Advance", "printMode",   data.printMode,   path);
    WriteString("Advance", "paper",       data.paper,       path);
    WriteString("Advance", "scale",       data.scale,       path);
    WriteString("Advance", "orientation", data.orientation, path);
    WriteString("Advance", "collate",     data.collate,     path);

    // ===== Margin =====
    WriteInt("Margin", "top",    (int)data.margin[0], path);
    WriteInt("Margin", "right",  (int)data.margin[1], path);
    WriteInt("Margin", "bottom", (int)data.margin[2], path);
    WriteInt("Margin", "left",   (int)data.margin[3], path);

    // ===== Files =====
    if (!data.files.empty()) {
        WriteInt("Files", "count", (int)data.files.size(), path);

        for (size_t i = 0; i < data.files.size(); ++i) {
            char key[64];

            // path{i}
            wsprintfA(key, "path%zu", i);
            WriteString("Files", key, data.files[i].path, path);

            // fromRange{i}
            wsprintfA(key, "fromRange%zu", i);
            WriteInt("Files", key, data.files[i].fromRange, path);

            // toRange{i}
            wsprintfA(key, "toRange%zu", i);
            WriteInt("Files", key, data.files[i].toRange, path);
        }
    }
    else {
        // nếu muốn sạch file cũ (optional):
        WritePrivateProfileStringA("Files", NULL, NULL, path.c_str());
    }
}

}