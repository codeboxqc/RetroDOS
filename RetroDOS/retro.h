#pragma once
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <atomic>

// Color constants
constexpr unsigned short BLACK = 0;
constexpr unsigned short BLUE = 1;
constexpr unsigned short GREEN = 2;
constexpr unsigned short CYAN = 3;
constexpr unsigned short RED = 4;
constexpr unsigned short MAGENTA = 5;
constexpr unsigned short BROWN = 6;
constexpr unsigned short LGRAY = 7;
constexpr unsigned short DGRAY = 8;
constexpr unsigned short LBLUE = 9;
constexpr unsigned short LGREEN = 10;
constexpr unsigned short LCYAN = 11;
constexpr unsigned short LRED = 12;
constexpr unsigned short LMAGENTA = 13;
constexpr unsigned short YELLOW = 14;
constexpr unsigned short WHITE = 15;

// Box drawing characters
constexpr wchar_t BOX_H = L'─';
constexpr wchar_t BOX_V = L'│';
constexpr wchar_t BOX_TL = L'┌';
constexpr wchar_t BOX_TR = L'┐';
constexpr wchar_t BOX_BL = L'└';
constexpr wchar_t BOX_BR = L'┘';

// Color scheme structure
struct ColorScheme {
    // File list colors
    unsigned short fileListBg = BLACK;
    unsigned short fileListFg = LGRAY;
    unsigned short fileListBorder = (GREEN << 4) | WHITE;
    unsigned short fileListTitle = WHITE | (GREEN << 4);
    unsigned short fileListHeaderBg = BLUE;
    unsigned short fileListHeaderFg = WHITE;
    unsigned short fileListSeparator = GREEN | (BLACK << 4);

    // Info panel colors
    unsigned short infoPanelBg = BLACK;
    unsigned short infoPanelFg = LGRAY;
    unsigned short infoPanelBorder = (CYAN << 4) | WHITE;
    unsigned short infoPanelTitle = WHITE | (CYAN << 4);

    // Selection colors
    unsigned short selectedBg = WHITE;
    unsigned short selectedFg = BLACK;
    unsigned short mouseOverBg = DGRAY;

    // Status bar colors
    unsigned short statusBarBg = LGRAY;
    unsigned short statusBarFg = BLACK;

    // File type colors
    unsigned short dirColor = CYAN;
    unsigned short fileColor = LGRAY;
    unsigned short extColor = YELLOW;
    unsigned short sizeColor = GREEN;
    unsigned short dateColor = MAGENTA;
    unsigned short attrColor = YELLOW;
};

// Global color scheme instance
extern ColorScheme colors;

class DoubleBuffer {
public:
    static constexpr int WIDTH = 120;
    static constexpr int HEIGHT = 40;

    static void Init();
    static void ClearBoth(WORD attr = (LGRAY | (BLACK << 4)));

    static void Set(int x, int y, wchar_t c, WORD attr);
    static void DrawText(int x, int y, const std::string& text, WORD attr);
    static void FillRect(int x, int y, int w, int h, wchar_t c, WORD attr);
    static void DrawBox(int x, int y, int w, int h, WORD attr);

    static void Flip();
    static void ScheduleFlip();
    static bool NeedsFlip();

    static int GetWidth() { return WIDTH; }
    static int GetHeight() { return HEIGHT; }

private:
    static void SetupConsole();

    static HANDLE hOut;
    static CHAR_INFO front[HEIGHT][WIDTH];
    static CHAR_INFO back[HEIGHT][WIDTH];
    static std::atomic<bool> needsFlip;
};