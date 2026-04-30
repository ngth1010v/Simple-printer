#include "counter/DocxWorker.h"

#include <windows.h>
#include <comdef.h>

#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

namespace counter {

namespace {

// UTF-8 -> UTF-16
static std::wstring ToW(const std::string& s) {
    if (s.empty()) return L"";

    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (len <= 0) return L"";

    std::wstring w;
    w.resize(static_cast<size_t>(len - 1));
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), len);
    return w;
}

// UTF-16 -> UTF-8
static std::string ToA(const std::wstring& s) {
    if (s.empty()) return "";

    int len = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return "";

    std::string a;
    a.resize(static_cast<size_t>(len - 1));
    WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, a.data(), len, nullptr, nullptr);
    return a;
}

static std::wstring GetAbsolutePathW(const std::wstring& path) {
    if (path.empty()) return L"";

    DWORD need = GetFullPathNameW(path.c_str(), 0, nullptr, nullptr);
    if (need == 0) return path;

    std::wstring out;
    out.resize(need);

    DWORD written = GetFullPathNameW(path.c_str(), need, out.data(), nullptr);
    if (written == 0) return path;

    out.resize(written);
    return out;
}

static bool FileExistsW(const std::wstring& path) {
    DWORD attr = GetFileAttributesW(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

static std::string HrToString(HRESULT hr) {
    wchar_t* msg = nullptr;
    DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS;

    DWORD n = FormatMessageW(
        flags,
        nullptr,
        hr,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&msg),
        0,
        nullptr
    );

    if (n && msg) {
        std::wstring wmsg(msg, n);
        LocalFree(msg);
        return ToA(wmsg);
    }

    if (msg) {
        LocalFree(msg);
    }

    wchar_t buf[32];
    swprintf_s(buf, L"0x%08X", static_cast<unsigned>(hr));
    return ToA(buf);
}

static VARIANT MakeBool(bool v) {
    VARIANT var;
    VariantInit(&var);
    var.vt = VT_BOOL;
    var.boolVal = v ? VARIANT_TRUE : VARIANT_FALSE;
    return var;
}

static VARIANT MakeI4(long v) {
    VARIANT var;
    VariantInit(&var);
    var.vt = VT_I4;
    var.lVal = v;
    return var;
}

static VARIANT MakeBstr(const std::wstring& s) {
    VARIANT var;
    VariantInit(&var);
    var.vt = VT_BSTR;
    var.bstrVal = SysAllocString(s.c_str());
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

static HRESULT GetProperty(IDispatch* obj, const wchar_t* name, VARIANT* result) {
    DISPPARAMS params{};
    return InvokeDispatch(obj, name, DISPATCH_PROPERTYGET, &params, result);
}

static HRESULT PutProperty(IDispatch* obj, const wchar_t* name, VARIANT* value) {
    DISPID dispidPut = DISPID_PROPERTYPUT;

    DISPPARAMS params{};
    params.cArgs = 1;
    params.rgvarg = value;
    params.cNamedArgs = 1;
    params.rgdispidNamedArgs = &dispidPut;

    return InvokeDispatch(obj, name, DISPATCH_PROPERTYPUT, &params, nullptr);
}

static HRESULT CallMethod(IDispatch* obj, const wchar_t* name, VARIANT* args, UINT argCount, VARIANT* result = nullptr) {
    DISPPARAMS params{};
    params.cArgs = argCount;
    params.rgvarg = args;

    return InvokeDispatch(obj, name, DISPATCH_METHOD, &params, result);
}

static IDispatch* CreateWordApp(std::string& error) {
    CLSID clsid{};
    HRESULT hr = CLSIDFromProgID(L"Word.Application", &clsid);
    if (FAILED(hr)) {
        error = "CLSIDFromProgID(Word.Application) failed: " + HrToString(hr);
        return nullptr;
    }

    IDispatch* word = nullptr;
    hr = CoCreateInstance(
        clsid,
        nullptr,
        CLSCTX_LOCAL_SERVER,
        IID_IDispatch,
        reinterpret_cast<void**>(&word)
    );

    if (FAILED(hr) || !word) {
        error = "CoCreateInstance(Word.Application) failed: " + HrToString(hr);
        return nullptr;
    }

    // Best-effort settings; ignore failures here because Word can still work without them.
    {
        VARIANT v = MakeBool(false);
        PutProperty(word, L"Visible", &v);
        VariantClear(&v);
    }
    {
        VARIANT v = MakeI4(0); // wdAlertsNone
        PutProperty(word, L"DisplayAlerts", &v);
        VariantClear(&v);
    }
    {
        VARIANT v = MakeI4(3); // msoAutomationSecurityForceDisable
        PutProperty(word, L"AutomationSecurity", &v);
        VariantClear(&v);
    }

    return word;
}

} // namespace

DocxWorker::DocxWorker() {}

DocxWorker::~DocxWorker() {
    Stop();
}

bool DocxWorker::CheckWordInstalled() {
    CLSID clsid{};
    return SUCCEEDED(CLSIDFromProgID(L"Word.Application", &clsid));
}

void DocxWorker::Start() {
    if (m_running.exchange(true)) {
        return;
    }

    m_hasWord = CheckWordInstalled();
    m_thread = std::thread(&DocxWorker::WorkerLoop, this);
}

void DocxWorker::Stop() {
    if (!m_running.exchange(false)) {
        return;
    }

    m_cv.notify_all();
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void DocxWorker::Enqueue(const std::string& path, Callback cb) {
    if (!cb) {
        return;
    }

    if (!m_running) {
        cb(0, "DocxWorker not started");
        return;
    }

    if (!m_hasWord) {
        cb(0, "Missing Word");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(Task{path, std::move(cb)});
    }

    m_cv.notify_one();
}

void DocxWorker::WorkerLoop() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        const std::string err = "COM init failed: " + HrToString(hr);
        while (m_running) {
            Task task;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait(lock, [&]() { return !m_running || !m_queue.empty(); });
                if (!m_running && m_queue.empty()) break;
                task = m_queue.front();
                m_queue.pop();
            }
            if (task.cb) task.cb(0, err);
        }
        return;
    }

    struct CoUninitGuard {
        ~CoUninitGuard() { CoUninitialize(); }
    } coGuard;

    std::string createError;
    IDispatch* word = CreateWordApp(createError);
    if (!word) {
        while (m_running) {
            Task task;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait(lock, [&]() { return !m_running || !m_queue.empty(); });
                if (!m_running && m_queue.empty()) break;
                task = m_queue.front();
                m_queue.pop();
            }
            if (task.cb) task.cb(0, createError);
        }
        return;
    }

    while (m_running) {
        Task task;

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [&]() { return !m_running || !m_queue.empty(); });

            if (!m_running && m_queue.empty()) {
                break;
            }

            task = m_queue.front();
            m_queue.pop();
        }

        int pages = 0;
        std::string error;

        do {
            const std::wstring absW = GetAbsolutePathW(ToW(task.path));
            if (absW.empty()) {
                error = "Empty path";
                break;
            }

            if (!FileExistsW(absW)) {
                error = "File not found: " + ToA(absW);
                break;
            }

            VARIANT docsVar;
            VariantInit(&docsVar);

            HRESULT hrDocs = GetProperty(word, L"Documents", &docsVar);
            if (FAILED(hrDocs) || docsVar.vt != VT_DISPATCH || !docsVar.pdispVal) {
                error = "Documents failed: " + HrToString(hrDocs);
                VariantClear(&docsVar);
                break;
            }

            IDispatch* docs = docsVar.pdispVal;

            // Open only needs FileName here.
            VARIANT openArg = MakeBstr(absW);
            VARIANT docVar;
            VariantInit(&docVar);

            HRESULT hrOpen = CallMethod(docs, L"Open", &openArg, 1, &docVar);
            VariantClear(&openArg);

            if (FAILED(hrOpen) || docVar.vt != VT_DISPATCH || !docVar.pdispVal) {
                error = "Open failed: " + HrToString(hrOpen);
                VariantClear(&docVar);
                VariantClear(&docsVar);
                break;
            }

            IDispatch* doc = docVar.pdispVal;

            VARIANT statArg = MakeI4(2); // wdStatisticPages
            VARIANT statRes;
            VariantInit(&statRes);

            HRESULT hrStat = CallMethod(doc, L"ComputeStatistics", &statArg, 1, &statRes);
            VariantClear(&statArg);

            if (FAILED(hrStat)) {
                error = "ComputeStatistics failed: " + HrToString(hrStat);
            } else if (statRes.vt == VT_I4) {
                pages = statRes.lVal;
            } else if (statRes.vt == VT_I2) {
                pages = statRes.iVal;
            } else {
                error = "ComputeStatistics returned unexpected type";
            }

            VariantClear(&statRes);

            // Close without saving to avoid prompts.
            VARIANT closeArg = MakeBool(false);
            CallMethod(doc, L"Close", &closeArg, 1, nullptr);
            VariantClear(&closeArg);

            VariantClear(&docVar);
            VariantClear(&docsVar);

        } while (false);

        if (pages <= 0 && error.empty()) {
            error = "Unknown docx error";
        }

        if (task.cb) {
            task.cb(pages, error);
        }
    }

    // Quit Word cleanly.
    {
        VARIANT quitArg = MakeBool(false);
        CallMethod(word, L"Quit", &quitArg, 1, nullptr);
        VariantClear(&quitArg);
    }

    word->Release();
}

} // namespace counter