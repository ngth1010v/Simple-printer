#pragma once

#include <string>
#include <functional>

namespace counter {

    // Callback nhận số trang
    using PdfCountCallback = std::function<void(int)>;

    // Hàm đếm số trang PDF (async)
    void countPdf(const std::string& path, PdfCountCallback callback);

}