#pragma once

#include <string>
#include "config/ConfigModel.h"

namespace printer {

void Init(config::ConfigData& cfg);
int PrintSimplex(std::string path, std::string& error);
int PrintDuplex(std::string frontPath, std::string backPath, std::string& error);
void ShutDown();

} // namespace printer
