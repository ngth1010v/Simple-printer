#pragma once
#include <string>
#include "config/ConfigModel.h"

namespace config {

void Write(const std::string& path, const ConfigData& data);

}