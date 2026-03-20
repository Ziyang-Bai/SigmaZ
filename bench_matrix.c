/*
 * SigmaZ
 * Copyright (c) 2026 Ziyang Bai
 * Licensed under the GNU General Public License v3.0
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "bench.h"
#include "timer.h"

/*
 * SigmaZ - Matrix Multiplication Benchmark
 * Computes C = A * B
 * O(N^3) standard algorithm.
 */

#define MAT_N 64

#ifdef _WIN32
  #define MATRIX_LOOPS 1000
#else
  #define MATRIX_LOOPS 1
#endif

/* 
 * We use a flattened array for matrices to be compatible with standard C malloc.
 * A[row][col] -> A[row * N + col]
 */

static void init_matrix(double *m, int n) {
    int i;
    for (i = 0; i < n * n; i++) {
        m[i] = (double)(rand() % 100) / 10.0;
    }
}

/* 
 * Naive implementation 
 * Compiler auto-vectorization target.
 * Returns number of rows processed.
 */
static int mat_mul(const double *A, const double *B, double *C, int n, BENCH_CALLBACK callback) {
    int i, j, k;
    for (i = 0; i < n; i++) {
        /* Check Timeout */
        Timer_Stop();
        if (Timer_GetElapsedMs() > BENCH_TIMEOUT_MS) return i;

        for (j = 0; j < n; j++) {
            double sum = 0.0;
            for (k = 0; k < n; k++) {
                sum += A[i * n + k] * B[k * n + j];
            }
            C[i * n + j] = sum;
        }
        /* Update progress every 5% of rows to keep UI responsive */
        if (callback && (i % (n/20) == 0)) {
            callback((i * 100) / n);
        }
    }
    if (callback) callback(100);
    return n;
}

double RunMatrixBenchmark(BENCH_CALLBACK callback) {
    double *A, *B, *C;
    double duration;

#ifndef _WIN32
    HGLOBAL hA = NULL;
    HGLOBAL hB = NULL;
    HGLOBAL hC = NULL;
#endif

#ifdef _WIN32
    A = (double*)malloc(MAT_N * MAT_N * sizeof(double));
    B = (double*)malloc(MAT_N * MAT_N * sizeof(double));
    C = (double*)malloc(MAT_N * MAT_N * sizeof(double));
#else
    /* Win16: 64*64*8 = 32KB per matrix. Total 96KB -> Exceeds 64KB DGROUP. Must use GlobalAlloc */
    hA = GlobalAlloc(GMEM_MOVEABLE, MAT_N * MAT_N * sizeof(double));
    if (hA) A = (double*)GlobalLock(hA); else A = NULL;

    hB = GlobalAlloc(GMEM_MOVEABLE, MAT_N * MAT_N * sizeof(double));
    if (hB) B = (double*)GlobalLock(hB); else B = NULL;

    hC = GlobalAlloc(GMEM_MOVEABLE, MAT_N * MAT_N * sizeof(double));
    if (hC) C = (double*)GlobalLock(hC); else C = NULL;
#endif
    
    if (!A || !B || !C) {
#ifdef _WIN32
        if (A) free(A);
        if (B) free(B);
        if (C) free(C);
#else
        if (A) GlobalUnlock(hA);
        if (B) GlobalUnlock(hB);
        if (C) GlobalUnlock(hC);
        if (hA) GlobalFree(hA);
        if (hB) GlobalFree(hB);
        if (hC) GlobalFree(hC);
#endif
        return 0.0;
    }
    
    /* Initialize with deterministic values */
    init_matrix(A, MAT_N);
    init_matrix(B, MAT_N);
    
    Timer_Init();
    Timer_Start();

    {
        int loop;
        double total_completed_matrices = 0.0;
        BENCH_CALLBACK inner_cb = (MATRIX_LOOPS == 1) ? callback : NULL;
        
        for (loop = 0; loop < MATRIX_LOOPS; loop++) {
            int rows_done = mat_mul(A, B, C, MAT_N, inner_cb);
            total_completed_matrices += (double)rows_done / MAT_N;
            
            if (MATRIX_LOOPS > 1 && callback) {
                callback((int)((total_completed_matrices * 100) / MATRIX_LOOPS));
            }
            
            Timer_Stop();
            if (Timer_GetElapsedMs() > BENCH_TIMEOUT_MS || rows_done < MAT_N) break;
        }

        if (callback) callback(100);

        Timer_Stop();
        duration = Timer_GetElapsedMs() / 1000.0;

#ifdef _WIN32
        free(A);
        free(B);
        free(C);
#else
        if (A) GlobalUnlock(hA);
        if (B) GlobalUnlock(hB);
        if (C) GlobalUnlock(hC);
        if (hA) GlobalFree(hA);
        if (hB) GlobalFree(hB);
        if (hC) GlobalFree(hC);
#endif
        
        if (duration <= 0.0) return 0.0;

        /*
         * Score: Matrices Per Second
         */
        return total_completed_matrices / duration;
    }
}
