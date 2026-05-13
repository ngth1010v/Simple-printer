// renderer/PdfWorker.h
#pragma once

#include "Renderer.h"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <atomic>

#include <fpdfview.h>

namespace renderer {

class PdfWorker {
public:
    PdfWorker();
    ~PdfWorker();

    void Start();
    void Stop();

    void SetDpi(int dpi);

    bool Enqueue(std::wstring srcPath,
                 std::wstring targetPath,
                 int page,
                 RenderCallback callback);

private:
    struct Task {
        std::wstring srcPath;
        std::wstring targetPath;
        int page = 1;
        RenderCallback callback;
    };

    void ThreadMain();
    void ProcessTask(const Task& task);

    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<Task> queue_;
    bool started_ = false;
    bool stopping_ = false;

    std::wstring currentSource_;
    FPDF_DOCUMENT currentDoc_ = nullptr;

    std::atomic<int> dpi_{96};
};

} // namespace renderer