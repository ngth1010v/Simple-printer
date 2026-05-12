// #include <windows.h>
// #include <commctrl.h>
// #include "app/CommandLine.h"
// #include "app/App.h"

// int main(int argc, char** argv) {
//     INITCOMMONCONTROLSEX icc = {};
//     icc.dwSize = sizeof(icc);
//     icc.dwICC = ICC_WIN95_CLASSES;
//     InitCommonControlsEx(&icc);

//     // parse CLI
//     CommandLine::Parse(argc, argv);

//     // run app
//     return App::Run();
// }


#include <windows.h>
#include <commctrl.h>

#include <string>
#include <sstream>


int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    if (!argv) {
        return 1;
    }

    CommandLine::Parse(argc, argv);

    int result = App::Run();

    LocalFree(argv);

    return result;
}