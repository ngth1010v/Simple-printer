#pragma once

#include "ui/BaseWindow.h"
#include "ui/home/BasicConfigSection.h"
#include "ui/home/AdvanceConfigSection.h"
#include "ui/home/InfoConfigSection.h"
#include "ui/home/ControlBlock.h"
#include "ui/home/FileListView.h"

class HomeWindow : public BaseWindow {
public:
    bool CreateWindowInstance();

protected:
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) override;

private:
    HFONT m_font = nullptr;

public:
    ui::home::BasicConfigSection m_basic;
    ui::home::AdvanceConfigSection m_adv;
    ui::home::InfoConfigSection m_info;
    ui::home::ControlBlock m_control;
    ui::home::FileListView m_files;

private:
    void OnCreate();
    void OnCommand(WPARAM wParam);
    void OnSize();
    void OnPaint();
};