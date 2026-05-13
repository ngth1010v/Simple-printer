// renderer/ImageWorker.h
#pragma once

#include "Renderer.h"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace renderer {

class ImageWorker {
public:
    ImageWorker();
    ~ImageWorker();

    void Start();
    void Stop();

    bool Enqueue(std::wstring srcPath,
                 std::wstring targetPath,
                 RenderCallback callback);

private:
    struct Task {
        std::wstring srcPath;
        std::wstring targetPath;
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
};

} // namespace renderer