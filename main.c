/*
 * SigmaZ
 * Copyright (c) 2026 Ziyang Bai
 * Licensed under the GNU General Public License v3.0
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <direct.h>
#include "resource.h"
#include "detect.h"
#include "bench.h"

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

/* Global HWND for callback to update */
static HWND g_hDlg = NULL;
static HINSTANCE g_hInst = NULL; /* Global Instance Handle */
static int g_ProgID = 0; /* ID of the static text to update with progress */

/* Scores across tabs */
static unsigned long g_ScoreIntS = 0;
static unsigned long g_ScoreIntM = 0;
static unsigned long g_ScoreFloat = 0;
static unsigned long g_ScoreMem = 0;
static double g_ScoreCrypto = 0.0; /* KB/sec */
static double g_ScoreCompress = 0.0; /* KB/sec */
static double g_ScoreMatrix = 0.0; /* Matrices/sec */
static unsigned long g_Timeouts = 0;
static unsigned long g_LastTotalScore = 0;
static int g_ReportReady = 0;

/* SigmaZ binary report (.SZR) */
#pragma pack(push, 1)
typedef struct {
    char magic[4];
    unsigned long version;
    char cpu_vendor[16];
    char cpu_name[64];
    char os_mode[32];
    char sys_version[64];
    char cpu_features[64];
    unsigned long cores;
    char cpu_signature[64];
    char cpu_cache[64];
    char motherboard[128];
    char memory_info[128];
    char bios_info[128];
    float raw_scores[6];
    float norm_scores[6];
    unsigned long timeouts;
    unsigned long total_score;
    unsigned long timestamp;
    unsigned long checksum;
} SZR_Report;
#pragma pack(pop)

#define SZR_VERSION 0x00020001UL
#define SZR_XOR_KEY "SigmaZ95"

/* Reference Machine 486 DX2-66 Baseline Constants */
#define REF_INT_OPS   84.0       /* Approx int ops/sec on 486 */
#define REF_FLOAT_OPS 170.0      /* Approx Mandelbrot iters/ms on 486 */
#define REF_MEM_BW    24.0       /* Approx MB/s memcpy on 486 */
#define REF_CRYPTO_BW 189.0      /* Approx KB/s CRC32 on 486 */
#define REF_COMPRESS_BW 0.58     /* Approx KB/s LZ on 486 */
#define REF_MATRIX_OPS 0.91      /* Approx Matrices/sec on 486 */

/* Helper to Calculate Normalized Score (100 = 486 DX2-66) */
static double CalcScore(double val, double ref) {
    if (ref <= 0.0001) return 0.0;
    return (val / ref) * 100.0;
}

/* Helper to Calculate Weighted Geometric Mean */
static double CalcTotalScore(double s_int, double s_float, double s_mem, double s_crypto, double s_comp, double s_mat) {
    /* Avoid log(0) */
    if (s_int < 1.0) s_int = 1.0;
    if (s_float < 1.0) s_float = 1.0;
    if (s_mem < 1.0) s_mem = 1.0;
    if (s_crypto < 1.0) s_crypto = 1.0;
    if (s_comp < 1.0) s_comp = 1.0;
    if (s_mat < 1.0) s_mat = 1.0;
    
    /* 
     * Weights:
     * Integer: 20%
     * Float: 20%
     * Mem Ops: 10%
     * Crypto: 15%
     * Compress: 15%
     * Matrix: 20%
     */
    {
        double log_sum =
            log(s_int) * 0.20 +
            log(s_float) * 0.20 +
            log(s_mem) * 0.10 +
            log(s_crypto) * 0.15 +
            log(s_comp) * 0.15 +
            log(s_mat) * 0.20;

        return exp(log_sum);
    }
}
    /* Actually we can just use simple weighted arithmetic mean if math lib is missing, 
       but user specific Geometric Mean. 
       Let's try to assume pow() is available.
    */
    /* 
       pow(s_int, 0.3) * pow(s_float, 0.3) * pow(s_mem, 0.2) * pow(s_crypto, 0.2)
    */

/* Active Tab State */
#define TAB_CPU     0
#define TAB_INT     1
#define TAB_FLOAT   2
#define TAB_MEM     3
#define TAB_CRYPTO  4
#define TAB_COMP    5
#define TAB_MATRIX  6
#define TAB_ALL     7
#define TAB_ABOUT   8
#define TAB_REPORT  9

static int g_CurrentTab = TAB_CPU;

/* Helper to Show/Hide Controls */
static void ShowCtrl(HWND hwnd, int nID, int show) {
    ShowWindow(GetDlgItem(hwnd, nID), show ? SW_SHOW : SW_HIDE);
}

/* Helper to Show/Hide Groups of Controls */
void UpdateTabs(HWND hwnd) {
    /* CPU Tab */
    int s = (g_CurrentTab == TAB_CPU);
    ShowCtrl(hwnd, IDC_CPU_GRP, s);
    ShowCtrl(hwnd, IDC_CPU_REPORT, s);
    
    /* Integer Tab */
    s = (g_CurrentTab == TAB_INT);
    ShowCtrl(hwnd, IDC_INT_GRP, s);
    ShowCtrl(hwnd, IDC_INT_BTN_S, s);
    ShowCtrl(hwnd, IDC_INT_BTN_M, s);
    ShowCtrl(hwnd, IDC_INT_REPORT, s);
    ShowCtrl(hwnd, IDC_INT_PROG, s);
    
    /* Float Tab */
    s = (g_CurrentTab == TAB_FLOAT);
    ShowCtrl(hwnd, IDC_FLOAT_GRP, s);
    ShowCtrl(hwnd, IDC_FLOAT_BTN, s);
    ShowCtrl(hwnd, IDC_FLOAT_REPORT, s);
    ShowCtrl(hwnd, IDC_FLOAT_PROG, s);

    /* Memory Tab */
    s = (g_CurrentTab == TAB_MEM);
    ShowCtrl(hwnd, IDC_MEM_GRP, s);
    ShowCtrl(hwnd, IDC_MEM_BTN, s);
    ShowCtrl(hwnd, IDC_MEM_REPORT, s);
    ShowCtrl(hwnd, IDC_MEM_PROG, s);

    /* Crypto Tab */
    s = (g_CurrentTab == TAB_CRYPTO);
    ShowCtrl(hwnd, IDC_CRY_GRP, s);
    ShowCtrl(hwnd, IDC_CRY_BTN, s);
    ShowCtrl(hwnd, IDC_CRY_REPORT, s);
    ShowCtrl(hwnd, IDC_CRY_PROG, s);

    /* Compress Tab */
    s = (g_CurrentTab == TAB_COMP);
    ShowCtrl(hwnd, IDC_CMP_GRP, s);
    ShowCtrl(hwnd, IDC_CMP_BTN, s);
    ShowCtrl(hwnd, IDC_CMP_REPORT, s);
    ShowCtrl(hwnd, IDC_CMP_PROG, s);

    /* Matrix Tab */
    s = (g_CurrentTab == TAB_MATRIX);
    ShowCtrl(hwnd, IDC_MAT_GRP, s);
    ShowCtrl(hwnd, IDC_MAT_BTN, s);
    ShowCtrl(hwnd, IDC_MAT_REPORT, s);
    ShowCtrl(hwnd, IDC_MAT_PROG, s);

    /* Summary Tab */
    s = (g_CurrentTab == TAB_ALL);

    ShowCtrl(hwnd, IDC_ALL_GRP, s);
    ShowCtrl(hwnd, IDC_ALL_BTN, s);
    ShowCtrl(hwnd, IDC_ALL_REPORT, s);
    ShowCtrl(hwnd, IDC_ALL_PROG, s);

    /* About Tab */
    s = (g_CurrentTab == TAB_ABOUT);
    ShowCtrl(hwnd, IDC_ABOUT_GRP, s);
    ShowCtrl(hwnd, IDC_ABOUT_REPORT, s);

    /* Report Save Tab */
    s = (g_CurrentTab == TAB_REPORT);
    ShowCtrl(hwnd, IDC_RPT_GRP, s);
    ShowCtrl(hwnd, IDC_RPT_PATHLBL, s);
    ShowCtrl(hwnd, IDC_RPT_PATH, s);
    ShowCtrl(hwnd, IDC_RPT_HINT, s);
    ShowCtrl(hwnd, IDC_RPT_DIRLIST, s);
    ShowCtrl(hwnd, IDC_RPT_FILELBL, s);
    ShowCtrl(hwnd, IDC_RPT_FILE, s);
    ShowCtrl(hwnd, IDC_RPT_SAVE, s);
    ShowCtrl(hwnd, IDC_RPT_STATUS, s);
    
    /* Forces redraw of group boxes which might have artifacts */
    InvalidateRect(hwnd, NULL, TRUE);
}

/* Callback wrapper matching BENCH_CALLBACK signature (int percent) */
void BenchCallbackBridge(int percent) {
    char buf[16];
    char title[64];
    
    if (g_hDlg && g_ProgID) {
        /* Simple percent display */
        sprintf(buf, "%d%%", percent);
        SetDlgItemText(g_hDlg, g_ProgID, buf);
        
        /* Sync Title Bar */
        if (percent < 100) {
            sprintf(title, "SigmaZ - Running... %d%%", percent);
        } else {
            strcpy(title, "SigmaZ Bench");
        }
        SetWindowText(g_hDlg, title);
        
        UpdateWindow(g_hDlg); /* Force repaint */
        
        /* Pump UI messages to prevent "Not Responding" */
        {
            MSG msg;
            while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                if(!IsDialogMessage(g_hDlg, &msg)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }
    }
}

/* Helper to disable buttons during test */
void SetButtonsEnable(HWND hwnd, BOOL enable) {
    EnableWindow(GetDlgItem(hwnd, IDC_INT_BTN_S), enable);
    EnableWindow(GetDlgItem(hwnd, IDC_INT_BTN_M), enable);
    EnableWindow(GetDlgItem(hwnd, IDC_FLOAT_BTN), enable);
    EnableWindow(GetDlgItem(hwnd, IDC_MEM_BTN), enable);
    EnableWindow(GetDlgItem(hwnd, IDC_CRY_BTN), enable);
    EnableWindow(GetDlgItem(hwnd, IDC_CMP_BTN), enable);
    EnableWindow(GetDlgItem(hwnd, IDC_MAT_BTN), enable);
    
    EnableWindow(GetDlgItem(hwnd, IDC_ALL_BTN), enable);
    EnableWindow(GetDlgItem(hwnd, IDC_EXIT_BTN), enable);
    
    /* Disable tabs too to prevent switching during bench */
    EnableWindow(GetDlgItem(hwnd, IDC_TAB_CPU), enable);
    EnableWindow(GetDlgItem(hwnd, IDC_TAB_INT), enable);
    EnableWindow(GetDlgItem(hwnd, IDC_TAB_FLOAT), enable);
    EnableWindow(GetDlgItem(hwnd, IDC_TAB_MEM), enable);
    EnableWindow(GetDlgItem(hwnd, IDC_TAB_CRY), enable);
    EnableWindow(GetDlgItem(hwnd, IDC_TAB_CMP), enable);
    EnableWindow(GetDlgItem(hwnd, IDC_TAB_MAT), enable);
    EnableWindow(GetDlgItem(hwnd, IDC_TAB_ALL), enable);
    EnableWindow(GetDlgItem(hwnd, IDC_TAB_ABOUT), enable);
    EnableWindow(GetDlgItem(hwnd, IDC_TAB_RPT), enable);

    if (enable && g_ReportReady) {
        EnableWindow(GetDlgItem(hwnd, IDC_RPT_SAVE), TRUE);
    } else {
        EnableWindow(GetDlgItem(hwnd, IDC_RPT_SAVE), FALSE);
    }
}

/* Helper to set report text */
static void SetReport(HWND hwnd, int nID, const char* text) {
    SetDlgItemText(hwnd, nID, text);
}

static void SZR_XorData(unsigned char *data, size_t len) {
    const char *key = SZR_XOR_KEY;
    size_t i;
    for (i = 0; i < len; i++) {
        data[i] ^= key[i % 8];
    }
}

static unsigned long SZR_CalcChecksum(const SZR_Report *r) {
    const unsigned char *ptr = (const unsigned char *)r;
    unsigned long sum = 0x7A69676DL;
    int i;
    for (i = 0; i < (int)(sizeof(SZR_Report) - sizeof(unsigned long)); i++) {
        sum = (sum << 1) + ptr[i];
    }
    return sum;
}

static unsigned long ParseLeadingUL(const char* text) {
    unsigned long value = 0;
    int seen_digit = 0;
    while (*text) {
        if (*text >= '0' && *text <= '9') {
            seen_digit = 1;
            value = (value * 10UL) + (unsigned long)(*text - '0');
        } else if (seen_digit) {
            break;
        }
        text++;
    }
    return value;
}

static int SaveSZRFile(const char* filename, unsigned long total_score) {
    FILE *fp;
    SZR_Report report;
    SZR_Report encoded;
    char temp_buf[128];
    char mode_buf[32];
    char bitness_buf[16];

    memset(&report, 0, sizeof(report));
    report.magic[0] = 'S';
    report.magic[1] = 'Z';
    report.magic[2] = 'R';
    report.magic[3] = 0x1A;
    report.version = SZR_VERSION;

    GetCPUVendor(temp_buf);
    strncpy(report.cpu_vendor, temp_buf, sizeof(report.cpu_vendor) - 1);
    
    GetCPUBrandString(temp_buf);
    strncpy(report.cpu_name, temp_buf, sizeof(report.cpu_name) - 1);
    
    GetPlatformMode(mode_buf);
    GetProcessBitnessString(bitness_buf);
    sprintf(temp_buf, "%s %s", mode_buf, bitness_buf);
    strncpy(report.os_mode, temp_buf, sizeof(report.os_mode) - 1);

    GetSystemVersionString(temp_buf);
    strncpy(report.sys_version, temp_buf, sizeof(report.sys_version) - 1);

    GetCPUFeatures(temp_buf);
    strncpy(report.cpu_features, temp_buf, sizeof(report.cpu_features) - 1);

    report.cores = (unsigned long)GetCPUCount();
    
    GetCPUSignatureString(temp_buf);
    strncpy(report.cpu_signature, temp_buf, sizeof(report.cpu_signature) - 1);
    
    GetCPUCacheString(temp_buf);
    strncpy(report.cpu_cache, temp_buf, sizeof(report.cpu_cache) - 1);
    
    GetMotherboardInfo(temp_buf);
    strncpy(report.motherboard, temp_buf, sizeof(report.motherboard) - 1);
    
    GetMemoryInfo(temp_buf);
    strncpy(report.memory_info, temp_buf, sizeof(report.memory_info) - 1);
    
    GetBIOSInfo(temp_buf);
    strncpy(report.bios_info, temp_buf, sizeof(report.bios_info) - 1);

    report.raw_scores[0] = (float)g_ScoreIntM;
    report.raw_scores[1] = (float)g_ScoreFloat;
    report.raw_scores[2] = (float)g_ScoreMem;
    report.raw_scores[3] = (float)g_ScoreCrypto;
    report.raw_scores[4] = (float)g_ScoreCompress;
    report.raw_scores[5] = (float)g_ScoreMatrix;

    report.norm_scores[0] = (float)CalcScore((double)g_ScoreIntM, REF_INT_OPS);
    report.norm_scores[1] = (float)CalcScore((double)g_ScoreFloat, REF_FLOAT_OPS);
    report.norm_scores[2] = (float)CalcScore((double)g_ScoreMem, REF_MEM_BW);
    report.norm_scores[3] = (float)CalcScore((double)g_ScoreCrypto, REF_CRYPTO_BW);
    report.norm_scores[4] = (float)CalcScore((double)g_ScoreCompress, REF_COMPRESS_BW);
    report.norm_scores[5] = (float)CalcScore((double)g_ScoreMatrix, REF_MATRIX_OPS);

    report.timeouts = g_Timeouts;
    
    report.total_score = total_score;
    report.timestamp = (unsigned long)time(NULL);

    report.checksum = SZR_CalcChecksum(&report);

    encoded = report;
    SZR_XorData((unsigned char *)&encoded, sizeof(encoded));

    fp = fopen(filename, "wb");
    if (!fp) return 0;

    if (fwrite(&encoded, sizeof(encoded), 1, fp) != 1) {
        fclose(fp);
        return 0;
    }

    fclose(fp);
    return 1;
}

static int HasExtension(const char* path) {
    const char *p = path + strlen(path);
    while (p > path) {
        p--;
        if (*p == '.') return 1;
        if (*p == '\\' || *p == '/' || *p == ':') break;
    }
    return 0;
}

static void RefreshReportDirList(HWND hwnd) {
    char spec[MAX_PATH];
    strcpy(spec, "*.*");
    DlgDirList(hwnd, spec, IDC_RPT_DIRLIST, IDC_RPT_PATH, DDL_DIRECTORY | DDL_DRIVES | DDL_EXCLUSIVE);
}

static void OnReportDirListSelect(HWND hwnd) {
    char selected[MAX_PATH];
    memset(selected, 0, sizeof(selected));

    if (DlgDirSelectEx(hwnd, selected, sizeof(selected), IDC_RPT_DIRLIST)) {
        if (strlen(selected) > 0) {
            chdir(selected);
            RefreshReportDirList(hwnd);
        }
    }
}

static void BuildReportOutputPath(HWND hwnd, char* out_path, size_t out_len) {
    char dir[MAX_PATH];
    char file[MAX_PATH];

    memset(dir, 0, sizeof(dir));
    memset(file, 0, sizeof(file));
    GetDlgItemText(hwnd, IDC_RPT_PATH, dir, sizeof(dir));
    GetDlgItemText(hwnd, IDC_RPT_FILE, file, sizeof(file));

    if (strlen(file) == 0) {
        strcpy(file, "SIGMAZ_REPORT.SZR");
    }

    strncpy(out_path, dir, out_len - 1);

    if (strlen(out_path) > 0) {
        char end = out_path[strlen(out_path) - 1];
        if (end != '\\' && end != '/' && end != ':') {
            strncat(out_path, "\\", out_len - strlen(out_path) - 1);
        }
    }

    strncat(out_path, file, out_len - strlen(out_path) - 1);

    if (!HasExtension(out_path)) {
        strncat(out_path, ".szr", out_len - strlen(out_path) - 1);
    }
}

static void SaveReportFromTab(HWND hwnd) {
    char final_name[MAX_PATH + 64];

    if (!g_ReportReady) {
        SetDlgItemText(hwnd, IDC_RPT_STATUS, "Run All first. Report data is not ready.");
        return;
    }

    memset(final_name, 0, sizeof(final_name));
    BuildReportOutputPath(hwnd, final_name, sizeof(final_name));

    if (SaveSZRFile(final_name, g_LastTotalScore)) {
        char status[256];
        sprintf(status, "Saved OK");
        SetDlgItemText(hwnd, IDC_RPT_STATUS, status);
    } else {
        SetDlgItemText(hwnd, IDC_RPT_STATUS, "Save failed");
    }
}

/* Helper to build CPU pane report text */
static void BuildCPUInfoReport(char* report_buf) {
    char vendor_buf[16];
    char brand_buf[65];
    char mode_buf[32];
    char bitness_buf[16];
    char sys_buf[64];
    char features_buf[64];
    char cores_buf[32];
    char sig_buf[128];
    char cache_buf[128];
    char mobo_buf[128];
    char mem_buf[128];
    char bios_buf[128];
    int num_cores;

    GetCPUVendor(vendor_buf);
    GetCPUBrandString(brand_buf);
    GetPlatformMode(mode_buf);
    GetProcessBitnessString(bitness_buf);
    GetSystemVersionString(sys_buf);
    GetCPUFeatures(features_buf);
    GetCPUSignatureString(sig_buf);
    GetCPUCacheString(cache_buf);
    GetMotherboardInfo(mobo_buf);
    GetMemoryInfo(mem_buf);
    GetBIOSInfo(bios_buf);
    num_cores = GetCPUCount();

#ifdef _WIN32
    sprintf(cores_buf, "%d (Logical)", num_cores);
#else
    sprintf(cores_buf, "1 (Win16 Limit)");
#endif

    sprintf(report_buf,
        "--- Processor Information ---\r\n\r\n"
        "Vendor:  %s\r\n"
        "Name:    %s\r\n"
        "Mode:    %s\r\n"
        "Process Bitness: %s\r\n"
        "Tech:    %s\r\n"
        "Cores:   %s\r\n"
        "Signature: %s\r\n"
        "Cache:   %s\r\n\r\n"
        "--- System & Hardware ---\r\n\r\n"
        "OS Version:  %s\r\n"
        "Motherboard: %s\r\n"
        "Memory:      %s\r\n"
        "BIOS:        %s",
        vendor_buf, brand_buf, mode_buf, bitness_buf, features_buf, cores_buf, sig_buf, cache_buf, sys_buf, mobo_buf, mem_buf, bios_buf);
}

/* Run Int Single */
void RunIntSingle(HWND hwnd) {
    char buf[1024];
    
    g_ProgID = IDC_INT_PROG;
    SetDlgItemText(hwnd, g_ProgID, "Running Single-Thread Integer...");
    SetButtonsEnable(hwnd, FALSE);
    
    /* Returns Normalized Score directly (Iterations per unit time) */
    g_ScoreIntS = RunSingleThreadBenchmark(BenchCallbackBridge);
    
    sprintf(buf, "--- Integer Performance Report ---\r\n\r\n"
                 "Mode: Single-Threaded\r\n"
                 "Score: %lu OPS\r\n"
                 "Normalized: %.2f%% (vs 486)",
                 g_ScoreIntS, CalcScore((double)g_ScoreIntS, REF_INT_OPS));
                 
    if (g_BenchTimedOut) {
        strcat(buf, "\r\n\r\n* Note: Test timed out. Score estimated by partial progress.");
    }
    
    SetReport(hwnd, IDC_INT_REPORT, buf);
    SetDlgItemText(hwnd, g_ProgID, "Completed.");
    SetButtonsEnable(hwnd, TRUE);
}

/* Run Int Multi */
void RunIntMulti(HWND hwnd) {
    char buf[1024];
    int cores = GetCPUCount();
    int any_timeout = 0;
    
    g_ProgID = IDC_INT_PROG;
    SetDlgItemText(hwnd, g_ProgID, "Running Multi-Thread Integer...");
    SetButtonsEnable(hwnd, FALSE);
    
    /* Returns Normalized Score directly */
    if (g_ScoreIntS == 0) {
        g_ScoreIntS = RunSingleThreadBenchmark(NULL);
        if (g_BenchTimedOut) any_timeout = 1;
    }
    g_ScoreIntM = RunMultiCoreBenchmark(BenchCallbackBridge);
    if (g_BenchTimedOut) any_timeout = 1;
    
    /* Efficiency */
    {
        double eff = 0.0;
        if (g_ScoreIntS > 0 && cores > 1) {
             eff = (double)g_ScoreIntM / (double)g_ScoreIntS;
        }
        
        sprintf(buf, "--- Integer Performance Report ---\r\n\r\n"
                     "Mode: Multi-Threaded (%d Cores)\r\n"
                     "Single Core Score: %lu OPS\r\n"
                     "Multi Core Score:  %lu OPS\r\n"
                     "Scaling Factor: %.2fx\r\n"
                     "Normalized: %.2f%% (vs 486)",
                     cores, g_ScoreIntS, g_ScoreIntM, eff,
                     CalcScore((double)g_ScoreIntM, REF_INT_OPS));
                     
        if (any_timeout) {
            strcat(buf, "\r\n\r\n* Note: Test timed out. Score estimated by partial progress.");
        }
                     
        SetReport(hwnd, IDC_INT_REPORT, buf);
    }
    
    SetDlgItemText(hwnd, g_ProgID, "Completed.");
    SetButtonsEnable(hwnd, TRUE);
}

/* Run Float */
void RunFloat(HWND hwnd) {
    char buf[1024];
    
    g_ProgID = IDC_FLOAT_PROG;
    SetDlgItemText(hwnd, g_ProgID, "Running Mandelbrot FPU...");
    SetButtonsEnable(hwnd, FALSE);
    
    g_ScoreFloat = RunFloatBenchmark(BenchCallbackBridge);

    sprintf(buf, "--- Float Performance Report ---\r\n\r\n"
                 "Algorithm: Mandelbrot Set (Double Precision)\r\n"
                 "Score: %lu Iterations/ms\r\n"
                 "Normalized: %.2f%% (vs 486)",
                 g_ScoreFloat, CalcScore((double)g_ScoreFloat, REF_FLOAT_OPS));

    if (g_BenchTimedOut) {
        strcat(buf, "\r\n\r\n* Note: Test timed out. Score estimated by partial progress.");
    }

    SetReport(hwnd, IDC_FLOAT_REPORT, buf);
    SetDlgItemText(hwnd, g_ProgID, "Completed.");
    SetButtonsEnable(hwnd, TRUE);
}

/* Run Mem */
void RunMem(HWND hwnd) {
    char buf[1024];
    
    g_ProgID = IDC_MEM_PROG;
    SetDlgItemText(hwnd, g_ProgID, "Running Memory Ops...");
    SetButtonsEnable(hwnd, FALSE);
    
    g_ScoreMem = RunMemoryBenchmark(BenchCallbackBridge);
    
    sprintf(buf, "--- Memory Ops Performance Report ---\r\n\r\n"
                 "Operation: RAM Bandwidth (Mix Read/Write)\r\n"
                 "Bandwidth: %lu MB/s\r\n"
                 "Normalized: %.2f%% (vs 486)",
                 g_ScoreMem, CalcScore((double)g_ScoreMem, REF_MEM_BW));
                 
    if (g_BenchTimedOut) {
        strcat(buf, "\r\n\r\n* Note: Test timed out. Score estimated by partial progress.");
    }
    
    SetReport(hwnd, IDC_MEM_REPORT, buf);
    SetDlgItemText(hwnd, g_ProgID, "Completed.");
    SetButtonsEnable(hwnd, TRUE);
}

/* Run Crypto */
void RunCrypto(HWND hwnd) {
    char buf[1024];
    
    g_ProgID = IDC_CRY_PROG;
    SetDlgItemText(hwnd, g_ProgID, "Running Cryptography...");
    SetButtonsEnable(hwnd, FALSE);
    
    g_ScoreCrypto = RunCryptoBenchmark(BenchCallbackBridge);

    sprintf(buf, "--- Cryptography Performance Report ---\r\n\r\n"
                 "Algorithm: CRC32 (Table Based)\r\n"
                 "Throughput: %.2f KB/s\r\n"
                 "Normalized: %.2f%% (vs 486)",
                 g_ScoreCrypto, CalcScore(g_ScoreCrypto, REF_CRYPTO_BW));

    if (g_BenchTimedOut) {
        strcat(buf, "\r\n\r\n* Note: Test timed out. Score estimated by partial progress.");
    }

    SetReport(hwnd, IDC_CRY_REPORT, buf);
    SetDlgItemText(hwnd, g_ProgID, "Completed.");
    SetButtonsEnable(hwnd, TRUE);
}

/* Run Compress */
void RunCompress(HWND hwnd) {
    char buf[1024];
    
    g_ProgID = IDC_CMP_PROG;
    SetDlgItemText(hwnd, g_ProgID, "Running Compression...");
    SetButtonsEnable(hwnd, FALSE);
    
    g_ScoreCompress = RunCompressionBenchmark(BenchCallbackBridge);

    sprintf(buf, "--- Compression Performance Report ---\r\n\r\n"
                 "Algorithm: LZ77 (Sliding Window)\r\n"
                 "Throughput: %.2f KB/s\r\n"
                 "Normalized: %.2f%% (vs 486)",
                 g_ScoreCompress, CalcScore(g_ScoreCompress, REF_COMPRESS_BW));

    if (g_BenchTimedOut) {
        strcat(buf, "\r\n\r\n* Note: Test timed out. Score estimated by partial progress.");
    }

    SetReport(hwnd, IDC_CMP_REPORT, buf);
    SetDlgItemText(hwnd, g_ProgID, "Completed.");
    SetButtonsEnable(hwnd, TRUE);
}

/* Run Matrix */
void RunMatrix(HWND hwnd) {
    char buf[1024];
    
    g_ProgID = IDC_MAT_PROG;
    SetDlgItemText(hwnd, g_ProgID, "Running Matrix Math...");
    SetButtonsEnable(hwnd, FALSE);
    
    g_ScoreMatrix = RunMatrixBenchmark(BenchCallbackBridge);
    
    sprintf(buf, "--- Matrix Math Report ---\r\n\r\n"
                 "Algorithm: O(N^3) Matrix Multiplication\r\n"
                   "Throughput: %.2f Matrices/s\r\n"
                   "Normalized: %.2f%% (vs 486)",
                 g_ScoreMatrix, CalcScore(g_ScoreMatrix, REF_MATRIX_OPS));

    if (g_BenchTimedOut) {
        strcat(buf, "\r\n\r\n* Note: Test timed out. Score estimated by partial progress.");
    }
    SetReport(hwnd, IDC_MAT_REPORT, buf);
    SetDlgItemText(hwnd, g_ProgID, "Completed.");
    SetButtonsEnable(hwnd, TRUE);
}


/* Run ALL */
void RunAll(HWND hwnd) {
    static char buf[4096];
    char cpu_info[1024];
    char line[256];
    char msg[128];
    double n_int, n_float, n_mem, n_crypto, n_comp, n_mat, total;
    unsigned long total_u32;
    int to_int = 0, to_float = 0, to_mem = 0, to_crypto = 0, to_comp = 0, to_mat = 0;
    int any_timeout = 0;

    g_ProgID = IDC_ALL_PROG;
    g_ReportReady = 0;
    SetButtonsEnable(hwnd, FALSE);
    
    /* Clear Report */
    SetReport(hwnd, IDC_ALL_REPORT, "Running Comprehensive Benchmark Suite...\r\nPlease Wait.");
    g_Timeouts = 0;

    /* 1. Int */
    SetDlgItemText(hwnd, g_ProgID, "Test 1/6: Integer...");
    if (GetCPUCount() > 1) {
         g_ScoreIntM = RunMultiCoreBenchmark(BenchCallbackBridge);
    } else {
         g_ScoreIntM = RunSingleThreadBenchmark(BenchCallbackBridge);
    }
    if (g_BenchTimedOut) { to_int = 1; any_timeout = 1; g_Timeouts |= 1; }

    /* 2. Float */
    SetDlgItemText(hwnd, g_ProgID, "Test 2/6: Float FPU...");
    g_ScoreFloat = RunFloatBenchmark(BenchCallbackBridge);
    if (g_BenchTimedOut) { to_float = 1; any_timeout = 1; g_Timeouts |= 2; }

    /* 3. Mem */
    SetDlgItemText(hwnd, g_ProgID, "Test 3/6: Memory Ops...");
    g_ScoreMem = RunMemoryBenchmark(BenchCallbackBridge);
    if (g_BenchTimedOut) { to_mem = 1; any_timeout = 1; g_Timeouts |= 4; }

    /* 4. Crypto */
    SetDlgItemText(hwnd, g_ProgID, "Test 4/6: Crypto...");
    g_ScoreCrypto = RunCryptoBenchmark(BenchCallbackBridge);
    if (g_BenchTimedOut) { to_crypto = 1; any_timeout = 1; g_Timeouts |= 8; }

    /* 5. Compress */
    SetDlgItemText(hwnd, g_ProgID, "Test 5/6: Compression...");
    g_ScoreCompress = RunCompressionBenchmark(BenchCallbackBridge);
    if (g_BenchTimedOut) { to_comp = 1; any_timeout = 1; g_Timeouts |= 16; }

    /* 6. Matrix */
    SetDlgItemText(hwnd, g_ProgID, "Test 6/6: Matrix Math...");
    g_ScoreMatrix = RunMatrixBenchmark(BenchCallbackBridge);
    if (g_BenchTimedOut) { to_mat = 1; any_timeout = 1; g_Timeouts |= 32; }
    n_int = CalcScore((double)g_ScoreIntM, REF_INT_OPS);
    n_float = CalcScore((double)g_ScoreFloat, REF_FLOAT_OPS);
    n_mem = CalcScore((double)g_ScoreMem, REF_MEM_BW);
    n_crypto = CalcScore(g_ScoreCrypto, REF_CRYPTO_BW);
    n_comp = CalcScore(g_ScoreCompress, REF_COMPRESS_BW);
    n_mat = CalcScore(g_ScoreMatrix, REF_MATRIX_OPS);

    total = CalcTotalScore(n_int, n_float, n_mem, n_crypto, n_comp, n_mat);
    total_u32 = (unsigned long)(total + 0.5);
    g_LastTotalScore = total_u32;
    g_ReportReady = 1;

    /* Generate Report */
    sprintf(buf, "--- SigmaZ Comprehensive Report ---\r\n\r\n");
    BuildCPUInfoReport(cpu_info);
    strcat(buf, cpu_info);
    strcat(buf, "\r\n\r\n");

    if (to_int) {
        sprintf(line, "*Integer:   %lu OPS  (Norm: %.1f) (Raw: %lu, Ref: %.1f)\r\n", g_ScoreIntM, n_int, g_ScoreIntM, (double)REF_INT_OPS);
    } else {
        sprintf(line, "Integer:   %lu OPS  (Norm: %.1f)\r\n", g_ScoreIntM, n_int);
    }
    strcat(buf, line);

    if (to_float) {
        sprintf(line, "*Float:     %lu It/ms (Norm: %.1f) (Raw: %lu, Ref: %.1f)\r\n", g_ScoreFloat, n_float, g_ScoreFloat, (double)REF_FLOAT_OPS);
    } else {
        sprintf(line, "Float:     %lu It/ms (Norm: %.1f)\r\n", g_ScoreFloat, n_float);
    }
    strcat(buf, line);

    if (to_mem) {
        sprintf(line, "*Mem Ops:   %lu MB/s (Norm: %.1f) (Raw: %lu, Ref: %.1f)\r\n", g_ScoreMem, n_mem, g_ScoreMem, (double)REF_MEM_BW);
    } else {
        sprintf(line, "Mem Ops:   %lu MB/s (Norm: %.1f)\r\n", g_ScoreMem, n_mem);
    }
    strcat(buf, line);

    if (to_crypto) {
        sprintf(line, "*Crypto:    %.0f KB/s (Norm: %.1f) (Raw: %.0f, Ref: %.1f)\r\n", g_ScoreCrypto, n_crypto, g_ScoreCrypto, (double)REF_CRYPTO_BW);
    } else {
        sprintf(line, "Crypto:    %.0f KB/s (Norm: %.1f)\r\n", g_ScoreCrypto, n_crypto);
    }
    strcat(buf, line);

    if (to_comp) {
        sprintf(line, "*Compress:  %.0f KB/s (Norm: %.1f) (Raw: %.0f, Ref: %.1f)\r\n", g_ScoreCompress, n_comp, g_ScoreCompress, (double)REF_COMPRESS_BW);
    } else {
        sprintf(line, "Compress:  %.0f KB/s (Norm: %.1f)\r\n", g_ScoreCompress, n_comp);
    }
    strcat(buf, line);

    if (to_mat) {
        sprintf(line, "*Matrix:    %.2f Mat/s (Norm: %.1f) (Raw: %.2f, Ref: %.4f)\r\n", g_ScoreMatrix, n_mat, g_ScoreMatrix, (double)REF_MATRIX_OPS);
    } else {
        sprintf(line, "Matrix:    %.2f Mat/s (Norm: %.1f)\r\n", g_ScoreMatrix, n_mat);
    }
    strcat(buf, line);

      strcat(buf, "\r\n=============================\r\n");
      sprintf(line, "SIGMAZ SCORE: %.0f\r\n", total);
      strcat(buf, line);
    strcat(buf, "(Baseline: 486 DX2-66 = 100)");

    if (any_timeout) {
        strcat(buf, "\r\n\r\n* Note: One or more tests timed out.\r\n  Scores were estimated by partial progress.");
    }

    SetReport(hwnd, IDC_ALL_REPORT, buf);
    SetDlgItemText(hwnd, IDC_RPT_STATUS, "Report ready. Switch to Rpt tab and click Save.");
    
    sprintf(msg, "SigmaZ Score: %.0f", total);
    MessageBox(hwnd, msg, "Benchmark Complete", MB_OK | MB_ICONINFORMATION);
    
    SetDlgItemText(hwnd, g_ProgID, "All Tests Completed.");
    SetButtonsEnable(hwnd, TRUE);
}

/* Dialog Procedure to handle messages */
BOOL CALLBACK MainDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_INITDIALOG:
        g_hDlg = hwnd;

        /* --- CPU INFO TAB --- */
        {
            char report_buf[1024];
            BuildCPUInfoReport(report_buf);
            SetDlgItemText(hwnd, IDC_CPU_REPORT, report_buf);
        }

        /* --- ABOUT TAB --- */
        SetDlgItemText(hwnd, IDC_ABOUT_REPORT,
            "SigmaZ Benchmark v1.1\r\n\r\n"
            "Workloads:\r\n"
            "- Integer: Machin-Pi (Single/Multi)\r\n"
            "- Float: Mandelbrot Set (Double Precision)\r\n"
            "- Mem Ops: Stream-like Bandwidth\r\n"
            "- Crypto: CRC32 Throughput\r\n"
            "- Compress: LZ77 Sliding Window\r\n"
            "- Matrix: Basic O(N^3) Multiplication\r\n\r\n"
            "Scoring:\r\n"
            "- Baseline: Intel 486 DX2-66 = 100\r\n"
            "- Total score: weighted geometric mean\r\n\r\n"
            "Timeout:\r\n"
              "- Int/Float/Crypto/Compress/Matrix: 60s cap\r\n"
            "- Mem Ops: fixed duration bandwidth window\r\n\r\n"
            "Open Source Project. Licensed under the GNU General Public License v3.0\r\n"
            "Copyright (c) Ziyang Bai 2026.");
        
        /* --- BENCH LABELS --- */
        SetDlgItemText(hwnd, IDC_INT_REPORT,  "---");
        SetDlgItemText(hwnd, IDC_FLOAT_REPORT,"---");
        SetDlgItemText(hwnd, IDC_MEM_REPORT,  "---");
        SetDlgItemText(hwnd, IDC_CRY_REPORT,  "---");
        SetDlgItemText(hwnd, IDC_CMP_REPORT,  "---");
        SetDlgItemText(hwnd, IDC_MAT_REPORT,  "---");
        
        SetDlgItemText(hwnd, IDC_INT_PROG,   "Ready");
        SetDlgItemText(hwnd, IDC_FLOAT_PROG, "Ready");
        SetDlgItemText(hwnd, IDC_MEM_PROG,   "Ready");
        SetDlgItemText(hwnd, IDC_CRY_PROG,   "Ready");
        SetDlgItemText(hwnd, IDC_CMP_PROG,   "Ready");
        SetDlgItemText(hwnd, IDC_MAT_PROG,   "Ready");
        SetDlgItemText(hwnd, IDC_ALL_PROG,   "Ready");
        SetDlgItemText(hwnd, IDC_RPT_FILE,   "SIGMAZ_REPORT.SZR");
        SetDlgItemText(hwnd, IDC_RPT_STATUS, "Run All first");
        RefreshReportDirList(hwnd);
        EnableWindow(GetDlgItem(hwnd, IDC_RPT_SAVE), FALSE);

        /* Initialize Tab State */
        g_CurrentTab = TAB_CPU;
        UpdateTabs(hwnd);
        
        /* Center window */
        {
            RECT rc;
            int screenX = GetSystemMetrics(SM_CXSCREEN);
            int screenY = GetSystemMetrics(SM_CYSCREEN);
            GetWindowRect(hwnd, &rc);
            SetWindowPos(hwnd, NULL, 
                (screenX - (rc.right - rc.left)) / 2,
                (screenY - (rc.bottom - rc.top)) / 2,
                0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }

        /* Show Startup Hint */
        {
            char hint[512];
            /* Load string from resource */
            if (LoadString(g_hInst, IDS_STARTUP_HINT, hint, sizeof(hint)) > 0) {
                /* Force window to appear before msgbox */
                ShowWindow(hwnd, SW_SHOW);
                UpdateWindow(hwnd);
                MessageBox(hwnd, hint, "SigmaZ Info", MB_OK | MB_ICONINFORMATION);
            }
        }

        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
            
        /* --- TAB SWITCHING --- */
        case IDC_TAB_CPU:
            g_CurrentTab = TAB_CPU; 
            UpdateTabs(hwnd);
            return TRUE;
        case IDC_TAB_INT:
            g_CurrentTab = TAB_INT; 
            UpdateTabs(hwnd);
            return TRUE;
        case IDC_TAB_FLOAT:
            g_CurrentTab = TAB_FLOAT; 
            UpdateTabs(hwnd);
            return TRUE;
        case IDC_TAB_MEM:
            g_CurrentTab = TAB_MEM; 
            UpdateTabs(hwnd);
            return TRUE;
        case IDC_TAB_CRY:
            g_CurrentTab = TAB_CRYPTO; 
            UpdateTabs(hwnd);
            return TRUE;
        case IDC_TAB_CMP:
            g_CurrentTab = TAB_COMP; 
            UpdateTabs(hwnd);
            return TRUE;
        case IDC_TAB_MAT:
            g_CurrentTab = TAB_MATRIX; 
            UpdateTabs(hwnd);
            return TRUE;
        case IDC_TAB_ALL:
            g_CurrentTab = TAB_ALL; 
            UpdateTabs(hwnd);
            return TRUE;
        case IDC_TAB_ABOUT:
            g_CurrentTab = TAB_ABOUT; 
            UpdateTabs(hwnd);
            return TRUE;
        case IDC_TAB_RPT:
            g_CurrentTab = TAB_REPORT;
            UpdateTabs(hwnd);
            return TRUE;

        /* --- BENCHMARKS --- */
        case IDC_INT_BTN_S:
            RunIntSingle(hwnd);
            return TRUE;
        case IDC_INT_BTN_M:
            RunIntMulti(hwnd);
            return TRUE;
        case IDC_FLOAT_BTN:
            RunFloat(hwnd);
            return TRUE;
        case IDC_MEM_BTN:
            RunMem(hwnd);
            return TRUE;
        case IDC_CRY_BTN:
            RunCrypto(hwnd);
            return TRUE;
        case IDC_CMP_BTN:
            RunCompress(hwnd);
            return TRUE;
        case IDC_MAT_BTN:
            RunMatrix(hwnd);
            return TRUE;
        case IDC_ALL_BTN:
            RunAll(hwnd);
            return TRUE;
        case IDC_RPT_SAVE:
            SaveReportFromTab(hwnd);
            return TRUE;
        case IDC_RPT_DIRLIST:
#ifdef _WIN32
            if (HIWORD(wParam) == LBN_DBLCLK || HIWORD(wParam) == LBN_SELCHANGE)
#else
            if (HIWORD(lParam) == LBN_DBLCLK || HIWORD(lParam) == LBN_SELCHANGE)
#endif
            {
                OnReportDirListSelect(hwnd);
            }
            return TRUE;

        case IDC_EXIT_BTN:
        case IDCANCEL:
            EndDialog(hwnd, 0);
            return TRUE;
        }
        break;
        
    case WM_CLOSE:
        EndDialog(hwnd, 0);
        return TRUE;
    }
    return FALSE;
}

/* Entry point */
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow) {
    g_hInst = hInstance;
    return DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN_DLG), NULL, (DLGPROC)MainDlgProc);
}
