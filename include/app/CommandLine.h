#pragma once

#include <string>
#include <vector>

namespace CommandLine {

    void Parse(int argc, wchar_t* argv[]);

    bool IsPrint();

    const std::vector<std::wstring>& GetFiles();

}