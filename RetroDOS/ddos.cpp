#include <algorithm>
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
#include <iomanip>
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
                        else if (vkey == VK_F10) key = 0x4400;
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

void Explorer::UpdateTop() {
    if (sel < top) {
        top = sel;
    }
    if (sel >= top + FILE_LIST_HEIGHT) {
        top = sel - FILE_LIST_HEIGHT + 1;
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

Explorer::Explorer() : sel(0), top(0), mouseOver(-1), needsRedraw(true), needsReload(true),
running(true), totalFiles(0), totalDirs(0), totalSize(0), statusMessageTime(0) {
    cur = fs::current_path();
}

void Explorer::Run() {
    DoubleBuffer::Init();
    InputManager::Start();

    while (running) {
        if (needsReload) {
            LoadDirectory();
            needsReload = false;
            needsRedraw = true;
        }

        if (DoubleBuffer::NeedsFlip() || needsRedraw) {
            DrawUI();
            DoubleBuffer::Flip();
            needsRedraw = false;
        }

        int key = InputManager::GetKey();
        if (key != 0) {
            HandleKeyboard(key);
            needsRedraw = true;
        }

        HandleMouse();

        // Check for status message timeout
        if (statusMessageTime > 0 && GetTickCount64() > statusMessageTime) {
            statusMessage.clear();
            statusMessageTime = 0;
            needsRedraw = true;
        }

        this_thread::sleep_for(chrono::milliseconds(10));
    }

    InputManager::Stop();
}

void Explorer::HandleKeyboard(int key) {
    switch (key) {
    case 0x4800: // Up
        sel = std::max(0, sel - 1);
        UpdateTop();
        break;
    case 0x5000: // Down
        sel = std::min(((int))files.size() - 1, sel + 1);
        UpdateTop();
        break;
    case 13: // Enter
        if (sel >= 0 && sel < files.size()) {
            if (files[sel].isDir) {
                if (files[sel].name == "..") {
                    cur = cur.parent_path();
                }
                else {
                    cur /= files[sel].name;
                }
                needsReload = true;
                sel = 0;
            }
        }
        break;
    case 27: // Escape
        // In a real app, this might trigger a quit confirmation
        // For now, we'll just go up a directory.
        cur = cur.parent_path();
        needsReload = true;
        sel = 0;
        break;
    case 0x4400: // F10
        running = false;
        break;
    }
}

void Explorer::DrawUI() {
    DoubleBuffer::ClearBoth();
    DrawFileList();
    DrawInfoPanel();
    DrawStatusBar();
}

void Explorer::HandleMouse() {
    int mx, my;
    InputManager::GetMousePos(mx, my);

    // Mouse wheel scrolling
    if (InputManager::IsMouseInRect(1, 1, DoubleBuffer::GetWidth() - 2, FILE_LIST_HEIGHT)) {
        if (InputManager::GetMouseWheelUp()) {
            sel = std::max(0, sel - 3); // Scroll by 3
            UpdateTop();
            needsRedraw = true;
        }
        if (InputManager::GetMouseWheelDown()) {
            sel = std::min(((int))files.size() - 1, sel + 3); // Scroll by 3
            UpdateTop();
            needsRedraw = true;
        }
    }

    // Mouse click selection
    if (InputManager::GetMouseLeftClick() && InputManager::IsMouseInRect(1, 2, DoubleBuffer::GetWidth() - 2, FILE_LIST_HEIGHT)) {
        int clickedItem = my - 2 + top;
        if (clickedItem >= 0 && clickedItem < files.size()) {
            sel = clickedItem;
            needsRedraw = true;
        }
    }

    // Double-click to open/run
    if (InputManager::GetMouseDoubleClick() && InputManager::IsMouseInRect(1, 2, DoubleBuffer::GetWidth() - 2, FILE_LIST_HEIGHT)) {
        int clickedItem = my - 2 + top;
        if (clickedItem >= 0 && clickedItem < files.size()) {
            sel = clickedItem;
            HandleKeyboard(13); // Simulate enter
        }
    }

    // Mouse-over highlighting
    int newMouseOver = -1;
    if (InputManager::IsMouseInRect(1, 2, DoubleBuffer::GetWidth() - 2, FILE_LIST_HEIGHT)) {
        newMouseOver = my - 2 + top;
        if (newMouseOver >= files.size()) {
            newMouseOver = -1;
        }
    }

    if (newMouseOver != mouseOver) {
        mouseOver = newMouseOver;
        needsRedraw = true;
    }
}

void Explorer::LoadDirectory() {
    files.clear();

    // Add ".." entry for parent directory navigation
    if (cur.has_parent_path()) {
        FileEntry parent;
        parent.name = "..";
        parent.isDir = true;
        parent.size = 0;
        files.push_back(parent);
    }

    // Basic implementation
    try {
        for (const auto& entry : fs::directory_iterator(cur)) {
            FileEntry fe;
            fe.name = entry.path().filename().string();
            fe.isDir = entry.is_directory();
            fe.size = entry.is_directory() ? 0 : entry.file_size();
            fe.date = fs::last_write_time(entry);
            fe.attr = GetFileAttributes(entry.path().c_str());
            files.push_back(fe);
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        SetStatus("Access Denied");
    }
}

void Explorer::DrawFileList() {
    // Basic implementation
    DoubleBuffer::DrawBox(0, 0, DoubleBuffer::GetWidth(), FILE_LIST_HEIGHT + 2, colors.fileListBorder);
    for (int i = 0; i < FILE_LIST_HEIGHT && (top + i) < files.size(); ++i) {
        int currentFile = top + i;
        WORD attr = (currentFile == sel) ? (colors.selectedBg << 4 | colors.selectedFg) : (colors.fileListBg << 4 | colors.fileListFg);
        if (currentFile == mouseOver) {
            attr = (colors.mouseOverBg << 4 | colors.selectedFg);
        }
        DoubleBuffer::DrawText(2, i + 2, files[currentFile].name, attr);
    }
}

void Explorer::DrawInfoPanel() {
    // Basic implementation
    DoubleBuffer::DrawBox(0, SPLIT_ROW, DoubleBuffer::GetWidth(), DoubleBuffer::GetHeight() - SPLIT_ROW - 2, colors.infoPanelBorder);
    if (sel >= 0 && sel < files.size()) {
        const auto& file = files[sel];
        DoubleBuffer::DrawText(2, SPLIT_ROW + 1, "Name: " + file.name, colors.infoPanelFg);
        DoubleBuffer::DrawText(2, SPLIT_ROW + 2, "Size: " + to_string(file.size), colors.infoPanelFg);
    }
}

void Explorer::DrawStatusBar() {
    // Basic implementation
    string text = " F1 Help | F2 Rename | F3 View | F4 Edit | F5 Copy | F6 Move | F7 MkDir | F8 Del | F10 Exit";
    DoubleBuffer::FillRect(0, DoubleBuffer::GetHeight() - 1, DoubleBuffer::GetWidth(), 1, L' ', colors.statusBarBg << 4 | colors.statusBarFg);
    DoubleBuffer::DrawText(0, DoubleBuffer::GetHeight() - 1, text, colors.statusBarBg << 4 | colors.statusBarFg);
}

void Explorer::SetStatus(const std::string& msg, unsigned long duration) {
    statusMessage = msg;
    statusMessageTime = GetTickCount64() + duration;
    needsRedraw = true;
}
