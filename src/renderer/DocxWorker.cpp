#include "renderer/DocxWorker.h"

#include <windows.h>
#include <oleauto.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>

namespace renderer {

namespace {

constexpr long kWordFormatPdf = 17;
constexpr long kWordAlertNone = 0;
constexpr long kWordSaveChangesNo = 0;

std::wstring ToWide(const std::string& s) {
    if (s.empty()) {
        return {};
    }

    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.c_str(), -1, nullptr, 0);
    UINT cp = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;

    if (count <= 0) {
        cp = CP_ACP;
        flags = 0;
        count = MultiByteToWideChar(cp, flags, s.c_str(), -1, nullptr, 0);
    }

    if (count <= 0) {
        return {};
    }

    std::wstring result(static_cast<size_t>(count - 1), L'\0');
    MultiByteToWideChar(cp, flags, s.c_str(), -1, result.data(), count);
    return result;
}

std::string WideToUtf8(const std::wstring& s) {
    if (s.empty()) {
        return {};
    }

    int count = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (count <= 0) {
        return {};
    }

    std::string result(static_cast<size_t>(count - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, result.data(), count, nullptr, nullptr);
    return result;
}

std::wstring GetAbsolutePathW(const std::wstring& path) {
    if (path.empty()) {
        return {};
    }

    DWORD need = GetFullPathNameW(path.c_str(), 0, nullptr, nullptr);
    if (need == 0) {
        return path;
    }

    std::wstring out;
    out.resize(need);

    DWORD written = GetFullPathNameW(path.c_str(), need, out.data(), nullptr);
    if (written == 0) {
        return path;
    }

    out.resize(written);
    return out;
}

std::string HrToString(const char* prefix, HRESULT hr) {
    std::ostringstream oss;
    oss << prefix << " (hr=0x" << std::hex << std::uppercase
        << static_cast<unsigned long>(hr) << ")";
    return oss.str();
}

bool EnsureParentDir(const std::wstring& filePath) {
    std::filesystem::path p(filePath);
    const auto parent = p.parent_path();
    if (parent.empty()) {
        return true;
    }

    std::error_code ec;
    std::filesystem::create_directories(parent, ec);
    return !ec;
}


static VARIANT MakeBool(bool v) {
    VARIANT var;
    VariantInit(&var);
    var.vt = VT_BOOL;
    var.boolVal = v ? VARIANT_TRUE : VARIANT_FALSE;
    return var;
}

static VARIANT MakeBstr(const std::wstring& s) {
    VARIANT var;
    VariantInit(&var);
    var.vt = VT_BSTR;
    var.bstrVal = SysAllocString(s.c_str());
    return var;
}

static VARIANT MakeI4(long v) {
    VARIANT var;
    VariantInit(&var);
    var.vt = VT_I4;
    var.lVal = v;
    return var;
}

static HRESULT InvokeDispatch(
    IDispatch* obj,
    const wchar_t* name,
    WORD flags,
    DISPPARAMS* params,
    VARIANT* result = nullptr
) {
    if (!obj || !name) return E_POINTER;

    LPOLESTR names[] = { const_cast<LPOLESTR>(name) };
    DISPID dispid = 0;

    HRESULT hr = obj->GetIDsOfNames(IID_NULL, names, 1, LOCALE_USER_DEFAULT, &dispid);
    if (FAILED(hr)) return hr;

    return obj->Invoke(
        dispid,
        IID_NULL,
        LOCALE_USER_DEFAULT,
        flags,
        params,
        result,
        nullptr,
        nullptr
    );
}

static HRESULT CallMethod(IDispatch* obj, const wchar_t* name, VARIANT* args, UINT argCount, VARIANT* result = nullptr) {
    DISPPARAMS params{};
    params.cArgs = argCount;
    params.rgvarg = args;

    return InvokeDispatch(obj, name, DISPATCH_METHOD, &params, result);
}


bool WriteBmp32TopDown(const std::wstring& filePath,
                       int width,
                       int height,
                       const uint8_t* pixels,
                       uint32_t stride,
                       double dpiX,
                       double dpiY,
                       std::string& error) {
    if (width <= 0 || height <= 0 || pixels == nullptr || stride == 0) {
        error = "invalid bitmap";
        return false;
    }

    if (!EnsureParentDir(filePath)) {
        error = "cannot create target directory";
        return false;
    }

    HANDLE hFile = CreateFileW(
        filePath.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        error = "cannot create target file";
        return false;
    }

    const uint32_t imageSize = stride * static_cast<uint32_t>(height);
    const uint32_t fileSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + imageSize;

    BITMAPFILEHEADER bfh{};
    bfh.bfType = 0x4D42;
    bfh.bfSize = fileSize;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    BITMAPINFOHEADER bih{};
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = width;
    bih.biHeight = -height;
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = BI_RGB;
    bih.biSizeImage = imageSize;

    const double pxPerMeter = 39.37007874015748;
    bih.biXPelsPerMeter = static_cast<LONG>(std::lround((dpiX > 0.0 ? dpiX : 96.0) * pxPerMeter));
    bih.biYPelsPerMeter = static_cast<LONG>(std::lround((dpiY > 0.0 ? dpiY : 96.0) * pxPerMeter));

    DWORD written = 0;
    bool ok = WriteFile(hFile, &bfh, sizeof(bfh), &written, nullptr) &&
              written == sizeof(bfh) &&
              WriteFile(hFile, &bih, sizeof(bih), &written, nullptr) &&
              written == sizeof(bih);

    if (ok) {
        for (int y = 0; y < height; ++y) {
            const BYTE* row = pixels + static_cast<size_t>(y) * stride;
            if (!WriteFile(hFile, row, stride, &written, nullptr) || written != stride) {
                ok = false;
                break;
            }
        }
    }

    CloseHandle(hFile);

    if (!ok) {
        DeleteFileW(filePath.c_str());
        error = "failed to write bmp";
        return false;
    }

    return true;
}

bool FileExistsW(const std::wstring& path) {
    const DWORD attrs = GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

DISPID GetDispId(IDispatch* disp, const wchar_t* name, std::string& error) {
    if (!disp || !name) {
        error = "invalid dispatch";
        return DISPID_UNKNOWN;
    }

    DISPID id = DISPID_UNKNOWN;
    LPOLESTR names[] = { const_cast<LPOLESTR>(name) };
    HRESULT hr = disp->GetIDsOfNames(IID_NULL, names, 1, LOCALE_USER_DEFAULT, &id);
    if (FAILED(hr)) {
        error = HrToString("GetIDsOfNames failed", hr);
        return DISPID_UNKNOWN;
    }

    return id;
}

// Caller passes args in natural order.
// This function reverses them for IDispatch::Invoke.
bool Invoke(IDispatch* disp,
            const wchar_t* name,
            WORD flags,
            const std::vector<VARIANT>& args,
            VARIANT* result,
            std::string& error) {
    if (!disp) {
        error = "invalid dispatch";
        return false;
    }

    const DISPID id = GetDispId(disp, name, error);
    if (id == DISPID_UNKNOWN) {
        return false;
    }

    std::vector<VARIANT> reversed;
    reversed.reserve(args.size());
    for (auto it = args.rbegin(); it != args.rend(); ++it) {
        reversed.push_back(*it);
    }

    DISPPARAMS params{};
    params.cArgs = static_cast<UINT>(reversed.size());
    params.rgvarg = reversed.empty() ? nullptr : reversed.data();

    DISPID dispidNamed = DISPID_PROPERTYPUT;
    if (flags == DISPATCH_PROPERTYPUT || flags == DISPATCH_PROPERTYPUTREF) {
        params.cNamedArgs = 1;
        params.rgdispidNamedArgs = &dispidNamed;
    }

    if (result) {
        VariantInit(result);
    }

    HRESULT hr = disp->Invoke(id,
                              IID_NULL,
                              LOCALE_USER_DEFAULT,
                              flags,
                              &params,
                              result,
                              nullptr,
                              nullptr);
    if (FAILED(hr)) {
        error = HrToString("Invoke failed", hr);
        return false;
    }

    return true;
}

VARIANT MakeStringVariant(const std::wstring& s) {
    VARIANT v;
    VariantInit(&v);
    v.vt = VT_BSTR;
    v.bstrVal = SysAllocString(s.c_str());
    return v;
}

VARIANT MakeLongVariant(long value) {
    VARIANT v;
    VariantInit(&v);
    v.vt = VT_I4;
    v.lVal = value;
    return v;
}

VARIANT MakeBoolVariant(bool value) {
    VARIANT v;
    VariantInit(&v);
    v.vt = VT_BOOL;
    v.boolVal = value ? VARIANT_TRUE : VARIANT_FALSE;
    return v;
}

class VariantHolder {
public:
    VariantHolder() {
        VariantInit(&m_var);
    }

    ~VariantHolder() {
        VariantClear(&m_var);
    }

    VariantHolder(const VariantHolder&) = delete;
    VariantHolder& operator=(const VariantHolder&) = delete;

    VARIANT* get() { return &m_var; }
    const VARIANT* get() const { return &m_var; }

    VARIANT* operator&() = delete;

    VARIANT& ref() { return m_var; }

private:
    VARIANT m_var;
};

bool GetDispatchProperty(IDispatch* disp, const wchar_t* name, IDispatch** out, std::string& error) {
    if (!out) {
        error = "invalid output";
        return false;
    }

    VARIANT result;
    VariantInit(&result);

    std::vector<VARIANT> args;
    if (!Invoke(disp, name, DISPATCH_PROPERTYGET, args, &result, error)) {
        VariantClear(&result);
        return false;
    }

    if (result.vt != VT_DISPATCH || result.pdispVal == nullptr) {
        VariantClear(&result);
        error = "property is not dispatch";
        return false;
    }

    *out = result.pdispVal;
    (*out)->AddRef();
    VariantClear(&result);
    return true;
}

bool PutLongProperty(IDispatch* disp, const wchar_t* name, long value, std::string& error) {
    VARIANT v = MakeLongVariant(value);
    std::vector<VARIANT> args;
    args.push_back(v);

    const bool ok = Invoke(disp, name, DISPATCH_PROPERTYPUT, args, nullptr, error);
    VariantClear(&v);
    return ok;
}

bool PutBoolProperty(IDispatch* disp, const wchar_t* name, bool value, std::string& error) {
    VARIANT v = MakeBoolVariant(value);
    std::vector<VARIANT> args;
    args.push_back(v);

    const bool ok = Invoke(disp, name, DISPATCH_PROPERTYPUT, args, nullptr, error);
    VariantClear(&v);
    return ok;
}

bool CallDocumentsOpen(IDispatch* documents,
                       const std::wstring& fileName,
                       IDispatch** outDoc,
                       std::string& error) {
    if (!documents || !outDoc) {
        error = "invalid argument";
        return false;
    }

    *outDoc = nullptr;

    // Word.Documents.Open(FileName, ConfirmConversions, ReadOnly)
    // rgvarg is reversed:
    //   [0] = ReadOnly
    //   [1] = ConfirmConversions (missing)
    //   [2] = FileName
    VARIANT args[3];
    for (int i = 0; i < 3; ++i) {
        VariantInit(&args[i]);
    }

    args[0] = MakeBool(true);   // ReadOnly = true
    args[1].vt = VT_ERROR;      // ConfirmConversions omitted
    args[1].scode = DISP_E_PARAMNOTFOUND;
    args[2] = MakeBstr(fileName);

    VariantHolder docVar;
    HRESULT hrOpen = CallMethod(
        documents,
        L"Open",
        args,
        3,
        docVar.get()
    );

    for (int i = 0; i < 3; ++i) {
        VariantClear(&args[i]);
    }

    if (FAILED(hrOpen) || docVar.ref().vt != VT_DISPATCH || !docVar.ref().pdispVal) {
        error = "Open failed: " ;//+ HrToString(hrOpen);
        return false;
    }

    *outDoc = docVar.ref().pdispVal;
    (*outDoc)->AddRef();
    return true;
}
bool CallExportAsPdf(IDispatch* doc,
                     const std::wstring& pdfPath,
                     std::string& error) {
    if (!doc) {
        error = "invalid argument";
        return false;
    }

    // ExportAsFixedFormat(OutputFileName, ExportFormat, OpenAfterExport, OptimizeFor, Range)
    // rgvarg is reversed:
    //   [0] = Range
    //   [1] = OptimizeFor
    //   [2] = OpenAfterExport
    //   [3] = ExportFormat
    //   [4] = OutputFileName
    VARIANT args[5];
    for (int i = 0; i < 5; ++i) {
        VariantInit(&args[i]);
    }

    args[0] = MakeI4(0);               // wdExportAllDocument
    args[1] = MakeI4(0);               // wdExportOptimizeForPrint
    args[2] = MakeBool(false);         // OpenAfterExport = false
    args[3] = MakeI4(kWordFormatPdf);  // wdExportFormatPDF
    args[4] = MakeBstr(pdfPath);        // OutputFileName

    HRESULT hr = CallMethod(
        doc,
        L"ExportAsFixedFormat",
        args,
        5,
        nullptr
    );

    for (int i = 0; i < 5; ++i) {
        VariantClear(&args[i]);
    }

    if (FAILED(hr)) {
        error = "ExportAsFixedFormat failed: " ;//+ HrToString(hr);
        return false;
    }

    return true;
}

bool CallCloseDocument(IDispatch* doc, std::string& error) {
    VARIANT saveChanges = MakeBoolVariant(false);
    std::vector<VARIANT> args;
    args.push_back(saveChanges);

    const bool ok = Invoke(doc, L"Close", DISPATCH_METHOD, args, nullptr, error);
    VariantClear(&saveChanges);
    return ok;
}

bool CallQuit(IDispatch* app, std::string& error) {
    VARIANT saveChanges = MakeBoolVariant(false);
    std::vector<VARIANT> args;
    args.push_back(saveChanges);

    const bool ok = Invoke(app, L"Quit", DISPATCH_METHOD, args, nullptr, error);
    VariantClear(&saveChanges);
    return ok;
}

struct ComInitGuard {
    bool ok = false;

    ComInitGuard() {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        ok = SUCCEEDED(hr);
    }

    ~ComInitGuard() {
        if (ok) {
            CoUninitialize();
        }
    }
};

template <class F>
class ScopeExit {
public:
    explicit ScopeExit(F f) : f_(std::move(f)), active_(true) {}
    ~ScopeExit() { if (active_) f_(); }

    ScopeExit(const ScopeExit&) = delete;
    ScopeExit& operator=(const ScopeExit&) = delete;

    void dismiss() { active_ = false; }

private:
    F f_;
    bool active_;
};

template <class F>
ScopeExit<F> MakeScopeExit(F f) {
    return ScopeExit<F>(std::move(f));
}

bool GetLongProperty(IDispatch* disp, const wchar_t* name, long* out, std::string& error) {
    if (!disp || !out) {
        error = "invalid argument";
        return false;
    }

    VARIANT result;
    VariantInit(&result);

    std::vector<VARIANT> args;
    if (!Invoke(disp, name, DISPATCH_PROPERTYGET, args, &result, error)) {
        VariantClear(&result);
        return false;
    }

    if (result.vt == VT_I4) {
        *out = result.lVal;
    } else if (result.vt == VT_INT) {
        *out = result.intVal;
    } else {
        error = "property is not integer";
        VariantClear(&result);
        return false;
    }

    VariantClear(&result);
    return true;
}

} // namespace

DocxWorker::DocxWorker() {
    WCHAR currentDir[MAX_PATH] = {};
    DWORD n = GetCurrentDirectoryW(MAX_PATH, currentDir);
    if (n > 0 && n < MAX_PATH) {
        tempDirWide_ = std::wstring(currentDir) + L"\\_temp_simple_printer_docx";
    } else {
        tempDirWide_ = L"_temp_simple_printer_docx";
    }
}

DocxWorker::~DocxWorker() {
    Stop();
}

void DocxWorker::Start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (started_) {
        return;
    }

    stopping_ = false;
    std::error_code ec;
    std::filesystem::create_directories(tempDirWide_, ec);

    thread_ = std::thread(&DocxWorker::ThreadMain, this);
    started_ = true;
}

void DocxWorker::Stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!started_) {
            return;
        }
        stopping_ = true;
    }

    cv_.notify_all();

    if (thread_.joinable()) {
        thread_.join();
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        started_ = false;
        stopping_ = false;
        std::queue<Task> empty;
        queue_.swap(empty);
    }

    std::error_code ec;
    std::filesystem::remove_all(tempDirWide_, ec);
}

void DocxWorker::SetDpi(int dpi) {
    if (dpi <= 0) {
        dpi = 96;
    }
    dpi_.store(dpi, std::memory_order_relaxed);
}

bool DocxWorker::Enqueue(std::string srcPath,
                         std::string targetPath,
                         int page,
                         RenderCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!started_ || stopping_) {
        return false;
    }

    queue_.push(Task{std::move(srcPath), std::move(targetPath), page, std::move(callback)});
    cv_.notify_one();
    return true;
}

std::wstring DocxWorker::MakeTempPdfPath(const std::string& srcPath) const {
    const size_t h = std::hash<std::string>{}(srcPath);
    std::wstringstream ss;
    ss << tempDirWide_ << L"\\" << std::hex << std::uppercase << h << L".pdf";
    return ss.str();
}

void DocxWorker::CloseCurrentPdf() {
    if (currentPdfDoc_) {
        FPDF_CloseDocument(currentPdfDoc_);
        currentPdfDoc_ = nullptr;
    }
    currentTempPdfWide_.clear();
    currentSource_.clear();
}

void DocxWorker::CloseWordApp() {
    if (!wordApp_) {
        return;
    }

    std::string error;

    IDispatch* documents = nullptr;
    if (GetDispatchProperty(wordApp_, L"Documents", &documents, error)) {
        long count = 0;
        if (GetLongProperty(documents, L"Count", &count, error)) {
            for (long i = count; i >= 1; --i) {
                VARIANT indexVar;
                VariantInit(&indexVar);
                indexVar = MakeLongVariant(i);

                VARIANT itemResult;
                VariantInit(&itemResult);

                std::vector<VARIANT> args;
                args.push_back(indexVar);

                if (Invoke(documents, L"Item", DISPATCH_PROPERTYGET, args, &itemResult, error)) {
                    if (itemResult.vt == VT_DISPATCH && itemResult.pdispVal) {
                        IDispatch* doc = itemResult.pdispVal;
                        std::string closeErr;
                        CallCloseDocument(doc, closeErr);
                        doc->Release();
                    }
                }

                VariantClear(&itemResult);
                VariantClear(&indexVar);
            }
        }

        documents->Release();
    }

    // thử Quit thêm 1 lần nữa sau khi đã đóng hết doc
    {
        std::string quitError;
        CallQuit(wordApp_, quitError);
    }

    wordApp_->Release();
    wordApp_ = nullptr;
}

bool DocxWorker::EnsureWordApp(std::string& error) {
    if (wordApp_) {
        return true;
    }

    CLSID clsid{};
    HRESULT hr = CLSIDFromProgID(L"Word.Application", &clsid);
    if (FAILED(hr)) {
        error = "Word is not installed";
        return false;
    }

    hr = CoCreateInstance(clsid,
                          nullptr,
                          CLSCTX_LOCAL_SERVER,
                          IID_IDispatch,
                          reinterpret_cast<void**>(&wordApp_));
    if (FAILED(hr) || !wordApp_) {
        error = HrToString("failed to create Word.Application", hr);
        wordApp_ = nullptr;
        return false;
    }

    // Best-effort settings, ignore failures.
    {
        std::string tmp;
        PutBoolProperty(wordApp_, L"Visible", false, tmp);
    }
    {
        std::string tmp;
        PutLongProperty(wordApp_, L"DisplayAlerts", kWordAlertNone, tmp);
    }
    {
        std::string tmp;
        PutLongProperty(wordApp_, L"AutomationSecurity", 3, tmp); // msoAutomationSecurityForceDisable
    }

    return true;
}

bool DocxWorker::ConvertDocxToTempPdf(const std::string& srcPath,
                                      const std::wstring& tempPdfPath,
                                      std::string& error) {
    if (!EnsureWordApp(error)) {
        return false;
    }

    if (tempPdfPath.empty()) {
        error = "invalid temp pdf path";
        return false;
    }

    DeleteFileW(tempPdfPath.c_str());

    IDispatch* documents = nullptr;
    IDispatch* doc = nullptr;

    if (!GetDispatchProperty(wordApp_, L"Documents", &documents, error)) {
        return false;
    }

    const std::wstring srcWide = GetAbsolutePathW(ToWide(srcPath));
    if (srcWide.empty()) {
        documents->Release();
        error = "invalid docx path";
        return false;
    }

    if (!FileExistsW(srcWide)) {
        documents->Release();
        error = "docx file not found";
        return false;
    }

    bool ok = CallDocumentsOpen(documents, srcWide, &doc, error);
    documents->Release();
    if (!ok) {
        return false;
    }

    ok = CallExportAsPdf(doc, tempPdfPath, error);

    std::string closeError;
    CallCloseDocument(doc, closeError);
    doc->Release();

    if (!ok) {
        DeleteFileW(tempPdfPath.c_str());
        return false;
    }

    return true;
}

void DocxWorker::ThreadMain() {
ComInitGuard com;

    auto cleanup = MakeScopeExit([this] {
        CloseCurrentPdf();
        CloseWordApp();
    });

    while (true) {
        Task task;

        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [&] { return stopping_ || !queue_.empty(); });

            if (queue_.empty()) {
                break;
            }

            task = std::move(queue_.front());
            queue_.pop();
        }

        ProcessTask(task);

        bool shouldStop = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            shouldStop = stopping_;
        }

        if (shouldStop) {
            std::queue<Task> pending;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                pending.swap(queue_);
            }

            while (!pending.empty()) {
                Task t = std::move(pending.front());
                pending.pop();
                if (t.callback) {
                    t.callback("renderer is shutting down");
                }
            }
            break;
        }
    }
}

void DocxWorker::ProcessTask(const Task& task) {
    std::string error;

    if (task.page <= 0) {
        if (task.callback) {
            task.callback("invalid page");
        }
        return;
    }

    const int dpi = dpi_.load(std::memory_order_relaxed);
    const int pageIndex = task.page - 1;

    if (task.srcPath != currentSource_) {
        CloseCurrentPdf();
        currentSource_ = task.srcPath;
    }

    std::wstring tempPdfPath;
    auto it = tempPdfMap_.find(task.srcPath);
    if (it == tempPdfMap_.end()) {
        tempPdfPath = MakeTempPdfPath(task.srcPath);
        tempPdfMap_[task.srcPath] = tempPdfPath;
    } else {
        tempPdfPath = it->second;
    }

    if (!FileExistsW(tempPdfPath)) {
        if (!ConvertDocxToTempPdf(task.srcPath, tempPdfPath, error)) {
            if (task.callback) {
                task.callback(error);
            }
            return;
        }
    }

    if (currentTempPdfWide_ != tempPdfPath || currentPdfDoc_ == nullptr) {
        CloseCurrentPdf();

        std::string tempPdfUtf8 = WideToUtf8(tempPdfPath);
        if (tempPdfUtf8.empty()) {
            error = "failed to convert temp pdf path";
            if (task.callback) {
                task.callback(error);
            }
            return;
        }

        currentPdfDoc_ = FPDF_LoadDocument(tempPdfUtf8.c_str(), nullptr);
        if (!currentPdfDoc_) {
            error = "failed to open temp pdf";
            if (task.callback) {
                task.callback(error);
            }
            return;
        }

        currentTempPdfWide_ = tempPdfPath;
    }

    const int pageCount = FPDF_GetPageCount(currentPdfDoc_);
    if (pageIndex < 0 || pageIndex >= pageCount) {
        if (task.callback) {
            task.callback("page out of range");
        }
        return;
    }

    FPDF_PAGE page = FPDF_LoadPage(currentPdfDoc_, pageIndex);
    if (!page) {
        if (task.callback) {
            task.callback("failed to load pdf page");
        }
        return;
    }

    const double pageWidthPt = FPDF_GetPageWidth(page);
    const double pageHeightPt = FPDF_GetPageHeight(page);

    int width = static_cast<int>(std::lround(pageWidthPt * dpi / 72.0));
    int height = static_cast<int>(std::lround(pageHeightPt * dpi / 72.0));
    width = std::max(width, 1);
    height = std::max(height, 1);

    FPDF_BITMAP bitmap = FPDFBitmap_Create(width, height, 1);
    if (!bitmap) {
        FPDF_ClosePage(page);
        if (task.callback) {
            task.callback("failed to create pdf bitmap");
        }
        return;
    }

    FPDFBitmap_FillRect(bitmap, 0, 0, width, height, 0xFFFFFFFF);
    FPDF_RenderPageBitmap(bitmap, page, 0, 0, width, height, 0, 0);

    const void* buffer = FPDFBitmap_GetBuffer(bitmap);
    const int stride = FPDFBitmap_GetStride(bitmap);

    std::wstring dstWide = GetAbsolutePathW(ToWide(task.targetPath));
    if (dstWide.empty()) {
        FPDFBitmap_Destroy(bitmap);
        FPDF_ClosePage(page);
        if (task.callback) {
            task.callback("invalid target path");
        }
        return;
    }

    if (!WriteBmp32TopDown(dstWide,
                           width,
                           height,
                           static_cast<const uint8_t*>(buffer),
                           static_cast<uint32_t>(stride),
                           static_cast<double>(dpi),
                           static_cast<double>(dpi),
                           error)) {
        FPDFBitmap_Destroy(bitmap);
        FPDF_ClosePage(page);
        if (task.callback) {
            task.callback(error);
        }
        return;
    }

    FPDFBitmap_Destroy(bitmap);
    FPDF_ClosePage(page);

    if (task.callback) {
        task.callback({});
    }
}

} // namespace renderer