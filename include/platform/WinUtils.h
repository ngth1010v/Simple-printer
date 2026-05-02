#pragma once
#include <string>
#include <vector>

namespace platform {

std::vector<std::string> GetPrinters();

std::vector<std::string> GetSupportedPapers(const std::string& printerName);

bool GetPrinterDPI(const std::string& printerName, int& dpiX, int& dpiY);

}