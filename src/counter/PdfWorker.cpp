#include "counter/PdfWorker.h"
#include "fpdfview.h"
#include <cstdlib>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
static std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size_needed = WideCharToMultiByte(CP_ACP, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_ACP, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}
#else
static std::string WStringToString(const std::wstring& wstr) {
    return std::string(wstr.begin(), wstr.end());
}
#endif

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

void PdfWorker::Enqueue(const std::wstring& path, Callback cb) {
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
    std::string path_str = WStringToString(task.path);

    FPDF_DOCUMENT doc = FPDF_LoadDocument(path_str.c_str(), nullptr);

    if (!doc) {
        task.cb(0, "Cannot open PDF");
        return;
    }

    int pages = FPDF_GetPageCount(doc);
    FPDF_CloseDocument(doc);

    task.cb(pages, "");
}

}