#include <windows.h>
#include <shellapi.h>

#include <string>
#include <vector>
#include <sstream>

#define MUTEX_NAME L"Global\\SimplePrinterLauncherMutex"
#define FILE_PATH  L"C:\\Temp\\SimplePrinter_args.txt"

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

    if (!text.empty()) {
        WriteFile(
            hFile,
            text.data(),
            static_cast<DWORD>(text.size() * sizeof(wchar_t)),
            &written,
            nullptr
        );
    }

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

std::wstring ReadAllArgsFileLocked()
{
    return ReadWholeUtf16File(FILE_PATH);
}

bool WriteAllArgsFileLocked(const std::wstring& text)
{
    return WriteTextUtf16(FILE_PATH, text);
}

bool AppendArgLocked(HANDLE hMutex, const std::wstring& arg)
{
    if (WaitForSingleObject(hMutex, INFINITE) != WAIT_OBJECT_0) {
        return false;
    }

    std::wstring current = ReadAllArgsFileLocked();
    current += arg;
    current += L"\r\n";

    bool ok = WriteAllArgsFileLocked(current);

    ReleaseMutex(hMutex);
    return ok;
}

std::vector<std::wstring> CollectArgsLocked(HANDLE hMutex)
{
    std::vector<std::wstring> args;

    if (WaitForSingleObject(hMutex, INFINITE) != WAIT_OBJECT_0) {
        return args;
    }

    std::wstring text = ReadAllArgsFileLocked();
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

std::wstring BuildCommand(
    const std::wstring& exePath,
    const std::vector<std::wstring>& args,
    bool isPrint
)
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

bool LaunchProcess(const std::wstring& cmdLine, const std::wstring& exeDir)
{
    std::wstring mutableCmd = cmdLine;

    STARTUPINFOW si{};
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi{};

    BOOL ok = CreateProcessW(
        nullptr,
        &mutableCmd[0],
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        exeDir.c_str(),
        &si,
        &pi
    );

    if (!ok) {
        return false;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    EnsureTempDir();

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) {
        return 1;
    }

    bool isPrint = false;
    std::vector<std::wstring> inputArgs;

    for (int i = 1; i < argc; ++i) {
        std::wstring arg = argv[i] ? argv[i] : L"";
        if (arg == L"--print") {
            isPrint = true;
        } else if (!arg.empty()) {
            inputArgs.push_back(arg);
        }
    }

    LocalFree(argv);

    if (inputArgs.empty()) {
        return 0;
    }

    HANDLE hMutex = CreateMutexW(nullptr, FALSE, MUTEX_NAME);
    if (!hMutex) {
        return 1;
    }

    DWORD mutexErr = GetLastError();
    bool isFirst = (mutexErr != ERROR_ALREADY_EXISTS);

    for (const auto& a : inputArgs) {
        if (!AppendArgLocked(hMutex, a)) {
            CloseHandle(hMutex);
            return 1;
        }
    }

    if (!isFirst) {
        CloseHandle(hMutex);
        return 0;
    }

    WaitForFileQuiet(120, 1000);

    std::vector<std::wstring> args = CollectArgsLocked(hMutex);
    CloseHandle(hMutex);

    if (args.empty()) {
        return 0;
    }

    std::wstring exeDir = GetExeDir();
    std::wstring printerExe = exeDir + L"\\SimplePrinter.exe";
    std::wstring cmd = BuildCommand(printerExe, args, isPrint);

    if (!LaunchProcess(cmd, exeDir)) {
        return 1;
    }
 
    return 0;
}