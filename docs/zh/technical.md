# 给硬核玩家看的

在开发 SigmaZ 的过程中，Win16的开发是极其操蛋的，特地写了这一章来吐槽一下。

## 闹人的内存模型

开发 Windows 16 位 (Win16) 应用程序需要处理复杂的内存分段机制。在实模式或早期的保护模式下，数据段大小被限制在 64KB 以内。因为远近指针，程序必须显式处理跨段内存访问，区分通过段内偏移寻址的 Near 指针和包含段选择子的 Far 指针。16 位环境下的堆栈空间非常有限。堆栈溢出是很容易的，特别是在声明大型局部变量时。SigmaZ 通过使用全局堆 (Global Heap) 及 `GlobalAlloc` API 来管理大型数据结构，从而规避这一限制，但这仍然有些问题，比如在Windows95中运行时总是会神秘的崩溃，由于我并没有实机，而模拟器装上工具链又太TM卡，只能指望社区能给我用华生医生抓点快照看看咯（如果你愿意送我一台能跑Windows95/98的笔记本我也不拒绝hhhhh）。

## 编译工具链

SigmaZ 使用混合工具链：

- **Open Watcom v2** 负责 Win16/Win32 构建（`build.bat`）
- **MSVC（Visual Studio 开发者命令提示符）** 负责 Win64 构建（`build_x64.bat`）

这样可以在同一仓库内同时维护老平台（Windows 3.1/95 时代）和现代 x64 构建目标。

## 源码编译说明（这部分没有吐槽）

### Win16 / Win32 (Open Watcom)

要编译 16 位和 32 位版本，您需要安装 **Open Watcom C/C++ 编译器** (推荐 v2.0 或更高版本)。

1.  将 Open Watcom 安装到 `C:\WATCOM` (或修改 `wmakefile` 中的路径)。
2.  在项目根目录下运行构建脚本：
    ```cmd
    .\build.bat
    ```
3.  脚本将在 `build/` 目录下生成 `sigma16.exe` 和 `sigma32.exe`。

### Win64 (Visual Studio / MSVC)

要编译 64 位版本，您需要 **Visual Studio** (MSVC 编译器) 或 **Windows SDK**。

1.  打开 **Developer Command Prompt for VS** (开发人员命令提示符)。
2.  在项目根目录下运行 x64 构建脚本：
    ```cmd
    .\build_x64.bat
    ```
3.  脚本将在 `build/` 目录下生成 `sigma64.exe`。

## 项目文件结构

*   `main.c`: 程序入口、UI 消息循环处理及整体测试流程控制。
*   `bench.c`: 核心整数基准测试 (Pi 计算) 实现，以及多线程调度逻辑。
*   `bench_*.c`: 其他特定基准测试的具体实现 (bench_float.c, bench_mem.c, bench_crypto.c, bench_compress.c, bench_matrix.c)。
*   `detect.c`: CPU 硬件检测例程 (如 CPUID 指令封装)。
*   `timer.c`: 跨平台高精度计时器封装 (Win32 下使用 `QueryPerformanceCounter`，Win16 下使用 `GetTickCount`)。
*   `sigmaz.rc`: 定义 GUI 布局、菜单、对话框的资源脚本文件。
*   `docs/`: Markdown 格式的文档源文件。
*   `build_docs.py`: 文档自动编译脚本。
