#pragma once

#include <functional>
#include <string>

namespace renderer {

using RenderCallback = std::function<void(std::string error)>;

void Init(int dpi);
void Render(const std::string& srcPath,
            const std::string& targetPath,
            int page = 1,
            RenderCallback callback = {});
void Shutdown();

} // namespace renderer