/*
 * SigmaZ
 * Copyright (c) 2026 Ziyang Bai
 * Licensed under the GNU General Public License v3.0
 */

/*
 * SigmaLM - Floating Point Benchmark (Mandelbrot)
 * Target: Win16/Win32
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "bench.h"
#include "timer.h"

/* 
 * Mandelbrot parameters 
 * -2.0 to 1.0 Real
 * -1.0 to 1.0 Imag
 */
#ifdef _WIN32
  #define MAX_ITER 1000
  #define WIDTH 800
  #define HEIGHT 600
#else
  /* Win16 targets (386/486) are much slower, use VGA resolution */
  #define MAX_ITER 1000
  #define WIDTH 320
  #define HEIGHT 240
#endif

/* 
 * Calculates Mandelbrot check for a point (cx, cy)
 * Returns number of iterations before escape.
 */
static int MandelbrotPt(double cx, double cy) {
    double zx = 0.0;
    double zy = 0.0;
    double zx2 = 0.0;
    double zy2 = 0.0;
    int iter = 0;
    
    while (iter < MAX_ITER && (zx2 + zy2) < 4.0) {
        zy = 2.0 * zx * zy + cy;
        zx = zx2 - zy2 + cx;
        zx2 = zx * zx;
        zy2 = zy * zy;
        iter++;
    }
    return iter;
}

#ifdef __AVX2__
/* 
 * [Disabled] AVX2 Optimized Path
 * Reason: Precision differences between x87 (80-bit/64-bit hybrid behavior on Win16),
 * SSE2 scalar (strict 64-bit IEEE 754), and AVX FMA (fused multiply-add rounding) 
 * can cause significant divergence in chaotic systems like Mandelbrot.
 * To maintain a fair and consistent baseline score across architectures (486 to modern Core/Zen),
 * we stick to the standard C implementation and let the compiler generate the default scalar code.
 * 
 * If a "SIMD Stress Test" mode is added in the future, this code could be reused there.
 */
/*
#include <immintrin.h>
/* 
 * AVX2 Optimized Path
 * Access via MSVC/GCC with -mavx2 or /arch:AVX2
 */
// DWORD RunFloatBenchmarkAVX2(BENCH_CALLBACK callback) {
//     double duration;
//     unsigned long total_iters = 0;
//     int x, y;
//     int percent_slice = HEIGHT / 20;

//     /* AVX2 Constants */
//     __m256d v_threshold = _mm256_set1_pd(4.0);
//     __m256d v_scale_x = _mm256_set1_pd(3.0 / WIDTH);
//     __m256d v_scale_y = _mm256_set1_pd(2.0 / HEIGHT);
//     __m256d v_offset_x = _mm256_set1_pd(-2.0);
//     __m256d v_offset_y = _mm256_set1_pd(-1.0);
//     /* _mm256_set_pd sets values in reverse order: e3, e2, e1, e0 */
//     __m256d v_step = _mm256_set_pd(3.0, 2.0, 1.0, 0.0);
//     __m256d v_one = _mm256_set1_pd(1.0);

//     Timer_Init();
//     Timer_Start();

//     for (y = 0; y < HEIGHT; y++) {
//         /* cy = -1.0 + (2.0 * y) / HEIGHT */
//         double cy_scalar = -1.0 + (2.0 * y) / HEIGHT; 
//         __m256d v_cy = _mm256_set1_pd(cy_scalar);

//         /* Report progress */
//         if (callback && (y % percent_slice == 0)) {
//             callback(y * 100 / HEIGHT);
//         }

//         /* Check Timeout */
//         Timer_Stop();
//         if (Timer_GetElapsedMs() > BENCH_TIMEOUT_MS) break;

//         for (x = 0; x < WIDTH; x += 4) {
//              /* cx = -2.0 + (3.0 * (x + offset)) / WIDTH */
//              __m256d v_x_broadcast = _mm256_set1_pd((double)x);
//              __m256d v_x_idx = _mm256_add_pd(v_x_broadcast, v_step);
//              __m256d v_cx = _mm256_add_pd(v_offset_x, _mm256_mul_pd(v_x_idx, v_scale_x));

//              __m256d v_zx = _mm256_setzero_pd();
//              __m256d v_zy = _mm256_setzero_pd();
//              __m256d v_zx2 = _mm256_setzero_pd();
//              __m256d v_zy2 = _mm256_setzero_pd();
//              __m256d v_iters = _mm256_setzero_pd();

//              int i;
//              for (i = 0; i < MAX_ITER; i++) {
//                  /* Check radius: zx2 + zy2 < 4.0 */
//                  __m256d v_sum_sq = _mm256_add_pd(v_zx2, v_zy2);
//                  __m256d v_mask = _mm256_cmp_pd(v_sum_sq, v_threshold, _CMP_LT_OQ);

//                  /* If all escaped (mask is 0), break */
//                  if (_mm256_movemask_pd(v_mask) == 0) break;

//                  /* Add 1.0 to iters where mask is set */
//                  /* Mask is all 1s (NaN/0xFF...) when True. AND with 1.0 gives 1.0 or 0.0 */
//                  __m256d v_inc = _mm256_and_pd(v_mask, v_one);
//                  v_iters = _mm256_add_pd(v_iters, v_inc);

//                  /* Mandelbrot iteration: zy = 2*zx*zy + cy */
//                  __m256d v_zy_new = _mm256_add_pd(_mm256_mul_pd(_mm256_add_pd(v_zx, v_zx), v_zy), v_cy);
                 
//                  /* zx = zx2 - zy2 + cx */
//                  v_zx = _mm256_add_pd(_mm256_sub_pd(v_zx2, v_zy2), v_cx);
//                  v_zy = v_zy_new;

//                  v_zx2 = _mm256_mul_pd(v_zx, v_zx);
//                  v_zy2 = _mm256_mul_pd(v_zy, v_zy);
//              }

//              /* Sum the 4 iters counters */
//              double buf[4];
//              _mm256_storeu_pd(buf, v_iters);
//              /* Cast to int before sum to avoid precision issues accumulate? No, iters are small integer. */
//              total_iters += (unsigned long)(buf[0] + buf[1] + buf[2] + buf[3]);
//         }
//     }
    
//     if (callback) callback(100);

//     Timer_Stop();
//     duration = Timer_GetElapsedMs();
//     if (duration < 0.001) duration = 0.001;

//     {
//         double score = (double)total_iters / duration;
//         return (DWORD)(score * 10.0);
//     }
// }
#endif

/*
 * Run Floating Point Benchmark
 * Returns Score (Iterations per millisecond * 10 or similar)
 */
DWORD RunFloatBenchmark(BENCH_CALLBACK callback) {
#ifdef __AVX2__
    return RunFloatBenchmarkAVX2(callback);
#endif
    /* Fallback to original scalar implementation */
    double duration;
    unsigned long total_iters = 0;
    int x, y;
    int percent_slice = HEIGHT / 20; /* Update every 5% */
    
    Timer_Init();
    Timer_Start();
    
    for (y = 0; y < HEIGHT; y++) {
        double cy = -1.0 + (2.0 * y) / HEIGHT;
        
        /* Report progress */
        if (callback && (y % percent_slice == 0)) {
            callback(y * 100 / HEIGHT);
        }

        /* Check Timeout */
        Timer_Stop();
        if (Timer_GetElapsedMs() > BENCH_TIMEOUT_MS) break;
        
        for (x = 0; x < WIDTH; x++) {
            double cx = -2.0 + (3.0 * x) / WIDTH;
            total_iters += MandelbrotPt(cx, cy);
        }
    }
    
    if (callback) callback(100);
    
    Timer_Stop();
    duration = Timer_GetElapsedMs();
    
    if (duration < 0.001) duration = 0.001;
    
    /* 
     * Score = Total Iterations / Duration (ms) 
     * Output physically meaningful generic: Iterations per ms.
     */
    {
        double score = (double)total_iters / duration;
        return (DWORD)score;
    }}
