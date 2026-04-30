#pragma once
#include <string>
#include <functional>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace counter {

class DocxWorker {
public:
    using Callback = std::function<void(int, const std::string&)>;

    DocxWorker();
    ~DocxWorker();

    void Start();
    void Stop();

    void Enqueue(const std::string& path, Callback cb);

private:
    struct Task {
        std::string path;
        Callback cb;
    };

    void WorkerLoop();
    bool CheckWordInstalled();
    int CountWithWord(const std::string& path);

private:
    std::thread m_thread;
    std::queue<Task> m_queue;

    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_running{false};

    bool m_hasWord = false;
};

}