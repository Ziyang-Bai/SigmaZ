# Benchmark Definitions

SigmaZ includes six core benchmark tests designed to evaluate different computational units of the CPU and the memory subsystem.

### 1. Integer (Integer Arithmetic)

**Test Content:** Calculates high-precision values of Pi (π) to 2000 decimal places using Machin-like formulas.

**Algorithm Formula:**
```
pi/4 = 4 * arctan(1/5) - arctan(1/239)
```

**Technical Specifications:** 
-   **Processor Unit:** Arithmetic Logic Unit (ALU), Integer Pipeline.
-   **Multi-core Support:** Win32/Win64 versions support multi-threaded concurrent calculation.
-   **Implementation Details:** Uses a custom BigInt library (based on `long` arrays) to ensure consistent results across 16-bit and 32/64-bit platforms.
-   **Scale Parameters:** 2000-digit precision, 5-sample run with trimmed mean in single-thread mode, and a global timeout cap of 60s.

### 2. Float (Floating Point Arithmetic)

**Test Content:** Generates fractal image data for the Mandelbrot set using double-precision floating-point numbers.

**Algorithm Formula:**
```
z(n+1) = z(n)^2 + c
```

**Technical Specifications:** 
-   **Processor Unit:** Floating Point Unit (FPU).
-   **Hardware Detection:** Effectively reflects coprocessor (e.g., 80387) performance on early systems.
-   **Calculation Scale:** Win16 uses 320x240 resolution, Win32/64 uses 800x600 resolution, with a max iteration limit of 1000.
-   **Stop Condition:** Controlled by the global timeout mechanism; timed-out runs are scored by completed work.

### 3. Memory Ops (Memory Operations)

**Test Content:** Performs streaming read/write operations (Stream Copy) on large blocks of contiguous memory.                                                  

**Technical Specifications:**
-   **Test Target:** CPU's capability to process memory operations, RAM bandwidth, bus speed.
-   **Runtime Model:** Uses repeated memcpy/_fmemcpy over large buffers and reports MB/s in a fixed 3-second measurement window.
-   **Buffer Size:** Win32 uses 32 MB, Win64 uses 128 MB, Win16 uses 8 x 60,000-byte blocks.

### 4. Crypto (Encryption/Decryption)

**Test Content:** Executes CRC32 hash calculations using standard table lookup.

**Algorithm Formula:**
```
CRC(x) = M(x) * x^32 mod G(x)
```

**Technical Specifications:** 
-   **Test Target:** Bitwise operations (XOR, Shift) and random memory access (Table Lookup).
-   **Load Characteristics:** Tests both the ALU and the CPU's efficiency in handling logical operations and cache lookups.
-   **Workload Size:** 16 KB buffer, 100000 loops on Win32/64 and 500 loops on Win16, with a 60s timeout cap.

### 5. Compress (Data Compression)

**Test Content:** Runs a simplified version of the LZ77 (LZ77-Simp) lossless compression algorithm.

**Algorithm Mechanism:** Searches for the longest match of the current string in a history window (4KB).

**Technical Specifications:** 
-   **Test Target:** Branch prediction, complex control flow, non-sequential memory access.
-   **Load Characteristics:** Simulates actual file compression workloads. This test is highly sensitive to CPU L1/L2 cache hit rates.
-   **Workload Size:** 16 KB input buffer, 4 KB sliding window, max match length 18, fixed 250 loops with 60s timeout cap.

### 6. Matrix (Matrix Calculation)

**Test Content:** Executes dense matrix multiplication (C_ij = sum(A_ik * B_kj)) with O(N^3) complexity.

**Calculation Scale:**
-   Win16: 64x64 matrix
-   Win32/64: 64x64 matrix (All platforms compute 64x64 matrices. Win32/64 tests ALU throughput peaks through high-frequency repeated calculations.)

**Technical Specifications:** 
-   **Test Target:** CPU cache hierarchy (L1/L2/L3), floating-point throughput.
-   **Scoring Unit:** Matrices/s (completed 64x64 matrix multiplications per second).
-   **Optimization Potential:** On modern processors, this test can demonstrate optimizations from SIMD instruction sets (like SSE/AVX) via auto-vectorization (if supported by compiler and OS).
-   **Loop Strategy:** Win32/64 repeats 1000 matrix multiplications, Win16 runs 1 pass; both are protected by the 60s timeout cap.