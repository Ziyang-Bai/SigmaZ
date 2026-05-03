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
 * AVX2 优化路径
x87、SSE2 标量与 AVX FMA 指令在舍入和精度上的微小差异可能导致曼德勃罗集计算结果发散。
 */
#endif

/*
 * 曼德勃罗集
 * 返回值：每毫秒迭代次数
 */
DWORD RunFloatBenchmark(BENCH_CALLBACK callback) {
    
    double duration;
    unsigned long total_iters = 0;
    int x, y;
    int percent_slice = HEIGHT / 20; 

    Timer_Init();
    g_BenchTimedOut = 0;

    
    for (y = 0; y < HEIGHT / 20; y++) {
        double cy = -1.0 + (2.0 * y) / HEIGHT;
        for (x = 0; x < WIDTH; x++) {
            double cx = -2.0 + (3.0 * x) / WIDTH;
            MandelbrotPt(cx, cy);
        }
    }

    Timer_Start();

    for (y = 0; y < HEIGHT; y++) {
        double cy = -1.0 + (2.0 * y) / HEIGHT;
        
        
        if (callback && (y % percent_slice == 0)) {
            callback(y * 100 / HEIGHT);
        }

        
        Timer_Stop();
        if (Timer_GetElapsedMs() > BENCH_TIMEOUT_MS) {
            g_BenchTimedOut = 1;
            break;
        }

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
    }
}
