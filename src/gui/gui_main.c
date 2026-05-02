#include "gui_main.h"
#include "gui_login.h"
#include "gui_patient.h"
#include "gui_doctor.h"
#include "gui_admin.h"
#include "../data_storage.h"
#include "../login.h"
#include "../public.h"
#include "../sha256.h"

/* 全局变量 */
HINSTANCE g_hInst = NULL;
User g_currentUser = {0};
static HWND g_hMainWnd = NULL;
static HWND g_hNavTree = NULL;
static HWND g_hContentView = NULL;
static HWND g_hStatusBar = NULL;
static HWND g_hLogoutBtn = NULL;
static int g_currentView = 0;

/* 内部函数声明 */
static BOOL InitMainWindow(HINSTANCE hInst, int nCmdShow);
static HWND CreateNavTree(HWND hParent);
static void PopulateNavTree(void);
static void OnNavSelChange(HWND hWnd, LPARAM lParam);
static void OnSize(HWND hWnd, UINT state, int cx, int cy);

/* ─── WinMain 入口 ─────────────────────────────────────────────────── */

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    MSG msg;

    /* 抑制未使用参数警告 */
    (void)hPrevInstance;
    (void)lpCmdLine;

    /* 初始化通用控件 */
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_WIN95_CLASSES | ICC_TREEVIEW_CLASSES | ICC_LISTVIEW_CLASSES |
                ICC_TAB_CLASSES | ICC_BAR_CLASSES | ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icc);

    /* 初始化数据存储 */
    init_console_encoding();
    if (init_data_storage() != SUCCESS) {
        MessageBoxA(NULL, "数据存储初始化失败!", "错误", MB_ICONERROR);
        return 1;
    }

    /* 初始化默认用户（同 main.c 逻辑） */
    UserNode *head = load_users_list();
    if (!head) {
        User default_users[3];
        const char *usernames[3] = { "admin", "doctor1", "patient1" };
        const char *plain_pwds[3] = { "admin123", "doctor123", "patient123" };
        const char *roles[3] = { ROLE_ADMIN, ROLE_DOCTOR, ROLE_PATIENT };
        for (int i = 0; i < 3; i++) {
            memset(&default_users[i], 0, sizeof(User));
            strcpy(default_users[i].username, usernames[i]);
            strcpy(default_users[i].role, roles[i]);
            uint8_t hash[SHA256_DIGEST_SIZE];
            char hex[SHA256_HEX_SIZE];
            sha256_hash((const uint8_t*)plain_pwds[i], strlen(plain_pwds[i]), hash);
            sha256_hex(hash, hex);
            strcpy(default_users[i].password, hex);
        }
        UserNode *dh = NULL, *tail = NULL;
        for (int i = 0; i < 3; i++) {
            UserNode *node = create_user_node(&default_users[i]);
            if (!node) { free_user_list(dh); return 1; }
            if (!dh) { dh = node; tail = node; }
            else { tail->next = node; tail = node; }
        }
        save_users_list(dh);
        free_user_list(dh);
        ensure_doctor_profile("doctor1");
        ensure_patient_profile("patient1");
    } else {
        migrate_user_passwords();
        UserNode *cur = head;
        while (cur) {
            if (strcmp(cur->data.role, ROLE_PATIENT) == 0)
                ensure_patient_profile(cur->data.username);
            else if (strcmp(cur->data.role, ROLE_DOCTOR) == 0)
                ensure_doctor_profile(cur->data.username);
            cur = cur->next;
        }
        free_user_list(head);
    }
    migrate_doctor_ids();
    ensure_default_templates();

    g_hInst = hInstance;

    /* 创建主窗口 */
    if (!InitMainWindow(hInstance, nCmdShow))
        return 1;

    /* 显示登录对话框 */
    User loggedUser;
    if (ShowLoginDialog(hInstance, g_hMainWnd, &loggedUser) != SUCCESS) {
        DestroyWindow(g_hMainWnd);
        return 0;
    }
    g_currentUser = loggedUser;

    /* 填入导航树 */
    PopulateNavTree();

    /* 切换到第一个页面 */
    int firstView = 0;
    if (strcmp(loggedUser.role, ROLE_PATIENT) == 0)
        firstView = NAV_PATIENT_REGISTER;
    else if (strcmp(loggedUser.role, ROLE_DOCTOR) == 0)
        firstView = NAV_DOCTOR_REMINDER;
    else if (strcmp(loggedUser.role, ROLE_ADMIN) == 0)
        firstView = NAV_ADMIN_DEPT;
    SwitchView(g_hMainWnd, firstView);

    char status[256];
    snprintf(status, sizeof(status), "  用户: %s  |  角色: %s",
             loggedUser.username, loggedUser.role);
    SetStatusText(g_hMainWnd, status);

    /* 消息循环 */
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

/* ─── 初始化主窗口 ─────────────────────────────────────────────────── */

static BOOL InitMainWindow(HINSTANCE hInst, int nCmdShow) {
    const char CLASS_NAME[] = "EMSMainWindow";

    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = MainWndProc;
    wc.hInstance     = hInst;
    wc.hIcon         = LoadIconA(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_NAME;

    if (!RegisterClassA(&wc))
        return FALSE;

    HWND hWnd = CreateWindowExA(
        0, CLASS_NAME, "电子医疗管理系统",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL, hInst, NULL
    );

    if (!hWnd)
        return FALSE;

    g_hMainWnd = hWnd;
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return TRUE;
}

/* ─── 创建导航树 ───────────────────────────────────────────────────── */

static HWND CreateNavTree(HWND hParent) {
    HWND hTree = CreateWindowA(
        WC_TREEVIEWA, "", WS_VISIBLE | WS_CHILD | TVS_HASLINES |
        TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
        0, 0, NAV_WIDTH, 100,
        hParent, (HMENU)1, g_hInst, NULL
    );
    return hTree;
}

static void PopulateNavTree(void) {
    if (!g_hNavTree) return;

    /* 清空 */
    TreeView_DeleteAllItems(g_hNavTree);

    HTREEITEM hRoot = NULL;
    TV_INSERTSTRUCTA tvis = {0};
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = TVIF_TEXT | TVIF_PARAM;

    const char *role = g_currentUser.role;

    if (strcmp(role, ROLE_PATIENT) == 0) {
        tvis.hParent = NULL;
        tvis.item.pszText = (LPSTR)"患者系统";
        tvis.item.lParam = 0;
        hRoot = TreeView_InsertItem(g_hNavTree, &tvis);

        struct { const char *name; int id; } items[] = {
            {"挂号",          NAV_PATIENT_REGISTER},
            {"预约查询",      NAV_PATIENT_APPOINTMENT},
            {"诊断结果",      NAV_PATIENT_DIAGNOSIS},
            {"处方查询",      NAV_PATIENT_PRESCRIPTION},
            {"住院信息",      NAV_PATIENT_WARD},
            {"治疗进度",      NAV_PATIENT_PROGRESS},
            {"个人信息",      NAV_PATIENT_PROFILE},
        };
        for (int i = 0; i < (int)(sizeof(items)/sizeof(items[0])); i++) {
            tvis.hParent = hRoot;
            tvis.item.pszText = (LPSTR)items[i].name;
            tvis.item.lParam = items[i].id;
            TreeView_InsertItem(g_hNavTree, &tvis);
        }
        TreeView_Expand(g_hNavTree, hRoot, TVE_EXPAND);

    } else if (strcmp(role, ROLE_DOCTOR) == 0) {
        tvis.hParent = NULL;
        tvis.item.pszText = (LPSTR)"医生系统";
        tvis.item.lParam = 0;
        hRoot = TreeView_InsertItem(g_hNavTree, &tvis);

        struct { const char *name; int id; } items[] = {
            {"待接诊",          NAV_DOCTOR_REMINDER},
            {"接诊",            NAV_DOCTOR_CONSULTATION},
            {"病房呼叫",        NAV_DOCTOR_WARD_CALL},
            {"紧急标记",        NAV_DOCTOR_EMERGENCY},
            {"进度更新",        NAV_DOCTOR_PROGRESS},
            {"病历模板",        NAV_DOCTOR_TEMPLATE},
        };
        for (int i = 0; i < (int)(sizeof(items)/sizeof(items[0])); i++) {
            tvis.hParent = hRoot;
            tvis.item.pszText = (LPSTR)items[i].name;
            tvis.item.lParam = items[i].id;
            TreeView_InsertItem(g_hNavTree, &tvis);
        }
        TreeView_Expand(g_hNavTree, hRoot, TVE_EXPAND);

    } else if (strcmp(role, ROLE_ADMIN) == 0) {
        tvis.hParent = NULL;
        tvis.item.pszText = (LPSTR)"管理员系统";
        tvis.item.lParam = 0;
        hRoot = TreeView_InsertItem(g_hNavTree, &tvis);

        struct { const char *name; int id; } items[] = {
            {"科室管理",   NAV_ADMIN_DEPT},
            {"医生管理",   NAV_ADMIN_DOCTOR},
            {"患者管理",   NAV_ADMIN_PATIENT},
            {"药品管理",   NAV_ADMIN_DRUG},
            {"病房管理",   NAV_ADMIN_WARD},
            {"排班管理",   NAV_ADMIN_SCHEDULE},
            {"操作日志",   NAV_ADMIN_LOG},
            {"数据管理",   NAV_ADMIN_DATA},
            {"报表统计",   NAV_ADMIN_ANALYSIS},
        };
        for (int i = 0; i < (int)(sizeof(items)/sizeof(items[0])); i++) {
            tvis.hParent = hRoot;
            tvis.item.pszText = (LPSTR)items[i].name;
            tvis.item.lParam = items[i].id;
            TreeView_InsertItem(g_hNavTree, &tvis);
        }
        TreeView_Expand(g_hNavTree, hRoot, TVE_EXPAND);
    }
}

/* ─── 视图切换 ─────────────────────────────────────────────────────── */

void SwitchView(HWND hWnd, int viewId) {
    if (g_hContentView) {
        DestroyWindow(g_hContentView);
        g_hContentView = NULL;
    }
    g_currentView = viewId;

    /* 获取内容区域矩形 */
    RECT rc;
    GetClientRect(hWnd, &rc);
    rc.left += NAV_WIDTH + 4;
    rc.top += 4;
    rc.right -= 4;
    rc.bottom -= STATUS_HEIGHT + 4;

    HWND hParent = hWnd;

    switch (viewId) {
    case NAV_PATIENT_REGISTER:
    case NAV_PATIENT_APPOINTMENT:
    case NAV_PATIENT_DIAGNOSIS:
    case NAV_PATIENT_PRESCRIPTION:
    case NAV_PATIENT_WARD:
    case NAV_PATIENT_PROGRESS:
    case NAV_PATIENT_PROFILE:
        g_hContentView = CreatePatientPage(hParent, viewId, &rc);
        break;
    case NAV_DOCTOR_REMINDER:
    case NAV_DOCTOR_CONSULTATION:
    case NAV_DOCTOR_WARD_CALL:
    case NAV_DOCTOR_EMERGENCY:
    case NAV_DOCTOR_PROGRESS:
    case NAV_DOCTOR_TEMPLATE:
        g_hContentView = CreateDoctorPage(hParent, viewId, &rc);
        break;
    case NAV_ADMIN_DEPT:
    case NAV_ADMIN_DOCTOR:
    case NAV_ADMIN_PATIENT:
    case NAV_ADMIN_DRUG:
    case NAV_ADMIN_WARD:
    case NAV_ADMIN_SCHEDULE:
    case NAV_ADMIN_LOG:
    case NAV_ADMIN_DATA:
    case NAV_ADMIN_ANALYSIS:
        g_hContentView = CreateAdminPage(hParent, viewId, &rc);
        break;
    default:
        break;
    }
}

HWND GetContentView(HWND hWnd) {
    (void)hWnd;
    return g_hContentView;
}

void SetStatusText(HWND hWnd, const char *text) {
    (void)hWnd;
    if (g_hStatusBar) {
        SendMessageA(g_hStatusBar, SB_SETTEXTA, 0, (LPARAM)text);
    }
}

/* ─── 导航选择变化 ─────────────────────────────────────────────────── */

static void OnNavSelChange(HWND hWnd, LPARAM lParam) {
    NM_TREEVIEW *pnmtv = (NM_TREEVIEW *)lParam;
    HTREEITEM hItem = pnmtv->itemNew.hItem;
    if (!hItem) return;

    TV_ITEM item;
    item.hItem = hItem;
    item.mask = TVIF_PARAM;
    TreeView_GetItem(g_hNavTree, &item);

    int viewId = (int)item.lParam;
    if (viewId > 0) {
        SwitchView(hWnd, viewId);
    }
}

/* ─── 窗口大小调整 ─────────────────────────────────────────────────── */

static void OnSize(HWND hWnd, UINT state, int cx, int cy) {
    (void)state;
    (void)hWnd;
    if (g_hNavTree) {
        SetWindowPos(g_hNavTree, NULL, 0, 0, NAV_WIDTH, cy - STATUS_HEIGHT - 30,
                     SWP_NOZORDER);
    }
    if (g_hLogoutBtn) {
        SetWindowPos(g_hLogoutBtn, NULL, 0, cy - STATUS_HEIGHT - 30,
                     NAV_WIDTH, 30, SWP_NOZORDER);
    }
    if (g_hStatusBar) {
        SendMessageA(g_hStatusBar, WM_SIZE, 0, 0);
    }
    if (g_hContentView) {
        RECT rc = { NAV_WIDTH + 4, 4, cx - 4, cy - STATUS_HEIGHT - 4 };
        SetWindowPos(g_hContentView, NULL, rc.left, rc.top,
                     rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER);
    }
}

/* ─── 主窗口过程 ───────────────────────────────────────────────────── */

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        /* 创建导航树 */
        g_hNavTree = CreateNavTree(hWnd);

        /* 创建退出登录按钮 */
        g_hLogoutBtn = CreateWindowA(
            "BUTTON", "退出登录",
            WS_VISIBLE | WS_CHILD | BS_FLAT,
            0, 0, NAV_WIDTH, 30,
            hWnd, (HMENU)100, g_hInst, NULL
        );

        /* 创建状态栏 */
        g_hStatusBar = CreateWindowA(
            STATUSCLASSNAMEA, "", WS_VISIBLE | WS_CHILD | SBARS_SIZEGRIP,
            0, 0, 0, 0, hWnd, (HMENU)2, g_hInst, NULL
        );

        /* 设置状态栏文本 */
        SetStatusText(hWnd, "  就绪");
        return 0;

    case WM_SIZE:
        OnSize(hWnd, (UINT)wParam, LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_NOTIFY:
        if (((NMHDR *)lParam)->idFrom == 1 &&
            ((NMHDR *)lParam)->code == TVN_SELCHANGEDA) {
            OnNavSelChange(hWnd, lParam);
        }
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == 100) { /* 退出登录 */
            memset(&g_currentUser, 0, sizeof(User));
            User newUser;
            if (ShowLoginDialog(g_hInst, hWnd, &newUser) == SUCCESS) {
                g_currentUser = newUser;
                PopulateNavTree();
                int firstView = 0;
                if (strcmp(newUser.role, ROLE_PATIENT) == 0)
                    firstView = NAV_PATIENT_REGISTER;
                else if (strcmp(newUser.role, ROLE_DOCTOR) == 0)
                    firstView = NAV_DOCTOR_REMINDER;
                else if (strcmp(newUser.role, ROLE_ADMIN) == 0)
                    firstView = NAV_ADMIN_DEPT;
                SwitchView(hWnd, firstView);
                char status[256];
                snprintf(status, sizeof(status), "  用户: %s  |  角色: %s",
                         newUser.username, newUser.role);
                SetStatusText(hWnd, status);
            } else {
                DestroyWindow(hWnd);
                PostQuitMessage(0);
            }
        } else if (LOWORD(wParam) == 200) { /* 关于 */
            MessageBoxA(hWnd,
                "电子医疗管理系统 v1.0\n"
                "基于 Win32 API 的原生桌面应用\n\n"
                "角色: 患者 / 医生 / 管理员",
                "关于", MB_OK | MB_ICONINFORMATION);
        }
        return 0;

    case WM_APP_REFRESH:
        /* 仅在未切换页面时才刷新（防止用户快速导航导致的错误跳转） */
        if ((int)wParam == g_currentView)
            SwitchView(hWnd, (int)wParam);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcA(hWnd, msg, wParam, lParam);
    }
}
