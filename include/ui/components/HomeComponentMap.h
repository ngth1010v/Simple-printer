#pragma once

namespace ui {
namespace home {
namespace Layout {

    // ===== WINDOW =====
    constexpr int MIN_WIDTH             = 470;
    constexpr int MIN_HEIGHT            = 538;

    constexpr int MARGIN                = 5;
    constexpr int SECTION_MARGIN        = 10;
    constexpr int SECTION_GAP           = 5;

    constexpr int LABEL_H               = 12;
    constexpr int INPUT_H               = 20;    
    constexpr int IMAGE_H               = 48;    
    constexpr int INPUT_GAP             = 3;
    constexpr int ROW_GAP               = 5;
    constexpr int INPUT_BORDER_OFFSET   = 2;    

    // ===== BASIC SECTION =====
    namespace BasicSection {
        constexpr int X = MARGIN;
        constexpr int Y = MARGIN;
        constexpr int W = 250;
        constexpr int H = 2 * SECTION_MARGIN + (LABEL_H + INPUT_GAP + INPUT_H + ROW_GAP) * 2 - ROW_GAP;

        // Printer
        constexpr int PRINTER_LABEL_X = SECTION_MARGIN;
        constexpr int PRINTER_LABEL_Y = SECTION_MARGIN;

        constexpr int PRINTER_INPUT_X = SECTION_MARGIN;
        constexpr int PRINTER_INPUT_Y = SECTION_MARGIN + LABEL_H + INPUT_GAP;

        // Copies
        constexpr int COPIES_LABEL_X = SECTION_MARGIN;
        constexpr int COPIES_LABEL_Y = SECTION_MARGIN + LABEL_H + INPUT_GAP + INPUT_H + ROW_GAP;

        constexpr int COPIES_INPUT_X = SECTION_MARGIN;
        constexpr int COPIES_INPUT_Y = SECTION_MARGIN + LABEL_H + INPUT_GAP + INPUT_H + ROW_GAP + LABEL_H + INPUT_GAP;
    }

    // ===== ADVANCE SECTION =====
    namespace AdvaceSection {
        constexpr int X = MARGIN;
        constexpr int Y = BasicSection::Y + BasicSection::H + SECTION_GAP;
        constexpr int W = 250;
        constexpr int H = 2 * SECTION_MARGIN + (LABEL_H + INPUT_GAP + INPUT_H + ROW_GAP) * 5 - ROW_GAP + IMAGE_H + INPUT_GAP;

        // PrintMode
        constexpr int PRINT_MODE_LABEL_X = SECTION_MARGIN;
        constexpr int PRINT_MODE_LABEL_Y = SECTION_MARGIN;

        constexpr int PRINT_MODE_INPUT_X = SECTION_MARGIN;
        constexpr int PRINT_MODE_INPUT_Y = PRINT_MODE_LABEL_Y + LABEL_H + INPUT_GAP;

        constexpr int PRINT_MODE_IMAGE_X = SECTION_MARGIN;
        constexpr int PRINT_MODE_IMAGE_Y = PRINT_MODE_INPUT_Y + INPUT_H + INPUT_GAP;

        // Paper
        constexpr int PAPER_LABEL_X = SECTION_MARGIN;
        constexpr int PAPER_LABEL_Y = PRINT_MODE_IMAGE_Y + IMAGE_H + ROW_GAP;

        constexpr int PAPER_INPUT_X = SECTION_MARGIN;
        constexpr int PAPER_INPUT_Y = PAPER_LABEL_Y + LABEL_H + INPUT_GAP;

        // Scale
        constexpr int SCALE_LABEL_X = SECTION_MARGIN;
        constexpr int SCALE_LABEL_Y = PAPER_INPUT_Y + INPUT_H + ROW_GAP;

        constexpr int SCALE_INPUT_X = SECTION_MARGIN;
        constexpr int SCALE_INPUT_Y = SCALE_LABEL_Y + LABEL_H + INPUT_GAP;

        // Orientation
        constexpr int ORIENTATION_LABEL_X = SECTION_MARGIN;
        constexpr int ORIENTATION_LABEL_Y = SCALE_INPUT_Y + INPUT_H + ROW_GAP;

        constexpr int ORIENTATION_INPUT_X = SECTION_MARGIN;
        constexpr int ORIENTATION_INPUT_Y = ORIENTATION_LABEL_Y + LABEL_H + INPUT_GAP;

        // Collate
        constexpr int COLLATE_LABEL_X = SECTION_MARGIN;
        constexpr int COLLATE_LABEL_Y = ORIENTATION_INPUT_Y + INPUT_H + ROW_GAP;

        constexpr int COLLATE_INPUT_X = SECTION_MARGIN;
        constexpr int COLLATE_INPUT_Y = COLLATE_LABEL_Y + LABEL_H + INPUT_GAP;
    }

} // namespace Layout
namespace Style {
    
    // General
    constexpr COLORREF WINDOW_BG                = RGB(220,220,220);
    

    // Section
    constexpr COLORREF SECTION_BG               = RGB(255,255,255);
    constexpr COLORREF SECTION_BORDER           = RGB(150,150,150);
    

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