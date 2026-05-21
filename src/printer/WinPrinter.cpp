// printer/WinPrinter.cpp
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

constexpr double kWhiteLumaThreshold = 252.0;       // pixel >= ngưỡng này xem là trắng
constexpr double kWhiteRatioThreshold = 0.9995;     // nếu > 99.95% pixel trắng -> coi là quá trắng
constexpr int kAnchorMarginPx = 4;                  // cách mép phải/dưới bao nhiêu px
constexpr int kAnchorSizePx = 2;                    // cụm pixel chèn vào
constexpr std::uint32_t kAnchorColor = 0xFF000000u; // đen đậm, chắc chắn không bị skip

struct PrintableArea {
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
};

std::string ToLowerAscii(std::string s) {
    for (char& ch : s) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return s;
}

std::wstring ToLowerAsciiWide(std::wstring s) {
    for (wchar_t& ch : s) {
        if (ch >= L'A' && ch <= L'Z') {
            ch = static_cast<wchar_t>(ch - L'A' + L'a');
        }
    }
    return s;
}

bool LoadBitmap32(const std::wstring& bmpPath, BmpImage& out, std::string& error) {
    if (bmpPath.empty()) {
        error = "BMP path is empty.";
        return false;
    }

    HBITMAP hBitmap = static_cast<HBITMAP>(
        LoadImageW(nullptr, bmpPath.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION));

    if (hBitmap == nullptr) {
        error = "LoadImageW failed for BMP.";
        return false;
    }

    BITMAP bm{};
    if (GetObjectW(hBitmap, sizeof(bm), &bm) == 0) {
        DeleteObject(hBitmap);
        error = "GetObjectW failed for BMP.";
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
        error = "GetDIBits failed for BMP.";
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

            dst.pixels[
                static_cast<std::size_t>(dstY) * static_cast<std::size_t>(dst.width) +
                static_cast<std::size_t>(dstX)
            ] = src.pixels[static_cast<std::size_t>(srcRow + x)];
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
        const int sy = std::min(
            src.height - 1,
            static_cast<int>((static_cast<long long>(y) * src.height) / dstHeight));

        const std::size_t dstRow =
            static_cast<std::size_t>(y) * static_cast<std::size_t>(dstWidth);

        const std::size_t srcRow =
            static_cast<std::size_t>(sy) * static_cast<std::size_t>(src.width);

        for (int x = 0; x < dstWidth; ++x) {
            const int sx = std::min(
                src.width - 1,
                static_cast<int>((static_cast<long long>(x) * src.width) / dstWidth));

            dst.pixels[dstRow + static_cast<std::size_t>(x)] =
                src.pixels[srcRow + static_cast<std::size_t>(sx)];
        }
    }

    return true;
}

void ClearWhite(BmpImage& img) {
    img.pixels.assign(
        static_cast<std::size_t>(img.width) * static_cast<std::size_t>(img.height),
        0xFFFFFFFFu);
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
        const std::size_t dstOffset =
            static_cast<std::size_t>(dstY + row) * static_cast<std::size_t>(dst.width) +
            static_cast<std::size_t>(dstX);

        const std::size_t srcOffset =
            static_cast<std::size_t>(srcY + row) * static_cast<std::size_t>(src.width) +
            static_cast<std::size_t>(srcX);

        std::memcpy(
            &dst.pixels[dstOffset],
            &src.pixels[srcOffset],
            static_cast<std::size_t>(copyWidth) * sizeof(std::uint32_t));
    }
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
    return scale == "No Scale";
}

bool IsFillScale(const std::string& scale) {
    return scale == "Fill page (ignore aspect ratio)";
}

enum class MarginPolicy {
    Auto,
    KeepOriginal,
    KeepOriginalForDocuments,
    KeepOriginalForImages
};

MarginPolicy ParseMarginPolicy(const std::string& margin) {
    if (margin == "Auto") {
        return MarginPolicy::Auto;
    }
    if (margin == "Keep Original") {
        return MarginPolicy::KeepOriginal;
    }
    if (margin == "Keep Original For Documents") {
        return MarginPolicy::KeepOriginalForDocuments;
    }
    if (margin == "Keep Original For Images") {
        return MarginPolicy::KeepOriginalForImages;
    }

    return MarginPolicy::Auto;
}

bool ShouldKeepOriginalMargin(MarginPolicy policy, bool isDocumentBmp) {
    switch (policy) {
    case MarginPolicy::KeepOriginal:
        return true;

    case MarginPolicy::KeepOriginalForDocuments:
        return isDocumentBmp;

    case MarginPolicy::KeepOriginalForImages:
        return !isDocumentBmp;

    case MarginPolicy::Auto:
    default:
        return false;
    }
}

struct ContentBounds {
    int left = 0;
    int top = 0;
    int right = -1;
    int bottom = -1;
    bool valid = false;
};


double GetLuma(std::uint32_t p) {
    const int b = static_cast<int>(p & 0xFFu);
    const int g = static_cast<int>((p >> 8) & 0xFFu);
    const int r = static_cast<int>((p >> 16) & 0xFFu);

    return 0.114 * static_cast<double>(b)
         + 0.587 * static_cast<double>(g)
         + 0.299 * static_cast<double>(r);
}

ContentBounds FindContentBounds(const BmpImage& img) {
    ContentBounds bounds;

    if (img.width <= 0 || img.height <= 0 || img.pixels.empty()) {
        return bounds;
    }

    bounds.left = img.width;
    bounds.top = img.height;
    bounds.right = -1;
    bounds.bottom = -1;

    for (int y = 0; y < img.height; ++y) {
        const std::size_t row = static_cast<std::size_t>(y) * static_cast<std::size_t>(img.width);

        for (int x = 0; x < img.width; ++x) {
            const std::uint32_t p = img.pixels[row + static_cast<std::size_t>(x)];

            if (GetLuma(p) < kWhiteLumaThreshold) {
                if (x < bounds.left)   bounds.left = x;
                if (y < bounds.top)    bounds.top = y;
                if (x > bounds.right)  bounds.right = x;
                if (y > bounds.bottom) bounds.bottom = y;
                bounds.valid = true;
            }
        }
    }

    return bounds;
}

// double CalcAutoSecondScaleFactor(
//     const BmpImage& rendered,
//     const ContentBounds& bounds,
//     int pageWidth,
//     int pageHeight) {
//     // Printable area giả lập bằng 95% page, nằm giữa page.
//     // Nếu sau scale lần 1 content vượt ra ngoài vùng này thì scale lần 2.
//     constexpr double kPrintableRatio = 0.95;

//     if (!bounds.valid || rendered.width <= 0 || rendered.height <= 0 ||
//         pageWidth <= 0 || pageHeight <= 0) {
//         return 1.0;
//     }

//     const double insetX = (1.0 - kPrintableRatio) * 0.5 * static_cast<double>(pageWidth);
//     const double insetY = (1.0 - kPrintableRatio) * 0.5 * static_cast<double>(pageHeight);

//     const double w = static_cast<double>(rendered.width);
//     const double h = static_cast<double>(rendered.height);

//     const double leftMarginInRendered = static_cast<double>(bounds.left);
//     const double rightMarginInRendered = w - static_cast<double>(bounds.right + 1);
//     const double topMarginInRendered = static_cast<double>(bounds.top);
//     const double bottomMarginInRendered = h - static_cast<double>(bounds.bottom + 1);

//     auto capFactor = [](double renderedHalf, double contentMargin, double pageHalf, double inset) -> double {
//         const double denom = renderedHalf - contentMargin;

//         // Nếu denom <= 0 thì cạnh đó không ép shrink thêm.
//         if (denom <= 0.0) {
//             return 1.0;
//         }

//         const double f = (pageHalf - inset) / denom;
//         if (f <= 0.0) {
//             return 0.0;
//         }

//         return f;
//     };

//     double factor = 1.0;
//     factor = std::min(factor, capFactor(w * 0.5, leftMarginInRendered,   static_cast<double>(pageWidth)  * 0.5, insetX));
//     factor = std::min(factor, capFactor(w * 0.5, rightMarginInRendered,  static_cast<double>(pageWidth)  * 0.5, insetX));
//     factor = std::min(factor, capFactor(h * 0.5, topMarginInRendered,    static_cast<double>(pageHeight) * 0.5, insetY));
//     factor = std::min(factor, capFactor(h * 0.5, bottomMarginInRendered, static_cast<double>(pageHeight) * 0.5, insetY));

//     if (factor > 1.0) {
//         factor = 1.0;
//     }

//     // Không để factor rơi về 0 vì sẽ làm hỏng scale.
//     if (factor < 0.01) {
//         factor = 0.01;
//     }

//     return factor;
// }

bool ApplySecondScaleIfNeeded(
    BmpImage& rendered,
    int pageWidth,
    int pageHeight,
    const PrintableArea& printable) {

    const ContentBounds bounds = FindContentBounds(rendered);

    if (!bounds.valid) {
        return true;
    }

    // rendered đang được center vào page
    const int dstX = (pageWidth - rendered.width) / 2;
    const int dstY = (pageHeight - rendered.height) / 2;

    const int contentLeft   = dstX + bounds.left;
    const int contentTop    = dstY + bounds.top;
    const int contentRight  = dstX + bounds.right;
    const int contentBottom = dstY + bounds.bottom;

    // hoàn toàn nằm trong printable
    if (
        contentLeft   >= printable.left  &&
        contentTop    >= printable.top   &&
        contentRight  <  printable.right &&
        contentBottom <  printable.bottom) {
        return true;
    }

    // scale factor thật sự cần thiết
    double factor = 1.0;

    if (contentLeft < printable.left) {
        const double f =
            static_cast<double>(printable.right - printable.left) /
            static_cast<double>(contentRight - contentLeft + 1);

        factor = std::min(factor, f);
    }

    if (contentRight >= printable.right) {
        const double f =
            static_cast<double>(printable.right - printable.left) /
            static_cast<double>(contentRight - contentLeft + 1);

        factor = std::min(factor, f);
    }

    if (contentTop < printable.top) {
        const double f =
            static_cast<double>(printable.bottom - printable.top) /
            static_cast<double>(contentBottom - contentTop + 1);

        factor = std::min(factor, f);
    }

    if (contentBottom >= printable.bottom) {
        const double f =
            static_cast<double>(printable.bottom - printable.top) /
            static_cast<double>(contentBottom - contentTop + 1);

        factor = std::min(factor, f);
    }

    factor *= 0.999; // tránh chạm mép do rounding

    if (factor >= 0.999999) {
        return true;
    }

    const int newWidth = std::max(
        1,
        static_cast<int>(std::lround(static_cast<double>(rendered.width) * factor)));

    const int newHeight = std::max(
        1,
        static_cast<int>(std::lround(static_cast<double>(rendered.height) * factor)));

    BmpImage scaled;

    if (!ScaleNearest(rendered, newWidth, newHeight, scaled)) {
        return false;
    }

    rendered = std::move(scaled);

    return true;
}

bool IsTooWhite(const BmpImage& img) {
    if (img.width <= 0 || img.height <= 0 || img.pixels.empty()) {
        return false;
    }

    const std::size_t total = img.pixels.size();
    std::size_t whiteCount = 0;

    for (std::uint32_t p : img.pixels) {
        if (GetLuma(p) >= kWhiteLumaThreshold) {
            ++whiteCount;
        }
    }

    const double whiteRatio =
        static_cast<double>(whiteCount) / static_cast<double>(total);

    return whiteRatio >= kWhiteRatioThreshold;
}

void AddAnchorMark(BmpImage& img) {
    if (img.width <= 0 || img.height <= 0 || img.pixels.empty()) {
        return;
    }

    const int markW = std::min(kAnchorSizePx, img.width);
    const int markH = std::min(kAnchorSizePx, img.height);

    const int startX = std::max(0, img.width - kAnchorMarginPx - markW);
    const int startY = std::max(0, img.height - kAnchorMarginPx - markH);

    for (int y = 0; y < markH; ++y) {
        const std::size_t row =
            static_cast<std::size_t>(startY + y) * static_cast<std::size_t>(img.width);

        for (int x = 0; x < markW; ++x) {
            img.pixels[row + static_cast<std::size_t>(startX + x)] = kAnchorColor;
        }
    }
}

bool EndsWith(const std::wstring& value, const std::wstring& suffix) {
    if (value.size() < suffix.size()) {
        return false;
    }

    return value.compare(
        value.size() - suffix.size(),
        suffix.size(),
        suffix) == 0;
}

bool IsDocumentBmpPath(const std::wstring& bmpPath) {
    const std::wstring s = ToLowerAsciiWide(bmpPath);

    if (EndsWith(s, L"-d.bmp")) {
        return true;
    }

    if (EndsWith(s, L"_d.bmp")) {
        return true;
    }

    return false;
}


bool GetPrintableArea(HDC hdc, PrintableArea& out) {
    if (hdc == nullptr) {
        return false;
    }

    const int physicalWidth  = GetDeviceCaps(hdc, PHYSICALWIDTH);
    const int physicalHeight = GetDeviceCaps(hdc, PHYSICALHEIGHT);

    const int offsetX = GetDeviceCaps(hdc, PHYSICALOFFSETX);
    const int offsetY = GetDeviceCaps(hdc, PHYSICALOFFSETY);

    const int printableWidth  = GetDeviceCaps(hdc, HORZRES);
    const int printableHeight = GetDeviceCaps(hdc, VERTRES);

    out.left   = offsetX;
    out.top    = offsetY;
    out.right  = offsetX + printableWidth;
    out.bottom = offsetY + printableHeight;

    return
        physicalWidth > 0 &&
        physicalHeight > 0 &&
        printableWidth > 0 &&
        printableHeight > 0;
}

} // namespace

void EnsureNotBlankPage(BmpImage& page) {
    if (IsTooWhite(page)) {
        AddAnchorMark(page);
    }
}

bool BuildPageImage(
    const std::wstring& bmpPath,
    const std::string& orientation,
    const std::string& scale,
    const std::string& margin,
    HDC printerDc,
    int pageWidth,
    int pageHeight,
    BmpImage& outPage,
    std::string& error) {
    if (pageWidth <= 0 || pageHeight <= 0) {
        error = "Invalid page size.";
        return false;
    }

    PrintableArea printable;
    GetPrintableArea(printerDc, printable);

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

    const bool isDocumentBmp = IsDocumentBmpPath(bmpPath);
    const MarginPolicy marginPolicy = ParseMarginPolicy(margin);
    const bool keepOriginalMargin = ShouldKeepOriginalMargin(marginPolicy, isDocumentBmp);

    if (IsNoScale(scale)) {
        const int dstX = (pageWidth - working.width) / 2;
        const int dstY = (pageHeight - working.height) / 2;

        BlitClipped(outPage, working, dstX, dstY);
        EnsureNotBlankPage(outPage);
        return true;
    }

    BmpImage rendered;

    // Scale lần 1: scale theo page đầy đủ, chưa trừ margin hard-code nữa.
    if (IsFillScale(scale)) {
        if (!ScaleNearest(working, pageWidth, pageHeight, rendered)) {
            error = "Failed to scale BMP.";
            return false;
        }
    } else {
        const double scaleX = static_cast<double>(pageWidth) / static_cast<double>(working.width);
        const double scaleY = static_cast<double>(pageHeight) / static_cast<double>(working.height);
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

    // Margin handling:
    // - Keep Original => không scale lần 2
    // - Auto => scan bitmap sau scale lần 1, nếu đè vùng unprintable thì scale lần 2
    if (!keepOriginalMargin) {
        if (!ApplySecondScaleIfNeeded(
            rendered,
            pageWidth,
            pageHeight,
            printable)) {
            error = "Failed to apply auto margin scaling.";
            return false;
        }
    }

    const int dstX = (pageWidth - rendered.width) / 2;
    const int dstY = (pageHeight - rendered.height) / 2;

    BlitClipped(outPage, rendered, dstX, dstY);
    EnsureNotBlankPage(outPage);

    return true;
}
} // namespace printer