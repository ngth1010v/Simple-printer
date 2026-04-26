#pragma once

namespace ui {
namespace home {
namespace Layout {

    // ===== WINDOW =====
    constexpr int MIN_WIDTH  = 450;
    constexpr int MIN_HEIGHT = 541;

    constexpr int MARGIN = 5;
    constexpr int GAP    = 5;

    // ===== BASIC SECTION =====
    namespace BasicSection {
        constexpr int X = MARGIN;
        constexpr int Y = MARGIN;
        constexpr int W = 220;
        constexpr int H = 90;

        // rows
        constexpr int ROW_H = 36;
        constexpr int LABEL_H = 14;
        constexpr int INPUT_H = 20;

        // Printer
        constexpr int PRINTER_LABEL_X = 10;
        constexpr int PRINTER_LABEL_Y = 10;

        constexpr int PRINTER_INPUT_X = 10;
        constexpr int PRINTER_INPUT_Y = 10 + LABEL_H;

        // Copies
        constexpr int COPIES_LABEL_X = 10;
        constexpr int COPIES_LABEL_Y = 10 + ROW_H;

        constexpr int COPIES_INPUT_X = 10;
        constexpr int COPIES_INPUT_Y = 10 + ROW_H + LABEL_H;
    }

} // namespace Layout
} // namespace HomeWindow
}