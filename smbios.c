#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <stdio.h>

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
