#pragma once
#include <string>
#include "config/ConfigModel.h"

namespace config {

ConfigData Parse(const std::string& path);

}