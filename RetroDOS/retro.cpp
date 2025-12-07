#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <conio.h>
#include <algorithm>

#include "retro.h"

// ---- STATIC DEFINITIONS ----
HANDLE DoubleBuffer::hOut = nullptr;
CHAR_INFO DoubleBuffer::front[DoubleBuffer::HEIGHT][DoubleBuffer::WIDTH];
CHAR_INFO DoubleBuffer::back[DoubleBuffer::HEIGHT][DoubleBuffer::WIDTH];
std::atomic<bool> DoubleBuffer::needsFlip(false);

// Global color scheme
ColorScheme colors;

// ---- PRIVATE: Setup console ----
void DoubleBuffer::SetupConsole()
{
    AllocConsole();
    SetConsoleTitleW(L"RETRO EXPLORER");

    // Disable resizing
    HWND hwnd = GetConsoleWindow();
    DWORD style = GetWindowLong(hwnd, GWL_STYLE);
    style &= ~WS_THICKFRAME & ~WS_MAXIMIZEBOX;
    SetWindowLong(hwnd, GWL_STYLE, style);

    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);

    // Wait until handles are valid
    int attempts = 0;
    while (hIn == INVALID_HANDLE_VALUE || hIn == NULL) {
        Sleep(10);
        hIn = GetStdHandle(STD_INPUT_HANDLE);
        if (++attempts > 50) break;
    }

    COORD size = { WIDTH, HEIGHT };
    SetConsoleScreenBufferSize(hOut, size);
    SMALL_RECT rect = { 0, 0, WIDTH - 1, HEIGHT - 1 };
    SetConsoleWindowInfo(hOut, TRUE, &rect);

    CONSOLE_CURSOR_INFO ci = { 1, FALSE };
    SetConsoleCursorInfo(hOut, &ci);

    // FINAL INPUT MODE — NO CRASH, FULL MOUSE
    DWORD mode = ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT | ENABLE_EXTENDED_FLAGS;
    mode &= ~ENABLE_QUICK_EDIT_MODE;
    SetConsoleMode(hIn, mode);
}

void DoubleBuffer::Init()
{
    SetupConsole();
    ClearBoth();
    needsFlip = false;
}

void DoubleBuffer::ClearBoth(WORD attr)
{
    for (int y = 0; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++)
        {
            front[y][x].Char.UnicodeChar = L' ';
            front[y][x].Attributes = attr;

            back[y][x].Char.UnicodeChar = L' ';
            back[y][x].Attributes = attr;
        }
}

void DoubleBuffer::Set(int x, int y, wchar_t c, WORD attr)
{
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT)
        return;

    back[y][x].Char.UnicodeChar = c;
    back[y][x].Attributes = attr;
}

void DoubleBuffer::DrawText(int x, int y, const std::string& text, WORD attr)
{
    for (size_t i = 0; i < text.size() && (x + (int)i) < WIDTH; i++)
        Set(x + (int)i, y, (wchar_t)text[i], attr);
}

void DoubleBuffer::FillRect(int x, int y, int w, int h, wchar_t c, WORD attr)
{
    for (int dy = 0; dy < h; dy++)
        for (int dx = 0; dx < w; dx++)
            Set(x + dx, y + dy, c, attr);
}

void DoubleBuffer::DrawBox(int x, int y, int w, int h, WORD attr)
{
    for (int i = 1; i < w - 1; i++)
    {
        Set(x + i, y, BOX_H, attr);
        Set(x + i, y + h - 1, BOX_H, attr);
    }

    for (int i = 1; i < h - 1; i++)
    {
        Set(x, y + i, BOX_V, attr);
        Set(x + w - 1, y + i, BOX_V, attr);
    }

    Set(x, y, BOX_TL, attr);
    Set(x + w - 1, y, BOX_TR, attr);
    Set(x, y + h - 1, BOX_BL, attr);
    Set(x + w - 1, y + h - 1, BOX_BR, attr);
}

void DoubleBuffer::Flip()
{
    SMALL_RECT rect = { 0, 0, WIDTH - 1, HEIGHT - 1 };
    COORD size = { WIDTH, HEIGHT };
    COORD pos = { 0, 0 };

    WriteConsoleOutputW(hOut, &back[0][0], size, pos, &rect);

    std::swap(front, back);
}

void DoubleBuffer::ScheduleFlip()
{
    needsFlip = true;
}

bool DoubleBuffer::NeedsFlip()
{
    return needsFlip.exchange(false);
}