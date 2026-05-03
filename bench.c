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
#include "detect.h"
#include "timer.h"

int g_BenchTimedOut = 0;


#define DIGITS 2000

#define BASE 10000


#define ARR_SIZE ((DIGITS / 4) + 10)


typedef long NUMBER;


typedef struct {
    NUMBER *pi;
    NUMBER *term;
    NUMBER *temp;
} BenchContext;


static NUMBER g_pi[ARR_SIZE];
static NUMBER g_term[ARR_SIZE];
static NUMBER g_temp[ARR_SIZE];


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
    return !is_zero; 
}


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





static void BigSetInv(NUMBER *target, NUMBER x) {
    NUMBER remainder = 1; 
    int i;
    for (i = 0; i < ARR_SIZE; i++) {
        NUMBER val = remainder * BASE;
        target[i] = val / x;
        remainder = val % x;
    }
}


static void BigMulShort(NUMBER *target, NUMBER multiplier) {
    NUMBER carry = 0;
    int i;
    for (i = ARR_SIZE - 1; i >= 0; i--) {
        NUMBER val = target[i] * multiplier + carry;
        target[i] = val % BASE;
        carry = val / BASE;
    }
}


static void AddArcTan(BenchContext *ctx, NUMBER *result, NUMBER x, NUMBER factor, int initial_sign) {
    NUMBER x2 = x * x;
    NUMBER divisor = 1;
    int sign = initial_sign; 
    int not_zero = 1;

    
    BigSetInv(ctx->term, x);

    
    while (not_zero) {
        
        memcpy(ctx->temp, ctx->term, sizeof(NUMBER) * ARR_SIZE);
        not_zero = BigDiv(ctx->temp, divisor);

        
        if (factor != 1) {
            BigMulShort(ctx->temp, factor);
        }

        
        if (sign > 0) 
            BigAdd(result, ctx->temp);
        else 
            BigSub(result, ctx->temp);

        
        if (not_zero) {
            if (BigDiv(ctx->term, x2) == 0) break;
        }
        
        divisor += 2;
        sign = -sign;
    }
}


static int DoBenchmarkWork(BenchContext *ctx) {
    int loop_idx;
    
    
    for (loop_idx = 0; loop_idx < BENCH_LOOPS; loop_idx++) {
        
        Timer_Stop();
        if (Timer_GetElapsedMs() > BENCH_TIMEOUT_MS) {
            g_BenchTimedOut = 1;
            return loop_idx;
        }

        
        memset(ctx->pi, 0, sizeof(NUMBER) * ARR_SIZE);

        
        AddArcTan(ctx, ctx->pi, 5, 16, 1);
    
        
        AddArcTan(ctx, ctx->pi, 239, 4, -1);
    }
    return BENCH_LOOPS;
}


static double RunOnePassStatic(int *loops_out) {
    BenchContext ctx;
    int completed;
    
    
    ctx.pi = g_pi;
    ctx.term = g_term;
    ctx.temp = g_temp;

    g_BenchTimedOut = 0;
    Timer_Start();
    completed = DoBenchmarkWork(&ctx);
    Timer_Stop();

    if (loops_out) *loops_out = completed;
    return Timer_GetElapsedMs();
}


#define SAMPLES 5

int CompareDouble(const void * a, const void * b) {
    double da = *(double*)a;
    double db = *(double*)b;
    if (da < db) return -1;
    if (da > db) return 1;
    return 0;
}


DWORD RunSingleThreadBenchmark(BENCH_CALLBACK callback) {
    double times[SAMPLES];
    int loops_done[SAMPLES];
    int i;
    double total = 0;
    int total_loops = 0;

    Timer_Init();
    
    RunOnePassStatic(NULL);

    for (i = 0; i < SAMPLES; i++) {
        if (callback) callback(i * 100 / SAMPLES);
        times[i] = RunOnePassStatic(&loops_done[i]);
        
        
        if (loops_done[i] < BENCH_LOOPS) {
             
             double time = times[i];
             double score;
             if (time < 0.001) time = 0.001;
             
             score = ((double)loops_done[i] * (double)BENCH_SCORE_SCALE) / time;
             if (callback) callback(100);
             return (DWORD)score;
        }
    }
    if (callback) callback(100);

    
    qsort(times, SAMPLES, sizeof(double), CompareDouble);
    
    
    for (i = 1; i < SAMPLES - 1; i++) {
        total += times[i];
        total_loops += BENCH_LOOPS; 
    }
    
    {
        double avg_time = total / (SAMPLES - 2);
        if (avg_time < 0.001) avg_time = 0.001; 

        
        return (DWORD)(((double)BENCH_LOOPS * (double)BENCH_SCORE_SCALE) / avg_time);
    }
}





#ifdef _WIN32

typedef struct {
    int thread_id;
    int loops_completed;
    BenchContext ctx;
} ThreadParams;

DWORD WINAPI BenchThreadProc(LPVOID lpParam) {
    ThreadParams* params = (ThreadParams*)lpParam;
    
    
    
    
    params->ctx.pi = (NUMBER*)malloc(sizeof(NUMBER) * ARR_SIZE);
    params->ctx.term = (NUMBER*)malloc(sizeof(NUMBER) * ARR_SIZE);
    params->ctx.temp = (NUMBER*)malloc(sizeof(NUMBER) * ARR_SIZE);
    
    if (!params->ctx.pi || !params->ctx.term || !params->ctx.temp) {
        
        return 0;
    }
    
    params->loops_completed = DoBenchmarkWork(&params->ctx);
    
    
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
    g_BenchTimedOut = 0;
    Timer_Start();

    
    for (i = 0; i < num_cores; i++) {
        params[i].thread_id = i;
        
        threads[i] = CreateThread(
            NULL,               
            0,                  
            BenchThreadProc,    
            &params[i],         
            0,                  
            &thread_ids[i]      
        );
        
        if (threads[i] == NULL) {
            
        }
    }
    
    
    if (callback) callback(50); 
    
    WaitForMultipleObjects(num_cores, threads, TRUE, INFINITE);
    
    Timer_Stop();
    
    
    for (i = 0; i < num_cores; i++) {
        CloseHandle(threads[i]);
        total_loops += params[i].loops_completed;
    }
    
    free(threads);
    free(params);
    free(thread_ids);
    
    if (callback) callback(100);
    
    
    
    total_time = Timer_GetElapsedMs();
    if (total_time < 0.001) total_time = 0.001; 
    
    
    {
        double score = ((double)total_loops * (double)BENCH_SCORE_SCALE) / total_time;
        return (DWORD)score;
    }
}

#else


DWORD RunMultiCoreBenchmark(BENCH_CALLBACK callback) {
    
    
    return RunSingleThreadBenchmark(callback);
}

#endif
