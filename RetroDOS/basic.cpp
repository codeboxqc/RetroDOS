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

// =====================
// Explorer Implementation
// =====================

Explorer::Explorer()
    : cur(fs::current_path()),
    sel(0),
    top(0),
    needsRedraw(true),
    needsReload(true),
    totalFiles(0),
    totalDirs(0),
    totalSize(0),
    statusMessageTime(0ULL),
    lastMouseX(-1),
    lastMouseY(-1),
    mouseMoved(false)
{
    // Initialize default color scheme
    colors.fileListBg = BLACK;
    colors.fileListFg = LGRAY;
    colors.fileListBorder = (GREEN << 4) | WHITE;
    colors.fileListTitle = (BLACK) | (GREEN << 4);
    colors.fileListHeaderBg = BLUE;
    colors.fileListHeaderFg = WHITE;
    colors.fileListSeparator = (BLACK << 4) | GREEN;
    colors.infoPanelBg = BLACK;
    colors.infoPanelFg = LGRAY;
    colors.infoPanelBorder = (CYAN << 4) | WHITE;
    colors.infoPanelTitle = (BLACK) | (CYAN << 4);
    colors.selectedBg = WHITE;
    colors.selectedFg = BLACK;
    colors.mouseOverBg = DGRAY;
    colors.statusBarBg = LGRAY;
    colors.statusBarFg = BLACK;
    colors.dirColor = CYAN;
    colors.fileColor = LGRAY;
    colors.extColor = YELLOW;
    colors.sizeColor = GREEN;
    colors.dateColor = MAGENTA;
    colors.attrColor = YELLOW;
}

std::string GetFileAttributes(const fs::path& path) {
    std::string attrs = "----";

    try {
        DWORD dwAttrs = GetFileAttributesW(path.wstring().c_str());

        if (dwAttrs & FILE_ATTRIBUTE_READONLY) attrs[0] = 'R';
        if (dwAttrs & FILE_ATTRIBUTE_ARCHIVE) attrs[1] = 'A';
        if (dwAttrs & FILE_ATTRIBUTE_SYSTEM) attrs[2] = 'S';
        if (dwAttrs & FILE_ATTRIBUTE_HIDDEN) attrs[3] = 'H';
    }
    catch (...) {}

    return attrs;
}

std::string GetFileExtension(const std::string& filename) {
    size_t dotPos = filename.find_last_of('.');
    if (dotPos != std::string::npos && dotPos != 0) {
        return filename.substr(dotPos);
    }
    return "";
}

void Explorer::SetStatus(const std::string& msg, unsigned long duration) {
    statusMessage = msg;
    statusMessageTime = GetTickCount64() + duration;
    needsRedraw = true;
}

void Explorer::LoadDirectory() {
    files.clear();
    totalFiles = 0;
    totalDirs = 0;
    totalSize = 0;

    // Add parent directory
    if (cur.has_parent_path() && cur.parent_path() != cur) {
        files.push_back({ "..", true, 0, "", cur.parent_path(), false });
        totalDirs++;
    }

    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(cur, fs::directory_options::skip_permission_denied, ec)) {
        try {
            FileEntry fe;
            fe.path = entry.path();
            fe.name = entry.path().filename().string();
            fe.dir = entry.is_directory();
            fe.selected = false;

            if (fe.dir) totalDirs++;
            else {
                totalFiles++;
                fe.size = entry.file_size();
                totalSize += fe.size;
            }

            // Get modified time
            auto ftime = entry.last_write_time();
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
            std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
            struct tm timeinfo;
            localtime_s(&timeinfo, &cftime);
            char timebuf[20];
            strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M", &timeinfo);
            fe.date = timebuf;

            files.push_back(std::move(fe));
        }
        catch (...) {}
    }

    // Sort directories first, then by name
    std::sort(files.begin(), files.end(), [](const FileEntry& a, const FileEntry& b) {
        if (a.dir != b.dir) return a.dir > b.dir;
        return _stricmp(a.name.c_str(), b.name.c_str()) < 0;
        });

    needsReload = false;
    SetStatus("Loaded " + std::to_string(files.size()) + " items");
}

void Explorer::DrawFileList() {
    int width = DoubleBuffer::GetWidth();

    // Main file list box
    DoubleBuffer::DrawBox(0, 0, width, FILE_LIST_HEIGHT + 3, colors.fileListBorder);

    // Title
    std::string title = " FILE EXPLORER - " + cur.filename().string() + " [F1-F4: Colors] ";
    int titleX = (width - (int)title.length()) / 2;
    DoubleBuffer::DrawText(titleX, 0, title, colors.fileListTitle);

    // Headers with BLUE background and WHITE text
    DoubleBuffer::FillRect(1, 1, width - 2, 1, L' ', colors.fileListHeaderFg | (colors.fileListHeaderBg << 4));
    DoubleBuffer::DrawText(2, 1, "Name", colors.fileListHeaderFg | (colors.fileListHeaderBg << 4));
    DoubleBuffer::DrawText(32, 1, "Ext", colors.fileListHeaderFg | (colors.fileListHeaderBg << 4));
    DoubleBuffer::DrawText(37, 1, "Size", colors.fileListHeaderFg | (colors.fileListHeaderBg << 4));
    DoubleBuffer::DrawText(52, 1, "Modified", colors.fileListHeaderFg | (colors.fileListHeaderBg << 4));
    DoubleBuffer::DrawText(75, 1, "Attrib", colors.fileListHeaderFg | (colors.fileListHeaderBg << 4));

    // Separator line
    for (int x = 1; x < width - 1; x++) {
        DoubleBuffer::Set(x, 2, L'-', colors.fileListSeparator);
    }

    // Get mouse position
    int mx, my;
    InputManager::GetMousePos(mx, my);

    // Draw files
    int visibleCount = std::min(FILE_LIST_HEIGHT - 1, static_cast<int>(files.size()) - top);
    if (visibleCount < 0) visibleCount = 0;

    for (int i = 0; i < visibleCount; i++) {
        int idx = top + i;
        const FileEntry& entry = files[idx];
        bool cursorOn = (idx == sel);
        bool fileSelected = entry.selected;

        // Check if mouse is over this row
        bool mouseOver = (my == 3 + i && mx >= 1 && mx < width - 1);

        // Determine colors based on selection state
        unsigned short bg;
        unsigned short fg;

        if (fileSelected) {
            bg = colors.selectedBg;
            fg = colors.selectedFg;
        }
        else if (cursorOn) {
            bg = mouseOver ? colors.mouseOverBg : colors.fileListBg;
            fg = entry.dir ? colors.dirColor : colors.fileColor;
        }
        else {
            bg = mouseOver ? colors.mouseOverBg : colors.fileListBg;
            fg = entry.dir ? colors.dirColor : colors.fileColor;
        }

        unsigned short attr = fg | (bg << 4);

        int y = 3 + i;

        // Highlight row
        if (fileSelected || cursorOn || mouseOver) {
            for (int x = 1; x < width - 1; x++) {
                DoubleBuffer::Set(x, y, L' ', (bg << 4));
            }
        }

        // Selection/Cursor indicator
        if (cursorOn) {
            DoubleBuffer::Set(1, y, fileSelected ? L'*' : L'>', attr);
        }
        else if (fileSelected) {
            DoubleBuffer::Set(1, y, L'*', attr);
        }

        // Name (truncated to fit)
        std::string name = entry.name;
        if (name.length() > 28) {
            name = name.substr(0, 25) + "...";
        }
        if (entry.dir) {
            name = "[ " + name + " ]";
        }
        DoubleBuffer::DrawText(3, y, name, attr);

        // Extension
        if (!entry.dir) {
            std::string ext = GetFileExtension(entry.name);
            if (ext.length() > 4) ext = ext.substr(0, 4);
            DoubleBuffer::DrawText(32, y, ext, colors.extColor | (bg << 4));
        }

        // Size
        char sizeStr[16];
        if (entry.dir) {
            strcpy_s(sizeStr, "<DIR>");
            DoubleBuffer::DrawText(37, y, sizeStr, colors.dirColor | (bg << 4));
        }
        else {
            if (entry.size < 1024) {
                sprintf_s(sizeStr, "%llu B", (unsigned long long)entry.size);
            }
            else if (entry.size < 1024 * 1024) {
                sprintf_s(sizeStr, "%.1f KB", entry.size / 1024.0);
            }
            else if (entry.size < 1024 * 1024 * 1024) {
                sprintf_s(sizeStr, "%.1f MB", entry.size / (1024.0 * 1024));
            }
            else {
                sprintf_s(sizeStr, "%.1f GB", entry.size / (1024.0 * 1024 * 1024));
            }
            DoubleBuffer::DrawText(37, y, sizeStr, colors.sizeColor | (bg << 4));
        }

        // Date
        DoubleBuffer::DrawText(52, y, entry.date, attr);

        // Attributes
        if (!entry.dir) {
            std::string attrs = GetFileAttributes(entry.path);
            DoubleBuffer::DrawText(75, y, attrs, colors.attrColor | (bg << 4));
        }
    }

    // Draw empty rows if needed
    for (int i = visibleCount; i < FILE_LIST_HEIGHT - 1; i++) {
        int y = 3 + i;
        for (int x = 1; x < width - 1; x++) {
            DoubleBuffer::Set(x, y, L' ', (colors.fileListBg << 4));
        }
    }
}

void Explorer::HandleMouse() {
    int mx, my;
    InputManager::GetMousePos(mx, my);

    // Check if mouse moved
    if (mx != lastMouseX || my != lastMouseY) {
        lastMouseX = mx;
        lastMouseY = my;
        mouseMoved = true;
        needsRedraw = true;
    }

    // Handle clicks in file list area
    if (my >= 3 && my < FILE_LIST_HEIGHT + 2 && mx >= 1 && mx < DoubleBuffer::GetWidth() - 1) {
        int clickedRow = my - 3;
        if (clickedRow < std::min(FILE_LIST_HEIGHT - 1, static_cast<int>(files.size()) - top)) {
            int clickedIdx = top + clickedRow;

            if (InputManager::GetMouseLeftClick()) {
                // Move cursor to clicked position
                sel = clickedIdx;

                // Check for double click
                if (InputManager::GetMouseDoubleClick()) {
                    // Double click - open if it's a directory
                    if (files[sel].dir) {
                        cur = files[sel].path;
                        sel = top = 0;
                        needsReload = true;
                        SetStatus("Opening: " + files[sel].name);
                    }
                }
                else {
                    // Single click - toggle selection
                    files[sel].selected = !files[sel].selected;
                    SetStatus(files[sel].selected ?
                        "Selected: " + files[sel].name :
                        "Unselected: " + files[sel].name);
                }

                needsRedraw = true;
            }
        }
    }

    // Handle mouse wheel scrolling
    if (InputManager::GetMouseWheelUp() && my >= 3 && my < FILE_LIST_HEIGHT + 2) {
        if (top > 0) {
            top = std::max(0, top - 3);
            needsRedraw = true;
        }
    }

    if (InputManager::GetMouseWheelDown() && my >= 3 && my < FILE_LIST_HEIGHT + 2) {
        if (top + FILE_LIST_HEIGHT < (int)files.size()) {
            top = std::min((int)files.size() - FILE_LIST_HEIGHT, top + 3);
            needsRedraw = true;
        }
    }
}

void Explorer::DrawStatusBar() {
    // Draw status bar at the bottom
    DoubleBuffer::FillRect(0, DoubleBuffer::GetHeight() - 1, DoubleBuffer::GetWidth(), 1,
        L' ', colors.statusBarFg | (colors.statusBarBg << 4));

    // Count selected items
    int selectedCount = 0;
    for (const auto& file : files) {
        if (file.selected) selectedCount++;
    }

    // Current selection info
    std::string status;
    if (!files.empty() && sel < (int)files.size()) {
        const FileEntry& selected = files[sel];
        status = selected.name;
        if (selected.dir) {
            status = "[DIR] " + status;
        }
        else {
            status = "[FILE] " + status;
        }
        if (selected.selected) {
            status = "[*]" + status;
        }
        status += " | ";
    }

    status += "Items: " + std::to_string(files.size());
    if (selectedCount > 0) {
        status += " | Selected: " + std::to_string(selectedCount);
    }
    status += " | F1-F4:Colors | Space:Toggle | Enter:Open | ESC:Quit";

    DoubleBuffer::DrawText(2, DoubleBuffer::GetHeight() - 1, status,
        colors.statusBarFg | (colors.statusBarBg << 4));

    // Page indicator
    if (files.size() > (size_t)FILE_LIST_HEIGHT) {
        int page = (top / FILE_LIST_HEIGHT) + 1;
        int totalPages = (files.size() + FILE_LIST_HEIGHT - 1) / FILE_LIST_HEIGHT;
        std::string pageStr = "Page " + std::to_string(page) + "/" + std::to_string(totalPages);
        DoubleBuffer::DrawText(DoubleBuffer::GetWidth() - 12, DoubleBuffer::GetHeight() - 1,
            pageStr, colors.statusBarFg | (colors.statusBarBg << 4));
    }
}

void Explorer::DrawInfoPanel() {
    int width = DoubleBuffer::GetWidth();
    int panelHeight = DoubleBuffer::GetHeight() - (FILE_LIST_HEIGHT + 3);

    // Info panel box
    DoubleBuffer::DrawBox(0, FILE_LIST_HEIGHT + 3, width, panelHeight, colors.infoPanelBorder);

    // Panel title
    std::string title = " INFORMATION ";
    int titleX = (width - (int)title.length()) / 2;
    DoubleBuffer::DrawText(titleX, FILE_LIST_HEIGHT + 3, title, colors.infoPanelTitle);

    // Separator
    for (int x = 1; x < width - 1; x++) {
        DoubleBuffer::Set(x, FILE_LIST_HEIGHT + 5, L'-', colors.fileListSeparator);
    }

    // Current path
    std::string pathStr = cur.string();
    if (pathStr.length() > (size_t)(width - 4)) {
        pathStr = "..." + pathStr.substr(pathStr.length() - (width - 7));
    }
    DoubleBuffer::DrawText(2, FILE_LIST_HEIGHT + 4, pathStr,
        colors.infoPanelFg | (colors.infoPanelBg << 4));

    // Mouse position
    int mouseX, mouseY;
    InputManager::GetMousePos(mouseX, mouseY);
    std::string mousePos = "Mouse: " + std::to_string(mouseX) + "," + std::to_string(mouseY);
    DoubleBuffer::DrawText(width - 20, FILE_LIST_HEIGHT + 4, mousePos,
        (YELLOW) | (colors.infoPanelBg << 4));

    // Selected file information
    if (!files.empty() && sel < (int)files.size()) {
        const FileEntry& selected = files[sel];

        DoubleBuffer::DrawText(2, FILE_LIST_HEIGHT + 6, "Name:",
            colors.infoPanelFg | (colors.infoPanelBg << 4));
        std::string displayName = selected.name;
        if (displayName.length() > 40) displayName = displayName.substr(0, 37) + "...";
        DoubleBuffer::DrawText(10, FILE_LIST_HEIGHT + 6, displayName,
            (selected.dir ? colors.dirColor : colors.fileColor) | (colors.infoPanelBg << 4));

        DoubleBuffer::DrawText(2, FILE_LIST_HEIGHT + 7, "Type:",
            colors.infoPanelFg | (colors.infoPanelBg << 4));
        if (selected.dir) {
            DoubleBuffer::DrawText(10, FILE_LIST_HEIGHT + 7, "Folder",
                colors.dirColor | (colors.infoPanelBg << 4));
        }
        else {
            std::string ext = GetFileExtension(selected.name);
            if (ext.empty()) ext = "File";
            else ext = ext.substr(1) + " File";
            DoubleBuffer::DrawText(10, FILE_LIST_HEIGHT + 7, ext,
                colors.extColor | (colors.infoPanelBg << 4));
        }

        DoubleBuffer::DrawText(2, FILE_LIST_HEIGHT + 8, "Selected:",
            colors.infoPanelFg | (colors.infoPanelBg << 4));
        if (selected.selected) {
            DoubleBuffer::DrawText(12, FILE_LIST_HEIGHT + 8, "YES",
                (GREEN) | (colors.infoPanelBg << 4));
        }
        else {
            DoubleBuffer::DrawText(12, FILE_LIST_HEIGHT + 8, "NO",
                (RED) | (colors.infoPanelBg << 4));
        }

        DoubleBuffer::DrawText(30, FILE_LIST_HEIGHT + 6, "Size:",
            colors.infoPanelFg | (colors.infoPanelBg << 4));
        if (selected.dir) {
            DoubleBuffer::DrawText(36, FILE_LIST_HEIGHT + 6, "<DIR>",
                colors.dirColor | (colors.infoPanelBg << 4));
        }
        else {
            char sizeStr[32];
            if (selected.size < 1024) {
                sprintf_s(sizeStr, "%llu B", (unsigned long long)selected.size);
            }
            else if (selected.size < 1024 * 1024) {
                sprintf_s(sizeStr, "%.1f KB", selected.size / 1024.0);
            }
            else {
                sprintf_s(sizeStr, "%.1f MB", selected.size / (1024.0 * 1024));
            }
            DoubleBuffer::DrawText(36, FILE_LIST_HEIGHT + 6, sizeStr,
                colors.sizeColor | (colors.infoPanelBg << 4));
        }

        DoubleBuffer::DrawText(30, FILE_LIST_HEIGHT + 7, "Modified:",
            colors.infoPanelFg | (colors.infoPanelBg << 4));
        DoubleBuffer::DrawText(40, FILE_LIST_HEIGHT + 7, selected.date,
            colors.dateColor | (colors.infoPanelBg << 4));

        DoubleBuffer::DrawText(2, FILE_LIST_HEIGHT + 9, "Path:",
            colors.infoPanelFg | (colors.infoPanelBg << 4));
        std::string fullPath = selected.path.string();
        if (fullPath.length() > 70) {
            fullPath = "..." + fullPath.substr(fullPath.length() - 67);
        }
        DoubleBuffer::DrawText(8, FILE_LIST_HEIGHT + 9, fullPath,
            (DGRAY) | (colors.infoPanelBg << 4));

        if (!selected.dir) {
            DoubleBuffer::DrawText(2, FILE_LIST_HEIGHT + 10, "Attributes:",
                colors.infoPanelFg | (colors.infoPanelBg << 4));
            std::string attrs = GetFileAttributes(selected.path);
            DoubleBuffer::DrawText(14, FILE_LIST_HEIGHT + 10, "[" + attrs + "]",
                colors.attrColor | (colors.infoPanelBg << 4));
        }
    }

    // Statistics
    int selectedCount = 0;
    for (const auto& file : files) {
        if (file.selected) selectedCount++;
    }

    DoubleBuffer::DrawText(60, FILE_LIST_HEIGHT + 6, "Statistics:",
        colors.infoPanelFg | (colors.infoPanelBg << 4));
    DoubleBuffer::DrawText(60, FILE_LIST_HEIGHT + 7, "Folders: " + std::to_string(totalDirs),
        colors.dirColor | (colors.infoPanelBg << 4));
    DoubleBuffer::DrawText(60, FILE_LIST_HEIGHT + 8, "Files: " + std::to_string(totalFiles),
        colors.fileColor | (colors.infoPanelBg << 4));

    if (selectedCount > 0) {
        DoubleBuffer::DrawText(60, FILE_LIST_HEIGHT + 9, "Selected: " + std::to_string(selectedCount),
            (YELLOW) | (colors.infoPanelBg << 4));
    }

    char totalSizeStr[32];
    if (totalSize < 1024) {
        sprintf_s(totalSizeStr, "Total: %llu B", (unsigned long long)totalSize);
    }
    else if (totalSize < 1024 * 1024) {
        sprintf_s(totalSizeStr, "Total: %.1f KB", totalSize / 1024.0);
    }
    else if (totalSize < 1024 * 1024 * 1024) {
        sprintf_s(totalSizeStr, "Total: %.1f MB", totalSize / (1024.0 * 1024));
    }
    else {
        sprintf_s(totalSizeStr, "Total: %.1f GB", totalSize / (1024.0 * 1024 * 1024));
    }
    DoubleBuffer::DrawText(60, FILE_LIST_HEIGHT + 10, totalSizeStr,
        colors.sizeColor | (colors.infoPanelBg << 4));

    // Controls
    DoubleBuffer::DrawText(2, FILE_LIST_HEIGHT + 13, "Mouse: Click=Toggle  DblClick=Open  Wheel=Scroll",
        colors.infoPanelFg | (colors.infoPanelBg << 4));
    DoubleBuffer::DrawText(2, FILE_LIST_HEIGHT + 14, "Keys: Arrows=Move  Space=Toggle  Enter=Open  F5=Refresh  ESC=Quit",
        colors.infoPanelFg | (colors.infoPanelBg << 4));

    // Status message
    if (GetTickCount64() < statusMessageTime) {
        int msgX = (DoubleBuffer::GetWidth() - (int)statusMessage.length()) / 2;
        DoubleBuffer::FillRect(msgX - 2, FILE_LIST_HEIGHT + 15,
            (int)statusMessage.length() + 4, 1, L' ', (BLACK) | (YELLOW << 4));
        DoubleBuffer::DrawText(msgX, FILE_LIST_HEIGHT + 15, statusMessage,
            (BLACK) | (YELLOW << 4));
    }
}

void Explorer::DrawUI() {
    DoubleBuffer::ClearBoth(colors.fileListFg | (colors.fileListBg << 4));

    DrawFileList();
    DrawInfoPanel();
    DrawStatusBar();

    DoubleBuffer::ScheduleFlip();
    needsRedraw = false;
}

void Explorer::Run() {
    DoubleBuffer::Init();
    InputManager::Start();

    LoadDirectory();
    DrawUI();

    while (true) {
        // Handle keyboard input
        int key = InputManager::GetKey();

        if (key != 0) {
            bool redrawNeeded = false;

            switch (key) {
            case 32: // SPACE BAR - Toggle selection
                if (!files.empty() && sel < (int)files.size()) {
                    files[sel].selected = !files[sel].selected;
                    SetStatus(files[sel].selected ?
                        "Selected: " + files[sel].name :
                        "Unselected: " + files[sel].name);
                    redrawNeeded = true;
                }
                break;

            case 27: // ESC
                InputManager::Stop();
                return;

            case 13: // ENTER
                if (!files.empty() && sel < (int)files.size()) {
                    if (files[sel].dir) {
                        cur = files[sel].path;
                        sel = top = 0;
                        needsReload = true;
                        SetStatus("Opening: " + files[sel].name);
                    }
                }
                break;

            case 0x4800: // Up
                if (sel > 0) { sel--; redrawNeeded = true; }
                break;

            case 0x5000: // Down
                if (sel < (int)files.size() - 1) { sel++; redrawNeeded = true; }
                break;

            case 0x4B00: // Left arrow
                if (sel > 0) sel = std::max(0, sel - 5);
                redrawNeeded = true;
                break;

            case 0x4D00: // Right arrow
                if (sel < static_cast<int>(files.size()) - 1)
                    sel = std::min(static_cast<int>(files.size()) - 1, sel + 5);
                redrawNeeded = true;
                break;

            case 0x4900: // Page Up
                sel = std::max(0, sel - FILE_LIST_HEIGHT);
                redrawNeeded = true;
                break;

            case 0x5100: // Page Down
                sel = std::min((int)files.size() - 1, sel + FILE_LIST_HEIGHT);
                redrawNeeded = true;
                break;

            case 0x4700: // Home
                sel = 0;
                redrawNeeded = true;
                break;

            case 0x4F00: // End
                sel = std::max(0, (int)files.size() - 1);
                redrawNeeded = true;
                break;

            case 0x3F00: // F5
                needsReload = true;
                SetStatus("Refreshing...");
                break;

            case 0x3B00: // F1 - Default color scheme
                colors.fileListBg = BLACK;
                colors.fileListFg = LGRAY;
                colors.fileListBorder = (GREEN << 4) | WHITE;
                colors.fileListTitle = WHITE | (GREEN << 4);
                colors.fileListHeaderBg = BLUE;
                colors.fileListHeaderFg = WHITE;
                colors.fileListSeparator = GREEN | (BLACK << 4);
                colors.infoPanelBg = BLACK;
                colors.infoPanelFg = LGRAY;
                colors.infoPanelBorder = (CYAN << 4) | WHITE;
                colors.infoPanelTitle = WHITE | (CYAN << 4);
                colors.selectedBg = WHITE;
                colors.selectedFg = BLACK;
                colors.mouseOverBg = DGRAY;
                colors.statusBarBg = LGRAY;
                colors.statusBarFg = BLACK;
                colors.dirColor = CYAN;
                colors.fileColor = LGRAY;
                colors.extColor = YELLOW;
                colors.sizeColor = GREEN;
                colors.dateColor = MAGENTA;
                colors.attrColor = YELLOW;
                SetStatus("Color scheme: Default");
                redrawNeeded = true;
                break;

            case 0x3C00: // F2 - Dark color scheme
                colors.fileListBg = BLACK;
                colors.fileListFg = LGRAY;
                colors.fileListBorder = (DGRAY << 4) | WHITE;
                colors.fileListTitle = WHITE | (DGRAY << 4);
                colors.fileListHeaderBg = BLUE;
                colors.fileListHeaderFg = WHITE;
                colors.fileListSeparator = DGRAY | (BLACK << 4);
                colors.infoPanelBg = BLACK;
                colors.infoPanelFg = LGRAY;
                colors.infoPanelBorder = (BLUE << 4) | WHITE;
                colors.infoPanelTitle = WHITE | (BLUE << 4);
                colors.selectedBg = BLUE;
                colors.selectedFg = WHITE;
                colors.mouseOverBg = DGRAY;
                colors.statusBarBg = DGRAY;
                colors.statusBarFg = WHITE;
                colors.dirColor = CYAN;
                colors.fileColor = LGRAY;
                colors.extColor = YELLOW;
                colors.sizeColor = GREEN;
                colors.dateColor = MAGENTA;
                colors.attrColor = YELLOW;
                SetStatus("Color scheme: Dark");
                redrawNeeded = true;
                break;

            case 0x3D00: // F3 - High contrast
                colors.fileListBg = WHITE;
                colors.fileListFg = BLACK;
                colors.fileListBorder = (BLACK << 4) | WHITE;
                colors.fileListTitle = WHITE | (BLACK << 4);
                colors.fileListHeaderBg = BLUE;
                colors.fileListHeaderFg = WHITE;
                colors.fileListSeparator = BLACK | (WHITE << 4);
                colors.infoPanelBg = WHITE;
                colors.infoPanelFg = BLACK;
                colors.infoPanelBorder = (BLACK << 4) | WHITE;
                colors.infoPanelTitle = WHITE | (BLACK << 4);
                colors.selectedBg = BLACK;
                colors.selectedFg = WHITE;
                colors.mouseOverBg = LGRAY;
                colors.statusBarBg = BLACK;
                colors.statusBarFg = WHITE;
                colors.dirColor = BLUE;
                colors.fileColor = BLACK;
                colors.extColor = RED;
                colors.sizeColor = GREEN;
                colors.dateColor = MAGENTA;
                colors.attrColor = RED;
                SetStatus("Color scheme: High Contrast");
                redrawNeeded = true;
                break;

            case 0x3E00: // F4 - Custom scheme
                colors.fileListBg = BLACK;
                colors.fileListFg = GREEN;
                colors.fileListBorder = (MAGENTA << 4) | WHITE;
                colors.fileListTitle = WHITE | (MAGENTA << 4);
                colors.fileListHeaderBg = BLUE;
                colors.fileListHeaderFg = WHITE;
                colors.fileListSeparator = MAGENTA | (BLACK << 4);
                colors.infoPanelBg = BLACK;
                colors.infoPanelFg = GREEN;
                colors.infoPanelBorder = (RED << 4) | WHITE;
                colors.infoPanelTitle = WHITE | (RED << 4);
                colors.selectedBg = RED;
                colors.selectedFg = WHITE;
                colors.mouseOverBg = DGRAY;
                colors.statusBarBg = BLUE;
                colors.statusBarFg = YELLOW;
                colors.dirColor = CYAN;
                colors.fileColor = GREEN;
                colors.extColor = YELLOW;
                colors.sizeColor = GREEN;
                colors.dateColor = MAGENTA;
                colors.attrColor = YELLOW;
                SetStatus("Color scheme: Custom");
                redrawNeeded = true;
                break;
            }

            if (redrawNeeded) {
                // Adjust viewport
                if (sel < top) top = sel;
                else if (sel >= top + FILE_LIST_HEIGHT) top = sel - FILE_LIST_HEIGHT + 1;
                needsRedraw = true;
            }
        }

        // Handle mouse
        HandleMouse();

        // Reload directory if requested
        if (needsReload) {
            LoadDirectory();
            needsRedraw = true;
            needsReload = false;
        }

        if (needsRedraw) DrawUI();

        if (DoubleBuffer::NeedsFlip()) DoubleBuffer::Flip();

        // Clear expired status message
        if (statusMessageTime > 0 && GetTickCount64() > statusMessageTime) {
            statusMessage.clear();
            statusMessageTime = 0;
            needsRedraw = true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}