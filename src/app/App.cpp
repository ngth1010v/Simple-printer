#include "app/App.h"
#include "app/CommandLine.h"
#include "config/ConfigParser.h"
#include "platform/WinUtils.h"

#include "controller/HomeController.h"
#include "controller/PrintController.h"

#include <windows.h> 
#include <string>
#include <iostream>




int App::Run()
{
    bool isPrint = CommandLine::IsPrint();
    const auto& files = CommandLine::GetFiles();

    // Config
    auto cfg = config::Parse(files.size() == 0);
    if (files.size() > 0){
        cfg.copies = 1;
        for (const auto& f : files) {

            config::FileData fd;
            fd.path      = f;
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


