// renderer/Renderer.cpp
#include "renderer/Renderer.h"

#include "renderer/DocxWorker.h"
#include "renderer/ImageWorker.h"
#include "renderer/PdfWorker.h"

#include <algorithm>
#include <cctype>
#include <memory>
#include <mutex>
#include <string>

#include <fpdfview.h>

namespace renderer {

namespace {

struct State {
    std::mutex mutex;
    bool initialized = false;
    bool pdfiumInitialized = false;
    int dpi = 96;

    std::shared_ptr<ImageWorker> imageWorker;
    std::shared_ptr<PdfWorker> pdfWorker;
    std::shared_ptr<DocxWorker> docxWorker;
};

State g_state;

std::wstring ToLowerAscii(std::wstring s) {
    for (wchar_t& ch : s) {
        if (ch >= L'A' && ch <= L'Z') {
            ch = static_cast<wchar_t>(ch - L'A' + L'a');
        }
    }
    return s;
}

std::wstring GetExtension(const std::wstring& path) {
    const auto slash = path.find_last_of(L"/\\");
    const auto dot = path.find_last_of(L'.');

    if (dot == std::wstring::npos ||
        (slash != std::wstring::npos && dot < slash)) {
        return {};
    }

    return ToLowerAscii(path.substr(dot));
}

bool IsImageExt(const std::wstring& ext) {
    return ext == L".bmp"  ||
           ext == L".png"  ||
           ext == L".jpg"  ||
           ext == L".jpeg" ||
           ext == L".gif"  ||
           ext == L".tif"  ||
           ext == L".tiff" ||
           ext == L".ico"  ||
           ext == L".webp";
}

void CallCb(const RenderCallback& cb, std::string error) {
    if (cb) {
        cb(std::move(error));
    }
}

} // namespace

void Init(int dpi) {
    if (dpi <= 0) {
        dpi = 96;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);

    if (g_state.initialized) {
        g_state.dpi = dpi;

        if (g_state.pdfWorker) {
            g_state.pdfWorker->SetDpi(dpi);
        }

        if (g_state.docxWorker) {
            g_state.docxWorker->SetDpi(dpi);
        }

        return;
    }

    FPDF_InitLibrary();
    g_state.pdfiumInitialized = true;

    auto imageWorker = std::make_shared<ImageWorker>();
    auto pdfWorker = std::make_shared<PdfWorker>();
    auto docxWorker = std::make_shared<DocxWorker>();

    pdfWorker->SetDpi(dpi);
    docxWorker->SetDpi(dpi);

    imageWorker->Start();
    pdfWorker->Start();
    docxWorker->Start();

    g_state.imageWorker = std::move(imageWorker);
    g_state.pdfWorker = std::move(pdfWorker);
    g_state.docxWorker = std::move(docxWorker);

    g_state.dpi = dpi;
    g_state.initialized = true;
}

void Render(const std::wstring& srcPath,
            const std::wstring& targetPath,
            int page,
            RenderCallback callback) {
    if (srcPath.empty() || targetPath.empty()) {
        CallCb(callback, "invalid srcPath or targetPath");
        return;
    }

    std::shared_ptr<ImageWorker> imageWorker;
    std::shared_ptr<PdfWorker> pdfWorker;
    std::shared_ptr<DocxWorker> docxWorker;

    {
        std::lock_guard<std::mutex> lock(g_state.mutex);

        if (!g_state.initialized) {
            CallCb(callback, "renderer not initialized");
            return;
        }

        const std::wstring ext = GetExtension(srcPath);

        if (ext == L".pdf") {
            pdfWorker = g_state.pdfWorker;
        } else if (ext == L".docx") {
            docxWorker = g_state.docxWorker;
        } else if (IsImageExt(ext)) {
            imageWorker = g_state.imageWorker;
        } else {
            return;
        }
    }

    if (imageWorker) {
        if (!imageWorker->Enqueue(srcPath, targetPath, std::move(callback))) {
            CallCb(callback, "renderer is shutting down");
        }

        return;
    }

    if (pdfWorker) {
        if (page <= 0) {
            CallCb(callback, "invalid page");
            return;
        }

        if (!pdfWorker->Enqueue(srcPath, targetPath, page, std::move(callback))) {
            CallCb(callback, "renderer is shutting down");
        }

        return;
    }

    if (docxWorker) {
        if (page <= 0) {
            CallCb(callback, "invalid page");
            return;
        }

        if (!docxWorker->Enqueue(srcPath, targetPath, page, std::move(callback))) {
            CallCb(callback, "renderer is shutting down");
        }

        return;
    }
}

void Shutdown() {
    std::shared_ptr<ImageWorker> imageWorker;
    std::shared_ptr<PdfWorker> pdfWorker;
    std::shared_ptr<DocxWorker> docxWorker;

    bool destroyPdfium = false;

    {
        std::lock_guard<std::mutex> lock(g_state.mutex);

        if (!g_state.initialized) {
            return;
        }

        imageWorker = std::move(g_state.imageWorker);
        pdfWorker = std::move(g_state.pdfWorker);
        docxWorker = std::move(g_state.docxWorker);

        g_state.initialized = false;

        destroyPdfium = g_state.pdfiumInitialized;
        g_state.pdfiumInitialized = false;
    }

    if (imageWorker) {
        imageWorker->Stop();
    }

    if (pdfWorker) {
        pdfWorker->Stop();
    }

    if (docxWorker) {
        docxWorker->Stop();
    }

    if (destroyPdfium) {
        FPDF_DestroyLibrary();
    }
}

} // namespace renderer