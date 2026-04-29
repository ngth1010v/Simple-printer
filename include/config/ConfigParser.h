#pragma once
#include <string>
#include "ConfigModel.h"

namespace config {

ConfigData Parse(const std::string& path);

}