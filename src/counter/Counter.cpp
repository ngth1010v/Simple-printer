// counter/Counter.cpp
#include "counter/Counter.h"
#include "counter/PdfWorker.h"
#include "counter/DocxWorker.h"

#include <algorithm>
#include <cwctype>

namespace counter {

static std::unique_ptr<PdfWorker> g_pdf;
static std::unique_ptr<DocxWorker> g_docx;

static std::wstring ToLower(std::wstring s) {
    std::transform(s.begin(), s.end(), s.begin(), ::towlower);
    return s;
}

static std::wstring GetExt(const std::wstring& path) {
    auto pos = path.find_last_of(L'.');
    if (pos == std::wstring::npos) return L"";
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

void Count(const std::wstring& path, CountCallback cb) {
    auto ext = GetExt(path);

    if (ext == L"png" || ext == L"jpg" || ext == L"jpeg") {
        cb(1, "");
        return;
    }

    if (ext == L"pdf") {
        g_pdf->Enqueue(path, cb);
        return;
    }

    if (ext == L"docx") {
        g_docx->Enqueue(path, cb);
        return;
    }

    cb(0, "Unsupported file");
}

}