#pragma once

namespace ui {
namespace home {
namespace Layout {

    // ===== WINDOW =====
    constexpr int MIN_WIDTH             = 450;
    constexpr int MIN_HEIGHT            = 541;

    constexpr int MARGIN                = 5;
    constexpr int SECTION_MARGIN        = 10;
    constexpr int SECTION_GAP           = 5;

    constexpr int LABEL_H               = 12;
    constexpr int INPUT_H               = 20;    
    constexpr int LABEL_GAP             = 3;
    constexpr int ROW_GAP               = 5;
    constexpr int INPUT_BORDER_OFFSET   = 2;    

    // ===== BASIC SECTION =====
    namespace BasicSection {
        constexpr int X = MARGIN;
        constexpr int Y = MARGIN;
        constexpr int W = 220;
        constexpr int H = 2 * SECTION_MARGIN + (LABEL_H + LABEL_GAP + INPUT_H + ROW_GAP) * 2 - ROW_GAP;

        // Printer
        constexpr int PRINTER_LABEL_X = SECTION_MARGIN;
        constexpr int PRINTER_LABEL_Y = SECTION_MARGIN;

        constexpr int PRINTER_INPUT_X = SECTION_MARGIN;
        constexpr int PRINTER_INPUT_Y = SECTION_MARGIN + LABEL_H + LABEL_GAP;

        // Copies
        constexpr int COPIES_LABEL_X = SECTION_MARGIN;
        constexpr int COPIES_LABEL_Y = SECTION_MARGIN + LABEL_H + LABEL_GAP + INPUT_H + ROW_GAP;

        constexpr int COPIES_INPUT_X = SECTION_MARGIN;
        constexpr int COPIES_INPUT_Y = SECTION_MARGIN + LABEL_H + LABEL_GAP + INPUT_H + ROW_GAP + LABEL_H + LABEL_GAP;
    }

} // namespace Layout
namespace Style {
    
    // General
    constexpr COLORREF WINDOW_BG                = RGB(200,200,200);
    

    // Section
    constexpr COLORREF SECTION_BG               = RGB(255,255,255);
    constexpr COLORREF SECTION_BORDER           = RGB(180,180,180);
    

    // Label
    constexpr COLORREF LABEL                    = RGB(0,0,0);
    

    // Input
    constexpr int INPUT_RADIUS = 6;

    constexpr COLORREF INPUT_TEXT               = RGB(0,0,0);
    constexpr COLORREF INPUT_TEXT_HOVER         = RGB(0,0,0);
    constexpr COLORREF INPUT_TEXT_FOCUS         = RGB(0,0,0);
    constexpr COLORREF INPUT_TEXT_DISABLED      = RGB(100,100,100);

    constexpr COLORREF INPUT_BG                 = RGB(255,255,255);
    constexpr COLORREF INPUT_BG_HOVER           = RGB(230,230,230);
    constexpr COLORREF INPUT_BG_FOCUS           = RGB(255,255,255);
    constexpr COLORREF INPUT_BG_DISABLED        = RGB(200,200,200);
    
    constexpr COLORREF INPUT_BORDER             = RGB(180,180,180);
    constexpr COLORREF INPUT_BORDER_HOVER       = RGB(180,180,180);
    constexpr COLORREF INPUT_BORDER_FOCUS       = RGB(180,180,180);
    constexpr COLORREF INPUT_BORDER_DISABLED    = RGB(220,220,220);
    
}
} // namespace HomeWindow
}