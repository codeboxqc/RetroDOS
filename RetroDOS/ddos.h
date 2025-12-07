#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <cstdint>
#include <atomic>
#include <thread>



class InputManager {
public:
    static void Start();
    static void Stop();

    static int GetKey();
    static void GetMousePos(int& x, int& y);

    static bool GetMouseLeftClick();
    static bool GetMouseDoubleClick();
    static bool GetMouseRightClick();
    static bool GetMouseWheelUp();
    static bool GetMouseWheelDown();
    static bool IsMouseInRect(int x1, int y1, int x2, int y2);

private:
    static void InputLoop();

    // -------- REQUIRED STATIC FIELDS --------
    static std::atomic<int>  lastKey;
    static std::atomic<bool> running;
    static std::thread       inputThread;

    static std::atomic<int>  mouseX;
    static std::atomic<int>  mouseY;

    static std::atomic<bool> mouseLeftClick;
    static std::atomic<bool> mouseDoubleClick;
    static std::atomic<bool> mouseWheelUp;
    static std::atomic<bool> mouseWheelDown;
};

 