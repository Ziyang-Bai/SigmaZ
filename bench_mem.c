/*
 * SigmaZ
 * Copyright (c) 2026 Ziyang Bai
 * Licensed under the GNU General Public License v3.0
 */



#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bench.h"
#include "timer.h"

volatile unsigned char g_mem_acc = 0;

#ifdef _WIN32
  #if defined(_WIN64)
    #define BUFFER_SIZE (128 * 1024 * 1024) 
  #else
    #define BUFFER_SIZE (32 * 1024 * 1024)  
  #endif
#else
  #define WIN16_NUM_BLOCKS 8
  #define WIN16_BLOCK_SIZE 60000 
#endif


DWORD RunMemoryBenchmark(BENCH_CALLBACK callback) {
    double duration = 0;
    unsigned long total_bytes = 0;
    int b;
    
#ifdef _WIN32
    char *src;
    char *dst;
#else
    HGLOBAL hSrc[WIN16_NUM_BLOCKS];
    HGLOBAL hDst[WIN16_NUM_BLOCKS];
    char FAR *pSrc[WIN16_NUM_BLOCKS];
    char FAR *pDst[WIN16_NUM_BLOCKS];
    int alloc_fail = 0;
#endif

    Timer_Init();

#ifdef _WIN32
    src = (char *)malloc(BUFFER_SIZE);
    dst = (char *)malloc(BUFFER_SIZE);

    if (!src || !dst) {
        if (src) free(src);
        if (dst) free(dst);
        return 0; 
    }

    
    for (b = 0; b < (int)BUFFER_SIZE; b += 4096) {
        src[b] = (char)(b & 0xFF);
        dst[b] = 0;
    }
#else
    
    for (b = 0; b < WIN16_NUM_BLOCKS; b++) {
        hSrc[b] = GlobalAlloc(GHND, WIN16_BLOCK_SIZE);
        hDst[b] = GlobalAlloc(GHND, WIN16_BLOCK_SIZE);
        if (!hSrc[b] || !hDst[b]) {
            alloc_fail = 1;
        } else {
            pSrc[b] = (char FAR *)GlobalLock(hSrc[b]);
            pDst[b] = (char FAR *)GlobalLock(hDst[b]);
            if (!pSrc[b] || !pDst[b]) alloc_fail = 1;
        }
    }
    if (alloc_fail) {
        for (b = 0; b < WIN16_NUM_BLOCKS; b++) {
            if (hSrc[b]) { if (pSrc[b]) GlobalUnlock(hSrc[b]); GlobalFree(hSrc[b]); }
            if (hDst[b]) { if (pDst[b]) GlobalUnlock(hDst[b]); GlobalFree(hDst[b]); }
        }
        return 0;
    }
    
    
    for (b = 0; b < WIN16_NUM_BLOCKS; b++) {
        pSrc[b][0] = 1;
        pDst[b][0] = 0;
    }
#endif

    Timer_Start();

    while (1) {
#ifdef _WIN32
        memcpy(dst, src, BUFFER_SIZE);
        g_mem_acc += dst[BUFFER_SIZE - 1];
        total_bytes += BUFFER_SIZE;
#else
        for (b = 0; b < WIN16_NUM_BLOCKS; b++) {
            _fmemcpy(pDst[b], pSrc[b], WIN16_BLOCK_SIZE);
            g_mem_acc += pDst[b][WIN16_BLOCK_SIZE - 1];
            total_bytes += WIN16_BLOCK_SIZE;
        }
#endif

        Timer_Stop();
        duration = Timer_GetElapsedMs();

        if (callback) {
            callback((int)(duration * 100.0 / 3000.0));
        }

        if (duration >= 3000.0) break;
    }

#ifdef _WIN32
    free(src);
    free(dst);
#else
    for (b = 0; b < WIN16_NUM_BLOCKS; b++) {
        if (hSrc[b]) { GlobalUnlock(hSrc[b]); GlobalFree(hSrc[b]); }
        if (hDst[b]) { GlobalUnlock(hDst[b]); GlobalFree(hDst[b]); }
    }
#endif

    if (duration < 0.001) duration = 0.001;

    
    {
        double mb_s = ((double)total_bytes / (1024.0 * 1024.0)) / (duration / 1000.0);
        return (DWORD)mb_s;
    }
}
