#include "app/CommandLine.h"

#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace CommandLine {

    // -------------------- storage --------------------

    static bool g_isPrint = false;
    static std::vector<std::wstring> g_files;

    // -------------------- helpers --------------------

    static bool IsValidExtension(const fs::path& path) {

        std::wstring ext = path.extension().wstring();

        std::transform(
            ext.begin(),
            ext.end(),
            ext.begin(),
            ::towlower
        );

        return
            ext == L".png"  ||
            ext == L".jpg"  ||
            ext == L".jpeg" ||
            ext == L".bmp"  ||
            ext == L".gif"  ||
            ext == L".webp" ||
            ext == L".tiff" ||
            ext == L".pdf"  ||
            ext == L".docx";
    }

    static bool IsValidFile(const std::wstring& filePath) {

        fs::path path(filePath);

        if (!fs::exists(path))
            return false;

        if (!fs::is_regular_file(path))
            return false;

        return IsValidExtension(path);
    }

    // -------------------- parse --------------------

    void Parse(int argc, wchar_t* argv[]) {

        g_isPrint = false;
        g_files.clear();

        for (int i = 1; i < argc; ++i) {

            std::wstring arg = argv[i] ? argv[i] : L"";

            // --print
            if (arg == L"--print") {
                g_isPrint = true;
                continue;
            }

            // file
            if (IsValidFile(arg)) {
                g_files.push_back(fs::absolute(arg).wstring());
            }
        }
    }

    // -------------------- query --------------------

    bool IsPrint() {
        return g_isPrint;
    }

    const std::vector<std::wstring>& GetFiles() {
        return g_files;
    }

}