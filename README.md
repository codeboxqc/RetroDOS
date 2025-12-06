# Retro Explorer — Classic File Manager Reborn

![Version](https://img.shields.io/badge/version-1.0-blue.svg)
![Platform](https://img.shields.io/badge/platform-Windows%2010%2B-blue.svg)
![C++](https://img.shields.io/badge/language-C%2B%2B17-red.svg)
![License](https://img.shields.io/badge/license-Freeware-green.svg)
![Console](https://img.shields.io/badge/interface-Console%20(TUI)-orange.svg)
![Size](https://img.shields.io/badge/size-%3C%20150%20KB-brightgreen.svg)
 

 

 

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

RetroExplorer/
├── basic.h          ─ Main class declaration
├── basic.cpp        ─ Core implementation
├── ddos.h           ─ Double-buffering console engine
├── retro.h          ─ Keyboard + mouse input manager
├── screenshot.png   ─ Preview image (optional but recommended)
└── README.md        ─ This file



---

### Keyboard Shortcuts

| Key                  | Action                              |
|----------------------|-------------------------------------|
| ↑ ↓ ← →              | Navigate                            |
| Enter                | Open folder                         |
| Space                | Toggle selection                    |
| Ctrl + C  /  F5      | Copy                                |
| Ctrl + X  /  F6      | Cut (move)                          |
| Ctrl + V  /  F7      | Paste                               |
| Del                  | Delete → Recycle Bin                |
| F2                   | Rename                              |
| F8                   | New Folder                          |
| Shift + A..Z         | Quick drive change (Shift + C → C:\) |
| Esc                  | Quit                                |

---

### Mouse Controls

- Left-click a file → select it  
- Click drive letters → instantly change drive  
- Click operation buttons ([F2-RENAME], [F5-COPY], etc.) → execute action  
- Hover effects on buttons and drive letters  

---

### Screenshots

![Main view](screenshot1.png)  
*Classic two-panel layout with drive bar and attribute buttons*

![Attribute toggling](screenshot2.png)  
*One-click R/H/A/S attribute buttons appear when a file is selected*

![Rename mode](screenshot3.png)  
*Inline rename with live feedback*

(Replace the placeholder image links with actual screenshots of your app.)

---

### Build Requirements

- Windows 7 or newer  
- Visual Studio 2022 (Community edition works perfectly)  
- No external libraries or packages needed  

---

Enjoy the nostalgia with modern speed — Happy exploring!

Made with love for retro computing ♥



# Retro Explorer — A Nostalgic Windows File Manager

 

> A blazing-fast, retro-style, keyboard-first file manager for Windows — built entirely in C++ using only the Windows console. Inspired by Norton Commander, Volkov Commander, and classic DOS utilities.

Retro Explorer brings back the speed, simplicity, and joy of old-school file management with modern enhancements like full mouse support, Recycle Bin integration, and instant drive switching.

---

### Features

- Classic **dual-panel layout** (file list + detailed info panel)  
- **Full keyboard navigation** — feels like Total Commander / Norton Commander  
- **Complete mouse support** — click, hover, drag-select (coming soon)  
- Multi-file selection with `Space`, `Ctrl+Click`, range selection  
- **Copy / Cut / Paste** with internal clipboard (recursive folder copy)  
- **Safe Delete → Recycle Bin** (undoable via Windows Shell)  
- Inline **F2 Rename** with real-time editing and validation  
- **Create New Folder** instantly  
- **Quick drive switching**:  
  - Clickable drive bar  
  - **Shift + Letter** (e.g. `Shift + C` → instantly go to C:\)  
- One-click **file attribute toggling** (R=Read-only, H=Hidden, A=Archive, S=System)  
- Beautiful retro color scheme with live status feedback  
- Fast in-memory copy for small files (<50 MB)  
- Zero external dependencies — pure WinAPI + C++ Standard Library  

---

### How to Compile (Visual Studio 2022)

1. Open **Visual Studio 2022**  
2. Create a new **Console Application** (C++) project  
3. Replace the default files with:  
   - `basic.h`  
   - `basic.cpp`  
   - `ddos.h` → console double-buffering engine  
   - `retro.h` → keyboard & mouse input manager  
4. Project Properties →  
   - **Character Set** → `Use Multi-Byte Character Set`  
   - **C/C++ → Language** → `ISO C++17 Standard` or higher  
   - **Linker → Input → Additional Dependencies** → add `shell32.lib`  
5. Build → **Release** or **Debug** (x86 or x64)  

Runs instantly — single `.exe`, no installer, no dependencies.

---

 



> A super-fast, nostalgic, keyboard-first file manager for Windows — written in pure C++ using only the console.  
> Inspired by **Norton Commander**, **Volkov Commander**, and the golden era of DOS tools.

Retro Explorer brings back the joy of instant navigation, two-panel workflow, and raw speed — now with mouse support, Recycle Bin, attribute toggling, and **Shift + Letter** drive switching.

---

### Features

- Classic **dual-panel layout** (file list + rich info panel)  
- Full **keyboard & mouse** support  
- Multi-select (`Space`, `Ctrl+Click`, range coming soon)  
- **Copy / Cut / Paste** (recursive folders)  
- **Safe Delete → Recycle Bin** (undoable!)  
- **F2 Rename** with live editing  
- **Create New Folder** instantly  
- **Drive switching**:  
  - Clickable drive bar  
  - **Shift + C** → instantly go to `C:\` (any drive!)  
- One-click **file attributes** [R][H][A][S] toggles  
- Retro color scheme with live status bar  
- Ultra-lightweight: **< 150 KB** executable  
- No dependencies — pure WinAPI + C++17  

---

### Screenshots

![Main Screen](screenshots/main.png)  
![Attributes](screenshots/attributes.png)  
![Rename](screenshots/rename.png)

---

### Keyboard Shortcuts

| Key                  | Action                              |
|----------------------|-------------------------------------|
| `↑ ↓ ← →`            | Navigate                            |
| `Enter`              | Open folder                         |
| `Space`              | Toggle selection                    |
| `Ctrl+C` / `F5`      | Copy                                |
| `Ctrl+X` / `F6`      | Cut (move)                          |
| `Ctrl+V` / `F7`      | Paste                               |
| `Del`                | Delete → Recycle Bin                |
| `F2`                 | Rename                              |
| `F8`                 | New Folder                          |
| `Shift + A..Z`       | Quick drive change (e.g. `Shift+D` → D:\) |
| `Esc`                | Exit                                |

---

### How to Compile (Visual Studio 2022)

1. Open Visual Studio 2022  
2. Create new **Console Application** (C++)  
3. Replace files with: `basic.h`, `basic.cpp`, `ddos.h`, `retro.h`  
4. Project Properties →  
   - Character Set → **Multi-Byte**  
   - C++ Language Standard → **C++17** or higher  
   - Linker → Input → Add `shell32.lib`  
5. Build → Done!  

Single `.exe`, runs anywhere on Windows 7–11.
