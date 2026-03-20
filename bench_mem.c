/*
 * SigmaZ
 * Copyright (c) 2026 Ziyang Bai
 * Licensed under the GNU General Public License v3.0
 */

/*
 * SigmaLM - Memory Bandwidth Benchmark (Stream Copy)
 * Target: Win16/Win32
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bench.h"
#include "timer.h"

#ifdef _WIN32
  #if defined(_WIN64)
    #define BUFFER_SIZE (512 * 1024 * 1024) /* 512 MB for Win64 */
  #else
    #define BUFFER_SIZE (64 * 1024 * 1024)  /* 64 MB for Win32 */
  #endif
#else
  #define BUFFER_SIZE (60 * 1024)         /* 60 KB for Win16 */
#endif

#define LOOP_COUNT 5

/*
 * Run Memory Bandwidth Benchmark
 * Returns Score (MB/s * 100 or similar scaled value)
 */
DWORD RunMemoryBenchmark(BENCH_CALLBACK callback) {
    char *src, *dst;
    double duration;
    int i;
    unsigned long total_bytes = 0;
    
    Timer_Init();
    
    src = (char *)malloc(BUFFER_SIZE);
    dst = (char *)malloc(BUFFER_SIZE);
    
    if (!src || !dst) {
        if (src) free(src);
        if (dst) free(dst);
        return 0; /* Alloc failed */
    }
    
    /* Initialize */
    for (i = 0; i < BUFFER_SIZE; i++) {
        src[i] = (char)(i & 0xFF);
    }
    
    Timer_Start();
    
    for (i = 0; i < LOOP_COUNT; i++) {
        /* Report progress */
        if (callback) {
            callback(i * 100 / LOOP_COUNT);
        }

        /* Check Timeout */
        Timer_Stop();
        if (Timer_GetElapsedMs() > BENCH_TIMEOUT_MS) break;
        
        /* Copy loop - simple memcpy for standard C behavior */
        memcpy(dst, src, BUFFER_SIZE);
        
        total_bytes += BUFFER_SIZE;
    }
    
    Timer_Stop();
    duration = Timer_GetElapsedMs();
    
    free(src);
    free(dst);
    
    if (duration < 0.001) duration = 0.001;
    
    /* 
     * Calculate MB/s 
     * (Total Bytes / 1024 / 1024) / (Duration / 1000)
     */
    {
        double mb_s = ((double)total_bytes / (1024.0 * 1024.0)) / (duration / 1000.0);
        return (DWORD)mb_s;
    }
}
