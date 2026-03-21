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

#define SZR_SIZE 116
#define SZR_SALT 0x7A69676DUL

#define OFF_MAGIC      0
#define OFF_VERSION    4
#define OFF_CPU_NAME   8
#define OFF_SCORES     40
#define OFF_TOTAL      64
#define OFF_TIMESTAMP  68
#define OFF_OS_NAME    72
#define OFF_MEMORY_MB  104
#define OFF_CORES      108
#define OFF_CHECKSUM   112

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
    unsigned char raw[512];
    unsigned char data[SZR_SIZE];
    unsigned int read_n;

    unsigned long version;
    float scores[6];
    unsigned long total_score;
    unsigned long timestamp;
    unsigned long memory_mb;
    unsigned long cores;
    unsigned long file_checksum;
    unsigned long calc;
    int magic_ok;
    int checksum_ok;

    char cpu_name[33];
    char os_name[33];
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
            append_line(out, out_cap, "[WARN] Force mode: file too long, using first 116 bytes");
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
    read_cstr(cpu_name, sizeof(cpu_name), data + OFF_CPU_NAME, 32);

    scores[0] = read_f32_le(data + OFF_SCORES + 0);
    scores[1] = read_f32_le(data + OFF_SCORES + 4);
    scores[2] = read_f32_le(data + OFF_SCORES + 8);
    scores[3] = read_f32_le(data + OFF_SCORES + 12);
    scores[4] = read_f32_le(data + OFF_SCORES + 16);
    scores[5] = read_f32_le(data + OFF_SCORES + 20);

    total_score = read_u32_le(data + OFF_TOTAL);
    timestamp = read_u32_le(data + OFF_TIMESTAMP);
    read_cstr(os_name, sizeof(os_name), data + OFF_OS_NAME, 32);
    memory_mb = read_u32_le(data + OFF_MEMORY_MB);
    cores = read_u32_le(data + OFF_CORES);
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

    sprintf(line, "Version   : 0x%08lX", version);
    append_line(out, out_cap, line);
    sprintf(line, "CPU       : %s", cpu_name);
    append_line(out, out_cap, line);
    sprintf(line, "OS Mode   : %s", os_name);
    append_line(out, out_cap, line);
    sprintf(line, "Cores     : %lu", cores);
    append_line(out, out_cap, line);
    sprintf(line, "Memory MB : %lu", memory_mb);
    append_line(out, out_cap, line);

    t = (time_t)timestamp;
    tmv = localtime(&t);
    if (tmv && strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", tmv) > 0) {
        sprintf(line, "Timestamp : %lu (%s)", timestamp, tbuf);
    } else {
        sprintf(line, "Timestamp : %lu", timestamp);
    }
    append_line(out, out_cap, line);

    sprintf(line, "Total     : %lu", total_score);
    append_line(out, out_cap, line);

    append_line(out, out_cap, "Raw Scores:");
    sprintf(line, "  - Integer : %.4f", scores[0]); append_line(out, out_cap, line);
    sprintf(line, "  - Float   : %.4f", scores[1]); append_line(out, out_cap, line);
    sprintf(line, "  - MemOps  : %.4f", scores[2]); append_line(out, out_cap, line);
    sprintf(line, "  - Crypto  : %.4f", scores[3]); append_line(out, out_cap, line);
    sprintf(line, "  - Compress: %.4f", scores[4]); append_line(out, out_cap, line);
    sprintf(line, "  - Matrix  : %.4f", scores[5]); append_line(out, out_cap, line);

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
