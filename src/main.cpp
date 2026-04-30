#include <windows.h>
#include <commctrl.h>
#include "app/CommandLine.h"
#include "app/App.h"

int main(int argc, char** argv) {
    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);

    // parse CLI
    CommandLine::Parse(argc, argv);

    // run app
    return App::Run();
}
