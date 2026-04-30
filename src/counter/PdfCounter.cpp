#include "counter/PdfCounter.h"

#include <thread>
#include <iostream>

// PDFium
#include <fpdfview.h>

namespace counter {

    void countPdf(const std::string& path, PdfCountCallback callback)
    {
        // chạy async
        std::thread([path, callback]() {

            int pageCount = -1;

            // Init PDFium
            FPDF_LIBRARY_CONFIG config;
            config.version = 2;
            config.m_pUserFontPaths = nullptr;
            config.m_pIsolate = nullptr;
            config.m_v8EmbedderSlot = 0;

            FPDF_InitLibraryWithConfig(&config);

            // Mở file
            FPDF_DOCUMENT doc = FPDF_LoadDocument(path.c_str(), nullptr);

            if (doc) {
                pageCount = FPDF_GetPageCount(doc);
                FPDF_CloseDocument(doc);
            }
            else {
                // lỗi mở file
                pageCount = -1;
            }

            // Destroy PDFium
            FPDF_DestroyLibrary();

            // gọi callback
            if (callback) {
                callback(pageCount);
            }

        }).detach(); // detach để chạy nền

    }

}