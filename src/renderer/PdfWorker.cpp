#include "renderer/PdfWorker.h"

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

namespace renderer {

namespace {

std::wstring ToWide(const std::string& s) {
    if (s.empty()) {
        return {};
    }

    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.c_str(), -1, nullptr, 0);
    UINT cp = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;

    if (count <= 0) {
        cp = CP_ACP;
        flags = 0;
        count = MultiByteToWideChar(cp, flags, s.c_str(), -1, nullptr, 0);
    }

    if (count <= 0) {
        return {};
    }

    std::wstring result(static_cast<size_t>(count - 1), L'\0');
    MultiByteToWideChar(cp, flags, s.c_str(), -1, result.data(), count);
    return result;
}

std::string HrToString(const char* prefix, HRESULT hr) {
    std::ostringstream oss;
    oss << prefix << " (hr=0x" << std::hex << std::uppercase << static_cast<unsigned long>(hr) << ")";
    return oss.str();
}

std::string PdfErrorToString(unsigned long err) {
    switch (err) {
        case FPDF_ERR_SUCCESS:
            return {};
        case FPDF_ERR_UNKNOWN:
            return "PDF error: unknown";
        case FPDF_ERR_FILE:
            return "PDF error: file";
        case FPDF_ERR_FORMAT:
            return "PDF error: format";
        case FPDF_ERR_PASSWORD:
            return "PDF error: password";
        case FPDF_ERR_SECURITY:
            return "PDF error: security";
        case FPDF_ERR_PAGE:
            return "PDF error: page";
        default: {
            std::ostringstream oss;
            oss << "PDF error code: " << err;
            return oss.str();
        }
    }
}

bool EnsureParentDir(const std::wstring& filePath) {
    std::filesystem::path p(filePath);
    const auto parent = p.parent_path();
    if (parent.empty()) {
        return true;
    }

    std::error_code ec;
    std::filesystem::create_directories(parent, ec);
    return !ec;
}

bool WriteBmp32TopDown(const std::wstring& filePath,
                       int width,
                       int height,
                       const uint8_t* pixels,
                       uint32_t stride,
                       double dpiX,
                       double dpiY,
                       std::string& error) {
    if (width <= 0 || height <= 0 || pixels == nullptr || stride == 0) {
        error = "invalid bitmap";
        return false;
    }

    if (!EnsureParentDir(filePath)) {
        error = "cannot create target directory";
        return false;
    }

    HANDLE hFile = CreateFileW(
        filePath.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        error = "cannot create target file";
        return false;
    }

    const uint32_t imageSize = stride * static_cast<uint32_t>(height);
    const uint32_t fileSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + imageSize;

    BITMAPFILEHEADER bfh{};
    bfh.bfType = 0x4D42;
    bfh.bfSize = fileSize;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    BITMAPINFOHEADER bih{};
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = width;
    bih.biHeight = -height;
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = BI_RGB;
    bih.biSizeImage = imageSize;

    const double pxPerMeter = 39.37007874015748;
    bih.biXPelsPerMeter = static_cast<LONG>(std::lround((dpiX > 0.0 ? dpiX : 96.0) * pxPerMeter));
    bih.biYPelsPerMeter = static_cast<LONG>(std::lround((dpiY > 0.0 ? dpiY : 96.0) * pxPerMeter));

    DWORD written = 0;
    bool ok = WriteFile(hFile, &bfh, sizeof(bfh), &written, nullptr) &&
              written == sizeof(bfh) &&
              WriteFile(hFile, &bih, sizeof(bih), &written, nullptr) &&
              written == sizeof(bih);

    if (ok) {
        for (int y = 0; y < height; ++y) {
            const BYTE* row = pixels + static_cast<size_t>(y) * stride;
            if (!WriteFile(hFile, row, stride, &written, nullptr) || written != stride) {
                ok = false;
                break;
            }
        }
    }

    CloseHandle(hFile);

    if (!ok) {
        DeleteFileW(filePath.c_str());
        error = "failed to write bmp";
        return false;
    }

    return true;
}

} // namespace

PdfWorker::PdfWorker() = default;

PdfWorker::~PdfWorker() {
    Stop();
}

void PdfWorker::Start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (started_) {
        return;
    }

    stopping_ = false;
    thread_ = std::thread(&PdfWorker::ThreadMain, this);
    started_ = true;
}

void PdfWorker::Stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!started_) {
            return;
        }
        stopping_ = true;
    }

    cv_.notify_all();

    if (thread_.joinable()) {
        thread_.join();
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        started_ = false;
        stopping_ = false;
        std::queue<Task> empty;
        queue_.swap(empty);
        currentSource_.clear();
    }

    if (currentDoc_) {
        FPDF_CloseDocument(currentDoc_);
        currentDoc_ = nullptr;
    }
}

void PdfWorker::SetDpi(int dpi) {
    if (dpi <= 0) {
        dpi = 96;
    }
    dpi_.store(dpi, std::memory_order_relaxed);
}

bool PdfWorker::Enqueue(std::string srcPath,
                        std::string targetPath,
                        int page,
                        RenderCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!started_ || stopping_) {
        return false;
    }

    queue_.push(Task{std::move(srcPath), std::move(targetPath), page, std::move(callback)});
    cv_.notify_one();
    return true;
}

void PdfWorker::ThreadMain() {
    while (true) {
        Task task;

        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [&] { return stopping_ || !queue_.empty(); });

            if (queue_.empty()) {
                break;
            }

            task = std::move(queue_.front());
            queue_.pop();
        }

        ProcessTask(task);

        bool shouldStop = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            shouldStop = stopping_;
        }

        if (shouldStop) {
            std::queue<Task> pending;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                pending.swap(queue_);
            }

            while (!pending.empty()) {
                Task t = std::move(pending.front());
                pending.pop();
                if (t.callback) {
                    t.callback("renderer is shutting down");
                }
            }
            break;
        }
    }

    if (currentDoc_) {
        FPDF_CloseDocument(currentDoc_);
        currentDoc_ = nullptr;
    }
}

void PdfWorker::ProcessTask(const Task& task) {
    std::string error;

    if (task.page <= 0) {
        if (task.callback) task.callback("invalid page");
        return;
    }

    const int dpi = dpi_.load(std::memory_order_relaxed);
    const int pageIndex = task.page - 1;

    // handle document switch
    if (task.srcPath != currentSource_) {
        if (currentDoc_) {
            FPDF_CloseDocument(currentDoc_);
            currentDoc_ = nullptr;
        }
        currentSource_ = task.srcPath;
    }

    // open document if needed
    if (!currentDoc_) {
        currentDoc_ = FPDF_LoadDocument(task.srcPath.c_str(), nullptr);
        if (!currentDoc_) {
            error = PdfErrorToString(FPDF_GetLastError());
            if (error.empty()) error = "failed to open pdf";

            if (task.callback) task.callback(error);
            return;
        }
    }

    const int pageCount = FPDF_GetPageCount(currentDoc_);
    if (pageIndex < 0 || pageIndex >= pageCount) {
        if (task.callback) task.callback("page out of range");
        return;
    }

    FPDF_PAGE page = FPDF_LoadPage(currentDoc_, pageIndex);
    if (!page) {
        if (task.callback) task.callback("failed to load pdf page");
        return;
    }

    struct PageGuard {
        FPDF_PAGE page;
        ~PageGuard() {
            if (page) FPDF_ClosePage(page);
        }
    } pageGuard{ page };

    const double pageWidthPt = FPDF_GetPageWidth(page);
    const double pageHeightPt = FPDF_GetPageHeight(page);

    int width = static_cast<int>(std::lround(pageWidthPt * dpi / 72.0));
    int height = static_cast<int>(std::lround(pageHeightPt * dpi / 72.0));
    width = std::max(width, 1);
    height = std::max(height, 1);

    FPDF_BITMAP bitmap = FPDFBitmap_Create(width, height, 1);
    if (!bitmap) {
        if (task.callback) task.callback("failed to create pdf bitmap");
        return;
    }

    struct BitmapGuard {
        FPDF_BITMAP bmp;
        ~BitmapGuard() {
            if (bmp) FPDFBitmap_Destroy(bmp);
        }
    } bmpGuard{ bitmap };

    FPDFBitmap_FillRect(bitmap, 0, 0, width, height, 0xFFFFFFFF);

    FPDF_RenderPageBitmap(bitmap, page, 0, 0, width, height, 0, 0);

    const void* buffer = FPDFBitmap_GetBuffer(bitmap);
    const int stride = FPDFBitmap_GetStride(bitmap);

    std::wstring dstWide = ToWide(task.targetPath);
    if (dstWide.empty()) {
        if (task.callback) task.callback("invalid target path");
        return;
    }

    if (!WriteBmp32TopDown(
            dstWide,
            width,
            height,
            static_cast<const uint8_t*>(buffer),
            static_cast<uint32_t>(stride),
            static_cast<double>(dpi),
            static_cast<double>(dpi),
            error)) {

        if (task.callback) task.callback(error);
        return;
    }

    error.clear();

    if (task.callback) {
        task.callback(error);
    }
}
} // namespace renderer