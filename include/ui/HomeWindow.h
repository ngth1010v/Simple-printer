#pragma once

#include "BaseWindow.h"
#include "BasicConfigSection.h"
#include "AdvanceConfigSection.h"
#include "MarginConfigSection.h"

class HomeWindow : public BaseWindow {
public:
    bool CreateWindowInstance();

protected:
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) override;

private:
    HFONT m_font = nullptr;
    ui::home::BasicConfigSection m_basic;
    ui::home::AdvanceConfigSection m_adv;
    ui::home::MarginConfigSection m_margin;

private:
    void OnCreate();
    void OnCommand(WPARAM wParam);
    void OnSize();
    void OnPaint();
};