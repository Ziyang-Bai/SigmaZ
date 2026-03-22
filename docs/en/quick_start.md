# Quick Start

## 1. Select the Version

SigmaZ provides three versions to adapt to different operating system environments:

*   `sigma16.exe` (Win16): Suitable for Windows 3.1 and the real-mode environment of Windows 9x. Recommended for early hardware such as 286/386/486.
*   `sigma32.exe` (Win32): Suitable for Windows 95 to Windows 11. Supports multi-threaded concurrent calculation.
*   `sigma64.exe` (Win64): Suitable for modern 64-bit operating systems like Windows 7/10/11, supporting larger memory addressing and new instruction set optimizations.

## 2. Run Tests

After launching the program, you can perform a complete performance evaluation by following these steps:

1.  Select the **All** tab on the main interface.
2.  Click the **Start All Tests** button.
3.  The program will execute all benchmark tests sequentially.
4.  Open the **Rpt** tab, choose a target directory from the directory list, enter a filename, and click **Save**.

The **Info** tab displays detected system and hardware information.

> **Note:** On lower-performance hardware (such as 386/486), the interface may appear "not responding" briefly during the test. This is because single-threaded calculations occupy CPU resources. Please wait patiently.

## 3. Read Exported Reports

SigmaZ provides a Python reader for `.szr` files:

*   Strict verification: `python read_szr_report.py report.szr`
*   Force parse arbitrary file: `python read_szr_report.py any.bin --force`

## 4. Disclaimer

This tool subjects your hardware (`CPU` and `Memory`) to significant stress. While protection mechanisms are built-in, use on unstable or overclocked systems is at your own risk.