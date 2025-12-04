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
    DoubleBuffer::Init(); // Basic console initialization

    try {
        Explorer explorer;
        explorer.Run();
    }
    catch (const std::exception& e) {
        // In case of error, restore console and print
        std::cerr << "An error occurred: " << e.what() << std::endl;
    }

    return 0;
}
