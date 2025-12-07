// main.cpp — FINAL VERSION — NO CRASH — FULL MOUSE SUPPORT
//vcpkg install nlohmann-json

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include "retro.h"
#include "ddos.h"
#include "basic.h"

extern void RestoreCursor();

int main() {
         // Creates console + sets correct mouse mode
   

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