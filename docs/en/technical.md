# For Hardcore Players

During the development of SigmaZ, Win16 development proved to be extremely challenging, so I wrote this chapter to share some insights.

## Annoying Memory Model

Developing 16-bit Windows (Win16) applications requires handling complex memory segmentation mechanisms. In real mode or early protected mode, data segment sizes are limited to 64KB. Due to near and far pointers, programs must explicitly handle cross-segment memory access, distinguishing between Near pointers (offset within a segment) and Far pointers (including a segment selector). Stack space in a 16-bit environment is extremely limited. Stack overflow is very easy, especially when declaring large local variables. SigmaZ avoids this limitation by managing large data structures using the Global Heap and `GlobalAlloc` API, but this still has some issues. For example, it always mysteriously crashes when running in Windows 95. Since I don't have a real machine, and installing the toolchain on an emulator is too slow, I can only hope the community can catch some Dr. Watson snapshots for me to look at (if you are willing to send me a laptop that can run Windows 95/98, I wouldn't refuse, haha).

## Compilation Toolchain

SigmaZ uses a mixed toolchain:

- **Open Watcom v2** for Win16 and Win32 builds (`build.bat`)
- **MSVC (Visual Studio Developer Command Prompt)** for Win64 builds (`build_x64.bat`)

This keeps legacy targets (Windows 3.1/95 era) and modern x64 targets maintainable in one repository.

## Source Code Compilation

### Win16 / Win32 (Open Watcom)

To compile 16-bit and 32-bit versions, you need the **Open Watcom C/C++ Compiler** (v2.0 or later is recommended).

1.  Install Open Watcom to `C:\WATCOM` (or modify paths in `wmakefile`).
2.  Run the build script in the project root:
    ```cmd
    .\build.bat
    ```
3.  The script will generate `sigma16.exe` and `sigma32.exe` in the `build/` directory.

### Win64 (Visual Studio / MSVC)

To compile the 64-bit version, you need **Visual Studio** or the **Windows SDK**.

1.  Open the **Developer Command Prompt for VS**.
2.  Run the x64 build script in the project root:
    ```cmd
    .\build_x64.bat
    ```
3.  The script will generate `sigma64.exe` in the `build/` directory.

## Project Structure

*   `main.c`: Program entry point, UI message loop handling, and overall test flow control.
*   `bench.c`: Core integer benchmark (Pi calculation) implementation and multitasking scheduling logic.
*   `bench_*.c`: Implementation of specific benchmarks (Floating point, Memory, Crypto, Compression, Matrix).
*   `detect.c`: CPU hardware detection routines (e.g., CPUID wrapper).
*   `timer.c`: Cross-platform high-precision timer wrapper (uses `QueryPerformanceCounter` on Win32, `GetTickCount` on Win16).
*   `sigmaz.rc`: Resource script defining GUI layout, menus, and dialogs.
*   `docs/`: Documentation source files in Markdown format.
*   `build_docs.py`: Automation script for documentation compilation.