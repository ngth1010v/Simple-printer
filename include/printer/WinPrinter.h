#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace printer {

struct BmpImage {
    int width = 0;
    int height = 0;
    std::vector<std::uint32_t> pixels; // 32-bit BGRA in little-endian memory
};

bool BuildPageImage(
    const std::string& bmpPath,
    const std::string& orientation,
    const std::string& scale,
    int pageWidth,
    int pageHeight,
    BmpImage& outPage,
    std::string& error
);

} // namespace printer
