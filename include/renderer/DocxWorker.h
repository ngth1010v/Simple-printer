#pragma once

#include "Renderer.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>

#include <fpdfview.h>

struct IDispatch;

namespace renderer {

class DocxWorker {
public:
    DocxWorker();
    ~DocxWorker();

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

    bool EnsureWordApp(std::string& error);
    bool ConvertDocxToTempPdf(const std::wstring& srcPath,
                              const std::wstring& tempPdfPath,
                              std::string& error);

    void CloseCurrentPdf();
    void CloseWordApp();

    std::wstring MakeTempPdfPath(const std::wstring& srcPath) const;

    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<Task> queue_;
    bool started_ = false;
    bool stopping_ = false;

    std::atomic<int> dpi_{96};

    std::wstring currentSource_;
    std::wstring currentTempPdfWide_;
    FPDF_DOCUMENT currentPdfDoc_ = nullptr;

    std::wstring tempDirWide_;

    IDispatch* wordApp_ = nullptr;
    std::unordered_map<std::wstring, std::wstring> tempPdfMap_;
};

} // namespace renderer