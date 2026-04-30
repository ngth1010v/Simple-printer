#include "counter/Counter.h"
#include "counter/PdfWorker.h"
#include "counter/DocxWorker.h"

#include <algorithm>

namespace counter {

static std::unique_ptr<PdfWorker> g_pdf;
static std::unique_ptr<DocxWorker> g_docx;

static std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

static std::string GetExt(const std::string& path) {
    auto pos = path.find_last_of('.');
    if (pos == std::string::npos) return "";
    return ToLower(path.substr(pos + 1));
}

void Init() {
    g_pdf = std::make_unique<PdfWorker>();
    g_docx = std::make_unique<DocxWorker>();

    g_pdf->Start();
    g_docx->Start();
}

void Shutdown() {
    if (g_pdf) {
        g_pdf->Stop();
        g_pdf.reset();
    }

    if (g_docx) {
        g_docx->Stop();
        g_docx.reset();
    }
}

void Count(const std::string& path, CountCallback cb) {
    auto ext = GetExt(path);

    if (ext == "png" || ext == "jpg" || ext == "jpeg") {
        cb(1, "");
        return;
    }

    if (ext == "pdf") {
        g_pdf->Enqueue(path, cb);
        return;
    }

    if (ext == "docx") {
        g_docx->Enqueue(path, cb);
        return;
    }

    cb(0, "Unsupported file");
}

}