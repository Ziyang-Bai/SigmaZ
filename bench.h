/*
 * SigmaZ
 * Copyright (c) 2026 Ziyang Bai
 * Licensed under the GNU General Public License v3.0
 */

#ifndef BENCH_H
#define BENCH_H

#include <windows.h>

/*
 * Benchmark loop count
 * Adjusted for platform speed differences to ensure measurable duration.
 * Win32 (Modern CPUs) requires more iterations to get significant timing.
 * Win16 (Vintage CPUs) requires fewer to avoid excessive wait times.
 */
#ifdef _WIN32
  #define BENCH_LOOPS 100
#else
  #define BENCH_LOOPS 1
#endif

/*
 * Score Scaling Factor
 * Score = (BaseWork * Scale) / Time
 * Allows integer math without losing precision.
 */
#define BENCH_SCORE_SCALE 1000000UL

/* Progress callback: percent range 0..100 */
typedef void (*BENCH_CALLBACK)(int percent);

/* 
 * Benchmark Timeout (ms)
 * If a test exceeds this time, it will be terminated and scored based on progress.
 * Useful for Very Old Hardware (e.g. 386/486) preventing freezes.
 */
#define BENCH_TIMEOUT_MS 60000

/* Flag to indicate if the last benchmark was terminated early due to timeout */
extern int g_BenchTimedOut;

/*
 * Run Integer Benchmark (Single Thread)
 * Returns SCORE (Iterations / Time normalized), not raw time.
 */
DWORD RunSingleThreadBenchmark(BENCH_CALLBACK callback);

/* 
 * Run Multi-Core Benchmark (Win32 Specific)
 * Returns SCORE (Total System Throughput normalized).
 */
DWORD RunMultiCoreBenchmark(BENCH_CALLBACK callback);

/* Format the PI result into a string buffer */
void GetSigmaLMPiResult(char* buffer);

/* FPU (Mandelbrot) Benchmark */
DWORD RunFloatBenchmark(BENCH_CALLBACK callback);

/* Memory (Stream Copy) Benchmark */
DWORD RunMemoryBenchmark(BENCH_CALLBACK callback);

/* Crypto Benchmark */
double RunCryptoBenchmark(BENCH_CALLBACK callback);

/* Compression Benchmark */
double RunCompressionBenchmark(BENCH_CALLBACK callback);

/* Matrix Benchmark */
double RunMatrixBenchmark(BENCH_CALLBACK callback);

#endif
