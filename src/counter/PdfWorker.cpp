#include "counter/PdfWorker.h"

#include "fpdfview.h"

namespace counter {

PdfWorker::PdfWorker() {}

PdfWorker::~PdfWorker() {
    Stop();
}

void PdfWorker::Start() {
    m_running = true;

    FPDF_InitLibrary();

    for (int i = 0; i < MAX_THREADS; i++) {
        m_threads.emplace_back(&PdfWorker::WorkerLoop, this);
    }
}

void PdfWorker::Stop() {
    m_running = false;
    m_cv.notify_all();

    for (auto& t : m_threads) {
        if (t.joinable()) t.join();
    }
    m_threads.clear();

    FPDF_DestroyLibrary();
}

void PdfWorker::Enqueue(const std::string& path, Callback cb) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push({path, cb});
    }
    m_cv.notify_one();
}

void PdfWorker::WorkerLoop() {
    while (m_running) {
        Task task;

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [&]() {
                return !m_running || !m_queue.empty();
            });

            if (!m_running && m_queue.empty()) return;

            task = m_queue.front();
            m_queue.pop();
        }

        ProcessTask(task);
    }
}

void PdfWorker::ProcessTask(Task task) {
    FPDF_DOCUMENT doc = FPDF_LoadDocument(task.path.c_str(), nullptr);

    if (!doc) {
        task.cb(0, "Cannot open PDF");
        return;
    }

    int pages = FPDF_GetPageCount(doc);
    FPDF_CloseDocument(doc);

    task.cb(pages, "");
}

}