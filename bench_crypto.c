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

/* 
 * SigmaZ - Cryptography Benchmark 
 * Algorithms: 
 * - Win16/Generic: CRC32 (Table based)
 * - Win32/64: AES-NI / SHA (Stub for future expansion)
 */

/* CRC32 Constants */
#define CRC_POLY 0xEDB88320

static unsigned long crc_table[256];
static int crc_table_computed = 0;

/* Precompute CRC table */
static void make_crc_table(void) {
    unsigned long c;
    int n, k;
    
    for (n = 0; n < 256; n++) {
        c = (unsigned long)n;
        for (k = 0; k < 8; k++) {
            if (c & 1)
                c = CRC_POLY ^ (c >> 1);
            else
                c = c >> 1;
        }
        crc_table[n] = c;
    }
    crc_table_computed = 1;
}

/* Update a running CRC with the bytes buf[0..len-1] */
static unsigned long update_crc(unsigned long crc, const unsigned char *buf, int len) {
    unsigned long c = crc;
    int n;
    
    if (!crc_table_computed)
        make_crc_table();
        
    for (n = 0; n < len; n++) {
        c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
    }
    return c;
}

/* Return the CRC of the bytes buf[0..len-1] */
static unsigned long crc32(const unsigned char *buf, int len) {
    return update_crc(0xffffffffL, buf, len) ^ 0xffffffffL;
}

/*
 * Run Crypto Benchmark
 * Returns: Throughput in KB/s
 */
double RunCryptoBenchmark(BENCH_CALLBACK callback) {
    unsigned char *buffer;
    unsigned long buf_size;
    unsigned long total_bytes = 0;
    double duration;
    int i;
    int loop_count;

#ifndef _WIN32
    HGLOBAL hBuf = NULL;
#endif
    
    (void)callback; /* Unused for now */
    
    /*
     * Sizing:
     * Standardize on 16KB for all platforms to ensure consistent buffer size.
     */
#ifdef _WIN32
    buf_size = 16384;       /* 16KB */
    loop_count = 100000;
#else
    buf_size = 16384;       /* 16KB */
    loop_count = 500;
#endif

#ifdef _WIN32
    buffer = (unsigned char*)malloc(buf_size);
#else
    hBuf = GlobalAlloc(GMEM_MOVEABLE, buf_size);
    if (hBuf) buffer = (unsigned char*)GlobalLock(hBuf);
    else buffer = NULL;
#endif

    if (!buffer) return 0.0;
    
    /* Fill buffer with some pattern */
    for (i = 0; i < (int)buf_size; i++) {
        buffer[i] = (unsigned char)(i & 0xFF);
    }
    
    /* Warmup */
    make_crc_table();
    
    Timer_Init();
    Timer_Start();
    
    for (i = 0; i < loop_count; i++) {
        volatile unsigned long crc = crc32(buffer, (int)buf_size);
        (void)crc;
        total_bytes += buf_size;

        if (callback) callback(((i + 1) * 100) / loop_count);

        /* Check for timeout */
        Timer_Stop();
        if (Timer_GetElapsedMs() > BENCH_TIMEOUT_MS) break;
    }
    
    Timer_Stop();
    duration = Timer_GetElapsedMs() / 1000.0;
    
#ifdef _WIN32
    free(buffer);
#else
    if (buffer) GlobalUnlock(hBuf);
    if (hBuf) GlobalFree(hBuf);
#endif

    if (duration <= 0.0) return 0.0;
    return ((double)total_bytes / 1024.0) / duration; /* KB/sec */
}

