#pragma once

#include <vector>
#include <string>

namespace platform {

/// Mở file picker của Windows
/// @return danh sách path (UTF-8). rỗng nếu user cancel
std::vector<std::string> OpenFilePicker();

}