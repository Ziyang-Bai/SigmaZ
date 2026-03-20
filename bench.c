/*
 * SigmaZ
 * Copyright (c) 2026 Ziyang Bai
 * Licensed under the GNU General Public License v3.0
 */

/*
 * SigmaLM - PI Benchmark Engine (Machin-like Formula)
 * Target: Win16/Win32/Win64
 * Standard: ANSI C (C89)
 * Compiler: Open Watcom v2 / MSVC 6.0 / GCC
 * Note: Uses basic integer arrays to simulate high-precision arithmetic.
 * Algorithm: Machin-like formula: pi/4 = 4 * arctan(1/5) - arctan(1/239)
 * Precision: 2000 decimal digits.
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bench.h"
#include "detect.h"
#include "timer.h"

/* Number of digits to calculate */
#define DIGITS 2000
/* Base for the array elements (10000 allows storing 4 digits per int) */
#define BASE 10000
/* Number of array elements needed */
/* +10 for safety buffer */
#define ARR_SIZE ((DIGITS / 4) + 10)

/* 
 * Use long to ensure at least 32-bit width on 16-bit systems (Win16).
 * In 16-bit mode, int is 16-bit (-32768 to 32767), which is too small for
 * intermediate calculations like remainder * BASE (can be up to ~239 * 10000 = 2,390,000).
 * long is 32-bit guaranteed in C89 on these platforms.
 */
typedef long NUMBER;

/* Context structure to hold thread-local buffers */
typedef struct {
    NUMBER *pi;
    NUMBER *term;
    NUMBER *temp;
} BenchContext;

/* Global arrays for Single-Thread / Win16 compatibility (avoid stack/heap issues on 16-bit) */
static NUMBER g_pi[ARR_SIZE];
static NUMBER g_term[ARR_SIZE];
static NUMBER g_temp[ARR_SIZE];

/* 
 * BigInt Division: divident = divident / divisor 
 * Returns non-zero if result is not zero (to check convergence) 
 */
static int BigDiv(NUMBER *n, NUMBER divisor) {
    NUMBER remainder = 0;
    int i;
    int is_zero = 1;

    for (i = 0; i < ARR_SIZE; i++) {
        NUMBER val = n[i] + remainder * BASE;
        NUMBER res = val / divisor;
        remainder = val % divisor;
        n[i] = res;
        if (res > 0) is_zero = 0;
    }
    return !is_zero; /* Returns true if array is NOT all zeros */
}

/* BigInt Addition: target = target + source */
static void BigAdd(NUMBER *target, NUMBER *source) {
    int i;
    for (i = ARR_SIZE - 1; i >= 0; i--) {
        target[i] += source[i];
        if (target[i] >= BASE) {
            target[i] -= BASE;
            if (i > 0) target[i - 1]++;
        }
    }
}

/* BigInt Subtraction: target = target - source */
static void BigSub(NUMBER *target, NUMBER *source) {
    int i;
    for (i = ARR_SIZE - 1; i >= 0; i--) {
        target[i] -= source[i];
        if (target[i] < 0) {
            target[i] += BASE;
            if (i > 0) target[i - 1]--;
        }
    }
}

/* Initialize array with a small integer value */
/* static void BigSet(NUMBER *n, NUMBER val) {
    int i;
    for (i = 0; i < ARR_SIZE; i++) n[i] = 0;
    n[0] = val;
} */

/* 
 * Simplified Set-and-Divide to initialize 1/x 
 * target = 1/x
 */
static void BigSetInv(NUMBER *target, NUMBER x) {
    NUMBER remainder = 1; /* Represents the numerator '1' */
    int i;
    for (i = 0; i < ARR_SIZE; i++) {
        NUMBER val = remainder * BASE;
        target[i] = val / x;
        remainder = val % x;
    }
}

/* Multiply by small int: target = target * multiplier */
static void BigMulShort(NUMBER *target, NUMBER multiplier) {
    NUMBER carry = 0;
    int i;
    for (i = ARR_SIZE - 1; i >= 0; i--) {
        NUMBER val = target[i] * multiplier + carry;
        target[i] = val % BASE;
        carry = val / BASE;
    }
}

/* 
 * Calculate arctan(1/x) and add/sub to result 
 * formula: 1/x - 1/3x^3 + 1/5x^5 ...
 * We multiply the entire result by 'factor' (e.g. 4 for 4*arctan(1/5))
 * operation: result += factor * arctan(1/x) 
 * 
 * initial_sign: 1 for adding ( + - + - ), -1 for subtracting ( - + - + ) 
 */
static void AddArcTan(BenchContext *ctx, NUMBER *result, NUMBER x, NUMBER factor, int initial_sign) {
    NUMBER x2 = x * x;
    NUMBER divisor = 1;
    int sign = initial_sign; 
    int not_zero = 1;

    /* Initialize term = 1/x */
    BigSetInv(ctx->term, x);

    /* Loop for Taylor series */
    while (not_zero) {
        /* Copy term to temp for division by 'divisor' (1, 3, 5...) */
        memcpy(ctx->temp, ctx->term, sizeof(NUMBER) * ARR_SIZE);
        not_zero = BigDiv(ctx->temp, divisor);

        /* Apply factor (e.g., 4) */
        if (factor != 1) {
            BigMulShort(ctx->temp, factor);
        }

        /* Add or Subtract from result */
        if (sign > 0) 
            BigAdd(result, ctx->temp);
        else 
            BigSub(result, ctx->temp);

        /* Prepare next term: term = term / x^2 */
        if (not_zero) {
            if (BigDiv(ctx->term, x2) == 0) break;
        }
        
        divisor += 2;
        sign = -sign;
    }
}

/*
 * Core workload function.
 * Calculates Pi one time (or loop times per pass).
 * Returns number of loops completed.
 */
static int DoBenchmarkWork(BenchContext *ctx) {
    int loop_idx;
    
    /* 
     * Machin Formula: pi/4 = 4 * arctan(1/5) - arctan(1/239)
     * So pi = 16 * arctan(1/5) - 4 * arctan(1/239)
     */
    for (loop_idx = 0; loop_idx < BENCH_LOOPS; loop_idx++) {
        /* Check for timeout */
        Timer_Stop();
        if (Timer_GetElapsedMs() > BENCH_TIMEOUT_MS) return loop_idx;

        /* Reset arrays */
        memset(ctx->pi, 0, sizeof(NUMBER) * ARR_SIZE);

        /* Add 16 * arctan(1/5) */
        AddArcTan(ctx, ctx->pi, 5, 16, 1);
    
        /* Subtract 4 * arctan(1/239) */
        AddArcTan(ctx, ctx->pi, 239, 4, -1);
    }
    return BENCH_LOOPS;
}

/*
 * Run one pass and return duration in ms.
 * Used by Single Thread Benchmark.
 * loops_out: (Optional) Output for actual loops completed.
 */
static double RunOnePassStatic(int *loops_out) {
    BenchContext ctx;
    int completed;
    
    /* Use global arrays for single/Win16 thread */
    ctx.pi = g_pi;
    ctx.term = g_term;
    ctx.temp = g_temp;
    
    Timer_Start();
    completed = DoBenchmarkWork(&ctx);
    Timer_Stop();

    if (loops_out) *loops_out = completed;
    return Timer_GetElapsedMs();
}

/*
 * SAMPLES for averaging results
 */
#define SAMPLES 5

int CompareDouble(const void * a, const void * b) {
    double da = *(double*)a;
    double db = *(double*)b;
    if (da < db) return -1;
    if (da > db) return 1;
    return 0;
}

/*
 * Single Thread Benchmark Entry Point
 */
DWORD RunSingleThreadBenchmark(BENCH_CALLBACK callback) {
    double times[SAMPLES];
    int loops_done[SAMPLES];
    int i;
    double total = 0;
    int total_loops = 0;

    Timer_Init();
    /* Warm up pass */
    /* RunOnePassStatic(NULL); */

    for (i = 0; i < SAMPLES; i++) {
        if (callback) callback(i * 100 / SAMPLES);
        times[i] = RunOnePassStatic(&loops_done[i]);
        
        /* If timed out, stop early */
        if (loops_done[i] < BENCH_LOOPS) {
             /* Calculate score based on THIS sample alone */
             double time = times[i];
             double score;
             if (time < 0.001) time = 0.001;
             
             score = ((double)loops_done[i] * (double)BENCH_SCORE_SCALE) / time;
             if (callback) callback(100);
             return (DWORD)score;
        }
    }
    if (callback) callback(100);

    /* Sort and drop min/max */
    qsort(times, SAMPLES, sizeof(double), CompareDouble);
    
    /* Average middle 3 */
    for (i = 1; i < SAMPLES - 1; i++) {
        total += times[i];
        total_loops += BENCH_LOOPS; /* Since we verified completion */
    }
    
    {
        double avg_time = total / (SAMPLES - 2);
        if (avg_time < 0.001) avg_time = 0.001; /* Prevent div by zero */

        /* 
         * CALCULATE NORMALIZED SCORE 
         * Score = Work (Loops) * Scale / Time(ms)
         */
        return (DWORD)(((double)BENCH_LOOPS * (double)BENCH_SCORE_SCALE) / avg_time);
    }
}

/* --------------------------------------------------------- */
/* Multi-Core Implementation */
/* --------------------------------------------------------- */

#ifdef _WIN32

typedef struct {
    int thread_id;
    int loops_completed;
    BenchContext ctx;
} ThreadParams;

DWORD WINAPI BenchThreadProc(LPVOID lpParam) {
    ThreadParams* params = (ThreadParams*)lpParam;
    
    /* Allocate memory for this thread */
    /* We must allocate dynamically because stack space per thread might be limited 
       and we can't use globals. */
    /* 2KB per array is small enough for stack on Win32 (1MB stack default), 
       but malloc is safer for large arrays if we extend precision later. */
    params->ctx.pi = (NUMBER*)malloc(sizeof(NUMBER) * ARR_SIZE);
    params->ctx.term = (NUMBER*)malloc(sizeof(NUMBER) * ARR_SIZE);
    params->ctx.temp = (NUMBER*)malloc(sizeof(NUMBER) * ARR_SIZE);
    
    if (!params->ctx.pi || !params->ctx.term || !params->ctx.temp) {
        /* Allocation failed */
        return 0;
    }
    
    params->loops_completed = DoBenchmarkWork(&params->ctx);
    
    /* Free memory */
    free(params->ctx.pi);
    free(params->ctx.term);
    free(params->ctx.temp);
    
    return 1;
}

DWORD RunMultiCoreBenchmark(BENCH_CALLBACK callback) {
    int num_cores = GetCPUCount();
    HANDLE *threads;
    ThreadParams *params;
    DWORD *thread_ids;
    double total_time;
    int i;
    unsigned long total_loops = 0;
    
    if (num_cores < 1) num_cores = 1;
    
    threads = (HANDLE*)malloc(sizeof(HANDLE) * num_cores);
    params = (ThreadParams*)malloc(sizeof(ThreadParams) * num_cores);
    thread_ids = (DWORD*)malloc(sizeof(DWORD) * num_cores);
    
    if (!threads || !params || !thread_ids) return 0;

    Timer_Init();
    Timer_Start();
    
    /* Launch threads */
    for (i = 0; i < num_cores; i++) {
        params[i].thread_id = i;
        /* CreateThread is Win32 API */
        threads[i] = CreateThread(
            NULL,               /* Default security */
            0,                  /* Default stack size */
            BenchThreadProc,    /* Thread function */
            &params[i],         /* Argument */
            0,                  /* Creation flags */
            &thread_ids[i]      /* Thread ID */
        );
        
        if (threads[i] == NULL) {
            /* Handle error - maybe just continue with what we have or abort */
        }
    }
    
    /* Wait for all threads to complete */
    if (callback) callback(50); /* Indiscriminate progress */
    
    WaitForMultipleObjects(num_cores, threads, TRUE, INFINITE);
    
    Timer_Stop();
    
    /* Cleanup handles and sum work */
    for (i = 0; i < num_cores; i++) {
        CloseHandle(threads[i]);
        total_loops += params[i].loops_completed;
    }
    
    free(threads);
    free(params);
    free(thread_ids);
    
    if (callback) callback(100);
    
    /* Return total time in milliseconds adjusted for work done */
    /* If we did N jobs in T time, equivalent single core time is T / N */
    total_time = Timer_GetElapsedMs();
    if (total_time < 0.001) total_time = 0.001; /* Avoid div by zero */
    
    /* Calculate Score: (Total Iterations across all Cores * Scale) / Duration */
    {
        double score = ((double)total_loops * (double)BENCH_SCORE_SCALE) / total_time;
        return (DWORD)score;
    }
}

#else

/* Win16 Fallback for MultiCore (Just run single core) */
DWORD RunMultiCoreBenchmark(BENCH_CALLBACK callback) {
    /* Since Single Thread already returns normalized score, just return it. */
    /* Efficiency will be 100% since 1 core = 1 core */
    return RunSingleThreadBenchmark(callback);
}

#endif
