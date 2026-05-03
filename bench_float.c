/*
 * SigmaZ
 * Copyright (c) 2026 Ziyang Bai
 * Licensed under the GNU General Public License v3.0
 */



#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "bench.h"
#include "timer.h"


#ifdef _WIN32
  #define MAX_ITER 1000
  #define WIDTH 800
  #define HEIGHT 600
#else
  
  #define MAX_ITER 1000
  #define WIDTH 320
  #define HEIGHT 240
#endif


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

#endif


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
    
    
    {
        double score = (double)total_iters / duration;
        return (DWORD)score;
    }
}
