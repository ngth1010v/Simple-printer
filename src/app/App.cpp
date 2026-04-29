#include "app/App.h"
#include "app/CommandLine.h"
#include "config/ConfigParser.h"
#include "platform/WinPrinter.h"

#include "controller/HomeController.h"
#include "controller/PrintController.h"

#include <windows.h> 

int App::Run()
{
    bool isPrint = CommandLine::HasFlag("print");
    const auto& files = CommandLine::GetArgs();

    auto cfg = config::Parse("./config.ini");

    // ===== UI MODE =====
    if (!isPrint) {
        controller::HomeController ctrl(cfg);
        return ctrl.Run();
    }

    // ===== PRINT MODE =====
    auto printers = platform::printer::GetPrinters();

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