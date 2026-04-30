#include "controller/home/FileListView.h"
#include "ui/HomeWindow.h"
#include "counter/Counter.h"

#include <windows.h>
#include <string>
#include <vector>

using namespace controller::home;

FileListViewController::FileListViewController(HomeWindow& win, config::ConfigData& cfg)
    : m_win(win), m_cfg(cfg) {
}

void FileListViewController::Init()
{
    (void)m_cfg;

    // hard test data, giữ nguyên để bạn thay sau
    m_win.m_files.Set({
        {
            "C:/Docs/Test1.docx",
            "Test1.docx",
            "green",
            "1",
            "10"
        },
        {
            "C:/Docs/Test2.docx",
            "Test2.docx",
            "green",
            "1",
            "1"
        },
        {
            "C:/Docs/Test3.docx",
            "Test3.docx",
            "red",
            "5",
            "20"
        },
        {
            "C:/Docs/Test4.docx",
            "Test4.docx",
            "green",
            "10",
            "10"
        },
        {
            "C:/Docs/Test4.docx",
            "Test4.docx",
            "green",
            "10",
            "10"
        },
        {
            "C:/Docs/Test4.docx",
            "Test4.docx",
            "green",
            "10",
            "10"
        },
        {
            "C:/Docs/Test4.docx",
            "Test4.docx",
            "green",
            "10",
            "10"
        }
    });
}

void FileListViewController::Bind()
{
    m_win.m_files.OnChangeRange([](const std::string& path, const std::string& fromRange, const std::string& toRange) {
        OutputDebugStringA(("ChangeRange: " + path + " | " + fromRange + " -> " + toRange + "\n").c_str());
    });

    m_win.m_files.OnMoveUp([](const std::string& path) {
        OutputDebugStringA(("MoveUp: " + path + "\n").c_str());
    });

    m_win.m_files.OnMoveDown([](const std::string& path) {
        OutputDebugStringA(("MoveDown: " + path + "\n").c_str());
    });

    m_win.m_files.OnRemove([](const std::string& path) {
        OutputDebugStringA(("Remove: " + path + "\n").c_str());
    });

    m_win.m_files.OnAdd([](const std::string& path) {
        OutputDebugStringA(("Add: " + path + "\n").c_str());
    });
}
