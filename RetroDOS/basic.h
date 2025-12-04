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

namespace fs = std::filesystem;

struct FileEntry {
    std::string name;
    bool dir = false;
    uint64_t size = 0;
    std::string date;
    fs::path path;
    bool selected = false;
};

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

    // Add these for mouse tracking
    int lastMouseX = -1;
    int lastMouseY = -1;
    bool mouseMoved = false;

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