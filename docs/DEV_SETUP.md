# Development Environment Setup

**Platform:** Windows 11 (via WSL 2) -> Alpine Linux
**Project:** LinearSeq

## 1. Prerequisites
* **Windows 11** (Build 22000 or higher recommended for GUI support).
* **Administrator Access** to PowerShell.
* **Visual Studio Code** installed on Windows.

## 2. Install WSL 2 & Alpine Linux
We will be running Alpine Linux virtually, but it will share your file system and network.

1.  **Enable WSL:**
    Open PowerShell as Administrator and run:
```powershell
    wsl --install
```
**(Restart your computer if prompted).**

2.  **Install Alpine:**
    * Open the **Microsoft Store**.
    * Search for **"Alpine WSL"**.
    * Click **Get** / **Install**.

3.  **Initialize:**
    * Launch "Alpine" from your Start Menu.
    * Create a UNIX username and password when prompted.

## 3. Configure VS Code
1.  Open VS Code on Windows.
2.  Install the **WSL** extension (Microsoft).
3.  Install the **C/C++** extension (Microsoft).
4.  **Connect to Alpine:**
    * Press `F1` (or `Ctrl+Shift+P`).
    * Type `WSL: Connect to WSL`.
    * Select **Alpine**.
    * *Result:* A new VS Code window will open. The bottom-left corner should say **"WSL: Alpine"**.

## 4. Install Build Dependencies
Open the **Integrated Terminal** in VS Code (`Ctrl + ~`). You are now typing commands inside Alpine Linux.

Copy and run the following block to install the compiler, build tools, and audio/GUI libraries:

```bash
# Switch to root to install packages (enter your user password if asked)
su

# 1. Update Repositories
apk update

# 2. Install Build Tools
apk add build-base cmake make git

# 3. Install Audio & GUI Libraries
apk add alsa-lib-dev fltk-dev fltk-fluid

# 4. Install Utilities
apk add gdb valgrind

# Return to your normal user
exit

```

## 5. Hardware Setup (MIDI over USB)
By default, WSL 2 does not see USB devices plugged into Windows. To test real MIDI hardware, you must "attach" it.

1.  **On Windows:** Install `usbipd` via PowerShell (Admin):
```powershell
    winget install usbipd-win
```
2.  **Connect your MIDI Device** to the PC.
3.  **List Devices** (PowerShell Admin):
```powershell
    usbipd list
```
4.  **Bind & Attach** (Replace `<BUSID>` with your device's ID, e.g., `1-2`):
```powershell
    usbipd bind --busid <BUSID>
    usbipd attach --wsl --busid <BUSID>
```
5.  **Verify in Alpine:**
    In the VS Code terminal:
```bash
    amidi -l
```
**You should see your MIDI device listed.**

---

## 6. Build & Run
We use **CMake** to manage the build process.

1.  **Create Build Directory:**
```bash
    mkdir build
    cd build
```

2.  **Configure:**
```bash
    cmake ..
```

3.  **Compile:**
```bash
    make
```

4.  **Run:**
```bash
    ./LinearSeq
```
**A window should appear on your Windows desktop.**