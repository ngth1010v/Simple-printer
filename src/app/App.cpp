#include "App.h"
#include "CommandLine.h"

#include <iostream>

int App::Run() {
    // ----- flags -----
    bool isPrint = CommandLine::HasFlag("print");
    bool isReview = CommandLine::HasFlag("review");

    // ----- options -----
    std::string printer = CommandLine::GetOption("printer", "default");
    std::string range   = CommandLine::GetOption("range", "all");

    // ----- args -----
    const auto& files = CommandLine::GetArgs();

    // ----- debug output -----
    std::cout << "=== COMMAND PARSER TEST ===\n";

    std::cout << "Mode: ";
    if (isPrint) std::cout << "PRINT\n";
    else if (isReview) std::cout << "REVIEW\n";
    else std::cout << "DEFAULT (REVIEW)\n";

    std::cout << "Printer: " << printer << "\n";
    std::cout << "Range: " << range << "\n";

    std::cout << "Files:\n";
    if (files.empty()) {
        std::cout << "  (none)\n";
    } else {
        for (const auto& f : files) {
            std::cout << "  - " << f << "\n";
        }
    }

    // ----- simulated flow -----
    if (files.empty()) {
        std::cout << "\nNo files provided. Exit.\n";
        return 0;
    }

    if (isPrint) {
        std::cout << "\n[Simulate] Direct printing...\n";
    } else {
        std::cout << "\n[Simulate] Open review window...\n";
    }

    return 0;
}