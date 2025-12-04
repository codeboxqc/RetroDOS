// RETRO FILE EXPLORER v5.2
#define NOMINMAX  
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <filesystem>

#include "ddos.h"      // <-- Explorer + InputManager
#include "retro.h"     // <-- Your DoubleBuffer
#include "basic.h"

namespace fs = std::filesystem;

int main() {

    // Hide console cursor
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hOut, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hOut, &cursorInfo);

    try {
        Explorer explorer;
        explorer.Run();
    }
    catch (...) {
        // Silent exit
    }

    return 0;
}
