/*
 * SigmaZ
 * Copyright (c) 2026 Ziyang Bai
 * Licensed under the GNU General Public License v3.0
 */

#ifndef BENCH_H
#define BENCH_H

#include <windows.h>


#ifdef _WIN32
  #define BENCH_LOOPS 100
#else
  #define BENCH_LOOPS 1
#endif


#define BENCH_SCORE_SCALE 1000000UL


typedef void (*BENCH_CALLBACK)(int percent);


#define BENCH_TIMEOUT_MS 60000


extern int g_BenchTimedOut;


DWORD RunSingleThreadBenchmark(BENCH_CALLBACK callback);


DWORD RunMultiCoreBenchmark(BENCH_CALLBACK callback);


void GetSigmaLMPiResult(char* buffer);


DWORD RunFloatBenchmark(BENCH_CALLBACK callback);


DWORD RunMemoryBenchmark(BENCH_CALLBACK callback);


double RunCryptoBenchmark(BENCH_CALLBACK callback);


double RunCompressionBenchmark(BENCH_CALLBACK callback);


double RunMatrixBenchmark(BENCH_CALLBACK callback);

#endif
