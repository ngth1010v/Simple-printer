#include "printer/WinPrinter.h"

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <string>
#include <utility>

namespace printer {
namespace {

std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) {
        return {};
    }

    const int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (sizeNeeded <= 0) {
        return {};
    }

    std::wstring out(static_cast<std::size_t>(sizeNeeded - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), sizeNeeded);
    return out;
}

std::string ToLowerAscii(std::string s) {
    for (char& ch : s) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return s;
}

std::string NormalizeKey(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char ch : s) {
        if (std::isalnum(ch)) {
            out.push_back(static_cast<char>(std::tolower(ch)));
        }
    }
    return out;
}

bool LoadBitmap32(const std::string& bmpPath, BmpImage& out, std::string& error) {
    const std::wstring wPath = Utf8ToWide(bmpPath);
    if (wPath.empty()) {
        error = "Failed to convert BMP path to wide string.";
        return false;
    }

    HBITMAP hBitmap = static_cast<HBITMAP>(
        LoadImageW(nullptr, wPath.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION));

    if (hBitmap == nullptr) {
        error = "LoadImageW failed for BMP: " + bmpPath;
        return false;
    }

    BITMAP bm{};
    if (GetObjectW(hBitmap, sizeof(bm), &bm) == 0) {
        DeleteObject(hBitmap);
        error = "GetObjectW failed for BMP: " + bmpPath;
        return false;
    }

    if (bm.bmWidth <= 0 || bm.bmHeight <= 0) {
        DeleteObject(hBitmap);
        error = "Invalid BMP size.";
        return false;
    }

    out.width = bm.bmWidth;
    out.height = bm.bmHeight;
    out.pixels.assign(static_cast<std::size_t>(out.width) * static_cast<std::size_t>(out.height), 0);

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = out.width;
    bmi.bmiHeader.biHeight = -out.height; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC screenDc = GetDC(nullptr);
    if (screenDc == nullptr) {
        DeleteObject(hBitmap);
        error = "GetDC failed.";
        return false;
    }

    const int scanLines = GetDIBits(
        screenDc,
        hBitmap,
        0,
        static_cast<UINT>(out.height),
        out.pixels.data(),
        &bmi,
        DIB_RGB_COLORS);

    ReleaseDC(nullptr, screenDc);
    DeleteObject(hBitmap);

    if (scanLines == 0) {
        error = "GetDIBits failed for BMP: " + bmpPath;
        return false;
    }

    return true;
}

bool Rotate90Clockwise(const BmpImage& src, BmpImage& dst) {
    if (src.width <= 0 || src.height <= 0 || src.pixels.empty()) {
        return false;
    }

    dst.width = src.height;
    dst.height = src.width;
    dst.pixels.assign(static_cast<std::size_t>(dst.width) * static_cast<std::size_t>(dst.height), 0);

    for (int y = 0; y < src.height; ++y) {
        const int srcRow = y * src.width;
        for (int x = 0; x < src.width; ++x) {
            const int dstX = src.height - 1 - y;
            const int dstY = x;
            dst.pixels[static_cast<std::size_t>(dstY) * static_cast<std::size_t>(dst.width) + static_cast<std::size_t>(dstX)] =
                src.pixels[static_cast<std::size_t>(srcRow + x)];
        }
    }

    return true;
}

bool ScaleNearest(const BmpImage& src, int dstWidth, int dstHeight, BmpImage& dst) {
    if (src.width <= 0 || src.height <= 0 || src.pixels.empty() || dstWidth <= 0 || dstHeight <= 0) {
        return false;
    }

    dst.width = dstWidth;
    dst.height = dstHeight;
    dst.pixels.assign(static_cast<std::size_t>(dstWidth) * static_cast<std::size_t>(dstHeight), 0);

    for (int y = 0; y < dstHeight; ++y) {
        const int sy = std::min(src.height - 1, static_cast<int>((static_cast<long long>(y) * src.height) / dstHeight));
        const std::size_t dstRow = static_cast<std::size_t>(y) * static_cast<std::size_t>(dstWidth);
        const std::size_t srcRow = static_cast<std::size_t>(sy) * static_cast<std::size_t>(src.width);

        for (int x = 0; x < dstWidth; ++x) {
            const int sx = std::min(src.width - 1, static_cast<int>((static_cast<long long>(x) * src.width) / dstWidth));
            dst.pixels[dstRow + static_cast<std::size_t>(x)] = src.pixels[srcRow + static_cast<std::size_t>(sx)];
        }
    }

    return true;
}

void ClearWhite(BmpImage& img) {
    img.pixels.assign(static_cast<std::size_t>(img.width) * static_cast<std::size_t>(img.height), 0xFFFFFFFFu);
}

void BlitClipped(BmpImage& dst, const BmpImage& src, int dstX, int dstY) {
    if (dst.width <= 0 || dst.height <= 0 || src.width <= 0 || src.height <= 0) {
        return;
    }

    int srcX = 0;
    int srcY = 0;
    int copyWidth = src.width;
    int copyHeight = src.height;

    if (dstX < 0) {
        srcX = -dstX;
        copyWidth -= srcX;
        dstX = 0;
    }

    if (dstY < 0) {
        srcY = -dstY;
        copyHeight -= srcY;
        dstY = 0;
    }

    if (dstX + copyWidth > dst.width) {
        copyWidth = dst.width - dstX;
    }

    if (dstY + copyHeight > dst.height) {
        copyHeight = dst.height - dstY;
    }

    if (copyWidth <= 0 || copyHeight <= 0) {
        return;
    }

    for (int row = 0; row < copyHeight; ++row) {
        const std::size_t dstOffset = static_cast<std::size_t>(dstY + row) * static_cast<std::size_t>(dst.width) + static_cast<std::size_t>(dstX);
        const std::size_t srcOffset = static_cast<std::size_t>(srcY + row) * static_cast<std::size_t>(src.width) + static_cast<std::size_t>(srcX);

        std::memcpy(
            &dst.pixels[dstOffset],
            &src.pixels[srcOffset],
            static_cast<std::size_t>(copyWidth) * sizeof(std::uint32_t));
    }
}

std::string NormalizeMode(std::string s) {
    s = ToLowerAscii(std::move(s));
    std::string out;
    out.reserve(s.size());
    for (char ch : s) {
        if (std::isalnum(static_cast<unsigned char>(ch))) {
            out.push_back(ch);
        }
    }
    return out;
}

bool IsLandscapeByConfig(const std::string& orientation, const BmpImage& src) {
    if (orientation == "Alway Landscape") {
        return true;
    }

    if (orientation == "Alway Portrait") {
        return false;
    }

    // Auto
    return src.width > src.height;
}

bool IsNoScale(const std::string& scale) {
    return scale == "No scale";
}

bool IsFillScale(const std::string& scale) {
    return scale == "Fill page (ignore aspect ratio)";
}

} // namespace

bool BuildPageImage(
    const std::string& bmpPath,
    const std::string& orientation,
    const std::string& scale,
    int pageWidth,
    int pageHeight,
    BmpImage& outPage,
    std::string& error) {
    if (pageWidth <= 0 || pageHeight <= 0) {
        error = "Invalid page size.";
        return false;
    }

    BmpImage source;
    if (!LoadBitmap32(bmpPath, source, error)) {
        return false;
    }

    BmpImage working = source;
    if (IsLandscapeByConfig(orientation, source)) {
        BmpImage rotated;
        if (!Rotate90Clockwise(source, rotated)) {
            error = "Failed to rotate source BMP.";
            return false;
        }
        working = std::move(rotated);
    }

    outPage.width = pageWidth;
    outPage.height = pageHeight;
    ClearWhite(outPage);

    if (working.width <= 0 || working.height <= 0 || working.pixels.empty()) {
        error = "Source BMP is empty.";
        return false;
    }

    if (IsNoScale(scale)) {
        const int dstX = (pageWidth - working.width) / 2;
        const int dstY = (pageHeight - working.height) / 2;
        BlitClipped(outPage, working, dstX, dstY);
        return true;
    }

    const double safeRatio = 0.95;
    const int safeWidth = std::max(1, static_cast<int>(std::floor(static_cast<double>(pageWidth) * safeRatio)));
    const int safeHeight = std::max(1, static_cast<int>(std::floor(static_cast<double>(pageHeight) * safeRatio)));

    BmpImage rendered;

    if (IsFillScale(scale)) {
        if (!ScaleNearest(working, safeWidth, safeHeight, rendered)) {
            error = "Failed to scale BMP.";
            return false;
        }
    } else {
        const double scaleX = static_cast<double>(safeWidth) / static_cast<double>(working.width);
        const double scaleY = static_cast<double>(safeHeight) / static_cast<double>(working.height);
        const double factor = std::min(scaleX, scaleY);

        int dstWidth = static_cast<int>(std::lround(static_cast<double>(working.width) * factor));
        int dstHeight = static_cast<int>(std::lround(static_cast<double>(working.height) * factor));

        dstWidth = std::max(1, dstWidth);
        dstHeight = std::max(1, dstHeight);

        if (!ScaleNearest(working, dstWidth, dstHeight, rendered)) {
            error = "Failed to scale BMP.";
            return false;
        }
    }

    const int dstX = (pageWidth - rendered.width) / 2;
    const int dstY = (pageHeight - rendered.height) / 2;
    BlitClipped(outPage, rendered, dstX, dstY);
    return true;
}

} // namespace printer
