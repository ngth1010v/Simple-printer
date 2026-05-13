// printer/Printer.h
#pragma once

#include <string>
#include "config/ConfigModel.h"

namespace printer {

void Init(config::ConfigData& cfg);
int PrintSimplex(std::wstring path, std::string& error);
int PrintDuplex(std::wstring frontPath, std::wstring backPath, std::string& error);
void ShutDown();

} // namespace printer