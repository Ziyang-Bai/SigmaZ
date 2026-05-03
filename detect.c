/*
 * SigmaZ
 * Copyright (c) 2026 Ziyang Bai
 * Licensed under the GNU General Public License v3.0
 */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN64
#include <intrin.h>
#endif
#include "detect.h"


void GetCPUVendor(char* vendor) {
    if (!vendor) return;
    
    lstrcpy(vendor, "Unknown");

#ifdef _WIN32
    {
        char buffer[13];
#ifdef _WIN64
        int cpuInfo[4];
        __cpuid(cpuInfo, 0);
        memcpy(buffer, &cpuInfo[1], 4);     
        memcpy(buffer+4, &cpuInfo[3], 4);   
        memcpy(buffer+8, &cpuInfo[2], 4);   
#else
        DWORD ebx_v = 0, edx_v = 0, ecx_v = 0;
        
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
#endif
        buffer[12] = '\0';
        lstrcpy(vendor, buffer);
    }
#else
    {
        int cpu_type = 0;
        _asm {
            
            push sp
            pop ax
            cmp ax, sp
            jne is_8086
            
            
            pushf
            pop ax
            or ax, 7000h
            push ax
            popf
            pushf
            pop ax
            and ax, 7000h
            jz is_286
            
            
            db 66h, 9Ch                
            db 66h, 58h                
            db 66h, 8Bh, 0D8h          
            db 66h, 35h, 00h, 00h, 04h, 00h 
            db 66h, 50h                
            db 66h, 9Dh                
            db 66h, 9Ch                
            db 66h, 58h                
            db 66h, 53h                
            db 66h, 9Dh                
            db 66h, 3Bh, 0C3h          
            je is_386
            
            mov cpu_type, 4
            jmp done
            
        is_8086:
            mov cpu_type, 0
            jmp done
        is_286:
            mov cpu_type, 2
            jmp done
        is_386:
            mov cpu_type, 3
            
        done:
        }
        
        switch(cpu_type) {
            case 0: lstrcpy(vendor, "Intel 8086/88"); break;
            case 2: lstrcpy(vendor, "Intel 286"); break;
            case 3: lstrcpy(vendor, "Intel 386"); break;
            case 4: lstrcpy(vendor, "Intel 486+"); break;
            default: lstrcpy(vendor, "x86 Compatible"); break;
        }
    }
#endif
}

void GetPlatformMode(char* mode) {
#ifdef _WIN32
    lstrcpy(mode, "[ Win32 / Modern Mode ]");
#else
    lstrcpy(mode, "[ Win16 / 386 Mode ]");
#endif
}

#ifdef _WIN32
typedef struct {
    DWORD dwOSVersionInfoSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformId;
    WCHAR szCSDVersion[128];
} MY_RTL_OSVERSIONINFOW;

typedef LONG (WINAPI *fnRtlGetVersion)(MY_RTL_OSVERSIONINFOW*);
#endif

void GetSystemVersionString(char* ver_string) {
    if (!ver_string) return;

#ifdef _WIN32
    {
        DWORD major = 0, minor = 0, build = 0;
        BOOL got_version = FALSE;
        HMODULE hNtdll = GetModuleHandle("ntdll.dll");
        
        if (hNtdll) {
            fnRtlGetVersion pRtlGetVersion = (fnRtlGetVersion)GetProcAddress(hNtdll, "RtlGetVersion");
            if (pRtlGetVersion) {
                MY_RTL_OSVERSIONINFOW rovi;
                rovi.dwOSVersionInfoSize = sizeof(rovi);
                if (pRtlGetVersion(&rovi) == 0) {
                    major = rovi.dwMajorVersion;
                    minor = rovi.dwMinorVersion;
                    build = rovi.dwBuildNumber;
                    got_version = TRUE;
                }
            }
        }

        if (!got_version) {
            OSVERSIONINFO osvi;
            osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)
#endif
            if (GetVersionEx(&osvi)) {
                major = osvi.dwMajorVersion;
                minor = osvi.dwMinorVersion;
                build = osvi.dwBuildNumber;
                got_version = TRUE;
            }
#ifdef _MSC_VER
#pragma warning(pop)
#endif
        }

        if (got_version) {
            const char* name = "Windows";
            if (major == 10 && minor == 0) {
                if (build >= 22000) name = "Windows 11";
                else name = "Windows 10";
            } else if (major == 6) {
                if (minor == 3) name = "Windows 8.1";
                else if (minor == 2) name = "Windows 8";
                else if (minor == 1) name = "Windows 7";
                else if (minor == 0) name = "Windows Vista";
            } else if (major == 5) {
                if (minor == 1) name = "Windows XP";
                else if (minor == 2) name = "Windows XP/2003";
                else if (minor == 0) name = "Windows 2000";
            } else if (major == 4) {
                if (build <= 1000) name = "Windows 95";
                else name = "Windows 98/NT4";
            }
            sprintf(ver_string, "%s %lu.%lu (Build %lu)", name, major, minor, build);
        } else {
            DWORD dwVersion = GetVersion();
            sprintf(ver_string, "Windows %u.%u", (UINT)LOBYTE(LOWORD(dwVersion)), (UINT)HIBYTE(LOWORD(dwVersion)));
        }
    }
#else
    {
        DWORD dwVersion = GetVersion();
        UINT uMajor = (UINT)LOBYTE(LOWORD(dwVersion));
        UINT uMinor = (UINT)HIBYTE(LOWORD(dwVersion));
        WORD dos_ver = HIWORD(dwVersion);
        sprintf(ver_string, "Windows %u.%u (DOS %u.%u)", uMajor, uMinor, (UINT)HIBYTE(dos_ver), (UINT)LOBYTE(dos_ver));
    }
#endif
}

void GetProcessBitnessString(char* bitness) {
    if (!bitness) return;
#if defined(_WIN64)
    lstrcpy(bitness, "64-bit");
#elif defined(_WIN32)
    lstrcpy(bitness, "32-bit");
#else
    lstrcpy(bitness, "16-bit");
#endif
}

void GetCPUFeatures(char* features) {
#ifdef _WIN32
#ifdef _WIN64
    int cpuInfo1[4] = {0}, cpuInfo7[4] = {0};
    DWORD edx_val = 0, ecx_val = 0, ebx_val_7 = 0;
    __cpuid(cpuInfo1, 1);
    edx_val = cpuInfo1[3];
    ecx_val = cpuInfo1[2];
    __cpuidex(cpuInfo7, 7, 0);
    ebx_val_7 = cpuInfo7[1];
#else
    DWORD edx_val = 0, ecx_val = 0, ebx_val_7 = 0;
    _asm {
        push ebx
        mov eax, 1
        cpuid
        mov edx_val, edx
        mov ecx_val, ecx
        pop ebx
        push ebx
        mov eax, 7
        mov ecx, 0
        cpuid
        mov ebx_val_7, ebx
        pop ebx
    }
#endif
    lstrcpy(features, "");
    if (edx_val & 0x00800000) lstrcat(features, "MMX ");
    if (edx_val & 0x02000000) lstrcat(features, "SSE ");
    if (edx_val & 0x04000000) lstrcat(features, "SSE2 ");
    if (ecx_val & 0x00000001) lstrcat(features, "SSE3 ");
    if (ecx_val & 0x00080000) lstrcat(features, "SSE4.1 ");
    if (ecx_val & 0x00100000) lstrcat(features, "SSE4.2 ");
    if (ecx_val & 0x10000000) lstrcat(features, "AVX ");
    if (ebx_val_7 & 0x00000020) lstrcat(features, "AVX2 ");
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
    return (int)si.dwNumberOfProcessors;
#else
    return 1;
#endif
}

void GetCPUBrandString(char* brand) {
    if (!brand) return;
    lstrcpy(brand, "");
#ifdef _WIN32
    {
        DWORD max_ext = 0;
        char buffer[65];
        char* p = buffer;
        DWORD eax_v = 0, ebx_v = 0, ecx_v = 0, edx_v = 0;
#ifdef _WIN64
        int cpuInfo[4];
        __cpuid(cpuInfo, 0x80000000);
        max_ext = cpuInfo[0];
#else
        _asm {
            mov eax, 0x80000000
            cpuid
            mov max_ext, eax
        }
#endif
        if (max_ext >= 0x80000004) {
            int i;
            for (i = 0; i < 3; i++) {
#ifdef _WIN64
                int cpuInfo[4];
                __cpuid(cpuInfo, 0x80000002 + i);
                eax_v = cpuInfo[0];
                ebx_v = cpuInfo[1];
                ecx_v = cpuInfo[2];
                edx_v = cpuInfo[3];
#else
                _asm {
                    push ebx
                    mov eax, 0x80000002
                    add eax, i
                    cpuid
                    mov eax_v, eax
                    mov ebx_v, ebx
                    mov ecx_v, ecx
                    mov edx_v, edx
                    pop ebx
                }
#endif
                memcpy(buffer + (i * 16), &eax_v, 4);
                memcpy(buffer + (i * 16) + 4, &ebx_v, 4);
                memcpy(buffer + (i * 16) + 8, &ecx_v, 4);
                memcpy(buffer + (i * 16) + 12, &edx_v, 4);
            }
            buffer[48] = '\0';
            while (*p == ' ') p++;
            lstrcpy(brand, p);
        } else {
            int family, model;
            DWORD eax_v_1 = 0;
            char vendor[13];
            GetCPUVendor(vendor);
#ifdef _WIN64
            int cpuInfo1[4];
            __cpuid(cpuInfo1, 1);
            eax_v_1 = cpuInfo1[0];
#else
            _asm {
                mov eax, 1
                cpuid
                mov eax_v_1, eax
            }
#endif
            family = (int)((eax_v_1 >> 8) & 0xF);
            model = (int)((eax_v_1 >> 4) & 0xF);
            if (family == 4) lstrcpy(brand, "Generic 486 Class");
            else if (family == 5) {
                if (lstrcmp(vendor, "AuthenticAMD") == 0) lstrcpy(brand, "AMD K5/K6 Class");
                else lstrcpy(brand, "Intel Pentium Class");
            } else if (family == 6) {
                if (lstrcmp(vendor, "AuthenticAMD") == 0) lstrcpy(brand, "AMD Athlon/Duron Class");
                else lstrcpy(brand, "Intel Pentium Pro/II/III Class");
            } else lstrcpy(brand, "Legacy x86 Processor");
        }
    }
#else
    GetCPUVendor(brand);
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
#ifdef _WIN64
        int cpuInfo[4];
        __cpuid(cpuInfo, 1);
        eax_v = cpuInfo[0];
#else
        _asm {
            mov eax, 1
            cpuid
            mov eax_v, eax
        }
#endif
        GetCPUVendor(vendor);
        stepping = (int)(eax_v & 0xF);
        model = (int)((eax_v >> 4) & 0xF);
        family = (int)((eax_v >> 8) & 0xF);
        if (family == 0xF || family == 0x6) model += ((int)((eax_v >> 16) & 0xF)) << 4;
        if (family == 0xF) family += (int)((eax_v >> 20) & 0xFF);
        lstrcpy(arch, "");
        if (lstrcmp(vendor, "GenuineIntel") == 0) {
            if (family == 6) {
                if (model == 0x5E || model == 0x4E) lstrcpy(arch, "Skylake");
                else if (model == 0x8E || model == 0x9E) lstrcpy(arch, "Kaby Lake");
                else if (model == 0xA5 || model == 0xA6) lstrcpy(arch, "Comet Lake");
                else if (model == 0x97 || model == 0x9A) lstrcpy(arch, "Alder Lake");
            }
        }
        sprintf(sig, "Family %x, Model %x, Stepping %x %s", family, model, stepping, arch);
    }
#else
    lstrcpy(sig, "N/A (16-bit)");
#endif
}

void GetCPUCacheString(char* cacheStr) {
    if (!cacheStr) return;
    lstrcpy(cacheStr, "N/A");
#ifdef _WIN32
    {
        char vendor[13];
        GetCPUVendor(vendor);
        if (lstrcmp(vendor, "AuthenticAMD") == 0) {
            DWORD ecx_v = 0, edx_v = 0, max_ext = 0;
#ifdef _WIN64
            int cpuInfo[4];
            __cpuid(cpuInfo, 0x80000000);
            max_ext = cpuInfo[0];
#else
            _asm {
                mov eax, 0x80000000
                cpuid
                mov max_ext, eax
            }
#endif
            if (max_ext >= 0x80000006) {
#ifdef _WIN64
                int cpuInfo[4];
                __cpuid(cpuInfo, 0x80000006);
                ecx_v = cpuInfo[2];
                edx_v = cpuInfo[3];
#else
                _asm {
                    mov eax, 0x80000006
                    cpuid
                    mov ecx_v, ecx
                    mov edx_v, edx
                }
#endif
                sprintf(cacheStr, "L2:%dKB, L3:%dMB", (int)((ecx_v >> 16) & 0xFFFF), (int)(((edx_v >> 18) & 0x3FFF) / 2));
            }
        } else if (lstrcmp(vendor, "GenuineIntel") == 0) {
            DWORD max_leaf = 0;
#ifdef _WIN64
            int cpuInfo[4];
            __cpuid(cpuInfo, 0);
            max_leaf = cpuInfo[0];
#else
            _asm {
                mov eax, 0
                cpuid
                mov max_leaf, eax
            }
#endif
            if (max_leaf >= 4) {
                int c = 0;
                lstrcpy(cacheStr, "");
                while (c < 4) {
                    DWORD eax_v = 0, ebx_v = 0, ecx_v = 0;
#ifdef _WIN64
                    int cpuInfo[4];
                    __cpuid(cpuInfo, 4);
                    eax_v = cpuInfo[0];
                    ebx_v = cpuInfo[1];
                    ecx_v = cpuInfo[2];
#else
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
#endif
                    if ((eax_v & 0x1F) == 0) break;
                    {
                        int level = (int)((eax_v >> 5) & 0x7);
                        int ways = (int)(((ebx_v >> 22) & 0x3FF) + 1);
                        int parts = (int)(((ebx_v >> 12) & 0x3FF) + 1);
                        int lines = (int)((ebx_v & 0xFFF) + 1);
                        int sets = (int)(ecx_v + 1);
                        char buf[32];
                        sprintf(buf, "L%d:%dKB ", level, (ways * parts * lines * sets) / 1024);
                        lstrcat(cacheStr, buf);
                    }
                    c++;
                }
            }
        }
    }
#endif
}

#ifdef _WIN32
typedef struct {
    BYTE Used20CallingMethod;
    BYTE SMBIOSMajorVersion;
    BYTE SMBIOSMinorVersion;
    BYTE DmiRevision;
    DWORD Length;
    BYTE SMBIOSTableData[1];
} RawSMBIOSData;

typedef struct {
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
        HMODULE hK32 = GetModuleHandle("kernel32.dll");
        GETSYSTEMFIRMWARETABLE pGSFT = (GETSYSTEMFIRMWARETABLE)GetProcAddress(hK32, "GetSystemFirmwareTable");
        if (pGSFT) {
            DWORD size = pGSFT('RSMB', 0, NULL, 0);
            if (size > 0) {
                RawSMBIOSData* data = (RawSMBIOSData*)GlobalAlloc(GPTR, size);
                if (data) {
                    BYTE* p;
                    BYTE* end;
                    pGSFT('RSMB', 0, data, size);
                    p = data->SMBIOSTableData;
                    end = p + data->Length;
                    while (p < end) {
                        SMBIOSHeader* h = (SMBIOSHeader*)p;
                        if (h->Type == 2) {
                            sprintf(outBuf, "%s %s", GetSMBIOSString(h, *((BYTE*)h + 4)), GetSMBIOSString(h, *((BYTE*)h + 5)));
                            break;
                        }
                        p += h->Length;
                        while (*p || *(p + 1)) p++;
                        p += 2;
                    }
                    GlobalFree(data);
                }
            }
        }
    }
#endif
}

#ifndef _WIN32
static void GetWin16Memory(char* outBuf) {
    unsigned int kb = 0;
    _asm {
        mov ah, 88h
        int 15h
        jc fail
        mov kb, ax
        jmp ok
    fail:
        mov kb, 0
    ok:
    }
    if (kb > 0) sprintf(outBuf, "%u MB", (kb / 1024U) + 1U);
    else lstrcpy(outBuf, "N/A");
}
#endif

void GetMemoryInfo(char* outBuf) {
    if (!outBuf) return;
    lstrcpy(outBuf, "N/A");
#ifdef _WIN32
    {
        int done = 0;
        HMODULE hK32 = GetModuleHandle("kernel32.dll");
        GETSYSTEMFIRMWARETABLE pGSFT = (GETSYSTEMFIRMWARETABLE)GetProcAddress(hK32, "GetSystemFirmwareTable");
        if (pGSFT) {
            DWORD size = pGSFT('RSMB', 0, NULL, 0);
            if (size > 0) {
                RawSMBIOSData* data = (RawSMBIOSData*)GlobalAlloc(GPTR, size);
                if (data) {
                    BYTE *p, *end;
                    DWORD total = 0, speed = 0;
                    pGSFT('RSMB', 0, data, size);
                    p = data->SMBIOSTableData;
                    end = p + data->Length;
                    while (p < end) {
                        SMBIOSHeader* h = (SMBIOSHeader*)p;
                        if (h->Type == 17) {
                            WORD mS = *((WORD*)((BYTE*)h + 12));
                            if (mS > 0 && mS != 0xFFFF) {
                                if (mS == 0x7FFF && h->Length >= 0x20) total += *((DWORD*)((BYTE*)h + 0x1C));
                                else if (mS & 0x8000) total += ((DWORD)(mS & 0x7FFF) + 1023) / 1024;
                                else total += (DWORD)mS;
                            }
                            if (h->Length > 21) speed = *((WORD*)((BYTE*)h + 21));
                        }
                        p += h->Length;
                        while (*p || *(p + 1)) p++;
                        p += 2;
                    }
                    if (total > 0) {
                        sprintf(outBuf, "%lu MB, %lu MHz", total, speed);
                        done = 1;
                    }
                    GlobalFree(data);
                }
            }
        }
        if (!done) {
            MEMORYSTATUS ms;
            ms.dwLength = sizeof(ms);
            GlobalMemoryStatus(&ms);
            sprintf(outBuf, "%lu MB", (unsigned long)(ms.dwTotalPhys / (1024UL * 1024UL)));
        }
    }
#else
    GetWin16Memory(outBuf);
#endif
}

void GetBIOSInfo(char* outBuf) {
    if (!outBuf) return;
    lstrcpy(outBuf, "N/A");
#ifdef _WIN32
    {
        HMODULE hK32 = GetModuleHandle("kernel32.dll");
        GETSYSTEMFIRMWARETABLE pGSFT = (GETSYSTEMFIRMWARETABLE)GetProcAddress(hK32, "GetSystemFirmwareTable");
        if (pGSFT) {
            DWORD size = pGSFT('RSMB', 0, NULL, 0);
            if (size > 0) {
                RawSMBIOSData* data = (RawSMBIOSData*)GlobalAlloc(GPTR, size);
                if (data) {
                    BYTE *p, *end;
                    pGSFT('RSMB', 0, data, size);
                    p = data->SMBIOSTableData;
                    end = p + data->Length;
                    while (p < end) {
                        SMBIOSHeader* h = (SMBIOSHeader*)p;
                        if (h->Type == 0) {
                            sprintf(outBuf, "%s %s", GetSMBIOSString(h, *((BYTE*)h + 4)), GetSMBIOSString(h, *((BYTE*)h + 5)));
                            break;
                        }
                        p += h->Length;
                        while (*p || *(p + 1)) p++;
                        p += 2;
                    }
                    GlobalFree(data);
                }
            }
        }
    }
#endif
}
