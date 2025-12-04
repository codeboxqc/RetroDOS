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

/*
// Forward-declare DoubleBuffer interface (defined in retro.h/DoubleBuffer.cpp)
class DoubleBuffer;

class Explorer {
public:
    Explorer();
    void Run();

private:
    void LoadDirectory();
    void SetStatus(const std::string& msg, unsigned long duration = 2000);

    void DrawFileList();
    void DrawInfoPanel();
    void DrawStatusBar();
    void DrawUI();

    void HandleMouse();

    // state
    std::vector<FileEntry> files;
    fs::path cur;
    int sel;
    int top;

    const int FILE_LIST_HEIGHT = 20;
    const int SPLIT_ROW = 22;

    bool needsRedraw;
    bool needsReload;

    uint64_t totalFiles;
    uint64_t totalDirs;
    uint64_t totalSize;

    std::string statusMessage;
    unsigned long long statusMessageTime; // use 64-bit time from GetTickCount64()
};
*/