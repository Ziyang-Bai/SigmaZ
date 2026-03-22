#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define APP_TITLE "SigmaZ SZR Reader (GUI)"

#ifndef BST_CHECKED
#define BST_CHECKED 1
#endif

#ifndef MB_ICONERROR
#ifdef MB_ICONHAND
#define MB_ICONERROR MB_ICONHAND
#else
#define MB_ICONERROR 0
#endif
#endif

#define IDC_PATH_EDIT   1001
#define IDC_FORCE_CHK   1002
#define IDC_PARSE_BTN   1003
#define IDC_EXIT_BTN    1004
#define IDC_OUTPUT_EDIT 1005

#define SZR_SIZE 828
#define SZR_SALT 0x7A69676DUL

#define OFF_MAGIC      0
#define OFF_VERSION    4
#define OFF_VENDOR     8
#define OFF_CPU_NAME   24
#define OFF_OS_MODE    88
#define OFF_SYS_VER    120
#define OFF_FEATURES   184
#define OFF_CORES      248
#define OFF_SIGNATURE  252
#define OFF_CACHE      316
#define OFF_MOBO       380
#define OFF_MEMORY     508
#define OFF_BIOS       636
#define OFF_RAW_SCORE  764
#define OFF_NORM_SCORE 788
#define OFF_TIMEOUTS   812
#define OFF_TOTAL      816
#define OFF_TIMESTAMP  820
#define OFF_CHECKSUM   824

static const char g_key[8] = { 'S', 'i', 'g', 'm', 'a', 'Z', '9', '5' };

static unsigned long read_u32_le(const unsigned char* p) {
    return ((unsigned long)p[0]) |
           (((unsigned long)p[1]) << 8) |
           (((unsigned long)p[2]) << 16) |
           (((unsigned long)p[3]) << 24);
}

static float read_f32_le(const unsigned char* p) {
    union {
        unsigned long u;
        float f;
    } cv;
    cv.u = read_u32_le(p);
    return cv.f;
}

static void xor_data(unsigned char* data, unsigned int len) {
    unsigned int i;
    for (i = 0; i < len; i++) {
        data[i] = (unsigned char)(data[i] ^ (unsigned char)g_key[i % 8]);
    }
}

static unsigned long calc_checksum(const unsigned char* plain) {
    unsigned long sum = SZR_SALT;
    unsigned int i;
    for (i = 0; i < OFF_CHECKSUM; i++) {
        sum = (sum << 1) + (unsigned long)plain[i];
    }
    return sum;
}

static void read_cstr(char* out, unsigned int out_size, const unsigned char* src, unsigned int src_len) {
    unsigned int i;
    unsigned int n = (out_size > 0) ? (out_size - 1) : 0;
    if (n > src_len) n = src_len;
    for (i = 0; i < n; i++) {
        if (src[i] == '\0') break;
        out[i] = (char)src[i];
    }
    out[i] = '\0';
}

static void append_line(char* out, unsigned int cap, const char* text) {
    unsigned int cur = (unsigned int)strlen(out);
    if (cur >= cap - 1) return;
    strncat(out, text, cap - cur - 1);
    cur = (unsigned int)strlen(out);
    if (cur < cap - 2) {
        strncat(out, "\r\n", cap - cur - 1);
    }
}

static int parse_szr_to_text(const char* path, int force, char* out, unsigned int out_cap) {
    FILE* fp;
    unsigned char raw[1024];
    unsigned char data[SZR_SIZE];
    unsigned int read_n;

    unsigned long version;
    char cpu_vendor[17];
    char cpu_name[65];
    char os_mode[33];
    char sys_version[65];
    char cpu_features[65];
    unsigned long cores;
    char cpu_signature[65];
    char cpu_cache[65];
    char motherboard[129];
    char memory_info[129];
    char bios_info[129];
    float raw_scores[6];
    float norm_scores[6];
    unsigned long timeouts;
    unsigned long total_score;
    unsigned long timestamp;
    unsigned long file_checksum;
    unsigned long calc;
    int magic_ok;
    int checksum_ok;

    char line[256];
    time_t t;
    struct tm* tmv;
    char tbuf[64];

    out[0] = '\0';

    fp = fopen(path, "rb");
    if (!fp) {
        append_line(out, out_cap, "[ERROR] Cannot open file.");
        return 0;
    }

    read_n = (unsigned int)fread(raw, 1, sizeof(raw), fp);
    fclose(fp);

    if (!force && read_n != SZR_SIZE) {
        sprintf(line, "[ERROR] File size mismatch: expected %u, got %u", (unsigned int)SZR_SIZE, read_n);
        append_line(out, out_cap, line);
        return 0;
    }

    memset(data, 0, sizeof(data));
    if (read_n >= SZR_SIZE) {
        memcpy(data, raw, SZR_SIZE);
        if (force && read_n > SZR_SIZE) {
            sprintf(line, "[WARN] Force mode: file too long, using first %u bytes", (unsigned int)SZR_SIZE);
            append_line(out, out_cap, line);
        }
    } else {
        memcpy(data, raw, read_n);
        if (force) {
            append_line(out, out_cap, "[WARN] Force mode: file too short, padded with zeros");
        }
    }

    xor_data(data, SZR_SIZE);

    magic_ok = (data[OFF_MAGIC + 0] == 'S' && data[OFF_MAGIC + 1] == 'Z' &&
                data[OFF_MAGIC + 2] == 'R' && data[OFF_MAGIC + 3] == 0x1A);

    if (!magic_ok && !force) {
        append_line(out, out_cap, "[ERROR] Magic mismatch");
        return 0;
    }

    version = read_u32_le(data + OFF_VERSION);
    read_cstr(cpu_vendor, sizeof(cpu_vendor), data + OFF_VENDOR, 16);
    read_cstr(cpu_name, sizeof(cpu_name), data + OFF_CPU_NAME, 64);
    read_cstr(os_mode, sizeof(os_mode), data + OFF_OS_MODE, 32);
    read_cstr(sys_version, sizeof(sys_version), data + OFF_SYS_VER, 64);
    read_cstr(cpu_features, sizeof(cpu_features), data + OFF_FEATURES, 64);
    cores = read_u32_le(data + OFF_CORES);
    read_cstr(cpu_signature, sizeof(cpu_signature), data + OFF_SIGNATURE, 64);
    read_cstr(cpu_cache, sizeof(cpu_cache), data + OFF_CACHE, 64);
    read_cstr(motherboard, sizeof(motherboard), data + OFF_MOBO, 128);
    read_cstr(memory_info, sizeof(memory_info), data + OFF_MEMORY, 128);
    read_cstr(bios_info, sizeof(bios_info), data + OFF_BIOS, 128);

    raw_scores[0] = read_f32_le(data + OFF_RAW_SCORE + 0);
    raw_scores[1] = read_f32_le(data + OFF_RAW_SCORE + 4);
    raw_scores[2] = read_f32_le(data + OFF_RAW_SCORE + 8);
    raw_scores[3] = read_f32_le(data + OFF_RAW_SCORE + 12);
    raw_scores[4] = read_f32_le(data + OFF_RAW_SCORE + 16);
    raw_scores[5] = read_f32_le(data + OFF_RAW_SCORE + 20);

    norm_scores[0] = read_f32_le(data + OFF_NORM_SCORE + 0);
    norm_scores[1] = read_f32_le(data + OFF_NORM_SCORE + 4);
    norm_scores[2] = read_f32_le(data + OFF_NORM_SCORE + 8);
    norm_scores[3] = read_f32_le(data + OFF_NORM_SCORE + 12);
    norm_scores[4] = read_f32_le(data + OFF_NORM_SCORE + 16);
    norm_scores[5] = read_f32_le(data + OFF_NORM_SCORE + 20);

    timeouts = read_u32_le(data + OFF_TIMEOUTS);
    total_score = read_u32_le(data + OFF_TOTAL);
    timestamp = read_u32_le(data + OFF_TIMESTAMP);
    file_checksum = read_u32_le(data + OFF_CHECKSUM);

    calc = calc_checksum(data);
    checksum_ok = (calc == file_checksum);

    if (!checksum_ok && !force) {
        sprintf(line, "[ERROR] Checksum mismatch (file=0x%08lX, calc=0x%08lX)", file_checksum, calc);
        append_line(out, out_cap, line);
        return 0;
    }

    if (magic_ok && checksum_ok) {
        append_line(out, out_cap, "SigmaZ Report OK");
    } else {
        append_line(out, out_cap, "SigmaZ Report Parsed (FORCED)");
    }

    t = (time_t)timestamp;
    tmv = localtime(&t);
    if (tmv && strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", tmv) > 0) {
        sprintf(line, "Timestamp : %lu (%s)", timestamp, tbuf);
    } else {
        sprintf(line, "Timestamp : %lu", timestamp);
    }
    append_line(out, out_cap, line);

    sprintf(line, "Version   : 0x%08lX", version);
    append_line(out, out_cap, line);
    append_line(out, out_cap, "");

    append_line(out, out_cap, "--- Processor Information ---");
    sprintf(line, "Vendor    : %s", cpu_vendor);
    append_line(out, out_cap, line);
    sprintf(line, "Name      : %s", cpu_name);
    append_line(out, out_cap, line);
    sprintf(line, "Mode/Bitness: %s", os_mode);
    append_line(out, out_cap, line);
    sprintf(line, "OS Version: %s", sys_version);
    append_line(out, out_cap, line);
    sprintf(line, "Tech      : %s", cpu_features);
    append_line(out, out_cap, line);
    sprintf(line, "Cores     : %lu", cores);
    append_line(out, out_cap, line);
    sprintf(line, "Signature : %s", cpu_signature);
    append_line(out, out_cap, line);
    sprintf(line, "Cache     : %s", cpu_cache);
    append_line(out, out_cap, line);
    append_line(out, out_cap, "");

    append_line(out, out_cap, "--- Hardware Information ---");
    sprintf(line, "Motherboard: %s", motherboard);
    append_line(out, out_cap, line);
    sprintf(line, "Memory    : %s", memory_info);
    append_line(out, out_cap, line);
    sprintf(line, "BIOS      : %s", bios_info);
    append_line(out, out_cap, line);
    append_line(out, out_cap, "");

    append_line(out, out_cap, "--- Test Results ---");
    sprintf(line, "Integer   : Raw %.2f, Norm %.2f", raw_scores[0], norm_scores[0]);
    append_line(out, out_cap, line);
    sprintf(line, "Float     : Raw %.2f, Norm %.2f", raw_scores[1], norm_scores[1]);
    append_line(out, out_cap, line);
    sprintf(line, "MemOps    : Raw %.2f, Norm %.2f", raw_scores[2], norm_scores[2]);
    append_line(out, out_cap, line);
    sprintf(line, "Crypto    : Raw %.2f, Norm %.2f", raw_scores[3], norm_scores[3]);
    append_line(out, out_cap, line);
    sprintf(line, "Compress  : Raw %.2f, Norm %.2f", raw_scores[4], norm_scores[4]);
    append_line(out, out_cap, line);
    sprintf(line, "Matrix    : Raw %.2f, Norm %.2f", raw_scores[5], norm_scores[5]);
    append_line(out, out_cap, line);
    if (timeouts) {
        sprintf(line, "Timeouts  : 0x%lX", timeouts);
        append_line(out, out_cap, line);
    }
    append_line(out, out_cap, "");

    sprintf(line, "Total Score: %lu", total_score);
    append_line(out, out_cap, line);

    if (!magic_ok) append_line(out, out_cap, "[WARN] Magic mismatch");
    if (!checksum_ok) {
        sprintf(line, "[WARN] Checksum mismatch (file=0x%08lX, calc=0x%08lX)", file_checksum, calc);
        append_line(out, out_cap, line);
    }

    return 1;
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        CreateWindow("STATIC", "File:", WS_CHILD | WS_VISIBLE,
            10, 12, 30, 16, hwnd, (HMENU)-1, NULL, NULL);

        CreateWindow("EDIT", "report.szr", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            45, 10, 340, 20, hwnd, (HMENU)IDC_PATH_EDIT, NULL, NULL);

        CreateWindow("BUTTON", "Force", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            390, 10, 60, 20, hwnd, (HMENU)IDC_FORCE_CHK, NULL, NULL);

        CreateWindow("BUTTON", "Parse", WS_CHILD | WS_VISIBLE,
            455, 10, 60, 20, hwnd, (HMENU)IDC_PARSE_BTN, NULL, NULL);

        CreateWindow("BUTTON", "Exit", WS_CHILD | WS_VISIBLE,
            520, 10, 60, 20, hwnd, (HMENU)IDC_EXIT_BTN, NULL, NULL);

        CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE |
            ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
            10, 40, 570, 310, hwnd, (HMENU)IDC_OUTPUT_EDIT, NULL, NULL);
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_PARSE_BTN:
            {
                char path[260];
                char out[8192];
                int force = (SendDlgItemMessage(hwnd, IDC_FORCE_CHK, BM_GETCHECK, 0, 0) == BST_CHECKED);
                GetDlgItemText(hwnd, IDC_PATH_EDIT, path, sizeof(path));
                if (path[0] == '\0') {
                    SetDlgItemText(hwnd, IDC_OUTPUT_EDIT, "[ERROR] Please enter a file path.");
                } else {
                    parse_szr_to_text(path, force, out, sizeof(out));
                    SetDlgItemText(hwnd, IDC_OUTPUT_EDIT, out);
                }
            }
            return 0;

        case IDC_EXIT_BTN:
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            return 0;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc;
    HWND hwnd;
    MSG msg;

    (void)hPrev;
    (void)lpCmdLine;

    memset(&wc, 0, sizeof(wc));
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = "SZRReaderGUIWnd";

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, "RegisterClass failed", APP_TITLE, MB_OK | MB_ICONERROR);
        return 1;
    }

    hwnd = CreateWindow(
        "SZRReaderGUIWnd",
        APP_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        610, 410,
        NULL, NULL,
        hInst,
        NULL
    );

    if (!hwnd) {
        MessageBox(NULL, "CreateWindow failed", APP_TITLE, MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow ? nCmdShow : SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
