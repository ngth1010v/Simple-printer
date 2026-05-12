#pragma once
#include <string>
#include "config/ConfigModel.h"

namespace config {

ConfigData Parse(bool keepFiles = false);

}