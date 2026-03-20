# Troubleshooting

**Program crashes or exits under Windows 3.1**

This is usually due to exhaustion of system resources (GDI Heap or User Heap) or lack of memory. Please try closing other running programs. If the problem persists, you can try increasing the memory allocated to Windows 3.1, or refer to the stack overflow solutions below.

**Interface unresponsive after clicking Start (Freezes)**

This is a normal phenomenon. On older single-core operating systems without preemptive multitasking, intensive computation threads will temporarily block the message loop. The program includes an internal heartbeat mechanism, and the interface will automatically refresh after the calculation is complete; please stay calm.

**Score shows as 0 or extremely high**

If a test score shows as 0 or extremely high (e.g., the memory item shows a score as high as 10239950 on Windows 98 using 16-bit), it may be because the runtime was too short (less than the timer precision) to calculate a valid score. This usually happens when running `sigma16.exe` (designed for vintage machines) on extremely modern hardware (VMware virtual machines). It is recommended to switch to `sigma64.exe` or `sigma32.exe`.

**Stack Overflow Error**

In some Win16 environments, the default stack space may be insufficient. If this error occurs, due to the Win16 memory model limitations, the stack size cannot be adjusted simply via compilation options. If possible, it is recommended to use `sigma32.exe` or `sigma64.exe`. If you are running on Windows 3.1, please try allocating as much memory as possible. If it still cannot be resolved, please try using `DRWATSON.EXE` to capture a register snapshot and report it to GitHub issues for analysis.