#include "app/CommandLine.h"
#include "app/App.h"

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