#pragma once

namespace ui {
namespace home {
namespace Layout {

    // ===== WINDOW =====
    constexpr int MIN_WIDTH             = 470;
    constexpr int MIN_HEIGHT            = 477;

    constexpr int MARGIN                = 5;
    constexpr int SECTION_MARGIN        = 10;
    constexpr int SECTION_GAP           = 5;

    constexpr int LABEL_H               = 12;
    constexpr int INPUT_H               = 20;    
    constexpr int IMAGE_H               = 48;    
    constexpr int INPUT_GAP             = 3;
    constexpr int ROW_GAP               = 5;
    constexpr int INPUT_BORDER_OFFSET   = 2;  
    
    
    // ===== INFO SECTION =====
    namespace InfoSection {
        constexpr int X = MARGIN;
        constexpr int Y = MARGIN;
        constexpr int W = 250;
        constexpr int H = 2 * SECTION_MARGIN + (LABEL_H + INPUT_GAP + INPUT_H + ROW_GAP) * 3 - ROW_GAP;
        
        constexpr int INPUT_W = W - 2 * SECTION_MARGIN;

        // Total files
        constexpr int TOTAL_FILES_LABEL_X = SECTION_MARGIN;
        constexpr int TOTAL_FILES_LABEL_Y = SECTION_MARGIN;

        constexpr int TOTAL_FILES_INPUT_X = SECTION_MARGIN;
        constexpr int TOTAL_FILES_INPUT_Y = TOTAL_FILES_LABEL_Y + LABEL_H + INPUT_GAP;

        // Pages to print
        constexpr int PAGES_TO_PRINT_LABEL_X = SECTION_MARGIN;
        constexpr int PAGES_TO_PRINT_LABEL_Y = TOTAL_FILES_INPUT_Y + INPUT_H + ROW_GAP;

        constexpr int PAGES_TO_PRINT_INPUT_X = SECTION_MARGIN;
        constexpr int PAGES_TO_PRINT_INPUT_Y = PAGES_TO_PRINT_LABEL_Y + LABEL_H + INPUT_GAP;

        // Sheets required
        constexpr int SHEETS_REQUIRED_LABEL_X = SECTION_MARGIN;
        constexpr int SHEETS_REQUIRED_LABEL_Y = PAGES_TO_PRINT_INPUT_Y + INPUT_H + ROW_GAP;

        constexpr int SHEETS_REQUIRED_INPUT_X = SECTION_MARGIN;
        constexpr int SHEETS_REQUIRED_INPUT_Y = SHEETS_REQUIRED_LABEL_Y + LABEL_H + INPUT_GAP;
    }


    // ===== BASIC SECTION =====
    namespace BasicSection {
        constexpr int X = MARGIN;
        constexpr int Y = InfoSection::Y + InfoSection::H + SECTION_GAP;
        constexpr int W = 250;
        constexpr int H = 2 * SECTION_MARGIN + (LABEL_H + INPUT_GAP + INPUT_H + ROW_GAP) * 2 - ROW_GAP;

        constexpr int INPUT_W = W - 2 * SECTION_MARGIN;

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
        constexpr int H = 2 * SECTION_MARGIN + (LABEL_H + INPUT_GAP + INPUT_H + ROW_GAP) * 4 - ROW_GAP + IMAGE_H + INPUT_GAP;
        
        constexpr int INPUT_W = W - 2 * SECTION_MARGIN;


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
    }



    
    // ===== CONTROL BLOCK =====
    namespace ControlBlock {

        constexpr int CONTROL_INPUT_H = 25;   
        constexpr int INPUT_W = (MIN_WIDTH - 2 * MARGIN - BasicSection::W - SECTION_GAP - INPUT_GAP) / 2;   

        // Print
        constexpr int PRINT_LABEL_W = 30;
        inline    int CALC_PRINT_INPUT_X (int windowW) {return windowW - MARGIN - INPUT_W;}
        inline    int CALC_PRINT_INPUT_Y (int windowH) {return windowH - MARGIN - CONTROL_INPUT_H;}

        inline    int CALC_PRINT_LABEL_X (int windowW) {return CALC_PRINT_INPUT_X(windowW) + (INPUT_W - PRINT_LABEL_W) / 2;}
        inline    int CALC_PRINT_LABEL_Y (int windowH) {return CALC_PRINT_INPUT_Y(windowH) + (INPUT_W - PRINT_LABEL_W) / 2;}

        // Cancle
        constexpr int CANCEL_LABEL_W = 36;
        inline    int CALC_CANCEL_INPUT_X (int windowW) {return windowW - MARGIN - 2 * INPUT_W - INPUT_GAP;}
        inline    int CALC_CANCEL_INPUT_Y (int windowH) {return windowH - MARGIN - CONTROL_INPUT_H;}

        inline    int CALC_CANCEL_LABEL_X (int windowW) {return CALC_CANCEL_INPUT_X(windowW) + (INPUT_W - CANCEL_LABEL_W) / 2;}
        inline    int CALC_CANCEL_LABEL_Y (int windowH) {return CALC_CANCEL_INPUT_Y(windowH) + (INPUT_W - CANCEL_LABEL_W) / 2;}
    }

    // ===== FILES SECTION =====
    namespace FilesSection {
        constexpr int X = BasicSection::X + BasicSection::W + SECTION_GAP;
        constexpr int Y = MARGIN;
        inline    int CALC_W (int windowW) {return windowW - 2 * MARGIN - BasicSection::W - SECTION_GAP;}
        inline    int CALC_H (int windowH) {return windowH - 2 * MARGIN - ControlBlock::CONTROL_INPUT_H - SECTION_GAP;}
        
        
        constexpr int CONTROL_LABEL_W = 6;
        inline    int CALC_CONTROL_INPUT_W (int windowW) {return (CALC_W(windowW) - 2 * SECTION_MARGIN - 3 * INPUT_GAP) / 4;}

        // Label
        constexpr int LABEL_X = SECTION_MARGIN;
        constexpr int LABEL_Y = SECTION_MARGIN;

        // Control Up
        inline    int CALC_CONTROL_UP_INPUT_X (int windowW) {return SECTION_MARGIN;}
        inline    int CALC_CONTROL_UP_INPUT_Y (int windowH) {return CALC_H(windowH) - SECTION_MARGIN - INPUT_H;}

        inline    int CALC_CONTROL_UP_LABEL_X (int windowW) {return CALC_CONTROL_UP_INPUT_X(windowW) + (CALC_CONTROL_INPUT_W(windowW) - CONTROL_LABEL_W) / 2;}
        inline    int CALC_CONTROL_UP_LABEL_Y (int windowH) {return CALC_CONTROL_UP_INPUT_Y(windowH) + (INPUT_H - LABEL_H) / 2;}

        // Control Down
        inline    int CALC_CONTROL_DOWN_INPUT_X (int windowW) {return SECTION_MARGIN + CALC_CONTROL_INPUT_W(windowW) + INPUT_GAP;}
        inline    int CALC_CONTROL_DOWN_INPUT_Y (int windowH) {return CALC_H(windowH) - SECTION_MARGIN - INPUT_H;}

        inline    int CALC_CONTROL_DOWN_LABEL_X (int windowW) {return CALC_CONTROL_DOWN_INPUT_X(windowW) + (CALC_CONTROL_INPUT_W(windowW) - CONTROL_LABEL_W) / 2;}
        inline    int CALC_CONTROL_DOWN_LABEL_Y (int windowH) {return CALC_CONTROL_DOWN_INPUT_Y(windowH) + (INPUT_H - LABEL_H) / 2;}

        // Control Remove
        inline    int CALC_CONTROL_REMOVE_INPUT_X (int windowW) {return SECTION_MARGIN + (CALC_CONTROL_INPUT_W(windowW) + INPUT_GAP) * 2;}
        inline    int CALC_CONTROL_REMOVE_INPUT_Y (int windowH) {return CALC_H(windowH) - SECTION_MARGIN - INPUT_H;}

        inline    int CALC_CONTROL_REMOVE_LABEL_X (int windowW) {return CALC_CONTROL_REMOVE_INPUT_X(windowW) + (CALC_CONTROL_INPUT_W(windowW) - CONTROL_LABEL_W) / 2;}
        inline    int CALC_CONTROL_REMOVE_LABEL_Y (int windowH) {return CALC_CONTROL_REMOVE_INPUT_Y(windowH) + (INPUT_H - LABEL_H) / 2;}

        // Control Add
        inline    int CALC_CONTROL_ADD_INPUT_X (int windowW) {return SECTION_MARGIN + (CALC_CONTROL_INPUT_W(windowW) + INPUT_GAP) * 3;}
        inline    int CALC_CONTROL_ADD_INPUT_Y (int windowH) {return CALC_H(windowH) - SECTION_MARGIN - INPUT_H;}

        inline    int CALC_CONTROL_ADD_LABEL_X (int windowW) {return CALC_CONTROL_ADD_INPUT_X(windowW) + (CALC_CONTROL_INPUT_W(windowW) - CONTROL_LABEL_W) / 2;}
        inline    int CALC_CONTROL_ADD_LABEL_Y (int windowH) {return CALC_CONTROL_ADD_INPUT_Y(windowH) + (INPUT_H - LABEL_H) / 2;}


        // File list vieww
        namespace FileListView {
            constexpr int X = FilesSection::X + SECTION_MARGIN;
            constexpr int Y = FilesSection::Y + SECTION_MARGIN + LABEL_H + INPUT_GAP;
            inline    int CALC_W (int windowW) {return FilesSection::CALC_W(windowW) - 2 * SECTION_MARGIN;}
            inline    int CALC_H (int windowH) {return FilesSection::CALC_H(windowH) - 2 * SECTION_MARGIN - INPUT_H - LABEL_H - 2 * INPUT_GAP;}
            
            
            // Row
            constexpr int ROW_GAP    = 3; 
            constexpr int ROW_MARGIN = 5; 
            constexpr int ROW_H      = 2 * ROW_MARGIN + LABEL_H + INPUT_GAP + INPUT_H; 
            inline    int CALC_ROW_W (int windowW) {return CALC_W(windowW);} 
            
            constexpr int ROW_X = 0; 
            inline    int CALC_ROW_Y (int index) {return (ROW_H + ROW_GAP) * index;} 
            

            // Status
            constexpr int STATUS_W = 2; 
            constexpr int STATUS_H = ROW_H; 

            constexpr int STATUS_X = 0; 
            inline    int CALC_STATUS_Y (int index) {return CALC_ROW_Y(index);} 
            

            // Label
            inline    int CALC_LABEL_MAX_W (int windowW) {return CALC_ROW_W(windowW) - STATUS_W - 2 * ROW_MARGIN;}

            constexpr int LABEL_X = ROW_MARGIN + STATUS_W; 
            inline    int CALC_LABEL_Y (int index) {return CALC_ROW_Y(index) + ROW_MARGIN;}


            // NOTE: InputWrapper, InputDiver, FromInput, ToInput dều có H = INPUT_H
            // Input wrapper
            inline    int CALC_INPUT_WRAPPER_W (int windowW) {return CALC_ROW_W(windowW) - STATUS_W - 2 * ROW_MARGIN;}

            constexpr int INPUT_WRAPPER_X = ROW_MARGIN + STATUS_W; 
            inline    int CALC_INPUT_WRAPPER_Y (int index) {return CALC_ROW_Y(index) + LABEL_H + INPUT_GAP;} 


            // Input diver (dấu '-' ở giữa 2 input)
            constexpr int INPUT_DIVER_W = 6;

            inline    int CALC_INPUT_DIVER_X (int windowW) {return INPUT_WRAPPER_X + (CALC_INPUT_WRAPPER_W(windowW) - INPUT_DIVER_W) / 2;}
            inline    int CALC_INPUT_DIVER_Y (int index) {return CALC_INPUT_WRAPPER_Y(index);} 


            // 'From' input 
            inline    int CALC_FROM_INPUT_W (int windowW) {return (CALC_INPUT_WRAPPER_W(windowW) - INPUT_DIVER_W) / 2;}

            constexpr int FROM_INPUT_X = INPUT_WRAPPER_X;
            inline    int CALC_FROM_INPUT_Y (int index) {return CALC_INPUT_WRAPPER_Y(index);} 


            // 'To' input 
            inline    int CALC_TO_INPUT_W (int windowW) {return (CALC_INPUT_WRAPPER_W(windowW) - INPUT_DIVER_W) / 2;}

            inline    int CALC_TO_INPUT_X (int windowW) {return INPUT_WRAPPER_X + CALC_FROM_INPUT_W(windowW) + INPUT_DIVER_W;}
            inline    int CALC_TO_INPUT_Y (int index) {return CALC_INPUT_WRAPPER_Y(index);} 

            
        }

    }
    


} // namespace Layout
namespace Style {
    
    // General
    constexpr COLORREF WINDOW_BG = RGB(230,230,230);
    

    // Section
    constexpr COLORREF SECTION_BG       = RGB(255,255,255);
    constexpr COLORREF SECTION_BORDER   = RGB(150,150,150);
    

    // Label
    constexpr COLORREF LABEL = RGB(0,0,0);
    

    // Input
    constexpr int INPUT_RADIUS                  = 6;

    constexpr COLORREF INPUT_TEXT               = RGB(0,0,0);
    constexpr COLORREF INPUT_TEXT_HOVER         = RGB(0,0,0);
    constexpr COLORREF INPUT_TEXT_FOCUS         = RGB(0,0,0);
    constexpr COLORREF INPUT_TEXT_DISABLED      = RGB(50,50,50);

    constexpr COLORREF INPUT_BG                 = RGB(255,255,255);
    constexpr COLORREF INPUT_BG_HOVER           = RGB(230,230,230);
    constexpr COLORREF INPUT_BG_FOCUS           = RGB(255,255,255);
    constexpr COLORREF INPUT_BG_DISABLED        = RGB(240,240,240);
    
    constexpr COLORREF INPUT_BORDER             = RGB(180,180,180);
    constexpr COLORREF INPUT_BORDER_HOVER       = RGB(180,180,180);
    constexpr COLORREF INPUT_BORDER_FOCUS       = RGB(180,180,180);
    constexpr COLORREF INPUT_BORDER_DISABLED    = RGB(220,220,220);


    // Control
    constexpr COLORREF PRINT_CONTROL_BG = RGB(200,255,200);
    
    // File list view
    constexpr COLORREF FILE_LIST_VIEW_SELECTED_ROW_BORDER   = RGB(50,50,50);

    constexpr COLORREF FILE_LIST_VIEW_GREEN_STATUS          = RGB(100,210,100);
    constexpr COLORREF FILE_LIST_VIEW_GREEN_BG              = RGB(230,250,230);
    
    constexpr COLORREF FILE_LIST_VIEW_RED_STATUS            = RGB(210,70,70);
    constexpr COLORREF FILE_LIST_VIEW_RED_BG                = RGB(250,230,230);
    
    constexpr COLORREF FILE_LIST_VIEW_YELLOW_STATUS         = RGB(200,140,50);
    constexpr COLORREF FILE_LIST_VIEW_YELLOW_BG             = RGB(250,240,230);
    
    
}
} // namespace HomeWindow
}