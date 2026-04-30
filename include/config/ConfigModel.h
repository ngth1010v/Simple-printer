#pragma once
#include <string>
#include <vector>

namespace config {

struct FileData {
    std::string path;
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
    std::string collate     = "Collated (1,2,3 | 1,2,3 | 1,2,3)";

    // {top, right, bottom, left}
    float margin[4] = {1, 1, 1, 1};

    std::vector<FileData> files;
};

}