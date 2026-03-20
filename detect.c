/*
 * SigmaZ
 * Copyright (c) 2026 Ziyang Bai
 * Licensed under the GNU General Public License v3.0
 */

#include <windows.h>
#include <stdio.h>
#include "detect.h"

/* 
 * Detect CPU Vendor String 
 * Uses CPUID instruction for 32-bit/64-bit systems.
 * For 16-bit Win16, returns a placeholder (CPUID requires 386+ and proper mode handling).
 */
void GetCPUVendor(char* vendor) {
    if (!vendor) return;
    
    lstrcpy(vendor, "Unknown");

#ifdef _WIN32
    {
        char buffer[13];
        DWORD ebx_v, edx_v, ecx_v;
        
        /* Inline Assembly for MSVC/Watcom (32-bit) */
        _asm {
            push ebx
            mov eax, 0
            cpuid
            mov ebx_v, ebx
            mov edx_v, edx
            mov ecx_v, ecx
            pop ebx
        }
        
        memcpy(buffer, &ebx_v, 4);
        memcpy(buffer+4, &edx_v, 4);
        memcpy(buffer+8, &ecx_v, 4);
        buffer[12] = '\0';
        lstrcpy(vendor, buffer);
    }
#else
    /* Win16 Implementation */
    lstrcpy(vendor, "Win16-CPU"); 
#endif
}

void GetPlatformMode(char* mode) {
#ifdef _WIN32
    lstrcpy(mode, "[ Win32 / Modern Mode ]");
#else
    lstrcpy(mode, "[ Win16 / 386 Mode ]");
#endif
}

void GetCPUFeatures(char* features) {
#ifdef _WIN32
    DWORD edx_val = 0;
    DWORD ecx_val = 0;
    DWORD ebx_val_7 = 0;
    
    _asm {
        push ebx
        mov eax, 1
        cpuid
        mov edx_val, edx
        mov ecx_val, ecx
        pop ebx
    }
    
    /* Check for Leaf 7 Support (if Max Leaf >= 7) */
    /* Assumed present on modern CPUs, but strictly should check leaf 0 first */
    
    _asm {
        push ebx
        mov eax, 7
        mov ecx, 0
        cpuid
        mov ebx_val_7, ebx
        pop ebx
    }
    
    lstrcpy(features, "");
    
    /* EDX Bit 23: MMX */
    if (edx_val & 0x00800000) lstrcat(features, "MMX ");
    
    /* EDX Bit 25: SSE */
    if (edx_val & 0x02000000) lstrcat(features, "SSE ");
    
    /* EDX Bit 26: SSE2 */
    if (edx_val & 0x04000000) lstrcat(features, "SSE2 ");
    
    /* ECX Bit 0: SSE3 */
    if (ecx_val & 0x00000001) lstrcat(features, "SSE3 ");
    
    /* ECX Bit 19: SSE4.1 */
    if (ecx_val & 0x00080000) lstrcat(features, "SSE4.1 ");
    
    /* ECX Bit 20: SSE4.2 */
    if (ecx_val & 0x00100000) lstrcat(features, "SSE4.2 ");
    
    /* ECX Bit 28: AVX */
    if (ecx_val & 0x10000000) lstrcat(features, "AVX ");
    
    /* EBX(Leaf 7) Bit 5: AVX2 */
    if (ebx_val_7 & 0x00000020) lstrcat(features, "AVX2 ");

    /* EBX(Leaf 7) Bit 16: AVX512F */
    if (ebx_val_7 & 0x00010000) lstrcat(features, "AVX512 ");
    
    if (lstrlen(features) == 0) lstrcpy(features, "Standard x86");
#else
    lstrcpy(features, "x87 FPU (Simulated)");
#endif
}

int GetCPUCount(void) {
#ifdef _WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwNumberOfProcessors;
#else
    return 1;
#endif
}

void GetCPUBrandString(char* brand) {
    if (!brand) return;
    lstrcpy(brand, "");

#ifdef _WIN32
    {
        /* Check extended CPUID support */
        DWORD max_ext = 0;
        char buffer[65];
        char* p = buffer;
        DWORD eax_v, ebx_v, ecx_v, edx_v;
        
        _asm {
            mov eax, 0x80000000
            cpuid
            mov max_ext, eax
        }
        
        if (max_ext >= 0x80000004) {
             /* Part 1: 80000002 */
             _asm {
                 push ebx
                 mov eax, 0x80000002
                 cpuid
                 mov eax_v, eax
                 mov ebx_v, ebx
                 mov ecx_v, ecx
                 mov edx_v, edx
                 pop ebx
             }
             memcpy(buffer, &eax_v, 4);
             memcpy(buffer+4, &ebx_v, 4);
             memcpy(buffer+8, &ecx_v, 4);
             memcpy(buffer+12, &edx_v, 4);

             /* Part 2: 80000003 */
             _asm {
                 push ebx
                 mov eax, 0x80000003
                 cpuid
                 mov eax_v, eax
                 mov ebx_v, ebx
                 mov ecx_v, ecx
                 mov edx_v, edx
                 pop ebx
             }
             memcpy(buffer+16, &eax_v, 4);
             memcpy(buffer+20, &ebx_v, 4);
             memcpy(buffer+24, &ecx_v, 4);
             memcpy(buffer+28, &edx_v, 4);
            
             /* Part 3: 80000004 */
             _asm {
                 push ebx
                 mov eax, 0x80000004
                 cpuid
                 mov eax_v, eax
                 mov ebx_v, ebx
                 mov ecx_v, ecx
                 mov edx_v, edx
                 pop ebx
             }
             memcpy(buffer+32, &eax_v, 4);
             memcpy(buffer+36, &ebx_v, 4);
             memcpy(buffer+40, &ecx_v, 4);
             memcpy(buffer+44, &edx_v, 4);

            buffer[48] = '\0'; /* Ensure termination */
            
            /* Trim leading spaces */
            while (*p == ' ') p++;
            lstrcpy(brand, p);
        } else {
            lstrcpy(brand, "Standard x86 Processor");
        }
    }
#else
    lstrcpy(brand, "Standard 386/486");
#endif
}
