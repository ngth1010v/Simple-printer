#pragma once
#include <string>
#include <vector>

namespace config {

struct FileData {
    std::wstring path;
    int pages = 0;
    int fromRange;
    int toRange;
    bool loaded = false;
}; 

struct ConfigData {
    std::string printer     = "Microsoft Print to PDF";
    int         copies      = 1;

    std::string printMode   = "Simplex";
    std::string paper       = "A4";
    std::string scale       = "Fit to page (keep aspect ratio)";
    std::string orientation = "Auto";
    std::string skipBlankPage = "Yes (Actual sheets may be less than calculated)";

    std::vector<FileData> files;
};

}