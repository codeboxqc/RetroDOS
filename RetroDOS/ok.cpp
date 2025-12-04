/*

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

namespace fs = std::filesystem;

// Box drawing characters
#define BOX_H   "─"
#define BOX_V   "│"
#define BOX_TL  "┌"
#define BOX_TR  "┐"
#define BOX_BL  "└"
#define BOX_BR  "┘"
#define BOX_VL  "├"
#define BOX_VR  "┤"

enum Color {
    BLACK, BLUE, GREEN, CYAN, RED, MAGENTA, BROWN, LGRAY,
    DGRAY, LBLUE, LGREEN, LCYAN, LRED, LMAGENTA, YELLOW, WHITE
};

class DoubleBuffer {
    static HANDLE hOut;
    static const int WIDTH = 120;
    static const int HEIGHT = 35;
    static CHAR_INFO front[HEIGHT][WIDTH];
    static CHAR_INFO back[HEIGHT][WIDTH];
    static std::atomic<bool> needsFlip;

public:
    static void Init() {
        hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleOutputCP(65001);

        // Set buffer size
        COORD size = { WIDTH, HEIGHT };
        SetConsoleScreenBufferSize(hOut, size);

        // Set window size to match buffer
        SMALL_RECT rect = { 0, 0, WIDTH - 1, HEIGHT - 1 };
        SetConsoleWindowInfo(hOut, TRUE, &rect);

        // Hide cursor
        CONSOLE_CURSOR_INFO ci = { 100, FALSE };
        SetConsoleCursorInfo(hOut, &ci);

        // Clear both buffers
        ClearBoth();
        needsFlip = false;

        SetConsoleTitleA("Retro Explorer v4.2 - Split Screen, Info Panel");
    }

    static void ClearBoth(WORD attr = LGRAY | (BLACK << 4)) {
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                front[y][x].Char.AsciiChar = ' ';
                front[y][x].Attributes = attr;
                back[y][x].Char.AsciiChar = ' ';
                back[y][x].Attributes = attr;
            }
        }
    }

    static void Set(int x, int y, wchar_t c, WORD attr = WHITE | (BLACK << 4)) {
        if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
            back[y][x].Char.UnicodeChar = c;
            back[y][x].Attributes = attr;
        }
    }

    static void Set(int x, int y, char c, WORD attr = WHITE | (BLACK << 4)) {
        Set(x, y, static_cast<wchar_t>(c), attr);
    }

    static void DrawBox(int x, int y, int w, int h, WORD attr = WHITE | (CYAN << 4), bool thick = false) {
        // Draw top border
        for (int i = 1; i < w - 1; i++) {
            Set(x + i, y, thick ? L'═' : L'─', attr);
        }
        Set(x, y, thick ? L'╔' : L'┌', attr);
        Set(x + w - 1, y, thick ? L'╗' : L'┐', attr);

        // Draw bottom border
        for (int i = 1; i < w - 1; i++) {
            Set(x + i, y + h - 1, thick ? L'═' : L'─', attr);
        }
        Set(x, y + h - 1, thick ? L'╚' : L'└', attr);
        Set(x + w - 1, y + h - 1, thick ? L'╝' : L'┘', attr);

        // Draw sides
        for (int i = 1; i < h - 1; i++) {
            Set(x, y + i, thick ? L'║' : L'│', attr);
            Set(x + w - 1, y + i, thick ? L'║' : L'│', attr);
        }
    }

    static void DrawText(int x, int y, const std::string& text, WORD attr) {
        for (size_t i = 0; i < text.size() && x + i < WIDTH; i++) {
            Set(x + i, y, text[i], attr);
        }
    }

    static void DrawText(int x, int y, const std::wstring& text, WORD attr) {
        for (size_t i = 0; i < text.size() && x + i < WIDTH; i++) {
            Set(x + i, y, text[i], attr);
        }
    }

    static void FillRect(int x, int y, int w, int h, char c, WORD attr) {
        for (int dy = 0; dy < h; dy++) {
            for (int dx = 0; dx < w; dx++) {
                Set(x + dx, y + dy, c, attr);
            }
        }
    }

    static void Flip() {
        // Fast buffer swap with single WriteConsoleOutput
        SMALL_RECT writeRegion = { 0, 0, WIDTH - 1, HEIGHT - 1 };
        COORD bufSize = { WIDTH, HEIGHT };
        COORD bufCoord = { 0, 0 };

        WriteConsoleOutputW(hOut, (CHAR_INFO*)back, bufSize, bufCoord, &writeRegion);

        // Swap buffers
        std::swap(front, back);
    }

    static void ScheduleFlip() {
        needsFlip = true;
    }

    static bool NeedsFlip() {
        return needsFlip.exchange(false);
    }
};

HANDLE DoubleBuffer::hOut = nullptr;
CHAR_INFO DoubleBuffer::front[35][120];
CHAR_INFO DoubleBuffer::back[35][120];
std::atomic<bool> DoubleBuffer::needsFlip(false);

class AsyncInput {
    static std::atomic<int> lastKey;
    static std::atomic<bool> running;
    static std::thread inputThread;

    static void InputLoop() {
        while (running) {
            if (_kbhit()) {
                int key = _getch();
                if (key == 0 || key == 0xE0) {
                    key = _getch() << 8;
                }
                lastKey.store(key, std::memory_order_release);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

public:
    static void Start() {
        running = true;
        lastKey = 0;
        inputThread = std::thread(InputLoop);
    }

    static void Stop() {
        running = false;
        if (inputThread.joinable()) {
            inputThread.join();
        }
    }

    static int GetKey() {
        return lastKey.exchange(0, std::memory_order_acquire);
    }

    static bool HasKey() {
        return lastKey.load(std::memory_order_acquire) != 0;
    }
};

std::atomic<int> AsyncInput::lastKey(0);
std::atomic<bool> AsyncInput::running(false);
std::thread AsyncInput::inputThread;

struct FileEntry {
    std::string name;
    std::string ext;
    std::string date;
    std::string attr;
    bool dir = false;
    uint64_t size = 0;
    fs::path path;
};

class Explorer {
    std::vector<FileEntry> files;
    fs::path cur = fs::current_path();
    int sel = 0;
    int top = 0;
    const int FILE_LIST_HEIGHT = 20;  // Reduced for info panel
    const int SPLIT_ROW = 21;         // Where the screen splits
    bool needsRedraw = true;
    bool needsReload = true;
    uint64_t totalFiles = 0;
    uint64_t totalDirs = 0;
    uint64_t totalSize = 0;

    void LoadDirectory() {
        files.clear();
        totalFiles = 0;
        totalDirs = 0;
        totalSize = 0;

        // Add parent directory entry
        if (cur.has_parent_path() && cur.parent_path() != cur) {
            files.push_back({
                ".. (Up)", "", "            ", "D",
                true, 0, cur.parent_path()
                });
            totalDirs++;
        }

        // Load directory contents
        std::error_code ec;
        auto dir_iter = fs::directory_iterator(cur,
            fs::directory_options::skip_permission_denied, ec);

        if (!ec) {
            for (const auto& entry : dir_iter) {
                try {
                    FileEntry fe;
                    fe.path = entry.path();
                    fe.name = entry.path().filename().string();
                    fe.dir = entry.is_directory();

                    if (fe.dir) {
                        totalDirs++;
                    }
                    else {
                        totalFiles++;
                    }

                    if (!fe.dir) {
                        fe.size = entry.file_size();
                        totalSize += fe.size;
                        size_t dotpos = fe.name.find_last_of('.');
                        if (dotpos != std::string::npos) {
                            fe.ext = fe.name.substr(dotpos);
                        }
                    }

                    // Get last write time using safe functions
                    auto ftime = entry.last_write_time();
                    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
                    std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);

                    struct tm timeinfo;
                    localtime_s(&timeinfo, &cftime);

                    char timebuf[20];
                    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M", &timeinfo);
                    fe.date = timebuf;

                    // Get attributes
                    DWORD attrs = GetFileAttributesW(entry.path().wstring().c_str());
                    if (attrs != INVALID_FILE_ATTRIBUTES) {
                        if (attrs & FILE_ATTRIBUTE_READONLY) fe.attr += 'R';
                        if (attrs & FILE_ATTRIBUTE_HIDDEN) fe.attr += 'H';
                        if (attrs & FILE_ATTRIBUTE_SYSTEM) fe.attr += 'S';
                        if (fe.dir) fe.attr += 'D';
                    }

                    files.push_back(std::move(fe));
                }
                catch (...) {
                    // Skip problematic entries
                }
            }
        }

        // Sort: directories first, then alphabetical
        std::sort(files.begin(), files.end(), [](const FileEntry& a, const FileEntry& b) {
            if (a.dir != b.dir) return a.dir > b.dir;
            return _stricmp(a.name.c_str(), b.name.c_str()) < 0;
            });

        needsReload = false;
    }

    void DrawFileList() {
        // Draw file list area
        DoubleBuffer::DrawBox(0, 0, 118, SPLIT_ROW, GREEN << 4 | WHITE, false);

        // Draw title
        std::string title = " FILE LIST - " + cur.string();
        title = title.substr(0, 110);
        DoubleBuffer::DrawText(2, 0, " " + title + " ", BLACK | (GREEN << 4));

        // Draw column headers with separator lines
        DoubleBuffer::DrawText(2, 2, "Name", WHITE | (BLACK << 4));
        DoubleBuffer::DrawText(38, 2, "Type", WHITE | (BLACK << 4));
        DoubleBuffer::DrawText(46, 2, "Size", WHITE | (BLACK << 4));
        DoubleBuffer::DrawText(56, 2, "Modified", WHITE | (BLACK << 4));
        DoubleBuffer::DrawText(75, 2, "Attrib", WHITE | (BLACK << 4));

        // Draw separator line under headers
        for (int x = 2; x < 116; x++) {
            DoubleBuffer::Set(x, 3, L'─', WHITE | (BLACK << 4));
        }
        DoubleBuffer::Set(1, 3, L'├', WHITE | (BLACK << 4));
        DoubleBuffer::Set(117, 3, L'┤', WHITE | (BLACK << 4));

        // Draw file list
        int visibleCount = std::min(FILE_LIST_HEIGHT, static_cast<int>(files.size()) - top);

        for (int i = 0; i < visibleCount; ++i) {
            int idx = top + i;
            const FileEntry& entry = files[idx];
            bool selected = (idx == sel);

            WORD bg = selected ? WHITE : BLACK;
            WORD fg = selected ? BLACK : (entry.dir ? YELLOW : LGRAY);
            WORD attr = fg | (bg << 4);

            int y = 4 + i;

            // Highlight the entire row
            if (selected) {
                for (int x = 2; x < 117; x++) {
                    DoubleBuffer::Set(x, y, ' ', bg << 4);
                }
            }

            // Name (truncated if too long)
            std::string name = entry.name;
            if (name.length() > 34) {
                name = name.substr(0, 31) + "...";
            }
            DoubleBuffer::DrawText(2, y, name, attr);

            // Type
            std::string type = entry.dir ? "<DIR>   " : entry.ext.empty() ? "        " : entry.ext;
            DoubleBuffer::DrawText(38, y, type, attr);

            // Size
            char sizeStr[16];
            if (entry.dir) {
                strcpy_s(sizeStr, "          ");
            }
            else if (entry.size < 1024) {
                sprintf_s(sizeStr, sizeof(sizeStr), "%llu B ", entry.size);
            }
            else if (entry.size < 1024 * 1024) {
                sprintf_s(sizeStr, sizeof(sizeStr), "%.1f KB", entry.size / 1024.0);
            }
            else if (entry.size < 1024ULL * 1024 * 1024) {
                sprintf_s(sizeStr, sizeof(sizeStr), "%.1f MB", entry.size / (1024.0 * 1024));
            }
            else {
                sprintf_s(sizeStr, sizeof(sizeStr), "%.1f GB", entry.size / (1024.0 * 1024 * 1024));
            }
            DoubleBuffer::DrawText(46, y, sizeStr, LGREEN | (bg << 4));

            // Date
            DoubleBuffer::DrawText(56, y, " " + entry.date, attr);

            // Attributes
            DoubleBuffer::DrawText(75, y, " " + entry.attr, LCYAN | (bg << 4));
        }

        // Draw scroll indicator
        if (files.size() > FILE_LIST_HEIGHT) {
            int scrollPos = (top * FILE_LIST_HEIGHT) / files.size();
            int scrollHeight = (FILE_LIST_HEIGHT * FILE_LIST_HEIGHT) / files.size();
            scrollHeight = std::max(1, scrollHeight);

            DoubleBuffer::Set(116, 4 + scrollPos, L'▓', WHITE | (DGRAY << 4));
            for (int i = 1; i < scrollHeight; i++) {
                if (4 + scrollPos + i < SPLIT_ROW - 1) {
                    DoubleBuffer::Set(116, 4 + scrollPos + i, L'▒', WHITE | (DGRAY << 4));
                }
            }
        }
    }

    void DrawInfoPanel() {
        // Draw info panel with thick borders
        DoubleBuffer::DrawBox(0, SPLIT_ROW, 118, 13, CYAN << 4 | WHITE, true);

        // Panel title
        std::wstring panelTitle = L" 📁 DIRECTORY INFORMATION ";
        DoubleBuffer::DrawText(2, SPLIT_ROW, panelTitle, BLACK | (CYAN << 4));

        // Draw separator
        for (int x = 2; x < 116; x++) {
            DoubleBuffer::Set(x, SPLIT_ROW + 2, L'─', WHITE | (BLACK << 4));
        }
        DoubleBuffer::Set(1, SPLIT_ROW + 2, L'├', WHITE | (BLACK << 4));
        DoubleBuffer::Set(117, SPLIT_ROW + 2, L'┤', WHITE | (BLACK << 4));

        // Current path
        std::string pathStr = "Path: " + cur.string();
        if (pathStr.length() > 110) {
            pathStr = "Path: ..." + pathStr.substr(pathStr.length() - 100);
        }
        DoubleBuffer::DrawText(2, SPLIT_ROW + 1, pathStr, LGRAY | (BLACK << 4));

        // Statistics in a grid
        DoubleBuffer::DrawText(2, SPLIT_ROW + 3, "Total Items:", WHITE | (BLACK << 4));
        DoubleBuffer::DrawText(20, SPLIT_ROW + 3, std::to_string(files.size()), LGREEN | (BLACK << 4));

        DoubleBuffer::DrawText(2, SPLIT_ROW + 4, "Directories:", WHITE | (BLACK << 4));
        DoubleBuffer::DrawText(20, SPLIT_ROW + 4, std::to_string(totalDirs), LBLUE | (BLACK << 4));

        DoubleBuffer::DrawText(2, SPLIT_ROW + 5, "Files:", WHITE | (BLACK << 4));
        DoubleBuffer::DrawText(20, SPLIT_ROW + 5, std::to_string(totalFiles), YELLOW | (BLACK << 4));

        // Total size
        std::string sizeStr;
        if (totalSize < 1024) {
            sizeStr = std::to_string(totalSize) + " B";
        }
        else if (totalSize < 1024 * 1024) {
            sizeStr = std::format("{:.1f} KB", totalSize / 1024.0);
        }
        else if (totalSize < 1024ULL * 1024 * 1024) {
            sizeStr = std::format("{:.1f} MB", totalSize / (1024.0 * 1024));
        }
        else {
            sizeStr = std::format("{:.1f} GB", totalSize / (1024.0 * 1024 * 1024));
        }

        DoubleBuffer::DrawText(2, SPLIT_ROW + 6, "Total Size:", WHITE | (BLACK << 4));
        DoubleBuffer::DrawText(20, SPLIT_ROW + 6, sizeStr, LRED | (BLACK << 4));

        // Selected file info (if any)
        if (!files.empty() && sel < files.size()) {
            const FileEntry& selected = files[sel];

            DoubleBuffer::DrawText(40, SPLIT_ROW + 3, "Selected:", WHITE | (BLACK << 4));
            std::string selName = selected.name;
            if (selName.length() > 30) {
                selName = selName.substr(0, 27) + "...";
            }
            DoubleBuffer::DrawText(50, SPLIT_ROW + 3, selName,
                selected.dir ? YELLOW : LGREEN | (BLACK << 4));

            DoubleBuffer::DrawText(40, SPLIT_ROW + 4, "Type:", WHITE | (BLACK << 4));
            DoubleBuffer::DrawText(50, SPLIT_ROW + 4,
                selected.dir ? "Directory" : "File",
                LCYAN | (BLACK << 4));

            if (!selected.dir) {
                DoubleBuffer::DrawText(40, SPLIT_ROW + 5, "Size:", WHITE | (BLACK << 4));
                char selSize[20];
                if (selected.size < 1024) {
                    sprintf_s(selSize, sizeof(selSize), "%llu B", selected.size);
                }
                else if (selected.size < 1024 * 1024) {
                    sprintf_s(selSize, sizeof(selSize), "%.1f KB", selected.size / 1024.0);
                }
                else {
                    sprintf_s(selSize, sizeof(selSize), "%.1f MB", selected.size / (1024.0 * 1024));
                }
                DoubleBuffer::DrawText(50, SPLIT_ROW + 5, selSize, LGREEN | (BLACK << 4));
            }

            DoubleBuffer::DrawText(40, SPLIT_ROW + 6, "Modified:", WHITE | (BLACK << 4));
            DoubleBuffer::DrawText(50, SPLIT_ROW + 6, selected.date, LCYAN | (BLACK << 4));
        }

        // Draw a decorative rectangle
        DoubleBuffer::DrawBox(80, SPLIT_ROW + 3, 35, 8, MAGENTA << 4 | WHITE, false);
        DoubleBuffer::DrawText(82, SPLIT_ROW + 3, " Quick Actions ", BLACK | (MAGENTA << 4));

        DoubleBuffer::DrawText(82, SPLIT_ROW + 5, "F1  - Help", LGRAY | (BLACK << 4));
        DoubleBuffer::DrawText(82, SPLIT_ROW + 6, "F5  - Refresh", LGRAY | (BLACK << 4));
        DoubleBuffer::DrawText(82, SPLIT_ROW + 7, "F10 - Options", LGRAY | (BLACK << 4));
        DoubleBuffer::DrawText(82, SPLIT_ROW + 8, "ESC - Quit", LGRAY | (BLACK << 4));
    }

    void DrawStatusBar() {
        // Draw status bar at the bottom
        DoubleBuffer::FillRect(0, 34, 118, 1, ' ', BLACK | (LGRAY << 4));

        std::string status;
        if (!files.empty() && sel < files.size()) {
            status = "Selected: " + files[sel].name + " | ";
        }
        status += "←↑↓→ Navigate | Enter Open | F5 Refresh | ESC Quit";

        DoubleBuffer::DrawText(2, 34, status, BLACK | (LGRAY << 4));

        // Page indicator
        if (files.size() > FILE_LIST_HEIGHT) {
            int page = (top / FILE_LIST_HEIGHT) + 1;
            int totalPages = (files.size() + FILE_LIST_HEIGHT - 1) / FILE_LIST_HEIGHT;
            std::string pageStr = "Page " + std::to_string(page) + "/" + std::to_string(totalPages);
            DoubleBuffer::DrawText(110, 34, pageStr, BLACK | (LGRAY << 4));
        }
    }

    void DrawUI() {
        DoubleBuffer::ClearBoth(LGRAY | (BLACK << 4));

        DrawFileList();
        DrawInfoPanel();
        DrawStatusBar();

        DoubleBuffer::ScheduleFlip();
        needsRedraw = false;
    }

public:
    Explorer() {
        cur = fs::current_path();
    }

    void Run() {
        DoubleBuffer::Init();
        AsyncInput::Start();

        LoadDirectory();
        DrawUI();

        while (true) {
            int key = AsyncInput::GetKey();

            if (key != 0) {
                bool processed = true;

                switch (key) {
                case 27: // ESC
                    AsyncInput::Stop();
                    return;

                case 13: // ENTER
                    if (!files.empty() && sel < files.size()) {
                        if (files[sel].dir) {
                            cur = files[sel].path;
                            sel = top = 0;
                            needsReload = true;
                        }
                    }
                    break;

                case 0x4800: // Up arrow
                    if (sel > 0) sel--;
                    needsRedraw = true;
                    break;

                case 0x5000: // Down arrow
                    if (sel < static_cast<int>(files.size()) - 1) sel++;
                    needsRedraw = true;
                    break;

                case 0x4B00: // Left arrow
                    if (sel > 0) sel = std::max(0, sel - 5);
                    needsRedraw = true;
                    break;

                case 0x4D00: // Right arrow
                    if (sel < static_cast<int>(files.size()) - 1)
                        sel = std::min(static_cast<int>(files.size()) - 1, sel + 5);
                    needsRedraw = true;
                    break;

                case 0x4900: // Page Up
                    sel = std::max(0, sel - FILE_LIST_HEIGHT);
                    needsRedraw = true;
                    break;

                case 0x5100: // Page Down
                    sel = std::min(static_cast<int>(files.size()) - 1, sel + FILE_LIST_HEIGHT);
                    needsRedraw = true;
                    break;

                case 0x4700: // Home
                    sel = 0;
                    needsRedraw = true;
                    break;

                case 0x4F00: // End
                    sel = std::max(0, static_cast<int>(files.size()) - 1);
                    needsRedraw = true;
                    break;

                case 0x3F00: // F5 - Refresh
                    needsReload = true;
                    break;

                case 0x3B00: // F1 - Help (placeholder)
                    needsRedraw = true;
                    break;

                case 0x4400: // F10 - Options (placeholder)
                    needsRedraw = true;
                    break;

                default:
                    processed = false;
                    break;
                }

                if (processed) {
                    // Adjust viewport
                    if (sel < top) {
                        top = sel;
                    }
                    else if (sel >= top + FILE_LIST_HEIGHT) {
                        top = sel - FILE_LIST_HEIGHT + 1;
                    }
                }
            }

            if (needsReload) {
                LoadDirectory();
                needsRedraw = true;
            }

            if (needsRedraw) {
                DrawUI();
            }

            if (DoubleBuffer::NeedsFlip()) {
                DoubleBuffer::Flip();
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
};

int main() {
    SetConsoleTitleA("Retro Explorer v4.2 - Split Screen, Info Panel");

    try {
        Explorer explorer;
        explorer.Run();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

*/