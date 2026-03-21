# SigmaZ - Windows个人电脑通用微处理器测试

[Documentation (English)](docs/en/index.md) | [中文文档 (Chinese)](docs/zh/index.md)

**SigmaZ** 是一款跨时代的 Windows个人电脑通用微处理器测试 程序，支持从 **Windows 3.1** 到 **Windows 11** 的 Windows 系统。包含 Win16、Win32 和 Win64 三个原生版本，可运行于 486 到现代多核处理器等不同硬件平台。

![SigmaZ On Windows 95](assets/win95.png)
![SigmaZ 64+32](assets/64+32.png)

## 核心功能

*   **多平台支持**：原生编译的 Win16 (286/386/486)、Win32 (Win9x/NT) 和 Win64 (现代 x64) 可执行文件。
*   **性能评分**：分数以 1994 年的主流配置 **Intel 486 DX2-66** 为基准（100 分）。
*   **全面测试**：包含整数 (Pi)、浮点 (Mandelbrot)、内存操作、加密 (CRC32)、压缩 (LZ77) 和矩阵运算。
*   **超时策略**：整数/浮点/加密/压缩/矩阵测试默认 20 秒超时结算；内存操作使用固定时长窗口测量带宽。

有关测试项目和算法的详细信息，请查阅 [基准测试定义 (Benchmark Definitions)](docs/zh/benchmark_defs.md)。

## 快速开始

1.  下载最新发布版或从源码编译。
2.  根据您的系统选择正确的版本：
    *   **`sigma64.exe`**: 现代 64 位 Windows (10/11)
    *   **`sigma32.exe`**: 32 位 Windows (95+)
    *   **`sigma16.exe`**: 16 位 Windows (3.1/3.11)
3.  运行程序并点击 **Start All Tests**。

更多详情请参阅 [快速开始 (Quick Start)](docs/zh/quick_start.md)。

## 构建指南

SigmaZ 需要 **Open Watcom v2** (用于 Win16/32) 和 **Visual Studio / MSVC** (用于 Win64)。

*   运行 `build.bat` 编译 Win16 和 Win32 版本。
*   运行 `build_x64.bat` 编译 Win64 版本。
*   编译产物位于 `build/` 目录下。

完整的编译说明请参见 [技术细节 (Technical Details)](docs/zh/technical.md)。

## 项目文档

详细文档位于 `docs/` 文件夹或编译后的 CHM 帮助文件中。

*   [系统要求与快速开始](docs/zh/quick_start.md)
*   [基准测试定义与公式](docs/zh/benchmark_defs.md)
*   [评分系统说明](docs/zh/scoring.md)
*   [技术细节](docs/zh/technical.md)
*   [疑难解答](docs/zh/troubleshooting.md)

## 许可证

本项目开源。详情请参阅 [LICENSE](LICENSE) 文件。

## 免责声明

本工具会对您的硬件 (`CPU` 和 `内存`) 施加巨大的压力。虽然已内置保护机制，但在不稳定或超频系统上运行带来的风险请自行承担。

---

*Copyright (c) 2026 Ziyang Bai*
