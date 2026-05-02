#include "app/App.h"
#include "app/CommandLine.h"
#include "config/ConfigParser.h"
#include "platform/WinUtils.h"

#include "controller/HomeController.h"
#include "controller/PrintController.h"

#include <windows.h> 
#include <string>

// helper: bỏ quote nếu có
static std::string StripQuotes(const std::string& s)
{
    if (s.size() >= 2) {
        if ((s.front() == '"' && s.back() == '"') ||
            (s.front() == '\'' && s.back() == '\'')) {
            return s.substr(1, s.size() - 2);
        }
    }
    return s;
}

// helper: check tồn tại file (WinAPI)
static bool FileExists(const std::string& path)
{
    DWORD attr = GetFileAttributesA(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES &&
            !(attr & FILE_ATTRIBUTE_DIRECTORY));
}



int App::Run()
{
    bool isPrint = CommandLine::HasFlag("print");
    const auto& files = CommandLine::GetArgs();

    // Config
    auto cfg = config::Parse("./config.ini", files.size() == 0);
    {
        for (const auto& f : files)
        {
            std::string path = StripQuotes(f);

            if (!FileExists(path)) {
                continue;
            }

            config::FileData fd;
            fd.path = path;
            fd.fromRange = 0;
            fd.toRange   = 0;
            fd.pages     = 0;
            fd.loaded    = false;

            cfg.files.push_back(fd);
        }
    }



    // ===== UI MODE =====
    if (!isPrint) {
        controller::HomeController ctrl(cfg);
        return ctrl.Run();
    }

    // ===== PRINT MODE =====
    auto printers = platform::GetPrinters();

    bool printerMatch = false;
    for (const auto& p : printers) {
        if (p == cfg.printer) {
            printerMatch = true;
            break;
        }
    }

    if (printerMatch) {
        controller::PrintController ctrl(cfg);
        return ctrl.Run();
    }

    // ===== NOT MATCH =====
    int result = MessageBoxA(
        nullptr,
        "The printer in config was not found.\nDo you want to open the UI to select another printer?",
        "Printer not found",
        MB_YESNO | MB_ICONQUESTION
    );

    if (result == IDYES) {
        controller::HomeController ctrl(cfg);
        return ctrl.Run();
    }

    return 0;
}