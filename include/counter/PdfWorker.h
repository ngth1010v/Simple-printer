#pragma once
#include <string>
#include <functional>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>

namespace counter {

class PdfWorker {
public:
    using Callback = std::function<void(int, const std::string&)>;

    PdfWorker();
    ~PdfWorker();

    void Start();
    void Stop();

    void Enqueue(const std::string& path, Callback cb);

private:
    struct Task {
        std::string path;
        Callback cb;
    };

    void WorkerLoop();
    void ProcessTask(Task task);

private:
    static constexpr int MAX_THREADS = 10;

    std::queue<Task> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_running{false};

    std::vector<std::thread> m_threads;
};

}