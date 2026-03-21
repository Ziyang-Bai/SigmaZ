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
void GetCPUSignatureString(char* sig) {
    if (!sig) return;
    lstrcpy(sig, "");

#ifdef _WIN32
    {
        DWORD eax_v = 0;
        int family, model, stepping;
        char arch[64];
        char vendor[13];

        _asm {
            mov eax, 1
            cpuid
            mov eax_v, eax
        }
        
        GetCPUVendor(vendor);

        stepping = eax_v & 0xF;
        model = (eax_v >> 4) & 0xF;
        family = (eax_v >> 8) & 0xF;

        if (family == 0xF || family == 0x6) {
            model += ((eax_v >> 16) & 0xF) << 4;
        }
        if (family == 0xF) {
            family += (eax_v >> 20) & 0xFF;
        }

        lstrcpy(arch, "");

        /* Basic heuristic for some popular archs */
        if (lstrcmp(vendor, "GenuineIntel") == 0) {
            if (family == 6) {
                if (model == 0x5E || model == 0x4E) lstrcpy(arch, "Skylake ");
                else if (model == 0x8E || model == 0x9E) lstrcpy(arch, "Kaby Lake ");
                else if (model == 0xA5 || model == 0xA6) lstrcpy(arch, "Comet Lake ");
                else if (model == 0x97 || model == 0x9A) lstrcpy(arch, "Alder Lake ");
                else if (model == 0xB7 || model == 0xBA) lstrcpy(arch, "Raptor Lake ");
                else if (model == 0x5C || model == 0x5F) lstrcpy(arch, "Goldmont ");
                else if (model == 0x7E || model == 0x7D) lstrcpy(arch, "Ice Lake ");
            }
        } else if (lstrcmp(vendor, "AuthenticAMD") == 0) {
            if (family == 0x17) {
                if (model >= 0x01 && model <= 0x0F) lstrcpy(arch, "Zen / Zen+ ");
                else if (model >= 0x31 && model <= 0x3F) lstrcpy(arch, "Zen 2 ");
                else if (model >= 0x71 && model <= 0x7F) lstrcpy(arch, "Zen 2 ");
            } else if (family == 0x19) {
                if (model >= 0x01 && model <= 0x0F) lstrcpy(arch, "Zen 3 ");
                else if (model >= 0x21 && model <= 0x2F) lstrcpy(arch, "Zen 3 ");
                else if (model >= 0x61 && model <= 0x6F) lstrcpy(arch, "Zen 4 ");
                else if (model >= 0x41 && model <= 0x4F) lstrcpy(arch, "Zen 4 ");
            }
        }

        sprintf(sig, "Family %x, Model %x, Stepping %x %s", family, model, stepping, (arch[0] != '\0') ? arch : "");
    }
#else
    lstrcpy(sig, "N/A (16-bit)");
#endif
}

void GetCPUCacheString(char* cacheStr) {
    if (!cacheStr) return;
    lstrcpy(cacheStr, "");

#ifdef _WIN32
    {
        /* Simple AMD L2/L3 check (Leaf 0x80000006) */
        char vendor[13];
        GetCPUVendor(vendor);

        if (lstrcmp(vendor, "AuthenticAMD") == 0) {
            DWORD ecx_v = 0, edx_v = 0;
            DWORD max_ext = 0;
            _asm {
                mov eax, 0x80000000
                cpuid
                mov max_ext, eax
            }
            if (max_ext >= 0x80000006) {
                _asm {
                    mov eax, 0x80000006
                    cpuid
                    mov ecx_v, ecx
                    mov edx_v, edx
                }
                sprintf(cacheStr, "L2: %d KB, L3: %d MB", (ecx_v >> 16) & 0xFFFF, ((edx_v >> 18) & 0x3FFF) / 2);
            } else {
                lstrcpy(cacheStr, "Unknown AMD Caches");
            }
        } 
        else if (lstrcmp(vendor, "GenuineIntel") == 0) {
            /* Intel Leaf 4 processing is complex. As a simplified approach for legacy compat, we query leaf 4. */
            DWORD max_leaf = 0;
            _asm {
                mov eax, 0
                cpuid
                mov max_leaf, eax
            }
            if (max_leaf >= 4) {
                 /* Read L1/L2/L3 by iterating leaf 4 */
                 int cacheLevel, sizeKB;
                 DWORD eax_v, ebx_v, ecx_v;
                 int ways, parts, lines, sets;
                 int c = 0;
                 char buf[64];
                 lstrcpy(cacheStr, "");
                 while (c < 4) {
                    _asm {
                        push ebx
                        mov eax, 4
                        mov ecx, c
                        cpuid
                        mov eax_v, eax
                        mov ebx_v, ebx
                        mov ecx_v, ecx
                        pop ebx
                    }
                    if ((eax_v & 0x1F) == 0) break; /* No more caches */
                    cacheLevel = (eax_v >> 5) & 0x7;
                    ways = ((ebx_v >> 22) & 0x3FF) + 1;
                    parts = ((ebx_v >> 12) & 0x3FF) + 1;
                    lines = (ebx_v & 0xFFF) + 1;
                    sets = ecx_v + 1;
                    sizeKB = (ways * parts * lines * sets) / 1024;
                    sprintf(buf, "L%d:%dKB ", cacheLevel, sizeKB);
                    lstrcat(cacheStr, buf);
                    c++;
                 }
            } else {
                 lstrcpy(cacheStr, "Unknown Intel Caches");
            }
        } else {
            lstrcpy(cacheStr, "Unknown Cache info");
        }
    }
#else
    lstrcpy(cacheStr, "N/A (16-bit)");
#endif
}
#ifdef _WIN32
typedef struct RawSMBIOSData_s {
    BYTE Used20CallingMethod;
    BYTE SMBIOSMajorVersion;
    BYTE SMBIOSMinorVersion;
    BYTE DmiRevision;
    DWORD Length;
    BYTE SMBIOSTableData[1];
} RawSMBIOSData;

typedef struct SMBIOSHeader_s {
    BYTE Type;
    BYTE Length;
    WORD Handle;
} SMBIOSHeader;

const char* GetSMBIOSString(SMBIOSHeader* header, BYTE index) {
    const char* str = (const char*)header + header->Length;
    BYTE i;
    if (index == 0) return "";
    for (i = 1; i < index && *str; i++) {
        str += lstrlen(str) + 1;
    }
    return str;
}

typedef UINT (WINAPI * GETSYSTEMFIRMWARETABLE)(DWORD, DWORD, PVOID, DWORD);
#endif

void GetMotherboardInfo(char* outBuf) {
    if (!outBuf) return;
    lstrcpy(outBuf, "N/A");
#ifdef _WIN32
    {
        HMODULE hKernel32 = GetModuleHandle("kernel32.dll");
        GETSYSTEMFIRMWARETABLE pGetSystemFirmwareTable = NULL;
        if (hKernel32) {
            pGetSystemFirmwareTable = (GETSYSTEMFIRMWARETABLE)GetProcAddress(hKernel32, "GetSystemFirmwareTable");
        }
        if (pGetSystemFirmwareTable) {
            DWORD size = 0;
            DWORD signature = 'RSMB';
            size = pGetSystemFirmwareTable(signature, 0, NULL, 0);
            if (size > 0) {
                RawSMBIOSData* smbiosData = (RawSMBIOSData*)GlobalAlloc(GPTR, size);
                if (smbiosData) {
                    BYTE* p;
                    BYTE* end;
                    
                    pGetSystemFirmwareTable(signature, 0, smbiosData, size);
                    
                    p = smbiosData->SMBIOSTableData;
                    end = p + smbiosData->Length;
                    
                    while (p < end) {
                        SMBIOSHeader* header = (SMBIOSHeader*)p;
                        if (header->Type == 2) { 
                            const char* manufacturer = GetSMBIOSString(header, *((BYTE*)header + 4));
                            const char* product = GetSMBIOSString(header, *((BYTE*)header + 5));
                            sprintf(outBuf, "%s %s", manufacturer, product);
                            break;
                        }
                        p += header->Length;
                        while (*p || *(p + 1)) {
                            p++;
                        }
                        p += 2;
                    }
                    GlobalFree(smbiosData);
                }
            }
        }
    }
#endif
}

void GetMemoryInfo(char* outBuf) {
    if (!outBuf) return;
    lstrcpy(outBuf, "N/A");
#ifdef _WIN32
    {
        HMODULE hKernel32 = GetModuleHandle("kernel32.dll");
        GETSYSTEMFIRMWARETABLE pGetSystemFirmwareTable = NULL;
        if (hKernel32) {
            pGetSystemFirmwareTable = (GETSYSTEMFIRMWARETABLE)GetProcAddress(hKernel32, "GetSystemFirmwareTable");
        }
        if (pGetSystemFirmwareTable) {
            DWORD size = 0;
            DWORD signature = 'RSMB';
            size = pGetSystemFirmwareTable(signature, 0, NULL, 0);
            if (size > 0) {
                RawSMBIOSData* smbiosData = (RawSMBIOSData*)GlobalAlloc(GPTR, size);
                if (smbiosData) {
                    BYTE* p;
                    BYTE* end;
                    DWORD totalMem = 0;
                    DWORD speed = 0;
                    
                    pGetSystemFirmwareTable(signature, 0, smbiosData, size);
                    
                    p = smbiosData->SMBIOSTableData;
                    end = p + smbiosData->Length;
                    
                    while (p < end) {
                        SMBIOSHeader* header = (SMBIOSHeader*)p;
                        if (header->Type == 17) {
                            WORD memSize = *((WORD*)((BYTE*)header + 12));
                            WORD memSpeed = *((WORD*)((BYTE*)header + 21));
                            
                            if (memSize > 0 && memSize != 0xFFFF) {
                                if (memSize & 0x8000) {
                                    totalMem += (memSize & 0x7FFF) * 1024;
                                } else {
                                    totalMem += memSize;
                                }
                            }
                            if (memSpeed > 0 && memSpeed != 0xFFFF) {
                                 speed = memSpeed;
                            }
                        }
                        p += header->Length;
                        while (*p || *(p + 1)) {
                            p++;
                        }
                        p += 2;
                    }
                    
                    if (totalMem > 0) {
                         sprintf(outBuf, "%lu MB, %lu MHz", totalMem, speed);
                    }
                    GlobalFree(smbiosData);
                }
            }
        }
    }
#endif
}

void GetBIOSInfo(char* outBuf) {
    if (!outBuf) return;
    lstrcpy(outBuf, "N/A");
#ifdef _WIN32
    {
        HMODULE hKernel32 = GetModuleHandle("kernel32.dll");
        GETSYSTEMFIRMWARETABLE pGetSystemFirmwareTable = NULL;
        if (hKernel32) {
            pGetSystemFirmwareTable = (GETSYSTEMFIRMWARETABLE)GetProcAddress(hKernel32, "GetSystemFirmwareTable");
        }
        if (pGetSystemFirmwareTable) {
            DWORD size = 0;
            DWORD signature = 'RSMB';
            size = pGetSystemFirmwareTable(signature, 0, NULL, 0);
            if (size > 0) {
                RawSMBIOSData* smbiosData = (RawSMBIOSData*)GlobalAlloc(GPTR, size);
                if (smbiosData) {
                    BYTE* p;
                    BYTE* end;

                    pGetSystemFirmwareTable(signature, 0, smbiosData, size);

                    p = smbiosData->SMBIOSTableData;
                    end = p + smbiosData->Length;

                    while (p < end) {
                        SMBIOSHeader* header = (SMBIOSHeader*)p;
                        if (header->Type == 0) {
                            const char* vendor = GetSMBIOSString(header, *((BYTE*)header + 4));
                            const char* version = GetSMBIOSString(header, *((BYTE*)header + 5));
                            sprintf(outBuf, "%s %s", vendor, version);
                            break;
                        }
                        p += header->Length;
                        while (*p || *(p + 1)) {
                            p++;
                        }
                        p += 2;
                    }
                    GlobalFree(smbiosData);
                }
            }
        }
    }
#endif
}
