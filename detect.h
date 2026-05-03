/*
 * SigmaZ
 * Copyright (c) 2026 Ziyang Bai
 * Licensed under the GNU General Public License v3.0
 */

#ifndef DETECT_H
#define DETECT_H

#include <windows.h>


void GetCPUVendor(char* vendor);


void GetPlatformMode(char* mode);
void GetSystemVersionString(char* ver_string);
void GetProcessBitnessString(char* bitness);


void GetCPUFeatures(char* features);


int GetCPUCount(void);


void GetCPUBrandString(char* brand);


void GetCPUSignatureString(char* sig);


void GetCPUCacheString(char* cacheStr);


void GetMotherboardInfo(char* outBuf);


void GetMemoryInfo(char* outBuf);


void GetBIOSInfo(char* outBuf);

#endif
