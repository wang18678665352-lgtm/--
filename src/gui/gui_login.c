#include "gui_login.h"
#include "../data_storage.h"
#include "../sha256.h"
#include "../public.h"
#include <commctrl.h>

/* 控件 ID */
#define IDC_ROLE_ADMIN    101
#define IDC_ROLE_DOCTOR   102
#define IDC_ROLE_PATIENT  103
#define IDC_USERNAME      104
#define IDC_PASSWORD      105
#define IDC_LOGIN         106
#define IDC_REGISTER      107
#define IDC_CANCEL        108
#define IDC_STATUS        109
#define IDC_TITLE         110

/* 注册对话框控件 */
#define IDC_REG_USERNAME  201
#define IDC_REG_PASSWORD  202
#define IDC_REG_CONFIRM   203
#define IDC_REG_ROLE_ADMIN   204
#define IDC_REG_ROLE_DOCTOR  205
#define IDC_REG_ROLE_PATIENT 206
#define IDC_REG_DEPT      207
#define IDC_REG_NAME      208
#define IDC_REG_TITLE     209
#define IDC_REG_OK        210
#define IDC_REG_CANCEL    211
#define IDC_REG_STATUS    212

/* 对话框大小（DLU 近似像素） */
#define DLG_W 360
#define DLG_H 320
#define REG_W 400
#define REG_H 400

static User g_loginResult;
static int g_loginResultCode = ERROR_INVALID_INPUT;

/* ─── 工具函数 ─────────────────────────────────────────────────────── */

static HWND CreateLabel(HWND hParent, int id, const char *text,
                        int x, int y, int w, int h) {
    return CreateWindowA("STATIC", text, WS_VISIBLE | WS_CHILD | SS_LEFT,
                         x, y, w, h, hParent, (HMENU)(INT_PTR)id,
                         GetModuleHandle(NULL), NULL);
}

static HWND CreateButton(HWND hParent, int id, const char *text,
                         int x, int y, int w, int h) {
    return CreateWindowA("BUTTON", text,
                         WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                         x, y, w, h, hParent, (HMENU)(INT_PTR)id,
                         GetModuleHandle(NULL), NULL);
}

static HWND CreateEditBox(HWND hParent, int id, int x, int y, int w, int h,
                          bool password) {
    DWORD style = WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL;
    if (password) style |= ES_PASSWORD;
    return CreateWindowA("EDIT", "", style,
                         x, y, w, h, hParent, (HMENU)(INT_PTR)id,
                         GetModuleHandle(NULL), NULL);
}

static HWND CreateRadio(HWND hParent, int id, const char *text,
                        int x, int y, int w, int h, bool checked) {
    DWORD style = WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON;
    HWND hCtrl = CreateWindowA("BUTTON", text, style,
                               x, y, w, h, hParent, (HMENU)(INT_PTR)id,
                               GetModuleHandle(NULL), NULL);
    if (checked)
        SendMessage(hCtrl, BM_SETCHECK, BST_CHECKED, 0);
    return hCtrl;
}

static HWND CreateComboBox(HWND hParent, int id, int x, int y, int w, int h) {
    return CreateWindowA("COMBOBOX", "",
                         WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST |
                         WS_VSCROLL | CBS_HASSTRINGS,
                         x, y, w, h, hParent, (HMENU)(INT_PTR)id,
                         GetModuleHandle(NULL), NULL);
}

/* ─── 登录对话框 ───────────────────────────────────────────────────── */

int ShowLoginDialog(HINSTANCE hInst, HWND hParent, User *user) {
    g_loginResultCode = ERROR_INVALID_INPUT;
    memset(&g_loginResult, 0, sizeof(g_loginResult));

    /* 创建自定义登录窗口 */
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = LoginDlgProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "LoginDialog";
    RegisterClassA(&wc);

    HWND hDlg = CreateWindowExA(0, "LoginDialog", "电子医疗管理系统 - 登录",
                           WS_VISIBLE | WS_POPUPWINDOW | WS_CAPTION |
                           WS_SYSMENU | DS_CENTER,
                           CW_USEDEFAULT, CW_USEDEFAULT,
                           DLG_W, DLG_H,
                           hParent, NULL, hInst, NULL);

    if (!hDlg) return ERROR_FILE_IO;

    /* 标题 */
    CreateLabel(hDlg, IDC_TITLE, "用户登录",
                20, 15, DLG_W - 40, 25);
    HFONT hTitleFont = CreateFontA(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                   CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                   DEFAULT_PITCH | FF_DONTCARE, "Microsoft YaHei UI");
    SendMessage(GetDlgItem(hDlg, IDC_TITLE), WM_SETFONT, (WPARAM)hTitleFont, TRUE);

    /* 角色选择 */
    CreateLabel(hDlg, 0, "选择角色:", 20, 50, 80, 20);
    CreateRadio(hDlg, IDC_ROLE_PATIENT, "患者", 110, 48, 60, 24, TRUE);
    CreateRadio(hDlg, IDC_ROLE_DOCTOR, "医生", 180, 48, 60, 24, FALSE);
    CreateRadio(hDlg, IDC_ROLE_ADMIN, "管理员", 250, 48, 70, 24, FALSE);

    /* 用户名 */
    CreateLabel(hDlg, 0, "用户名:", 20, 90, 60, 20);
    CreateEditBox(hDlg, IDC_USERNAME, 90, 88, 240, 24, FALSE);

    /* 密码 */
    CreateLabel(hDlg, 0, "密  码:", 20, 125, 60, 20);
    CreateEditBox(hDlg, IDC_PASSWORD, 90, 123, 240, 24, TRUE);

    /* 分隔线 */
    CreateWindowA("STATIC", "", WS_VISIBLE | WS_CHILD | SS_ETCHEDHORZ,
                  20, 160, DLG_W - 40, 2, hDlg, NULL, hInst, NULL);

    /* 按钮 */
    CreateButton(hDlg, IDC_LOGIN, "登录", 50, 180, 80, 30);
    CreateButton(hDlg, IDC_REGISTER, "注册", 145, 180, 80, 30);
    CreateButton(hDlg, IDC_CANCEL, "退出", 240, 180, 80, 30);

    /* 状态条 */
    CreateWindowA("STATIC", "", WS_VISIBLE | WS_CHILD | SS_CENTER,
                  20, 225, DLG_W - 40, 20, hDlg, (HMENU)(INT_PTR)IDC_STATUS,
                  hInst, NULL);

    /* 居中显示 */
    RECT rc;
    GetWindowRect(hDlg, &rc);
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(hDlg, NULL, (sw - (rc.right - rc.left)) / 2,
                 (sh - (rc.bottom - rc.top)) / 2, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER);

    /* 模态消息循环 */
    EnableWindow(hParent, FALSE);
    MSG msg;
    while (g_loginResultCode == ERROR_INVALID_INPUT &&
           GetMessage(&msg, NULL, 0, 0)) {
        if (!IsDialogMessage(hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    EnableWindow(hParent, TRUE);
    SetForegroundWindow(hParent);
    DestroyWindow(hDlg);

    if (g_loginResultCode == SUCCESS && user) {
        *user = g_loginResult;
    }
    return g_loginResultCode;
}

INT_PTR CALLBACK LoginDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    switch (msg) {
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_LOGIN: {
            char username[50] = {0};
            char password[100] = {0};
            char userRole[20] = {0};

            GetDlgItemTextA(hDlg, IDC_USERNAME, username, sizeof(username));
            GetDlgItemTextA(hDlg, IDC_PASSWORD, password, sizeof(password));

            if (SendDlgItemMessage(hDlg, IDC_ROLE_ADMIN, BM_GETCHECK, 0, 0) == BST_CHECKED)
                strcpy(userRole, ROLE_ADMIN);
            else if (SendDlgItemMessage(hDlg, IDC_ROLE_DOCTOR, BM_GETCHECK, 0, 0) == BST_CHECKED)
                strcpy(userRole, ROLE_DOCTOR);
            else
                strcpy(userRole, ROLE_PATIENT);

            if (strlen(username) == 0 || strlen(password) == 0) {
                SetDlgItemTextA(hDlg, IDC_STATUS, "请输入用户名和密码");
                return TRUE;
            }

            /* 手动验证 */
            UserNode *users = load_users_list();
            if (!users) {
                SetDlgItemTextA(hDlg, IDC_STATUS, "用户数据加载失败");
                return TRUE;
            }

            UserNode *cur = users;
            int found = 0;
            while (cur) {
                if (strcmp(cur->data.username, username) == 0 &&
                    strcmp(cur->data.role, userRole) == 0) {
                    /* 验证密码 */
                    uint8_t hash[SHA256_DIGEST_SIZE];
                    char hex[SHA256_HEX_SIZE];
                    sha256_hash((const uint8_t*)password, strlen(password), hash);
                    sha256_hex(hash, hex);

                    if (strcmp(cur->data.password, hex) == 0) {
                        g_loginResult = cur->data;
                        found = 1;
                    }
                    break;
                }
                cur = cur->next;
            }
            free_user_list(users);

            if (found) {
                g_loginResultCode = SUCCESS;
                PostMessage(hDlg, WM_CLOSE, 0, 0);
            } else {
                SetDlgItemTextA(hDlg, IDC_STATUS,
                    "用户名或密码错误，请重试");
            }
            return TRUE;
        }

        case IDC_REGISTER:
            /* 打开注册对话框（在当前对话框上模态） */
            ShowRegisterDialog(GetModuleHandle(NULL), hDlg);
            return TRUE;

        case IDC_CANCEL:
            g_loginResultCode = ERROR_INVALID_INPUT;
            PostMessage(hDlg, WM_CLOSE, 0, 0);
            return TRUE;
        }
        return TRUE;

    case WM_CLOSE:
        DestroyWindow(hDlg);
        return TRUE;

    default:
        return DefWindowProcA(hDlg, msg, wParam, lParam);
    }
}

/* ─── 注册对话框 ───────────────────────────────────────────────────── */

int ShowRegisterDialog(HINSTANCE hInst, HWND hParent) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = RegisterDlgProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "RegisterDialog";
    RegisterClassA(&wc);

    HWND hDlg = CreateWindowExA(0, "RegisterDialog", "注册新用户",
                                WS_VISIBLE | WS_POPUPWINDOW | WS_CAPTION |
                                WS_SYSMENU | DS_CENTER,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                REG_W, REG_H,
                                hParent, NULL, hInst, NULL);
    if (!hDlg) return ERROR_FILE_IO;

    int y = 20;
    CreateLabel(hDlg, 0, "注册新用户", 20, y, REG_W - 40, 25);
    y += 35;

    CreateLabel(hDlg, 0, "用户名:", 20, y, 70, 20);
    CreateEditBox(hDlg, IDC_REG_USERNAME, 100, y, 250, 24, FALSE);
    y += 35;

    CreateLabel(hDlg, 0, "密码:", 20, y, 70, 20);
    CreateEditBox(hDlg, IDC_REG_PASSWORD, 100, y, 250, 24, TRUE);
    y += 35;

    CreateLabel(hDlg, 0, "确认密码:", 20, y, 70, 20);
    CreateEditBox(hDlg, IDC_REG_CONFIRM, 100, y, 250, 24, TRUE);
    y += 35;

    CreateLabel(hDlg, 0, "角色:", 20, y, 70, 20);
    CreateRadio(hDlg, IDC_REG_ROLE_PATIENT, "患者", 100, y, 60, 24, TRUE);
    CreateRadio(hDlg, IDC_REG_ROLE_DOCTOR, "医生", 170, y, 60, 24, FALSE);
    CreateRadio(hDlg, IDC_REG_ROLE_ADMIN, "管理员", 240, y, 70, 24, FALSE);
    y += 35;

    CreateLabel(hDlg, 0, "姓名:", 20, y, 70, 20);
    CreateEditBox(hDlg, IDC_REG_NAME, 100, y, 250, 24, FALSE);
    y += 35;

    CreateLabel(hDlg, 0, "科室:", 20, y, 70, 20);
    HWND hDept = CreateComboBox(hDlg, IDC_REG_DEPT, 100, y, 250, 200);
    /* 加载科室列表 */
    DepartmentNode *depts = load_departments_list();
    if (depts) {
        DepartmentNode *dc = depts;
        while (dc) {
            SendMessageA(hDept, CB_ADDSTRING, 0, (LPARAM)dc->data.name);
            dc = dc->next;
        }
        SendMessage(hDept, CB_SETCURSEL, 0, 0);
        free_department_list(depts);
    }
    y += 35;

    CreateWindowA("STATIC", "", WS_VISIBLE | WS_CHILD | SS_ETCHEDHORZ,
                  20, y, REG_W - 40, 2, hDlg, NULL, hInst, NULL);
    y += 15;

    CreateButton(hDlg, IDC_REG_OK, "注册", 80, y, 90, 30);
    CreateButton(hDlg, IDC_REG_CANCEL, "取消", 200, y, 90, 30);
    y += 40;

    CreateWindowA("STATIC", "", WS_VISIBLE | WS_CHILD | SS_CENTER,
                  20, y, REG_W - 40, 20, hDlg,
                  (HMENU)(INT_PTR)IDC_REG_STATUS, hInst, NULL);

    /* 居中 */
    RECT rc;
    GetWindowRect(hDlg, &rc);
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(hDlg, NULL, (sw - (rc.right - rc.left)) / 2,
                 (sh - (rc.bottom - rc.top)) / 2, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER);

    /* 模态循环 */
    EnableWindow(hParent, FALSE);
    MSG msg;
    int result = ERROR_INVALID_INPUT;
    BOOL closed = FALSE;
    while (!closed && GetMessage(&msg, NULL, 0, 0)) {
        if (!IsDialogMessage(hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        /* 检查对话框是否已关闭 */
        if (!IsWindow(hDlg)) closed = TRUE;
    }
    EnableWindow(hParent, TRUE);
    SetForegroundWindow(hParent);

    return result;
}

INT_PTR CALLBACK RegisterDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    switch (msg) {
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_REG_OK: {
            char username[50] = {0};
            char password[100] = {0};
            char confirm[100] = {0};
            char name[100] = {0};
            char role[20] = {0};
            char deptName[100] = {0};

            GetDlgItemTextA(hDlg, IDC_REG_USERNAME, username, sizeof(username));
            GetDlgItemTextA(hDlg, IDC_REG_PASSWORD, password, sizeof(password));
            GetDlgItemTextA(hDlg, IDC_REG_CONFIRM, confirm, sizeof(confirm));
            GetDlgItemTextA(hDlg, IDC_REG_NAME, name, sizeof(name));
            GetDlgItemTextA(hDlg, IDC_REG_DEPT, deptName, sizeof(deptName));

            if (SendDlgItemMessage(hDlg, IDC_REG_ROLE_DOCTOR, BM_GETCHECK, 0, 0) == BST_CHECKED)
                strcpy(role, ROLE_DOCTOR);
            else if (SendDlgItemMessage(hDlg, IDC_REG_ROLE_ADMIN, BM_GETCHECK, 0, 0) == BST_CHECKED)
                strcpy(role, ROLE_ADMIN);
            else
                strcpy(role, ROLE_PATIENT);

            /* 校验 */
            if (strlen(username) == 0 || strlen(password) == 0 || strlen(name) == 0) {
                SetDlgItemTextA(hDlg, IDC_REG_STATUS, "请填写必填字段");
                return TRUE;
            }
            if (strlen(username) < 3) {
                SetDlgItemTextA(hDlg, IDC_REG_STATUS, "用户名至少3个字符");
                return TRUE;
            }
            if (strlen(password) < 6) {
                SetDlgItemTextA(hDlg, IDC_REG_STATUS, "密码至少6个字符");
                return TRUE;
            }
            if (strcmp(password, confirm) != 0) {
                SetDlgItemTextA(hDlg, IDC_REG_STATUS, "两次密码不一致");
                return TRUE;
            }

            /* 检查用户名唯一性 */
            UserNode *users = load_users_list();
            if (users) {
                UserNode *cur = users;
                while (cur) {
                    if (strcmp(cur->data.username, username) == 0) {
                        SetDlgItemTextA(hDlg, IDC_REG_STATUS, "用户名已存在");
                        free_user_list(users);
                        return TRUE;
                    }
                    cur = cur->next;
                }
                free_user_list(users);
            }

            /* 创建用户 */
            User newUser;
            memset(&newUser, 0, sizeof(newUser));
            strcpy(newUser.username, username);
            strcpy(newUser.role, role);
            uint8_t hash[SHA256_DIGEST_SIZE];
            char hex[SHA256_HEX_SIZE];
            sha256_hash((const uint8_t*)password, strlen(password), hash);
            sha256_hex(hash, hex);
            strcpy(newUser.password, hex);

            UserNode *allUsers = load_users_list();
            UserNode *newNode = create_user_node(&newUser);
            if (!newNode) {
                free_user_list(allUsers);
                SetDlgItemTextA(hDlg, IDC_REG_STATUS, "内存不足");
                return TRUE;
            }
            newNode->next = allUsers;
            if (save_users_list(newNode) != SUCCESS) {
                SetDlgItemTextA(hDlg, IDC_REG_STATUS, "保存失败");
                free_user_list(newNode);
                return TRUE;
            }
            free_user_list(newNode);

            /* 创建角色档案 */
            if (strcmp(role, ROLE_PATIENT) == 0) {
                ensure_patient_profile(username);
            } else if (strcmp(role, ROLE_DOCTOR) == 0) {
                /* 查找科室ID */
                DepartmentNode *depts = load_departments_list();
                char deptId[20] = {0};
                if (depts) {
                    DepartmentNode *dc = depts;
                    while (dc) {
                        if (strcmp(dc->data.name, deptName) == 0) {
                            strcpy(deptId, dc->data.department_id);
                            break;
                        }
                        dc = dc->next;
                    }
                    free_department_list(depts);
                }
                create_doctor_profile_with_details(username, name, "医师", deptId);
            }

            SetDlgItemTextA(hDlg, IDC_REG_STATUS, "注册成功！");
            MessageBoxA(hDlg, "注册成功，请返回登录", "成功", MB_OK | MB_ICONINFORMATION);
            DestroyWindow(hDlg);
            return TRUE;
        }

        case IDC_REG_CANCEL:
            DestroyWindow(hDlg);
            return TRUE;
        }
        return TRUE;

    case WM_CLOSE:
        DestroyWindow(hDlg);
        return TRUE;

    default:
        return DefWindowProcA(hDlg, msg, wParam, lParam);
    }
}
