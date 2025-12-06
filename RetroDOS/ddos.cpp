#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <atomic>
#include <thread>
#include <chrono>
#include <conio.h>
#include <sstream>

#include "ddos.h"
#include "retro.h"
#include "basic.h"

using namespace std;
namespace fs = std::filesystem;

// ==========================
// InputManager STATIC FIELDS
// ==========================
std::atomic<int>  InputManager::lastKey(0);
std::atomic<bool> InputManager::running(false);
std::thread       InputManager::inputThread;

std::atomic<int>  InputManager::mouseX(0);
std::atomic<int>  InputManager::mouseY(0);

std::atomic<bool> InputManager::mouseLeftClick(false);
std::atomic<bool> InputManager::mouseDoubleClick(false);
std::atomic<bool> InputManager::mouseWheelUp(false);
std::atomic<bool> InputManager::mouseWheelDown(false);

// =====================
// InputManager METHODS
// =====================

void InputManager::Start() {
    // Enable mouse input BEFORE starting the thread
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hIn, &mode);

    // Enable mouse and extended flags, disable quick edit
    mode |= ENABLE_MOUSE_INPUT;
    mode |= ENABLE_WINDOW_INPUT;        // ← NECESSARY FOR MOUSE MOVE!
    mode |= ENABLE_EXTENDED_FLAGS;
    mode &= ~ENABLE_QUICK_EDIT_MODE;

    SetConsoleMode(hIn, mode);

    running.store(true, std::memory_order_release);
    lastKey.store(0, std::memory_order_release);
    mouseX.store(0, std::memory_order_release);
    mouseY.store(0, std::memory_order_release);
    mouseLeftClick.store(false, std::memory_order_release);
    mouseDoubleClick.store(false, std::memory_order_release);
    mouseWheelUp.store(false, std::memory_order_release);
    mouseWheelDown.store(false, std::memory_order_release);

    inputThread = std::thread(InputLoop);
}

void InputManager::InputLoop() {
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    INPUT_RECORD ir[128];
    DWORD eventsRead;

    unsigned long long lastClickTime = 0;
    const unsigned long long doubleClickTime = 500;

    while (running.load(std::memory_order_acquire)) {
        DWORD result = WaitForSingleObject(hIn, 50);

        if (result == WAIT_OBJECT_0) {
            if (ReadConsoleInput(hIn, ir, 128, &eventsRead)) {
                for (DWORD i = 0; i < eventsRead; i++) {

                    // Handle keyboard events
                    if (ir[i].EventType == KEY_EVENT && ir[i].Event.KeyEvent.bKeyDown) {
                        int key = ir[i].Event.KeyEvent.uChar.AsciiChar;
                        int vkey = ir[i].Event.KeyEvent.wVirtualKeyCode;

                        // Special key mapping
                        if (vkey == VK_UP) key = 0x4800;
                        else if (vkey == VK_DOWN) key = 0x5000;
                        else if (vkey == VK_LEFT) key = 0x4B00;
                        else if (vkey == VK_RIGHT) key = 0x4D00;
                        else if (vkey == VK_PRIOR) key = 0x4900;
                        else if (vkey == VK_NEXT) key = 0x5100;
                        else if (vkey == VK_HOME) key = 0x4700;
                        else if (vkey == VK_END) key = 0x4F00;
                        else if (vkey == VK_F1) key = 0x3B00;
                        else if (vkey == VK_F2) key = 0x3C00;
                        else if (vkey == VK_F3) key = 0x3D00;
                        else if (vkey == VK_F4) key = 0x3E00;
                        else if (vkey == VK_F5) key = 0x3F00;
                        else if (vkey == VK_ESCAPE) key = 27;
                        else if (vkey == VK_RETURN) key = 13;

                        if (key != 0) {
                            lastKey.store(key, std::memory_order_release);
                        }
                    }

                    // Handle mouse events
                    else if (ir[i].EventType == MOUSE_EVENT) {
                        MOUSE_EVENT_RECORD mer = ir[i].Event.MouseEvent;

                        // Always update mouse position for any mouse event
                        int newX = mer.dwMousePosition.X;
                        int newY = mer.dwMousePosition.Y;
                        mouseX.store(newX, std::memory_order_release);
                        mouseY.store(newY, std::memory_order_release);

                        DWORD buttonState = mer.dwButtonState;
                        DWORD eventFlags = mer.dwEventFlags;

                        // Handle mouse wheel
                        if (eventFlags == MOUSE_WHEELED) {
                            short wheelDelta = HIWORD(buttonState);
                            if (wheelDelta > 0) {
                                mouseWheelUp.store(true, std::memory_order_release);
                            }
                            else if (wheelDelta < 0) {
                                mouseWheelDown.store(true, std::memory_order_release);
                            }
                        }
                        // Handle left mouse button click
                        else if (buttonState & FROM_LEFT_1ST_BUTTON_PRESSED) {
                            unsigned long long currentTime = GetTickCount64();
                            bool isDoubleClick = (currentTime - lastClickTime) < doubleClickTime;
                            lastClickTime = currentTime;

                            mouseLeftClick.store(true, std::memory_order_release);
                            if (isDoubleClick) {
                                mouseDoubleClick.store(true, std::memory_order_release);
                            }
                        }
                    }
                }
            }
        }
    }
}

void InputManager::Stop() {
    running.store(false, std::memory_order_release);
    if (inputThread.joinable()) inputThread.join();
}

int InputManager::GetKey() {
    return lastKey.exchange(0, std::memory_order_acquire);
}

void InputManager::GetMousePos(int& x, int& y) {
    x = mouseX.load(std::memory_order_acquire);
    y = mouseY.load(std::memory_order_acquire);
}

bool InputManager::GetMouseLeftClick() {
    return mouseLeftClick.exchange(false, std::memory_order_acquire);
}

bool InputManager::GetMouseDoubleClick() {
    return mouseDoubleClick.exchange(false, std::memory_order_acquire);
}

bool InputManager::GetMouseWheelUp() {
    return mouseWheelUp.exchange(false, std::memory_order_acquire);
}

bool InputManager::GetMouseWheelDown() {
    return mouseWheelDown.exchange(false, std::memory_order_acquire);
}

bool InputManager::IsMouseInRect(int x1, int y1, int x2, int y2) {
    int x, y;
    GetMousePos(x, y);
    return (x >= x1 && x <= x2 && y >= y1 && y <= y2);
}