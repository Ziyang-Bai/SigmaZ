/*
 * SigmaZ
 * Copyright (c) 2026 Ziyang Bai
 * Licensed under the GNU General Public License v3.0
 */

#ifndef DETECT_H
#define DETECT_H

#include <windows.h>

/* Get CPU Vendor String (e.g. "GenuineIntel"). Buffer must be at least 13 bytes. */
void GetCPUVendor(char* vendor);

/* Get Platform Mode string */
void GetPlatformMode(char* mode);
void GetSystemVersionString(char* ver_string);
void GetProcessBitnessString(char* bitness);

/* Get CPU Features (MMX, SSE, etc). Buffer must be at least 64 bytes. */
void GetCPUFeatures(char* features);

/* Get number of logical processors (returns 1 on Win16) */
int GetCPUCount(void);

/* Get CPU Brand String (e.g. "Intel(R) Core(TM) i9-14900K"). Buffer must be 65 bytes. */
void GetCPUBrandString(char* brand);

/* Get CPU Signature and Architecture (Family, Model, Stepping). Buffer at least 128 bytes. */
void GetCPUSignatureString(char* sig);

/* Get CPU Cache Information (L1/L2/L3) based on CPUID. Buffer at least 128 bytes. */
void GetCPUCacheString(char* cacheStr);

/* Get Motherboard SMBIOS Information. Buffer at least 128 bytes. */
void GetMotherboardInfo(char* outBuf);

/* Get Physical Memory Information. Buffer at least 128 bytes. */
void GetMemoryInfo(char* outBuf);

#endif
