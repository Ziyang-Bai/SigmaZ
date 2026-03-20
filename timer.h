/*
 * SigmaZ
 * Copyright (c) 2026 Ziyang Bai
 * Licensed under the GNU General Public License v3.0
 */

#ifndef TIMER_H
#define TIMER_H

#include <windows.h>

/*
 * High-Resolution Timer Wrapper
 * - Win32/x64 Uses QueryPerformanceCounter
 * - Win16 Uses GetTickCount
 */

void Timer_Init(void);
void Timer_Start(void);
void Timer_Stop(void);
double Timer_GetElapsedMs(void);

#endif
