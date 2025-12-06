// main.cpp — FINAL VERSION — NO CRASH — FULL MOUSE SUPPORT
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include "retro.h"
#include "ddos.h"
#include "basic.h"

int main() {
         // Creates console + sets correct mouse mode
   

    Explorer explorer;
    explorer.Run();            // Explorer itself starts InputManager

    InputManager::Stop();      // Clean shutdown
    return 0;
}