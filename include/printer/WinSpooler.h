#pragma once

#include <string>
#include <vector>

#include <windows.h>

#include "WinPrinter.h"

namespace printer {

struct PrinterPageInfo {
    int horzRes = 0;
    int vertRes = 0;
    int physicalOffsetX = 0;
    int physicalOffsetY = 0;
    int dpiX = 0;
    int dpiY = 0;
};

bool PrinterSupportsDuplex(const std::string& printerName, std::string& error);

bool OpenPrinterDc(
    const std::string& printerName,
    const std::string& paper,
    bool duplex,
    HDC& hdc,
    PrinterPageInfo& info,
    std::string& error
);

bool PrintBitmapPages(
    HDC hdc,
    const std::vector<BmpImage>& pages,
    std::string& error
);

} // namespace printer
