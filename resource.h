/*
 * SigmaZ
 * Copyright (c) 2026 Ziyang Bai
 * Licensed under the GNU General Public License v3.0
 */

#define IDD_MAIN_DLG    101

/* Common Controls */
#define IDC_EXIT_BTN    1003

/* Tab Switch Buttons */
#define IDC_TAB_CPU     2001
#define IDC_TAB_INT     2002    /* Renamed from BENCH */
#define IDC_TAB_FLOAT   2003    /* New */
#define IDC_TAB_MEM     2004    /* New */
#define IDC_TAB_CRY     2005    /* New: Crypto */
#define IDC_TAB_CMP     2006    /* New: Compress */
#define IDC_TAB_MAT     2007    /* New: Matrix */
#define IDC_TAB_ALL     2008    /* New */
#define IDC_TAB_ABOUT   2009    /* Moved */
#define IDC_TAB_RPT     2010    /* New: Report Save */

/* CPU Tab Controls (Labels need unique IDs for show/hide) */
#define IDC_CPU_GRP     3000
#define IDC_CPU_REPORT  3001

/* Integer Bench Tab Controls (Ex-Bench) */
#define IDC_INT_GRP     4000
#define IDC_INT_BTN_S   4001
#define IDC_INT_BTN_M   4002
#define IDC_INT_REPORT  4003    /* New Report Edit */
#define IDC_INT_PROG    4006    /* Progress Bar (Simulated text) */

/* Float Bench Tab Controls */
#define IDC_FLOAT_GRP   5000
#define IDC_FLOAT_BTN   5001
#define IDC_FLOAT_REPORT 5002   /* New Report Edit */
#define IDC_FLOAT_PROG  5003

/* Memory Bench Tab Controls */
#define IDC_MEM_GRP     6000
#define IDC_MEM_BTN     6001
#define IDC_MEM_REPORT  6002    /* New Report Edit */
#define IDC_MEM_PROG    6003

/* New Tabs: Crypto, Compress, Matrix */
#define IDC_CRY_GRP     6100
#define IDC_CRY_BTN     6101
#define IDC_CRY_REPORT  6102
#define IDC_CRY_PROG    6103

#define IDC_CMP_GRP     6200
#define IDC_CMP_BTN     6201
#define IDC_CMP_REPORT  6202
#define IDC_CMP_PROG    6203

/* String IDs */
#define IDS_STARTUP_HINT 7001

#define IDC_MAT_GRP     6300
#define IDC_MAT_BTN     6301
#define IDC_MAT_REPORT  6302
#define IDC_MAT_PROG    6303

/* Run All / Summary Tab Controls */
#define IDC_ALL_GRP     7000
#define IDC_ALL_BTN     7001
#define IDC_ALL_REPORT  7002
#define IDC_ALL_PROG    7003
/* Previous IDs removed to switch to report box */

#define IDC_ABOUT_GRP   8000
#define IDC_ABOUT_REPORT 8001

/* Report Save Tab Controls */
#define IDC_RPT_GRP     9000
#define IDC_RPT_PATH    9001
#define IDC_RPT_SAVE    9002
#define IDC_RPT_STATUS  9003
#define IDC_RPT_PATHLBL 9004
#define IDC_RPT_DIRLIST 9005
#define IDC_RPT_FILELBL 9006
#define IDC_RPT_FILE    9007
