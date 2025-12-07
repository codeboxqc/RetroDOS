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
#include <shellapi.h>
#include <fstream>
#include "json.hpp"
#pragma comment(lib, "shell32.lib")

#include "ddos.h"
#include "retro.h"
#include "basic.h"

using json = nlohmann::json;

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
    mouseMoved(false),

    showDrivePanel(true),
    drivePanelX(2),
    drivePanelY(1),
    btnCopyX(50), btnCopyY(1),
    btnPasteX(62), btnPasteY(1),
    btnDeleteX(74), btnDeleteY(1),
    btnRenameX(88), btnRenameY(1),
    btnNewFolderX(102), btnNewFolderY(1),

    showingAttribButtons(false),
    attribButtonsX(0),
    attribButtonsY(0),

    selectedDrive(-1)


{
    std::ifstream f("theme.json");
    if (f.good()) {
        json data = json::parse(f);
        colors.fileListBg = data.value("fileListBg", BLACK);
        colors.fileListFg = data.value("fileListFg", LGRAY);
        colors.fileListBorder = data.value("fileListBorder", (GREEN << 4) | WHITE);
        colors.fileListTitle = data.value("fileListTitle", (BLACK) | (GREEN << 4));
        colors.fileListHeaderBg = data.value("fileListHeaderBg", BLUE);
        colors.fileListHeaderFg = data.value("fileListHeaderFg", WHITE);
        colors.fileListSeparator = data.value("fileListSeparator", (BLACK << 4) | GREEN);
        colors.infoPanelBg = data.value("infoPanelBg", BLACK);
        colors.infoPanelFg = data.value("infoPanelFg", LGRAY);
        colors.infoPanelBorder = data.value("infoPanelBorder", (CYAN << 4) | WHITE);
        colors.infoPanelTitle = data.value("infoPanelTitle", (BLACK) | (CYAN << 4));
        colors.selectedBg = data.value("selectedBg", WHITE);
        colors.selectedFg = data.value("selectedFg", BLACK);
        colors.mouseOverBg = data.value("mouseOverBg", DGRAY);
        colors.statusBarBg = data.value("statusBarBg", LGRAY);
        colors.statusBarFg = data.value("statusBarFg", BLACK);
        colors.dirColor = data.value("dirColor", CYAN);
        colors.fileColor = data.value("fileColor", LGRAY);
        colors.extColor = data.value("extColor", YELLOW);
        colors.sizeColor = data.value("sizeColor", GREEN);
        colors.dateColor = data.value("dateColor", MAGENTA);
        colors.attrColor = data.value("attrColor", YELLOW);
    } else {
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


// =====================
//  load drive letters
// =====================
void Explorer::LoadDriveLetters() {
    driveLetters.clear();
    DWORD drives = GetLogicalDrives();

    for (char letter = 'A'; letter <= 'Z'; letter++) {
        if (drives & 1) {
            std::string drive = std::string(1, letter) + ":";
            driveLetters.push_back(drive);
        }
        drives >>= 1;
    }

    // Find current drive
    std::string currentDrive = cur.string().substr(0, 2);
    for (size_t i = 0; i < driveLetters.size(); i++) {
        if (driveLetters[i] == currentDrive) {
            selectedDrive = i;
            break;
        }
    }
}


// =====================
// Helper function to get drive type
// =====================
std::string Explorer::GetDriveType(const std::string& drive) {


        std::string path = drive + "\\";

    // Call Windows API function directly with 'A' suffix
    UINT type = ::GetDriveTypeA(path.c_str());


    switch (type) {
    case DRIVE_FIXED: return "HDD";
    case DRIVE_REMOVABLE: return "REM";
    case DRIVE_CDROM: return "CD";
    case DRIVE_RAMDISK: return "RAM";
    case DRIVE_REMOTE: return "NET";
    default: return "UNK";
    }
}


// =====================
//   draw drive letters
// =====================
// =====================
//   draw drive letters with Shift+ hint
// =====================
void Explorer::DrawDriveLetters()
{
    if (driveLetters.empty()) return;

    const int y = 1;
    int x = 2;

    // Draw hint text "Shift+" before drive letters
    DoubleBuffer::DrawText(x, y, "Shift+", WHITE | (BLUE << 4));
    x += 7;  // Move past "Shift+ "

    int mx, my;
    InputManager::GetMousePos(mx, my);

    for (size_t i = 0; i < driveLetters.size(); ++i)
    {
        std::string drive = driveLetters[i];            // "C:", "D:", etc.
        if (drive.back() != ':') drive += ":";

        bool current = (int)i == selectedDrive;
        bool hover = (my == y && mx >= x && mx < x + 4);

        WORD color = current ? (YELLOW | (BLUE << 4))     // current drive = yellow on blue
            : hover ? (LRED | (RED << 4))        // mouse over = light red on red
            : (WHITE | (BLUE << 4));     // normal = white on blue

        // Draw the drive letter with space around it
        char buf[8];
        sprintf_s(buf, " %s ", drive.c_str());        // " C: "  " D: "  etc.

        DoubleBuffer::DrawText(x, y, buf, color);

        // Click → change drive instantly
        if (hover && InputManager::GetMouseLeftClick())
        {
            ChangeDrive((int)i);
        }

        x += 4;   // spacing between drives
    }
}


// =====================
// Helper to toggle file attributes
// =====================
void Explorer::ToggleFileAttribute(const fs::path& path, DWORD attribute) {
    try {
        DWORD currentAttrs = GetFileAttributesW(path.wstring().c_str());
        if (currentAttrs == INVALID_FILE_ATTRIBUTES) return;

        DWORD newAttrs;
        if (currentAttrs & attribute) {
            // Remove attribute
            newAttrs = currentAttrs & ~attribute;  // ← This removes it
        }
        else {
            // Add attribute
            newAttrs = currentAttrs | attribute;   // ← This adds it
        }

        SetFileAttributesW(path.wstring().c_str(), newAttrs);
        needsReload = true;
    }
    catch (...) {
        SetStatus("Failed to change attribute", 3000);
    }
}


// =====================
// Draw attribute buttons when a file is clicked
// =====================

void Explorer::DrawAttributeButtons() {
    if (!showingAttribButtons || sel >= (int)files.size() || files.empty())
        return;

    const FileEntry& file = files[sel];
    if (file.dir || file.name == "..") return;  // Don't show for folders

    int mx, my;
    InputManager::GetMousePos(mx, my);

    // Get current attributes
    DWORD attrs = GetFileAttributesW(file.path.wstring().c_str());
    bool isReadOnly = (attrs & FILE_ATTRIBUTE_READONLY) != 0;
    bool isHidden = (attrs & FILE_ATTRIBUTE_HIDDEN) != 0;
    bool isArchive = (attrs & FILE_ATTRIBUTE_ARCHIVE) != 0;
    bool isSystem = (attrs & FILE_ATTRIBUTE_SYSTEM) != 0;

    // Position RIGHT AFTER DEL-DELETE button on same line (y=2)
    int x = 97;
    int y = 2;

    // COLOR SCHEME:
    // - Background: ALWAYS BLACK
    // - Default text (attribute OFF): GREEN
    // - Active text (file HAS attribute): BLUE
    // - Mouse over: YELLOW

    // Draw R button (ReadOnly)
    bool rHover = (my == y && mx >= x && mx < x + 3);
    WORD rColor;
    if (rHover) {
        rColor = YELLOW | (BLACK << 4);  // Mouse over
    }
    else if (isReadOnly) {
        rColor = BLUE | (BLACK << 4);    // File HAS this attribute = BLUE
    }
    else {
        rColor = GREEN | (BLACK << 4);   // Default (attribute OFF) = GREEN
    }
    DoubleBuffer::DrawText(x, y, "[R]", rColor);
    x += 4;

    // Draw H button (Hidden)
    bool hHover = (my == y && mx >= x && mx < x + 3);
    WORD hColor;
    if (hHover) {
        hColor = YELLOW | (BLACK << 4);
    }
    else if (isHidden) {
        hColor = BLUE | (BLACK << 4);
    }
    else {
        hColor = GREEN | (BLACK << 4);
    }
    DoubleBuffer::DrawText(x, y, "[H]", hColor);
    x += 4;

    // Draw A button (Archive)
    bool aHover = (my == y && mx >= x && mx < x + 3);
    WORD aColor;
    if (aHover) {
        aColor = YELLOW | (BLACK << 4);
    }
    else if (isArchive) {
        aColor = BLUE | (BLACK << 4);
    }
    else {
        aColor = GREEN | (BLACK << 4);
    }
    DoubleBuffer::DrawText(x, y, "[A]", aColor);
    x += 4;

    // Draw S button (System)
    bool sHover = (my == y && mx >= x && mx < x + 3);
    WORD sColor;
    if (sHover) {
        sColor = YELLOW | (BLACK << 4);
    }
    else if (isSystem) {
        sColor = BLUE | (BLACK << 4);
    }
    else {
        sColor = GREEN | (BLACK << 4);
    }
    DoubleBuffer::DrawText(x, y, "[S]", sColor);
}

// =====================
// New function to handle drive selection
// =====================
void Explorer::HandleDriveSelection(int mx, int my) {
    if (!showDrivePanel) return;

    int startX = drivePanelX;
    int startY = drivePanelY;

    for (size_t i = 0; i < driveLetters.size(); i++) {
        int x = startX + (i % 8) * 5;
        int y = startY + (i / 8) * 2;

        if (mx >= x && mx <= x + 4 && my >= y && my <= y + 1) {
            if (InputManager::GetMouseLeftClick()) {
                ChangeDrive(i);
                break;
            }
        }
    }
}




// =====================
// NEW: Change drive by letter (Shift + Letter)
// =====================
void Explorer::ChangeDriveByLetter(char letter) {
    // Convert to uppercase
    letter = toupper(letter);

    // Find the drive in our list
    std::string targetDrive = std::string(1, letter) + ":";

    for (size_t i = 0; i < driveLetters.size(); i++) {
        if (driveLetters[i] == targetDrive) {
            ChangeDrive(i);
            return;
        }
    }

    SetStatus("Drive " + targetDrive + " not found", 2000);
}

// =====================
// Function to change drive
// =====================
void Explorer::ChangeDrive(int driveIndex)
{
    if (driveIndex < 0 || driveIndex >= (int)driveLetters.size()) return;

    std::string newDrive = driveLetters[driveIndex];          // e.g. "C:"
    fs::path newPath = fs::path(newDrive + "\\");             // "C:\"

    try {
        // THIS LINE WAS MISSING IN MOST VERSIONS → this actually changes the drive!
        fs::current_path(newPath);        // ← THIS IS THE KEY LINE

        cur = newPath;
        selectedDrive = driveIndex;
        sel = top = 0;
        needsReload = true;
        SetStatus("Drive changed → " + newDrive);
    }
    catch (const std::exception& e) {
        SetStatus("Access denied: " + newDrive, 3000);
    }
}



// =====================
// Handle button clicks
// =====================
void Explorer::HandleButtonClicks(int mx, int my) {
    if (!InputManager::GetMouseLeftClick()) return;

    // Check COPY button
    if (mx >= btnCopyX && mx <= btnCopyX + 6 && my == btnCopyY) {
        CopySelected();
        return;
    }

    // Check PASTE button
    if (mx >= btnPasteX && mx <= btnPasteX + 7 && my == btnPasteY) {
        PasteClipboard();
        return;
    }

    // Check DELETE button
    if (mx >= btnDeleteX && mx <= btnDeleteX + 8 && my == btnDeleteY) {
        DeleteSelected();
        return;
    }

    // Check RENAME button
    if (mx >= btnRenameX && mx <= btnRenameX + 8 && my == btnRenameY) {
        RenameSelected();
        return;
    }

    // Check NEW FOLDER button
    if (mx >= btnNewFolderX && mx <= btnNewFolderX + 12 && my == btnNewFolderY) {
        CreateNewFolder();
        return;
    }
}





// =====================
// New function to draw operation buttons
// =====================
void Explorer::DrawOperationButtons()
{
    // Adjust position to fit all buttons
    const int BUTTONS_X = 2;      // Start from left edge
    const int BUTTONS_Y = 2;       // move whole toolbar up/down (1 = right under title)

    int mx, my;
    InputManager::GetMousePos(mx, my);

    struct Button {
        std::string text;
        void(Explorer::* action)();
        int width; // Pre-calculate width
    };

    // Shorter button texts to fit on screen
    Button buttons[] = {
    {"F2-RENAME",    &Explorer::RenameSelected, 10},
    {"F5-COPY",      &Explorer::CopySelected, 9},
    {"F6-CUT",       &Explorer::CutSelected, 8},
    {"F7-PASTE",     &Explorer::PasteClipboard, 10},
    {"F8-NEW",       &Explorer::CreateNewFolder, 8},
    {"F9-CLEAR",     &Explorer::ClearSelection, 10},
    {"DEL-DELETE",   &Explorer::DeleteSelected, 11}
    };

    int x = BUTTONS_X;
    int screenWidth = DoubleBuffer::GetWidth();

    for (const auto& btn : buttons)
    {
        // Check if button would go off screen
        if (x + btn.width > screenWidth - 2) {
            break; // Don't draw off-screen buttons
        }

        bool hover = (my == BUTTONS_Y && mx >= x && mx < x + btn.width);

        WORD bg = hover ? BLACK : BLACK;
        WORD fg = hover ? RED : GREEN;

        // Draw the button with brackets
        std::string displayText = "[" + btn.text + "]";
        DoubleBuffer::DrawText(x + 1, BUTTONS_Y, displayText, fg | (bg << 4));

        if (hover && InputManager::GetMouseLeftClick())
            (this->*(btn.action))();

        x += btn.width + 4;  // Reduced space between buttons
    }
}





// =====================
// Helper function for Copy
// =====================
void Explorer::CopySelected() {
    clipboard.clear();
    cutMode = false;  // Copy mode, not cut/move

    int count = 0;

    // Add all selected files
    for (size_t i = 0; i < files.size(); i++) {
        if (files[i].selected || i == (size_t)sel) {
            if (files[i].name != "..") {
                clipboard.push_back(files[i]);
                count++;
                // Clear selection after copying?
                files[i].selected = false; // Optional: clear selection
            }
        }
    }

    // If nothing selected with spacebar, just copy the current selection
    if (count == 0 && sel < (int)files.size() && files[sel].name != "..") {
        clipboard.push_back(files[sel]);
        count = 1;
    }

    if (count > 0) {
        SetStatus("Copied " + std::to_string(count) + " items to clipboard");
        needsRedraw = true;
    }
    else {
        SetStatus("No items selected to copy", 2000);
    }
}



// =====================
// Helper function for Rename
// =====================
void Explorer::RenameSelected() {
    if (files.empty() || sel >= (int)files.size() || files[sel].name == "..") {
        SetStatus("Cannot rename this item", 2000);
        return;
    }

    // Enter rename mode
    inRenameMode = true;
    renameIndex = sel;
    renameBuffer = files[sel].name;

    SetStatus("RENAME: Type new name | ENTER=Save | ESC=Cancel | BACKSPACE=Delete char", 99999);
    needsRedraw = true;
}




// =====================
// Helper function for New Folder
// =====================
void Explorer::CreateNewFolder() {
    std::string folderName = "New Folder";
    int counter = 1;

    while (fs::exists(cur / folderName)) {
        folderName = "New Folder (" + std::to_string(counter++) + ")";
    }

    try {
        fs::create_directory(cur / folderName);
        needsReload = true;
        SetStatus("Created folder: " + folderName);
    }
    catch (...) {
        SetStatus("Failed to create folder", 3000);
    }
}



void Explorer::FastCopyClipboard() {
    if (clipboard.empty()) return;
    int done = 0;
    for (const auto& src : clipboard) {
        fs::path dest = cur / src.name;
        if (fs::exists(dest)) dest = cur / ("Copy_of_" + src.name);

        if (src.size < 50 * 1024 * 1024) {  // < 50 MB → load in RAM
            std::vector<char> buffer(src.size);
            FILE* f; fopen_s(&f, src.path.string().c_str(), "rb");
            if (f) { fread(buffer.data(), 1, src.size, f); fclose(f); }
            FILE* out; fopen_s(&out, dest.string().c_str(), "wb");
            if (out) { fwrite(buffer.data(), 1, src.size, out); fclose(out); }
        }
        else {
            fs::copy_file(src.path, dest, fs::copy_options::overwrite_existing);
        }
        done++;
        SetStatus("FastCopy: " + std::to_string(done) + "/" + std::to_string(clipboard.size()));
    }
    if (cutMode) { for (const auto& f : clipboard) fs::remove(f.path); }
    clipboard.clear();
    needsReload = true;
}

void Explorer::PasteClipboard() {
    if (clipboard.empty()) {
        SetStatus("Clipboard is empty", 2000);
        return;
    }

    int successCount = 0;
    int errorCount = 0;

    for (const auto& item : clipboard) {
        try {
            fs::path source = item.path;
            fs::path destination = cur / item.name;

            // Handle duplicate names
            if (fs::exists(destination)) {
                int counter = 1;
                std::string baseName = item.name;
                std::string extension = "";

                // Split filename and extension
                size_t dotPos = baseName.find_last_of('.');
                if (dotPos != std::string::npos) {
                    extension = baseName.substr(dotPos);
                    baseName = baseName.substr(0, dotPos);
                }

                do {
                    destination = cur / (baseName + " (" + std::to_string(counter) + ")" + extension);
                    counter++;
                } while (fs::exists(destination));
            }

            if (item.dir) {
                // Copy directory recursively
                fs::copy(source, destination, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
            }
            else {
                // Copy file
                fs::copy_file(source, destination, fs::copy_options::overwrite_existing);
            }

            successCount++;
        }
        catch (const std::exception& e) {
            errorCount++;
            // Optional: log error
        }
    }

    // If this was a cut operation, remove source files
    if (cutMode) {
        for (const auto& item : clipboard) {
            try {
                if (item.dir) {
                    fs::remove_all(item.path);
                }
                else {
                    fs::remove(item.path);
                }
            }
            catch (...) {
                // Ignore removal errors
            }
        }
        cutMode = false;
    }

    clipboard.clear();
    needsReload = true;

    std::string status = "Pasted " + std::to_string(successCount) + " items";
    if (errorCount > 0) {
        status += " (" + std::to_string(errorCount) + " failed)";
    }
    SetStatus(status);
}


void Explorer::ClearSelection() {
    int count = 0;
    for (auto& file : files) {
        if (file.selected) {
            file.selected = false;
            count++;
        }
    }
    if (count > 0) {
        SetStatus("Cleared " + std::to_string(count) + " selections");
        needsRedraw = true;
    }
}

void Explorer::CutSelected() {
    clipboard.clear();
    cutMode = true;  // This is a cut/move operation

    int count = 0;

    for (size_t i = 0; i < files.size(); i++) {
        if (files[i].selected || i == (size_t)sel) {
            if (files[i].name != "..") {
                clipboard.push_back(files[i]);
                count++;
            }
        }
    }

    // If nothing selected with spacebar, just cut the current selection
    if (count == 0 && sel < (int)files.size() && files[sel].name != "..") {
        clipboard.push_back(files[sel]);
        count = 1;
    }

    if (count > 0) {
        SetStatus("Cut " + std::to_string(count) + " items");
        needsRedraw = true;
    }
    else {
        SetStatus("No items selected to cut", 2000);
    }
}

/*

void Explorer::DeleteSelected() {
    int count = 0;
    for (const auto& f : files) if (f.selected || (&f - &files[0]) == sel) {
        fs::remove_all(f.path);
        count++;
    }
    SetStatus("Deleted " + std::to_string(count) + " items");
    needsReload = true;
}
*/

void Explorer::DeleteSelected() {
    std::vector<std::string> toDelete;
    int count = 0;

    // Collect files to delete
    for (size_t i = 0; i < files.size(); i++) {
        if (files[i].selected || i == (size_t)sel) {
            if (files[i].name != "..") {
                toDelete.push_back(files[i].path.string());
                count++;
            }
        }
    }

    if (count == 0) {
        SetStatus("No items selected to delete", 2000);
        return;
    }

    // Build double-null-terminated string for Windows API
    std::string paths;
    for (const auto& path : toDelete) {
        paths += path;
        paths += '\0';  // Null separator between paths
    }
    paths += '\0';  // Extra null at end (double-null terminated)

    // Use Windows SHFileOperation to send to Recycle Bin
    SHFILEOPSTRUCTA fileOp = {};
    fileOp.hwnd = NULL;
    fileOp.wFunc = FO_DELETE;
    fileOp.pFrom = paths.c_str();
    fileOp.pTo = NULL;
    fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;
    fileOp.fAnyOperationsAborted = FALSE;
    fileOp.hNameMappings = NULL;
    fileOp.lpszProgressTitle = NULL;

    int result = SHFileOperationA(&fileOp);

    if (result == 0 && !fileOp.fAnyOperationsAborted) {
        SetStatus("Moved " + std::to_string(count) + " item(s) to Recycle Bin");
        needsReload = true;
    }
    else {
        SetStatus("Failed to delete some items", 3000);
    }
}








void Explorer::SetStatus(const std::string& msg, unsigned long duration) {
    statusMessage = msg;
    statusMessageTime = GetTickCount64() + duration;
    needsRedraw = true;
}

void Explorer::LoadDirectory() {
    allFiles.clear();
    totalFiles = 0;
    totalDirs = 0;
    totalSize = 0;

    // Add parent directory
    if (cur.has_parent_path() && cur.parent_path() != cur) {
        allFiles.push_back({ "..", true, 0, "", cur.parent_path(), false });
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

            allFiles.push_back(std::move(fe));
        }
        catch (...) {}
    }

    // Sort directories first, then by name
    std::sort(allFiles.begin(), allFiles.end(), [](const FileEntry& a, const FileEntry& b) {
        if (a.dir != b.dir) return a.dir > b.dir;
        return _stricmp(a.name.c_str(), b.name.c_str()) < 0;
        });

    ApplyFilter();
    needsReload = false;
    SetStatus("Loaded " + std::to_string(files.size()) + " items");
}

void Explorer::DrawFileList() {
    int width = DoubleBuffer::GetWidth();

    DoubleBuffer::DrawBox(0, 0, width, FILE_LIST_HEIGHT + 3, colors.fileListBorder);

    std::string title = " RETRO EXPLORER " + cur.string() + " ";
    int tx = (width - (int)title.length()) / 2;
    DoubleBuffer::DrawText(tx, 0, title, colors.fileListTitle);

    // Header
    DoubleBuffer::FillRect(1, 1, width - 2, 1, L' ', colors.fileListHeaderFg | (colors.fileListHeaderBg << 4));
    DoubleBuffer::DrawText(2, 1, "                                                           ",
        colors.fileListHeaderFg | (colors.fileListHeaderBg << 4));

    for (int x = 1; x < width - 1; x++)
        DoubleBuffer::Set(x, 2, BOX_H, colors.fileListSeparator);

    int mouseX, mouseY;
    InputManager::GetMousePos(mouseX, mouseY);

    for (int i = 0; i < FILE_LIST_HEIGHT; i++) {
        int y = 3 + i;
        int idx = top + i;
        if (idx >= (int)files.size()) {
            for (int x = 1; x < width - 1; x++)
                DoubleBuffer::Set(x, y, L' ', colors.fileListFg | (colors.fileListBg << 4));
            continue;
        }

        bool isSelected = (idx == sel);
        bool isMultiSelected = files[idx].selected;  // Check if file is selected with spacebar
        bool isMouseOver = (mouseY == y && mouseX >= 1 && mouseX < width - 1);
        bool isRenaming = (inRenameMode && idx == renameIndex);

        WORD bg = colors.fileListBg;
        WORD fg = files[idx].dir ? colors.dirColor : colors.fileColor;

        if (isRenaming) {
            bg = YELLOW;
            fg = BLACK;
        }
        else if (isMultiSelected) {
            // Highlight selected files with different color
            bg = BLUE;  // Changed from WHITE to BLUE for better visibility
            fg = WHITE;
        }
        else if (isSelected && isMouseOver) {
            bg = LBLUE;
            fg = WHITE;
        }
        else if (isSelected) {
            bg = WHITE;
            fg = BLACK;
        }
        else if (isMouseOver) {
            bg = DGRAY;
        }

        WORD attr = fg | (bg << 4);

        // Full row background
        for (int x = 1; x < width - 1; x++)
            DoubleBuffer::Set(x, y, L' ', attr);

        // Draw content - Add * prefix for selected items
        std::string name;

        if (isRenaming) {
            // Show rename buffer with blinking cursor
            name = "> " + renameBuffer + "_";
        }
        else {
            name = files[idx].name;
            if (files[idx].dir) name = "[ " + name + " ]";

            // ADD * prefix if selected (but not for rename mode)
            if (files[idx].selected && !isRenaming) {
                name = "* " + name;
            }
        }

        if (name.length() > 34) name = name.substr(0, 31) + "...";

        DoubleBuffer::DrawText(2, y, name, attr);

        // Don't draw size/date/attrs during rename
        if (!isRenaming) {
            if (!files[idx].dir) {
                char sz[20];
                sprintf_s(sz, "%12llu", files[idx].size);
                DoubleBuffer::DrawText(37, y, sz, colors.sizeColor | (bg << 4));
            }
            else {
                DoubleBuffer::DrawText(37, y, "<DIR>      ", colors.dirColor | (bg << 4));
            }

            DoubleBuffer::DrawText(52, y, files[idx].date, colors.dateColor | (bg << 4));
            DoubleBuffer::DrawText(70, y, GetFileAttributes(files[idx].path), colors.attrColor | (bg << 4));
        }
        else {
            // Show rename instructions
            DoubleBuffer::DrawText(37, y, "ENTER=Save ESC=Cancel", (RED) | (bg << 4));
        }
    }
}

// =====================
//  HandleMouse()
// =====================
void Explorer::HandleMouse() {
    int mx, my;
    InputManager::GetMousePos(mx, my);

    if (mx != lastMouseX || my != lastMouseY) {
        lastMouseX = mx;
        lastMouseY = my;
        needsRedraw = true;
    }

    if (inRenameMode) {
        return;
    }

    // Wheel scrolling
    if (InputManager::GetMouseWheelUp()) {
        sel = std::max(0, sel - 3);
        if (sel < top) top = sel;
        needsRedraw = true;
    }
    if (InputManager::GetMouseWheelDown()) {
        sel = std::min((int)files.size() - 1, sel + 3);
        if (sel >= top + FILE_LIST_HEIGHT) top = sel - FILE_LIST_HEIGHT + 1;
        needsRedraw = true;
    }

    // LEFT CLICK HANDLING
    if (InputManager::GetMouseLeftClick()) {

        // ===== PRIORITY 1: CHECK ATTRIBUTE BUTTONS FIRST =====
        if (showingAttribButtons && my == 2 && sel < (int)files.size() && !files.empty()) {
            const FileEntry& file = files[sel];
            if (!file.dir && file.name != "..") {
                int btnX = 97;

                // R button (x: 97-99)
                if (mx >= btnX && mx < btnX + 3) {
                    ToggleFileAttribute(file.path, FILE_ATTRIBUTE_READONLY);
                    DWORD attrs = GetFileAttributesW(file.path.wstring().c_str());
                    bool isReadOnly = (attrs & FILE_ATTRIBUTE_READONLY) != 0;
                    SetStatus((isReadOnly ? "Set" : "Removed") + std::string(" Read-only"));
                    needsReload = true;
                    needsRedraw = true;
                    return;
                }
                btnX += 4;

                // H button (x: 101-103)
                if (mx >= btnX && mx < btnX + 3) {
                    ToggleFileAttribute(file.path, FILE_ATTRIBUTE_HIDDEN);
                    DWORD attrs = GetFileAttributesW(file.path.wstring().c_str());
                    bool isHidden = (attrs & FILE_ATTRIBUTE_HIDDEN) != 0;
                    SetStatus((isHidden ? "Set" : "Removed") + std::string(" Hidden"));
                    needsReload = true;
                    needsRedraw = true;
                    return;
                }
                btnX += 4;

                // A button (x: 105-107)
                if (mx >= btnX && mx < btnX + 3) {
                    ToggleFileAttribute(file.path, FILE_ATTRIBUTE_ARCHIVE);
                    DWORD attrs = GetFileAttributesW(file.path.wstring().c_str());
                    bool isArchive = (attrs & FILE_ATTRIBUTE_ARCHIVE) != 0;
                    SetStatus((isArchive ? "Set" : "Removed") + std::string(" Archive"));
                    needsReload = true;
                    needsRedraw = true;
                    return;
                }
                btnX += 4;

                // S button (x: 109-111)
                if (mx >= btnX && mx < btnX + 3) {
                    ToggleFileAttribute(file.path, FILE_ATTRIBUTE_SYSTEM);
                    DWORD attrs = GetFileAttributesW(file.path.wstring().c_str());
                    bool isSystem = (attrs & FILE_ATTRIBUTE_SYSTEM) != 0;
                    SetStatus((isSystem ? "Set" : "Removed") + std::string(" System"));
                    needsReload = true;
                    needsRedraw = true;
                    return;
                }
            }
        }

        // ===== PRIORITY 2: CHECK DRIVE LETTERS =====
        if (my == 1) {
            int driveX = 10;
            for (size_t i = 0; i < driveLetters.size(); ++i) {
                if (mx >= driveX && mx < driveX + 4) {
                    ChangeDrive((int)i);
                    return;
                }
                driveX += 4;
            }
        }

        // ===== PRIORITY 3: CHECK OPERATION BUTTONS =====
        if (my == 2) {
            const int BUTTONS_X = 2;
            int x = BUTTONS_X;
            int screenWidth = DoubleBuffer::GetWidth();

            struct Button {
                std::string text;
                void(Explorer::* action)();
                int width;
            };

            Button buttons[] = {
                {"F2-RENAME",    &Explorer::RenameSelected, 10},
                {"F5-COPY",      &Explorer::CopySelected, 9},
                {"F6-CUT",       &Explorer::CutSelected, 8},
                {"F7-PASTE",     &Explorer::PasteClipboard, 10},
                {"F8-NEW",       &Explorer::CreateNewFolder, 8},
                {"F9-CLEAR",     &Explorer::ClearSelection, 10},
                {"DEL-DELETE",   &Explorer::DeleteSelected, 11}
            };

            for (const auto& btn : buttons) {
                if (x + btn.width > screenWidth - 2) {
                    break;
                }

                if (mx >= x && mx < x + btn.width) {
                    (this->*(btn.action))();
                    return;
                }
                x += btn.width + 4;
            }
        }

        // ===== PRIORITY 4: FILE LIST CLICKS =====
        if (my >= 3 && my < 3 + FILE_LIST_HEIGHT) {
            int clicked = top + (my - 3);
            if (clicked < (int)files.size()) {
                bool ctrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                bool shiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

                if (shiftPressed) {
                    int start = sel;
                    int end = clicked;
                    if (start > end) std::swap(start, end);

                    for (int i = start; i <= end; i++) {
                        if (i < (int)files.size() && files[i].name != "..") {
                            files[i].selected = true;
                        }
                    }
                    sel = clicked;
                }
                else if (ctrlPressed) {
                    files[clicked].selected = !files[clicked].selected;
                    sel = clicked;
                    SetStatus(files[clicked].selected ?
                        "Selected: " + files[clicked].name :
                        "Unselected: " + files[clicked].name);
                }
                else {
                    files[clicked].selected = !files[clicked].selected;
                    sel = clicked;

                    // Show attribute buttons for files (not folders)
                    if (!files[clicked].dir && files[clicked].name != "..") {
                        showingAttribButtons = true;
                    }
                    else {
                        showingAttribButtons = false;
                    }

                    SetStatus(files[clicked].selected ?
                        "Selected: " + files[clicked].name :
                        "Unselected: " + files[clicked].name);
                }

                needsRedraw = true;

                // Check for DOUBLE-CLICK to open
                if (InputManager::GetMouseDoubleClick()) {
                    const FileEntry& f = files[sel];
                    if (f.dir) {
                        cur = f.path;
                        sel = top = 0;
                        needsReload = true;
                        SetStatus("Opening: " + f.name);
                    }
                    else {
                        ShellExecuteA(nullptr, "open", f.path.string().c_str(),
                            nullptr, nullptr, SW_SHOW);
                        SetStatus("Running: " + f.name);
                    }
                }
            }
        }
        else {
            // Click outside file list - hide attribute buttons
            if (showingAttribButtons) {
                showingAttribButtons = false;
                needsRedraw = true;
            }
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
    uint64_t selectedSize = 0;
    for (const auto& file : files) {
        if (file.selected) {
            selectedCount++;
            if (!file.dir) {
                selectedSize += file.size;
            }
        }
    }

    DoubleBuffer::DrawText(60, FILE_LIST_HEIGHT + 6, "Statistics:",
        colors.infoPanelFg | (colors.infoPanelBg << 4));
    DoubleBuffer::DrawText(60, FILE_LIST_HEIGHT + 7, "Folders: " + std::to_string(totalDirs),
        colors.dirColor | (colors.infoPanelBg << 4));
    DoubleBuffer::DrawText(60, FILE_LIST_HEIGHT + 8, "Files: " + std::to_string(totalFiles),
        colors.fileColor | (colors.infoPanelBg << 4));

    if (selectedCount > 0) {
        // Show selected count
        DoubleBuffer::DrawText(60, FILE_LIST_HEIGHT + 9, "Selected: " + std::to_string(selectedCount),
            (YELLOW) | (colors.infoPanelBg << 4));

        // Calculate and display selected size
        char selectedSizeStr[32];
        if (selectedSize == 0) {
            strcpy_s(selectedSizeStr, "Selected: (folders only)");
        }
        else if (selectedSize < 1024) {
            sprintf_s(selectedSizeStr, "Selected: %llu B", (unsigned long long)selectedSize);
        }
        else if (selectedSize < 1024 * 1024) {
            sprintf_s(selectedSizeStr, "Selected: %.1f KB", selectedSize / 1024.0);
        }
        else if (selectedSize < 1024 * 1024 * 1024) {
            sprintf_s(selectedSizeStr, "Selected: %.1f MB", selectedSize / (1024.0 * 1024));
        }
        else {
            sprintf_s(selectedSizeStr, "Selected: %.1f GB", selectedSize / (1024.0 * 1024 * 1024));
        }
        DoubleBuffer::DrawText(60, FILE_LIST_HEIGHT + 10, selectedSizeStr,
            (GREEN) | (colors.infoPanelBg << 4));

        // Show total size at a different position if selection exists
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
        DoubleBuffer::DrawText(60, FILE_LIST_HEIGHT + 11, totalSizeStr,
            colors.sizeColor | (colors.infoPanelBg << 4));
    }
    else {
        // No selection, show total size at the normal position
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
        DoubleBuffer::DrawText(60, FILE_LIST_HEIGHT + 9, totalSizeStr,
            colors.sizeColor | (colors.infoPanelBg << 4));
    }

    // Controls - Updated with more shortcuts
    DoubleBuffer::DrawText(2, FILE_LIST_HEIGHT + 13, "Mouse: Click=Select  Ctrl+Click=Toggle  Shift+Click=Range",
        colors.infoPanelFg | (colors.infoPanelBg << 4));
    DoubleBuffer::DrawText(2, FILE_LIST_HEIGHT + 14, "Keys: Space=Toggle  Enter=Open  F5=Copy  F6=Paste  Del=Delete",
        colors.infoPanelFg | (colors.infoPanelBg << 4));
    DoubleBuffer::DrawText(2, FILE_LIST_HEIGHT + 15, "Ctrl+A=SelectAll  Ctrl+C=Copy  Ctrl+V=Paste  Ctrl+X=Cut  ESC=Quit",
        colors.infoPanelFg | (colors.infoPanelBg << 4));

    // Status message
    if (GetTickCount64() < statusMessageTime) {
        int msgX = (DoubleBuffer::GetWidth() - (int)statusMessage.length()) / 2;
        DoubleBuffer::FillRect(msgX - 2, FILE_LIST_HEIGHT + 17,
            (int)statusMessage.length() + 4, 1, L' ', (BLACK) | (YELLOW << 4));
        DoubleBuffer::DrawText(msgX, FILE_LIST_HEIGHT + 17, statusMessage,
            (BLACK) | (YELLOW << 4));
    }
}



// =====================
//  HandleRenameInput()
// =====================
void Explorer::HandleRenameInput(int key) {
    if (!inRenameMode) return;

    if (key == 13) {  // ENTER - Confirm rename
        if (renameBuffer.empty()) {
            SetStatus("ERROR: Name cannot be empty!", 2000);
            return;
        }

        // Check if name actually changed
        if (renameBuffer == files[renameIndex].name) {
            inRenameMode = false;
            renameIndex = -1;
            SetStatus("Rename cancelled (no change)", 2000);
            needsRedraw = true;
            return;
        }

        // Check for invalid characters
        if (renameBuffer.find_first_of("<>:\"/\\|?*") != std::string::npos) {
            SetStatus("ERROR: Invalid characters in filename!", 2000);
            return;
        }

        // Perform the rename
        try {
            fs::path oldPath = files[renameIndex].path;
            fs::path newPath = oldPath.parent_path() / renameBuffer;

            if (fs::exists(newPath)) {
                SetStatus("ERROR: '" + renameBuffer + "' already exists!", 3000);
                return;
            }

            // THIS IS THE KEY LINE - Actually rename the file
            fs::rename(oldPath, newPath);

            // Success!
            inRenameMode = false;
            renameIndex = -1;
            needsReload = true;
            SetStatus("Successfully renamed to: " + renameBuffer);
        }
        catch (const std::exception& e) {
            SetStatus("RENAME FAILED: " + std::string(e.what()), 3000);
            inRenameMode = false;
            renameIndex = -1;
        }
    }
    else if (key == 27) {  // ESC - Cancel rename
        inRenameMode = false;
        renameIndex = -1;
        SetStatus("Rename cancelled", 2000);
        needsRedraw = true;
    }
    else if (key == 8) {  // BACKSPACE - Delete character
        if (!renameBuffer.empty()) {
            renameBuffer.pop_back();
            needsRedraw = true;
        }
    }
    else if (key >= 32 && key <= 126) {  // Printable ASCII characters
        char ch = (char)key;

        // Block invalid filename characters
        if (ch == '<' || ch == '>' || ch == ':' || ch == '"' ||
            ch == '/' || ch == '\\' || ch == '|' || ch == '?' || ch == '*') {
            SetStatus("Invalid character! Cannot use: < > : \" / \\ | ? *", 1500);
            return;
        }

        renameBuffer += ch;
        needsRedraw = true;
    }
}

void Explorer::HandleSearchInput(int key) {
    if (!inSearchMode) return;

    if (key == 13 || key == 27) { // Enter or Esc
        inSearchMode = false;
        if (key == 27) { // If Esc, clear the search buffer
            searchBuffer.clear();
            ApplyFilter();
        }
        SetStatus("Search finished", 2000);
        needsRedraw = true;
    }
    else if (key == 8) { // Backspace
        if (!searchBuffer.empty()) {
            searchBuffer.pop_back();
            ApplyFilter();
            needsRedraw = true;
        }
    }
    else if (key >= 32 && key <= 126) { // Printable characters
        searchBuffer += (char)key;
        ApplyFilter();
        needsRedraw = true;
    }
}

void Explorer::ApplyFilter() {
    if (searchBuffer.empty()) {
        files = allFiles;
        needsRedraw = true;
        return;
    }

    files.clear();
    for (const auto& file : allFiles) {
        if (file.name.find(searchBuffer) != std::string::npos) {
            files.push_back(file);
        }
    }

    sel = 0;
    top = 0;
    needsRedraw = true;
}


void Explorer::DrawUI() {
    DoubleBuffer::ClearBoth(colors.fileListFg | (colors.fileListBg << 4));

    DrawFileList();
    DrawInfoPanel();
    DrawStatusBar();


    DrawDriveLetters();
    DrawOperationButtons();
    DrawAttributeButtons();

    if (inSearchMode) {
        std::string search_text = "Search: " + searchBuffer;
        DoubleBuffer::DrawText(2, DoubleBuffer::GetHeight() - 2, search_text, colors.statusBarFg | (colors.statusBarBg << 4));
    }

    DoubleBuffer::ScheduleFlip();
    needsRedraw = false;
}

void Explorer::Run() {
    DoubleBuffer::Init();
    Sleep(50);

    SetConsoleTitleW(L"RETRO EXPLORER v6.0 ");
    LoadDriveLetters();

    InputManager::Start();

    LoadDirectory();
    needsRedraw = true;
    DrawUI();




    while (true) {
        // Handle keyboard input
        int key = InputManager::GetKey();

        //////////////////////////////////
        // DEBUG: print every key
       // char buf2[64];
       // sprintf_s(buf2, "KEY PRESSED: 0x%04X (%d)", key & 0xFFFF, key & 0xFFFF);
       // DoubleBuffer::DrawText(25, 500, buf2, WHITE | (RED << 4));

        ///////////////////////////////////////////////////////

        if (key != 0) {

            if (inRenameMode) {
                HandleRenameInput(key);
                continue;  // Skip other key handling during rename
            }

            if (inSearchMode) {
                HandleSearchInput(key);
                continue;
            }

            bool redrawNeeded = false;

            // ===== CHECK FOR SHIFT + LETTER FIRST (BEFORE SWITCH) =====
            if (GetKeyState(VK_SHIFT) & 0x8000) {
                // Check if it's a letter key (A-Z or a-z)
                if ((key >= 'A' && key <= 'Z') || (key >= 'a' && key <= 'z')) {
                    ChangeDriveByLetter((char)key);
                    redrawNeeded = true;

                }
            }

            else {  //////SHIFT

                switch (key) {
                case 'f':
                case 'F':
                    if (GetKeyState(VK_CONTROL) & 0x8000) {
                        inSearchMode = true;
                        searchBuffer.clear();
                        SetStatus("SEARCH: Type to filter | ENTER=Accept | ESC=Cancel", 99999);
                        needsRedraw = true;
                    }
                    break;
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

                case 0x4300:  // F9 - Clear selection
                case 8: // BACKSPACE
                    ClearSelection();
                    redrawNeeded = true;
                    break;

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



                    // ===== LETTER KEYS (handle both upper and lower case) =====
                case 0x3F00:   // F5 = COPY (no Ctrl needed)
                    CopySelected();
                    redrawNeeded = true;
                    break;

                case 'c':
                case 'C':       // Ctrl+C

                    if (GetKeyState(VK_CONTROL) & 0x8000) {
                        CopySelected();
                        redrawNeeded = true;
                    }
                    break;

                    // === PASTE ===
                case 0x4100:  // F7
                case 'p':
                case 'P':
                    PasteClipboard();
                    redrawNeeded = true;
                    break;

                case 'v':
                case 'V':        // Ctrl+V
                    if (GetKeyState(VK_CONTROL) & 0x8000) {
                        PasteClipboard();
                        redrawNeeded = true;
                    }
                    break;


                case 'x':
                case 'X':       // Ctrl+X for Cut
                case 0x4000:     // F6
                    if (GetKeyState(VK_CONTROL) & 0x8000) {
                        CutSelected();
                        redrawNeeded = true;
                    }
                    break;

                case 'd':
                case 'D':
                case VK_DELETE:  // 0x2E - Standard Delete
                    //case 0x2E:       // Same as VK_DELETE
                case 0x5300:     // Delete without E0 prefix
                case 0x53E0:     // Delete with E0 prefix
                    //case 0x4F00:     // Numpad Delete when NumLock is OFF (common issue!)
                case 0x4FE0:     // Numpad Delete with E0 when NumLock is OFF

                    DeleteSelected();
                    redrawNeeded = true;
                    break;

                    // ───── RENAME ─────
                case 0x3C00:        // F2
                case 'r':
                case 'R':
                    RenameSelected();
                    redrawNeeded = true;
                    break;

                    // ───── NEW FOLDER ─────
                case 'n':
                case 'N':
                case 0x4200:  // F8
                    CreateNewFolder();
                    redrawNeeded = true;
                    break;


                }

            } //////SHIFT
               /*

             case 0x3F00:  // F5
             case 9:       // TAB KEY (ASCII 9)
                needsReload = true;
                SetStatus("Refreshing directory...");
                needsRedraw = true;
                break;
                */

                /*
                // === COLOR SCHEMES — ALT+1 / ALT+2 / ALT+3 / ALT+4 ===
                if (key >= 0x7800 && key <= 0x7B00) {        // Alt+1 to Alt+4 range
                    int scheme = key - 0x7800 + 1;          // 1,2,3,4

                    switch (scheme) {
                    case 1: // Alt+1 - Classic (Green)
                        colors.fileListBg = BLACK;
                        colors.fileListFg = LGRAY;
                        colors.fileListBorder = (GREEN << 4) | WHITE;
                        colors.fileListTitle = WHITE | (GREEN << 4);
                        colors.selectedBg = LRED;
                        colors.selectedFg = WHITE;
                        colors.mouseOverBg = DGRAY;
                        colors.dirColor = CYAN;
                        colors.fileColor = LGRAY;
                        colors.extColor = YELLOW;
                        SetStatus("Color scheme: Classic (Alt+1)");
                        break;

                    case 2: // Alt+2 - Dark Blue
                        colors.fileListBg = BLACK;
                        colors.fileListFg = LGRAY;
                        colors.fileListBorder = (DGRAY << 4) | WHITE;
                        colors.fileListTitle = WHITE | (DGRAY << 4);
                        colors.selectedBg = BLUE;
                        colors.selectedFg = WHITE;
                        colors.mouseOverBg = DGRAY;
                        colors.statusBarBg = DGRAY;
                        colors.statusBarFg = WHITE;
                        SetStatus("Color scheme: Dark (Alt+2)");
                        break;

                    case 3: // Alt+3 - High Contrast
                        colors.fileListBg = WHITE;
                        colors.fileListFg = BLACK;
                        colors.fileListBorder = (BLACK << 4) | WHITE;
                        colors.fileListTitle = WHITE | (BLACK << 4);
                        colors.selectedBg = BLACK;
                        colors.selectedFg = WHITE;
                        colors.mouseOverBg = LGRAY;
                        colors.statusBarBg = BLACK;
                        colors.statusBarFg = WHITE;
                        colors.dirColor = BLUE;
                        colors.fileColor = BLACK;
                        colors.extColor = RED;
                        SetStatus("Color scheme: High Contrast (Alt+3)");
                        break;

                    case 4: // Alt+4 - Hacker Green
                        colors.fileListBg = BLACK;
                        colors.fileListFg = GREEN;
                        colors.fileListBorder = (MAGENTA << 4) | WHITE;
                        colors.fileListTitle = WHITE | (MAGENTA << 4);
                        colors.selectedBg = RED;
                        colors.selectedFg = WHITE;
                        colors.mouseOverBg = DGRAY;
                        colors.statusBarBg = BLUE;
                        colors.statusBarFg = YELLOW;
                        colors.fileColor = GREEN;
                        SetStatus("Color scheme: Hacker (Alt+4)");
                        break;
                    }

                    needsRedraw = true;

                }*/


            if (redrawNeeded) {
                // Adjust viewport
                if (sel < top) top = sel;
                else if (sel >= top + FILE_LIST_HEIGHT) top = sel - FILE_LIST_HEIGHT + 1;
                needsRedraw = true;
            }
        }

        // Handle mouse
        HandleMouse();
        int mx, my;
        InputManager::GetMousePos(mx, my);
        if (mx != lastMouseX || my != lastMouseY) {
            lastMouseX = mx;
            lastMouseY = my;
            needsRedraw = true;  // ← Force redraw on mouse move!
        }

        HandleDriveSelection(mx, my);
        // ==========================================================================
    // DEBUG MOUSE — PUT THIS IN YOUR MAIN LOOP

        InputManager::GetMousePos(mx, my);
        char buf[80];
        sprintf_s(buf, "MOUSE: X=%03d Y=%03d | LClick=%d | WheelUp=%d | Alive=%s",
            mx, my,
            InputManager::GetMouseLeftClick() ? 1 : 0,
            InputManager::GetMouseWheelUp() ? 1 : 0,
            (mx > 0 || my > 0) ? "YES!" : "NO...");

        DoubleBuffer::DrawText(2, 37, buf, YELLOW | (BLUE << 4));
        DoubleBuffer::DrawText(2, 38, "MOVE MOUSE NOW -> NUMBERS CHANGE = WORKING!", WHITE | (RED << 4));
        // ==========================================================================


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