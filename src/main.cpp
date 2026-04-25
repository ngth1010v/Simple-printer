#include "CommandLine.h"
#include "App.h"

int main(int argc, char** argv) {
    // parse CLI
    CommandLine::Parse(argc, argv);

    // run app
    return App::Run();
}