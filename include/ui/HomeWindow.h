#pragma once

#include "BaseWindow.h"
#include "BasicConfigSection.h"
#include "AdvanceConfigSection.h"
#include "MarginConfigSection.h"
#include "InfoConfigSection.h"
#include "ControlBlock.h"
#include "FileListView.h"

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
    ui::home::MarginConfigSection m_margin;
    ui::home::InfoConfigSection m_info;
    ui::home::ControlBlock m_control;
    ui::home::FileListView m_files;

private:
    void OnCreate();
    void OnCommand(WPARAM wParam);
    void OnSize();
    void OnPaint();
};