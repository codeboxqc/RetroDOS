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

    void FastCopyClipboard();
    void PasteClipboard();
    void DeleteSelected();
    void CopySelected();
    void RenameSelected();
    void CreateNewFolder();
    void CutSelected();
    void ClearSelection();

    void LoadDriveLetters();
    void HandleButtonClicks(int mx, int my);
    std::string GetDriveType(const std::string& drive);
    void ChangeDrive(int driveIndex);
    void ChangeDriveByLetter(char letter);  // NEW: Shift+Letter to change drive
    void HandleDriveSelection(int mx, int my);
    void DrawOperationButtons();
    void DrawDriveLetters();
    void HandleRenameInput(int key);

    void DrawAttributeButtons();
    void ToggleFileAttribute(const fs::path& path, DWORD attribute);
    void  DrawSysPanel();


    int btnCopyX = 5, btnCopyY = 1;
    int btnPasteX = 18, btnPasteY = 1;
    int btnDeleteX = 32, btnDeleteY = 1;
    int btnRenameX = 47, btnRenameY = 1;
    int btnNewFolderX = 62, btnNewFolderY = 1;

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
    unsigned long long statusMessageTime;

    std::vector<FileEntry> clipboard;
    bool cutMode = false;
    std::string renameOldName;
    bool inRenameMode = false;
    int renameY = 0;

    std::vector<std::string> driveLetters;
    int selectedDrive;
    bool showDrivePanel;
    int drivePanelX, drivePanelY;

    std::string renameBuffer;
    int renameIndex = -1;

    bool showingAttribButtons;
    int attribButtonsX;
    int attribButtonsY;
};