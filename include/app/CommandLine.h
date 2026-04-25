#pragma once

#include <string>
#include <vector>

namespace CommandLine {

    // ---- parse ----
    void Parse(int argc, char** argv);

    // ---- query ----
    bool HasFlag(const std::string& key);
    bool HasOption(const std::string& key);
    std::string GetOption(const std::string& key, const std::string& defaultValue = "");
    const std::vector<std::string>& GetArgs();

}