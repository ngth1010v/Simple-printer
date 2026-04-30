#pragma once
#include <string>
#include <functional>
#include <memory>

namespace counter {

using CountCallback = std::function<void(int pages, const std::string& error)>;

void Init();
void Count(const std::string& path, CountCallback cb);
void Shutdown(); // rename từ Close

}