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

### 2. Float (Floating Point Arithmetic)

**Test Content:** Generates fractal image data for the Mandelbrot set using double-precision floating-point numbers.

**Algorithm Formula:**
```
z(n+1) = z(n)^2 + c
```

**Technical Specifications:** 
-   **Processor Unit:** Floating Point Unit (FPU).
-   **Hardware Detection:** Affectively measures coprocessor (e.g., 80387) performance on early systems.
-   **Calculation Scale:** Win16 version uses 320x240 resolution, Win32/64 versions use 800x600 resolution.

### 3. Memory (Memory Throughput)

**Test Content:** Performs streaming read/write operations (Stream Copy) on large blocks of contiguous memory.

**Technical Specifications:** 
-   **Test Target:** RAM bandwidth, bus speed.

### 4. Crypto (Encryption/Decryption)

**Test Content:** Executes CRC32 hash calculations using standard table lookup.

**Algorithm Formula:**
```
CRC(x) = M(x) * x^32 mod G(x)
```

**Technical Specifications:** 
-   **Test Target:** Bitwise operations (XOR, Shift) and random memory access (Table Lookup).
-   **Load Characteristics:** Tests both the ALU and the CPU's efficiency in handling logical operations and cache lookups.

### 5. Compress (Data Compression)

**Test Content:** Runs a simplified version of the LZ77 (LZ77-Simp) lossless compression algorithm.

**Algorithm Mechanism:** Searches for the longest match of the current string in a history window (4KB).

**Technical Specifications:** 
-   **Test Target:** Branch prediction, complex control flow, non-sequential memory access.
-   **Load Characteristics:** Simulates actual file compression workloads. This test is highly sensitive to CPU L1/L2 cache hit rates.

### 6. Matrix (Matrix Calculation)

**Test Content:** Executes dense matrix multiplication (C_ij = sum(A_ik * B_kj)) with O(N^3) complexity.

**Calculation Scale:**
-   Win16: 64x64 matrix
-   Win32/64: 64x64 matrix (All platforms compute 64x64 matrices. Win32/64 tests ALU throughput peaks through high-frequency repeated calculations.)

**Technical Specifications:** 
-   **Test Target:** CPU cache hierarchy (L1/L2/L3), floating-point throughput.
-   **Optimization Potential:** On modern processors, this test can demonstrate optimizations from SIMD instruction sets (like SSE/AVX) via auto-vectorization (if supported by compiler and OS).