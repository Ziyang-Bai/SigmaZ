/*
 * SigmaZ
 * Copyright (c) 2026 Ziyang Bai
 * Licensed under the GNU General Public License v3.0
 */

#include <windows.h>
#include "timer.h"

/* 
 * Timer Implementation
 * Win32: QueryPerformanceCounter (Resolution < 1us)
 * Win16: GetTickCount (Resolution ~15ms - 55ms)
 */

#ifdef _WIN32

static LARGE_INTEGER g_Freq;
static LARGE_INTEGER g_Start;
static LARGE_INTEGER g_End;
static int g_HasPerf = 0;
static int g_Initialized = 0;

void Timer_Init(void) {
    if (QueryPerformanceFrequency(&g_Freq)) {
        g_HasPerf = 1;
    } else {
        g_HasPerf = 0;
    }
    g_Initialized = 1;
}

void Timer_Start(void) {
    if (!g_Initialized) Timer_Init();

    if (g_HasPerf) {
        QueryPerformanceCounter(&g_Start);
    } else {
        g_Start.LowPart = GetTickCount();
    }
}

void Timer_Stop(void) {
    if (g_HasPerf) {
        QueryPerformanceCounter(&g_End);
    } else {
        g_End.LowPart = GetTickCount();
    }
}

double Timer_GetElapsedMs(void) {
    if (g_HasPerf) {
        LONGLONG diff = g_End.QuadPart - g_Start.QuadPart;
        return ((double)diff * 1000.0) / (double)g_Freq.QuadPart;
    } else {
        return (double)(g_End.LowPart - g_Start.LowPart);
    }
}

#else

/* Win16 Implementation */
static DWORD g_Start;
static DWORD g_End;

void Timer_Init(void) {
    /* No-op */
}

void Timer_Start(void) {
    g_Start = GetTickCount();
}

void Timer_Stop(void) {
    g_End = GetTickCount();
}

double Timer_GetElapsedMs(void) {
    return (double)(g_End - g_Start);
}

#endif
