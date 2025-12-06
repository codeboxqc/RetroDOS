# Retro Explorer — A Nostalgic Windows File Manager

![Retro Explorer](screenshot.png)

> A lightweight, retro-style file manager for Windows, built entirely in C++ using only the Windows console. Inspired by Norton Commander, Volkov Commander, and the golden age of DOS file managers.

**Retro Explorer** delivers blazing-fast navigation, full keyboard control, mouse support, and modern conveniences — all while staying true to that classic two-panel aesthetic.

---

### Features

- Classic dual-panel layout (file list + detailed information panel)  
- Full keyboard-driven workflow (Norton Commander / Total Commander style)  
- Complete mouse support – click to select, hover highlights, clickable buttons  
- Multi-select with `Space`, `Ctrl+Click`, `Shift+Click` range selection  
- Copy / Cut / Paste with internal clipboard (recursive folder copy)  
- Safe Delete → Recycle Bin (undoable via Windows Shell)  
- Inline rename (F2) with real-time editing  
- Create new folder on-the-fly  
- Quick drive switching: clickable drive bar + Shift + Letter (e.g. Shift + C → C:\)  
- One-click file attribute toggling (Read-only, Hidden, Archive, System)  
- Color-coded retro UI with instant feedback status bar  
- Fast in-memory copy for files < 50 MB  
- Zero external dependencies – pure WinAPI + STL  

---

### How to Compile (Visual Studio 2022)

1. Open Visual Studio 2022  
2. Create a new Console Application (C++) project  
3. Replace the generated files with the project source:  
   - `basic.h`  
   - `basic.cpp`  
   - `ddos.h` and `retro.h` (console and input helpers)  
4. Project Properties →  
   - Character Set → Use Multi-Byte Character Set  
   - C/C++ → Language → C++17 or higher  
   - Linker → Input → Additional Dependencies → `shell32.lib`  
5. Build → Release or Debug (x86 or x64)  

The executable runs instantly — no installer, no runtime required.

---

### File Structure
