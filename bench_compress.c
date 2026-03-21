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
 * SigmaZ - Data Compression Benchmark
 * Algorithm: Simplified LZ77 (Sliding Window)
 * Focus: Integer performance, Branch Prediction, Memory Access Patterns.
 */

#define WINDOW_SIZE 4096
#define MAX_MATCH_LEN 18
#define MIN_MATCH_LEN 3

/* 
 * Compression Buffer Sizes
 * Standardize on 16KB for all platforms to ensure identical data patterns
 * and identical window/match characteristics.
 */
#define COMP_INPUT_SIZE 16384

#ifdef _WIN32
  #define COMP_LOOPS 250
#else
  #define COMP_LOOPS 250
#endif
/* Simple hash for data generation */
static void fill_random_data(unsigned char *buf, size_t size) {
    size_t i;
    unsigned long seed = 12345;
    /* Create some repeatable patterns for compression to find */
    for (i = 0; i < size; i++) {
        seed = seed * 1664525 + 1013904223;
        /* Bias towards printable chars and repeated sequences */
        buf[i] = (unsigned char)((seed >> 24) % 128); 
        if (i % 50 == 0) {
           /* Inject a run of As */
           int j, run = (seed % 10) + 3;
           for (j=0; j<run && (i+j)<size; j++) buf[i+j] = 'A';
           i += j;
        }
    }
}

/* 
 * Simplified LZ77 Compress 
 * Returns compressed size (simulated, we don't strictly enable bit-packing 
 * to keep code clean and focused on the search algo).
 */
static unsigned long lz_compress(const unsigned char *src, unsigned long src_len, unsigned char *dst) {
    unsigned long src_pos = 0;
    unsigned long dst_pos = 0;
    
    while (src_pos < src_len) {
        int best_match_len = 0;
        int best_match_dist = 0;
        int max_search;
        int i, len;

        /* Skip if near end */
        if (src_pos + MIN_MATCH_LEN >= src_len) {
            dst[dst_pos++] = src[src_pos++];
            continue;
        }

        /* Search backwards in window */
        max_search = (src_pos > WINDOW_SIZE) ? WINDOW_SIZE : (int)src_pos;
        
        /* Brute force search (Good for CPU branching/ALU stress) */
        /* Optimization: In real world, use hash chain. Here, O(N*Window) stresses CPU more. */
        for (i = 1; i <= max_search; i++) {
            const unsigned char *prev = &src[src_pos - i];
            const unsigned char *curr = &src[src_pos];
            
            /* Check match at this distance */
            if (prev[0] == curr[0] && prev[1] == curr[1] && prev[2] == curr[2]) {
                /* Found candidate, extend */
                for (len = 0; len < MAX_MATCH_LEN && (src_pos + len < src_len); len++) {
                    if (prev[len] != curr[len]) break;
                }
                
                if (len > best_match_len) {
                    best_match_len = len;
                    best_match_dist = i;
                }
            }
        }

        if (best_match_len >= MIN_MATCH_LEN) {
            /* Output token (Simulated: Tag + Dist + Len) */
            /* In real deflate: Huffman encoding here */
            dst[dst_pos++] = 0xFF; /* Marker */
            dst[dst_pos++] = (unsigned char)(best_match_dist & 0xFF);
            dst[dst_pos++] = (unsigned char)(best_match_len);
            src_pos += best_match_len;
        } else {
            /* Literal */
            dst[dst_pos++] = src[src_pos++];
        }
    }
    return dst_pos;
}

double RunCompressionBenchmark(BENCH_CALLBACK callback) {
    unsigned char *in_buf;
    unsigned char *out_buf;
    double duration;
    unsigned long total_bytes = 0;
    int i;

#ifndef _WIN32
    HGLOBAL hIn = NULL;
    HGLOBAL hOut = NULL;
#endif
    
    /* Alloc buffers */
#ifdef _WIN32
    in_buf = (unsigned char*)malloc(COMP_INPUT_SIZE);
    out_buf = (unsigned char*)malloc(COMP_INPUT_SIZE + 4096); 
#else
    /* Win16: Use GlobalAlloc to avoid DGROUP/Stack overflow */
    hIn = GlobalAlloc(GMEM_MOVEABLE, COMP_INPUT_SIZE);
    if (hIn) in_buf = (unsigned char*)GlobalLock(hIn);
    else in_buf = NULL;

    hOut = GlobalAlloc(GMEM_MOVEABLE, COMP_INPUT_SIZE + 4096);
    if (hOut) out_buf = (unsigned char*)GlobalLock(hOut);
    else out_buf = NULL;
#endif
    
    if (!in_buf || !out_buf) {
#ifdef _WIN32
        if (in_buf) free(in_buf);
        if (out_buf) free(out_buf);
#else
        if (in_buf) GlobalUnlock(hIn);
        if (out_buf) GlobalUnlock(hOut);
        if (hIn) GlobalFree(hIn);
        if (hOut) GlobalFree(hOut);
#endif
        return 0.0;
    }
    
    fill_random_data(in_buf, COMP_INPUT_SIZE);

    Timer_Init();
    g_BenchTimedOut = 0;
    Timer_Start();

    for (i = 0; i < COMP_LOOPS; i++) {
        unsigned long c_size = lz_compress(in_buf, COMP_INPUT_SIZE, out_buf);
        (void)c_size;
        total_bytes += COMP_INPUT_SIZE;
        
        if (callback) callback(((i + 1) * 100) / COMP_LOOPS);

        /* Check for timeout */
        Timer_Stop();
        if (Timer_GetElapsedMs() > BENCH_TIMEOUT_MS) {
            g_BenchTimedOut = 1;
            break;
        }
    }
    
    Timer_Stop();
    duration = Timer_GetElapsedMs() / 1000.0;
    
#ifdef _WIN32
    free(in_buf);
    free(out_buf);
#else
    if (in_buf) GlobalUnlock(hIn);
    if (out_buf) GlobalUnlock(hOut);
    if (hIn) GlobalFree(hIn);
    if (hOut) GlobalFree(hOut);
#endif
    
    if (duration <= 0.0) return 0.0;
    return ((double)total_bytes / 1024.0) / duration; /* KB/sec */
}
