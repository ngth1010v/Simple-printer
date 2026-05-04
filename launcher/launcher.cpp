#include <windows.h>

#include <string>
#include <vector>
#include <sstream>

#define MUTEX_NAME L"Global\\SimplePrinterLauncherMutex"
#define FILE_PATH  L"C:\\Temp\\SimplePrinter_args.txt"
#define LOG_PATH   L"C:\\Temp\\SimplePrinterLauncher.log"

// ===== Utils =====
std::wstring GetExeDir()
{
    wchar_t path[MAX_PATH] = {};
    DWORD len = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        return L".";
    }

    std::wstring full(path);
    size_t pos = full.find_last_of(L"\\/");
    if (pos == std::wstring::npos) {
        return L".";
    }
    return full.substr(0, pos);
}

void EnsureTempDir()
{
    CreateDirectoryW(L"C:\\Temp", nullptr);
}

std::wstring GetTimeStamp()
{
    SYSTEMTIME st{};
    GetLocalTime(&st);

    wchar_t buf[64] = {};
    swprintf(
        buf,
        64,
        L"%04u-%02u-%02u %02u:%02u:%02u.%03u",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds
    );

    return buf;
}

void AppendUtf16LineToFile(const wchar_t* path, const std::wstring& line)
{
    HANDLE hFile = CreateFileW(
        path,
        FILE_APPEND_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        return;
    }

    LARGE_INTEGER size{};
    if (GetFileSizeEx(hFile, &size) && size.QuadPart == 0) {
        // Write UTF-16 BOM once for a new file
        const wchar_t bom = 0xFEFF;
        DWORD written = 0;
        WriteFile(hFile, &bom, sizeof(bom), &written, nullptr);
    }

    std::wstring text = line + L"\r\n";
    DWORD written = 0;
    WriteFile(
        hFile,
        text.data(),
        static_cast<DWORD>(text.size() * sizeof(wchar_t)),
        &written,
        nullptr
    );

    CloseHandle(hFile);
}

void LogLine(const std::wstring& msg)
{
    std::wstring line = L"[" + GetTimeStamp() + L"] " + msg;

    // Always write to UTF-16 log file
    AppendUtf16LineToFile(LOG_PATH, line);

    // Debug output for debugger
    OutputDebugStringW((line + L"\r\n").c_str());

    // Console output only if a real console exists
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE && hOut != nullptr) {
        DWORD mode = 0;
        if (GetConsoleMode(hOut, &mode)) {
            DWORD written = 0;
            std::wstring consoleLine = line + L"\r\n";
            WriteConsoleW(hOut, consoleLine.c_str(),
                          static_cast<DWORD>(consoleLine.size()),
                          &written, nullptr);
        }
    }
}

bool WriteTextUtf16(const wchar_t* path, const std::wstring& text)
{
    HANDLE hFile = CreateFileW(
        path,
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    const wchar_t bom = 0xFEFF;
    DWORD written = 0;
    WriteFile(hFile, &bom, sizeof(bom), &written, nullptr);

    WriteFile(
        hFile,
        text.data(),
        static_cast<DWORD>(text.size() * sizeof(wchar_t)),
        &written,
        nullptr
    );

    CloseHandle(hFile);
    return true;
}

std::wstring ReadWholeUtf16File(const wchar_t* path)
{
    HANDLE hFile = CreateFileW(
        path,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        return L"";
    }

    LARGE_INTEGER size{};
    if (!GetFileSizeEx(hFile, &size) || size.QuadPart <= 0) {
        CloseHandle(hFile);
        return L"";
    }

    DWORD bytesToRead = static_cast<DWORD>(size.QuadPart);
    std::vector<wchar_t> buf((bytesToRead / sizeof(wchar_t)) + 2, 0);

    DWORD bytesRead = 0;
    BOOL ok = ReadFile(hFile, buf.data(), bytesToRead, &bytesRead, nullptr);
    CloseHandle(hFile);

    if (!ok || bytesRead < sizeof(wchar_t)) {
        return L"";
    }

    size_t count = bytesRead / sizeof(wchar_t);
    size_t start = 0;

    // Strip UTF-16 BOM if present
    if (count > 0 && buf[0] == 0xFEFF) {
        start = 1;
    }

    if (start >= count) {
        return L"";
    }

    return std::wstring(buf.data() + start, count - start);
}

std::vector<std::wstring> SplitLines(const std::wstring& text)
{
    std::vector<std::wstring> lines;

    size_t i = 0;
    while (i < text.size()) {
        size_t j = i;
        while (j < text.size() && text[j] != L'\r' && text[j] != L'\n') {
            ++j;
        }

        if (j > i) {
            lines.push_back(text.substr(i, j - i));
        }

        if (j >= text.size()) {
            break;
        }

        if (text[j] == L'\r' && (j + 1) < text.size() && text[j + 1] == L'\n') {
            i = j + 2;
        } else {
            i = j + 1;
        }
    }

    return lines;
}

bool AppendArgLocked(HANDLE hMutex, const std::wstring& arg)
{
    if (WaitForSingleObject(hMutex, INFINITE) != WAIT_OBJECT_0) {
        return false;
    }

    std::wstring line = arg + L"\r\n";
    bool ok = WriteTextUtf16(FILE_PATH, ReadWholeUtf16File(FILE_PATH) + line);

    ReleaseMutex(hMutex);
    return ok;
}

std::vector<std::wstring> CollectArgsLocked(HANDLE hMutex)
{
    std::vector<std::wstring> args;

    if (WaitForSingleObject(hMutex, INFINITE) != WAIT_OBJECT_0) {
        return args;
    }

    std::wstring text = ReadWholeUtf16File(FILE_PATH);
    args = SplitLines(text);

    DeleteFileW(FILE_PATH);

    ReleaseMutex(hMutex);
    return args;
}

bool WaitForFileQuiet(DWORD quietMs, DWORD maxMs)
{
    ULONGLONG start = GetTickCount64();
    ULONGLONG lastChange = start;
    ULONGLONG lastWrite = 0;
    bool hasLastWrite = false;

    while (true) {
        WIN32_FILE_ATTRIBUTE_DATA fad{};
        if (GetFileAttributesExW(FILE_PATH, GetFileExInfoStandard, &fad)) {
            ULARGE_INTEGER t{};
            t.LowPart = fad.ftLastWriteTime.dwLowDateTime;
            t.HighPart = fad.ftLastWriteTime.dwHighDateTime;

            if (!hasLastWrite || t.QuadPart != lastWrite) {
                hasLastWrite = true;
                lastWrite = t.QuadPart;
                lastChange = GetTickCount64();
            }

            if (GetTickCount64() - lastChange >= quietMs) {
                return true;
            }
        } else {
            if (GetLastError() == ERROR_FILE_NOT_FOUND) {
                if (GetTickCount64() - start >= quietMs) {
                    return true;
                }
            }
        }

        if (GetTickCount64() - start >= maxMs) {
            return true;
        }

        Sleep(20);
    }
}

std::wstring QuoteWindowsArg(const std::wstring& arg)
{
    if (arg.empty()) {
        return L"\"\"";
    }

    bool needQuotes = false;
    for (wchar_t c : arg) {
        if (c == L' ' || c == L'\t' || c == L'\n' || c == L'\v' || c == L'"') {
            needQuotes = true;
            break;
        }
    }

    if (!needQuotes) {
        return arg;
    }

    std::wstring out;
    out.push_back(L'"');

    size_t backslashes = 0;
    for (wchar_t c : arg) {
        if (c == L'\\') {
            ++backslashes;
        } else if (c == L'"') {
            out.append(backslashes * 2 + 1, L'\\');
            out.push_back(L'"');
            backslashes = 0;
        } else {
            out.append(backslashes, L'\\');
            backslashes = 0;
            out.push_back(c);
        }
    }

    out.append(backslashes * 2, L'\\');
    out.push_back(L'"');

    return out;
}

std::wstring BuildCommand(const std::wstring& exePath,
                          const std::vector<std::wstring>& args,
                          bool isPrint)
{
    std::wstringstream ss;

    ss << QuoteWindowsArg(exePath);

    if (isPrint) {
        ss << L" --print";
    }

    for (const auto& a : args) {
        ss << L" " << QuoteWindowsArg(a);
    }

    return ss.str();
}

bool LaunchProcess(const std::wstring& cmdLine)
{
    std::wstring mutableCmd = cmdLine;

    STARTUPINFOW si{};
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi{};

    BOOL ok = CreateProcessW(
        nullptr,
        mutableCmd.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    if (!ok) {
        DWORD err = GetLastError();
        std::wstringstream ss;
        ss << L"CreateProcessW failed, err=" << err;
        LogLine(ss.str());
        return false;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

// ===== Entry =====
int wmain(int argc, wchar_t* argv[])
{
    EnsureTempDir();

    LogLine(L"Launcher started");
    {
        std::wstringstream ss;
        ss << L"argc = " << argc;
        LogLine(ss.str());
    }

    bool isPrint = false;
    std::vector<std::wstring> inputArgs;

    for (int i = 1; i < argc; ++i) {
        std::wstring arg = argv[i] ? argv[i] : L"";

        {
            std::wstringstream ss;
            ss << L"argv[" << i << L"] = " << arg;
            LogLine(ss.str());
        }

        if (arg == L"--print") {
            isPrint = true;
        } else if (!arg.empty()) {
            inputArgs.push_back(arg);
        }
    }

    {
        std::wstringstream ss;
        ss << L"isPrint = " << (isPrint ? L"true" : L"false");
        LogLine(ss.str());
    }

    if (inputArgs.empty()) {
        LogLine(L"No file argument, exit.");
        return 0;
    }

    HANDLE hMutex = CreateMutexW(nullptr, FALSE, MUTEX_NAME);
    if (!hMutex) {
        DWORD err = GetLastError();
        std::wstringstream ss;
        ss << L"CreateMutexW failed, err=" << err;
        LogLine(ss.str());
        return 1;
    }

    DWORD mutexErr = GetLastError();
    bool isFirst = (mutexErr != ERROR_ALREADY_EXISTS);

    {
        std::wstringstream ss;
        ss << L"isFirst = " << (isFirst ? L"true" : L"false");
        LogLine(ss.str());
    }

    // Append all file args to shared temp file
    for (const auto& a : inputArgs) {
        if (WaitForSingleObject(hMutex, INFINITE) != WAIT_OBJECT_0) {
            LogLine(L"WaitForSingleObject failed while appending.");
            CloseHandle(hMutex);
            return 1;
        }

        std::wstring current = ReadWholeUtf16File(FILE_PATH);
        current += a;
        current += L"\r\n";
        WriteTextUtf16(FILE_PATH, current);

        ReleaseMutex(hMutex);
    }

    if (!isFirst) {
        LogLine(L"Secondary instance -> exit.");
        CloseHandle(hMutex);
        return 0;
    }

    // Leader waits a short quiet period so other instances can append
    WaitForFileQuiet(120, 1000);

    std::vector<std::wstring> args = CollectArgsLocked(hMutex);
    CloseHandle(hMutex);

    {
        std::wstringstream ss;
        ss << L"Collected args count = " << args.size();
        LogLine(ss.str());
    }

    if (args.empty()) {
        LogLine(L"No collected args, exit.");
        return 0;
    }

    std::wstring exeDir = GetExeDir();
    std::wstring printerExe = exeDir + L"\\SimplePrinter.exe";

    std::wstring cmd = BuildCommand(printerExe, args, isPrint);

    {
        std::wstring line = L"CMD = " + cmd;
        LogLine(line);
    }

    if (!LaunchProcess(cmd)) {
        return 1;
    }

    LogLine(L"Launcher finished.");
    return 0;
}