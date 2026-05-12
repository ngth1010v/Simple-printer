// counter/WordWorker.cpp
#include "counter/DocxWorker.h"

#include <windows.h>
#include <oleauto.h>

#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <utility>
#include <iostream>

namespace counter {

namespace {

static std::string ToA(const std::wstring& s) {
    if (s.empty()) return "";

    int len = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return "";

    std::string a;
    a.resize(static_cast<size_t>(len - 1));

    WideCharToMultiByte(
        CP_UTF8,
        0,
        s.c_str(),
        -1,
        a.data(),
        len,
        nullptr,
        nullptr
    );

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
    return attr != INVALID_FILE_ATTRIBUTES &&
           !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

static std::string HrToString(HRESULT hr) {
    wchar_t* msg = nullptr;

    DWORD flags =
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
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

template <typename T>
class ComPtr {
public:
    ComPtr() = default;

    explicit ComPtr(T* p)
        : m_ptr(p) {}

    ~ComPtr() {
        reset();
    }

    ComPtr(const ComPtr&) = delete;
    ComPtr& operator=(const ComPtr&) = delete;

    ComPtr(ComPtr&& other) noexcept
        : m_ptr(other.m_ptr) {
        other.m_ptr = nullptr;
    }

    ComPtr& operator=(ComPtr&& other) noexcept {
        if (this != &other) {
            reset();
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr;
        }

        return *this;
    }

    T* get() const {
        return m_ptr;
    }

    T** put() {
        reset();
        return &m_ptr;
    }

    T* detach() {
        T* tmp = m_ptr;
        m_ptr = nullptr;
        return tmp;
    }

    void reset(T* p = nullptr) {
        if (m_ptr) {
            m_ptr->Release();
        }

        m_ptr = p;
    }

    T* operator->() const {
        return m_ptr;
    }

    explicit operator bool() const {
        return m_ptr != nullptr;
    }

private:
    T* m_ptr = nullptr;
};

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

    VARIANT* get() {
        return &m_var;
    }

    const VARIANT* get() const {
        return &m_var;
    }

    VARIANT* operator&() = delete;

    VARIANT& ref() {
        return m_var;
    }

private:
    VARIANT m_var;
};

struct CoInitGuard {
    bool ok = false;

    explicit CoInitGuard(HRESULT hr)
        : ok(SUCCEEDED(hr)) {}

    ~CoInitGuard() {
        if (ok) {
            CoUninitialize();
        }
    }
};

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
    if (!obj || !name) {
        return E_POINTER;
    }

    LPOLESTR names[] = {
        const_cast<LPOLESTR>(name)
    };

    DISPID dispid = 0;

    HRESULT hr = obj->GetIDsOfNames(
        IID_NULL,
        names,
        1,
        LOCALE_USER_DEFAULT,
        &dispid
    );

    if (FAILED(hr)) {
        return hr;
    }

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

static HRESULT GetProperty(
    IDispatch* obj,
    const wchar_t* name,
    VARIANT* result
) {
    DISPPARAMS params{};
    return InvokeDispatch(
        obj,
        name,
        DISPATCH_PROPERTYGET,
        &params,
        result
    );
}

static HRESULT PutProperty(
    IDispatch* obj,
    const wchar_t* name,
    VARIANT* value
) {
    DISPID dispidPut = DISPID_PROPERTYPUT;

    DISPPARAMS params{};
    params.cArgs = 1;
    params.rgvarg = value;
    params.cNamedArgs = 1;
    params.rgdispidNamedArgs = &dispidPut;

    return InvokeDispatch(
        obj,
        name,
        DISPATCH_PROPERTYPUT,
        &params,
        nullptr
    );
}

static HRESULT CallMethod(
    IDispatch* obj,
    const wchar_t* name,
    VARIANT* args,
    UINT argCount,
    VARIANT* result = nullptr
) {
    DISPPARAMS params{};
    params.cArgs = argCount;
    params.rgvarg = args;

    return InvokeDispatch(
        obj,
        name,
        DISPATCH_METHOD,
        &params,
        result
    );
}

static IDispatch* CreateWordApp(std::string& error) {
    CLSID clsid{};

    HRESULT hr = CLSIDFromProgID(
        L"Word.Application",
        &clsid
    );

    if (FAILED(hr)) {
        error =
            "CLSIDFromProgID(Word.Application) failed: " +
            HrToString(hr);

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
        error =
            "CoCreateInstance(Word.Application) failed: " +
            HrToString(hr);

        return nullptr;
    }

    {
        VARIANT v = MakeBool(false);
        PutProperty(word, L"Visible", &v);
        VariantClear(&v);
    }

    {
        VARIANT v = MakeI4(0);
        PutProperty(word, L"DisplayAlerts", &v);
        VariantClear(&v);
    }

    {
        VARIANT v = MakeI4(3);
        PutProperty(word, L"AutomationSecurity", &v);
        VariantClear(&v);
    }

    return word;
}

static void SafeInvokeCallback(
    const DocxWorker::Callback& cb,
    int pages,
    const std::string& error
) noexcept {
    if (!cb) {
        return;
    }

    try {
        cb(pages, error);
    } catch (...) {
    }
}

} // namespace

DocxWorker::DocxWorker() {}

DocxWorker::~DocxWorker() {
    Stop();
}

bool DocxWorker::CheckWordInstalled() {
    CLSID clsid{};
    return SUCCEEDED(
        CLSIDFromProgID(
            L"Word.Application",
            &clsid
        )
    );
}

void DocxWorker::Start() {
    bool expected = false;

    if (!m_running.compare_exchange_strong(expected, true)) {
        return;
    }

    m_hasWord = CheckWordInstalled();

    m_thread = std::thread(
        &DocxWorker::WorkerLoop,
        this
    );
}

void DocxWorker::Stop() {
    bool expected = true;

    if (!m_running.compare_exchange_strong(expected, false)) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::queue<Task> empty;
        std::swap(m_queue, empty);
    }

    m_cv.notify_all();

    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void DocxWorker::Enqueue(
    const std::wstring& path,
    Callback cb
) {
    if (!cb) {
        return;
    }

    if (!m_running.load()) {
        cb(0, "DocxWorker not started");
        return;
    }

    if (!m_hasWord) {
        cb(0, "Missing Word");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(Task{
            path,
            std::move(cb)
        });
    }

    m_cv.notify_one();
}

void DocxWorker::WorkerLoop() {
    HRESULT hr = CoInitializeEx(
        nullptr,
        COINIT_APARTMENTTHREADED
    );

    CoInitGuard coGuard(hr);

    if (FAILED(hr)) {
        const std::string err =
            "COM init failed: " +
            HrToString(hr);

        while (true) {
            Task task;

            {
                std::unique_lock<std::mutex> lock(m_mutex);

                m_cv.wait(lock, [&]() {
                    return !m_running.load() ||
                           !m_queue.empty();
                });

                if (!m_running.load()) {
                    break;
                }

                task = std::move(m_queue.front());
                m_queue.pop();
            }

            SafeInvokeCallback(task.cb, 0, err);
        }

        return;
    }

    std::string createError;

    ComPtr<IDispatch> word(
        CreateWordApp(createError)
    );

    if (!word) {
        while (true) {
            Task task;

            {
                std::unique_lock<std::mutex> lock(m_mutex);

                m_cv.wait(lock, [&]() {
                    return !m_running.load() ||
                           !m_queue.empty();
                });

                if (!m_running.load()) {
                    break;
                }

                task = std::move(m_queue.front());
                m_queue.pop();
            }

            SafeInvokeCallback(
                task.cb,
                0,
                createError
            );
        }

        return;
    }

    while (true) {
        Task task;

        {
            std::unique_lock<std::mutex> lock(m_mutex);

            m_cv.wait(lock, [&]() {
                return !m_running.load() ||
                       !m_queue.empty();
            });

            if (!m_running.load()) {
                break;
            }

            task = std::move(m_queue.front());
            m_queue.pop();
        }

        int pages = 0;
        std::string error;

        do {
            const std::wstring absW =
                GetAbsolutePathW(task.path);

            if (absW.empty()) {
                error = "Empty path";
                break;
            }

            if (!FileExistsW(absW)) {
                error =
                    "File not found: " +
                    ToA(absW);

                break;
            }

            VariantHolder docsVar;

            HRESULT hrDocs = GetProperty(
                word.get(),
                L"Documents",
                docsVar.get()
            );

            if (
                FAILED(hrDocs) ||
                docsVar.ref().vt != VT_DISPATCH ||
                !docsVar.ref().pdispVal
            ) {
                error =
                    "Documents failed: " +
                    HrToString(hrDocs);

                break;
            }

            ComPtr<IDispatch> docs(
                docsVar.ref().pdispVal
            );

            docsVar.ref().pdispVal->AddRef();

            VARIANT args[3];

            for (int i = 0; i < 3; ++i) {
                VariantInit(&args[i]);
            }

            args[0] = MakeBool(true);
            args[1].vt = VT_ERROR;
            args[1].scode = DISP_E_PARAMNOTFOUND;
            args[2] = MakeBstr(absW);

            VariantHolder docVar;

            HRESULT hrOpen = CallMethod(
                docs.get(),
                L"Open",
                args,
                3,
                docVar.get()
            );

            for (int i = 0; i < 3; ++i) {
                VariantClear(&args[i]);
            }

            if (
                FAILED(hrOpen) ||
                docVar.ref().vt != VT_DISPATCH ||
                !docVar.ref().pdispVal
            ) {
                error =
                    "Open failed: " +
                    HrToString(hrOpen);

                std::cout
                    << HrToString(hrOpen)
                    << "[" << hrOpen << "]\n";

                break;
            }

            ComPtr<IDispatch> doc(
                docVar.ref().pdispVal
            );

            docVar.ref().pdispVal->AddRef();

            struct DocCloseGuard {
                IDispatch* doc = nullptr;

                ~DocCloseGuard() {
                    if (!doc) {
                        return;
                    }

                    VariantHolder closeArg;
                    closeArg.ref() = MakeBool(false);

                    CallMethod(
                        doc,
                        L"Close",
                        closeArg.get(),
                        1,
                        nullptr
                    );
                }
            } closeGuard{doc.get()};

            VariantHolder statArg;
            statArg.ref() = MakeI4(2);

            VariantHolder statRes;

            HRESULT hrStat = CallMethod(
                doc.get(),
                L"ComputeStatistics",
                statArg.get(),
                1,
                statRes.get()
            );

            if (FAILED(hrStat)) {
                error =
                    "ComputeStatistics failed: " +
                    HrToString(hrStat);
            } else if (statRes.ref().vt == VT_I4) {
                pages = statRes.ref().lVal;
            } else if (statRes.ref().vt == VT_I2) {
                pages = statRes.ref().iVal;
            } else {
                error =
                    "ComputeStatistics returned unexpected type";
            }

        } while (false);

        if (pages <= 0 && error.empty()) {
            error = "Unknown docx error";
        }

        SafeInvokeCallback(
            task.cb,
            pages,
            error
        );
    }

    if (word) {
        VariantHolder quitArg;
        quitArg.ref() = MakeBool(false);

        CallMethod(
            word.get(),
            L"Quit",
            quitArg.get(),
            1,
            nullptr
        );

        word.reset();
        CoFreeUnusedLibraries();
    }
}

int DocxWorker::CountWithWord(
    const std::wstring&
) {
    return 0;
}

} // namespace counter