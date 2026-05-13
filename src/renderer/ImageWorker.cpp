// renderer/ImageWorker.cpp
#include "renderer/ImageWorker.h"

#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <vector>

namespace renderer {

namespace {

std::string HrToString(const char* prefix, HRESULT hr) {
    std::ostringstream oss;
    oss << prefix << " (hr=0x" << std::hex << std::uppercase << static_cast<unsigned long>(hr) << ")";
    return oss.str();
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

ImageWorker::ImageWorker() = default;

ImageWorker::~ImageWorker() {
    Stop();
}

void ImageWorker::Start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (started_) {
        return;
    }

    stopping_ = false;
    thread_ = std::thread(&ImageWorker::ThreadMain, this);
    started_ = true;
}

void ImageWorker::Stop() {
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
    }
}

bool ImageWorker::Enqueue(std::wstring srcPath,
                          std::wstring targetPath,
                          RenderCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!started_ || stopping_) {
        return false;
    }

    queue_.push(Task{std::move(srcPath), std::move(targetPath), std::move(callback)});
    cv_.notify_one();
    return true;
}

void ImageWorker::ThreadMain() {
    const HRESULT hrInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool comReady = SUCCEEDED(hrInit);

    IWICImagingFactory* factory = nullptr;
    if (comReady) {
        CoCreateInstance(
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&factory)
        );
    }

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

        if (factory) {
            ProcessTask(task);
        } else if (task.callback) {
            task.callback("WIC is not available");
        }

        bool shouldDrain = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            shouldDrain = stopping_;
        }

        if (shouldDrain) {
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

    if (factory) {
        factory->Release();
    }

    if (comReady) {
        CoUninitialize();
    }
}

using Microsoft::WRL::ComPtr;

void ImageWorker::ProcessTask(const Task& task) {
    const std::wstring& srcWide = task.srcPath;
    const std::wstring& dstWide = task.targetPath;

    std::string error;

    if (srcWide.empty() || dstWide.empty()) {
        if (task.callback) {
            task.callback("invalid image path");
        }
        return;
    }

    ComPtr<IWICImagingFactory> factory;
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(factory.GetAddressOf())
    );

    if (FAILED(hr) || !factory) {
        if (task.callback) {
            task.callback("WIC factory creation failed");
        }
        return;
    }

    ComPtr<IWICBitmapDecoder> decoder;
    ComPtr<IWICBitmapFrameDecode> frame;
    ComPtr<IWICFormatConverter> converter;

    UINT width = 0;
    UINT height = 0;

    double dpiX = 96.0;
    double dpiY = 96.0;

    do {
        hr = factory->CreateDecoderFromFilename(
            srcWide.c_str(),
            nullptr,
            GENERIC_READ,
            WICDecodeMetadataCacheOnLoad,
            decoder.GetAddressOf()
        );

        if (FAILED(hr) || !decoder) {
            error = HrToString("failed to open image", hr);
            break;
        }

        hr = decoder->GetFrame(0, frame.GetAddressOf());
        if (FAILED(hr) || !frame) {
            error = HrToString("failed to get image frame", hr);
            break;
        }

        hr = factory->CreateFormatConverter(converter.GetAddressOf());
        if (FAILED(hr) || !converter) {
            error = HrToString("failed to create format converter", hr);
            break;
        }

        hr = converter->Initialize(
            frame.Get(),
            GUID_WICPixelFormat32bppBGRA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0,
            WICBitmapPaletteTypeMedianCut
        );

        if (FAILED(hr)) {
            error = HrToString("failed to initialize format converter", hr);
            break;
        }

        hr = converter->GetSize(&width, &height);
        if (FAILED(hr) || width == 0 || height == 0) {
            error = HrToString("failed to get image size", hr);
            break;
        }

        converter->GetResolution(&dpiX, &dpiY);

        const uint32_t stride = width * 4;
        std::vector<uint8_t> pixels(static_cast<size_t>(stride) * height);

        hr = converter->CopyPixels(
            nullptr,
            stride,
            static_cast<UINT>(pixels.size()),
            pixels.data()
        );

        if (FAILED(hr)) {
            error = HrToString("failed to copy image pixels", hr);
            break;
        }

        if (!WriteBmp32TopDown(
                dstWide,
                static_cast<int>(width),
                static_cast<int>(height),
                pixels.data(),
                stride,
                dpiX,
                dpiY,
                error)) {
            break;
        }

    } while (false);

    if (task.callback) {
        task.callback(error);
    }
}

} // namespace renderer