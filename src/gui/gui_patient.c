#include "gui_patient.h"
#include "gui_main.h"
#include "../data_storage.h"
#include "../public.h"

/* ─── ListView 工具 ─────────────────────────────────────────────────── */

static HWND CreateListView(HWND hParent, int id, int x, int y, int w, int h) {
    HWND hLV = CreateWindowA(WC_LISTVIEWA, "",
        WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS |
        LVS_SINGLESEL | WS_BORDER,
        x, y, w, h, hParent, (HMENU)(INT_PTR)id, g_hInst, NULL);
    ListView_SetExtendedListViewStyle(hLV, LVS_EX_FULLROWSELECT |
                                      LVS_EX_GRIDLINES | LVS_EX_ALTERNATINGROWCOLORS);
    return hLV;
}

static void AddCol(HWND hLV, int idx, const char *text, int width) {
    LV_COLUMNA col = {0};
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    col.pszText = (char *)text;
    col.cx = width;
    ListView_InsertColumn(hLV, idx, &col);
}

static void AddRow(HWND hLV, int row, int cols, const char **items) {
    LV_ITEMA li = {0};
    li.mask = LVIF_TEXT;
    li.pszText = (char *)items[0];
    li.iItem = row;
    ListView_InsertItem(hLV, &li);
    for (int c = 1; c < cols; c++) {
        li.iSubItem = c;
        li.pszText = (char *)items[c];
        ListView_SetItem(hLV, &li);
    }
}

/* 获取当前患者 ID */
static const char* GetPatientId(void) {
    Patient *p = find_patient_by_username(g_currentUser.username);
    return p ? p->patient_id : "";
}

/* ─── 字段编辑对话框（给个人信息用） ──────────────────────────────── */

#define PF_FIELDS_MAX 8

typedef struct {
    char label[32];
    char value[256];
    int  max_len;
    BOOL read_only;
} PatFieldDef;

static int g_pfResult = 0;

static LRESULT CALLBACK PatFieldDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCT *cs = (CREATESTRUCT *)lParam;
        SetWindowLongPtrA(hDlg, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        PatFieldDef *fields = (PatFieldDef *)cs->lpCreateParams;
        int count = 0;
        while (count < PF_FIELDS_MAX && fields[count].label[0]) count++;
        RECT rc;
        GetClientRect(hDlg, &rc);
        int w = rc.right - rc.left, y = 15;
        for (int i = 0; i < count; i++) {
            CreateWindowA("STATIC", fields[i].label,
                WS_VISIBLE|WS_CHILD|SS_RIGHT, 10, y+2, 80, 20,
                hDlg, NULL, g_hInst, NULL);
            DWORD style = WS_VISIBLE|WS_CHILD|WS_BORDER|ES_AUTOHSCROLL;
            if (fields[i].read_only) style |= ES_READONLY;
            HWND hEdit = CreateWindowA("EDIT", fields[i].value, style,
                100, y, w - 120, 22, hDlg, (HMENU)(2000+i), g_hInst, NULL);
            SendMessageA(hEdit, EM_SETLIMITTEXT, fields[i].max_len-1, 0);
            y += 30;
        }
        y += 10;
        CreateWindowA("BUTTON", "确定", WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON,
            w/2-95, y, 80, 28, hDlg, (HMENU)1, g_hInst, NULL);
        CreateWindowA("BUTTON", "取消", WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON,
            w/2+15, y, 80, 28, hDlg, (HMENU)2, g_hInst, NULL);
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == 1) {
            PatFieldDef *fields = (PatFieldDef *)GetWindowLongPtrA(hDlg, GWLP_USERDATA);
            if (fields) {
                for (int i = 0; i < PF_FIELDS_MAX && fields[i].label[0]; i++)
                    GetDlgItemTextA(hDlg, 2000+i, fields[i].value, fields[i].max_len);
            }
            g_pfResult = 1; DestroyWindow(hDlg);
        } else if (id == 2) {
            g_pfResult = 0; DestroyWindow(hDlg);
        }
        return 0;
    }
    case WM_CLOSE:
        g_pfResult = 0; DestroyWindow(hDlg); return 0;
    default:
        return DefWindowProcA(hDlg, msg, wParam, lParam);
    }
}

static int ShowPatFieldDialog(HWND hParent, const char *title,
                              PatFieldDef *fields, int field_count) {
    if (field_count <= 0 || field_count > PF_FIELDS_MAX) return 0;
    fields[field_count].label[0] = 0;
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = PatFieldDlgProc;
    wc.hInstance = g_hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszClassName = "PatFieldDialog";
    RegisterClassA(&wc);
    int w = 400, h = 50 + field_count * 30 + 55;
    HWND hDlg = CreateWindowExA(0, "PatFieldDialog", title,
        WS_VISIBLE|WS_POPUPWINDOW|WS_CAPTION|WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, w, h,
        hParent, NULL, g_hInst, (LPVOID)fields);
    if (!hDlg) return 0;
    RECT pr, rc;
    GetWindowRect(hParent, &pr);
    GetWindowRect(hDlg, &rc);
    SetWindowPos(hDlg, NULL,
        pr.left+(pr.right-pr.left-(rc.right-rc.left))/2,
        pr.top+(pr.bottom-pr.top-(rc.bottom-rc.top))/2,
        0, 0, SWP_NOSIZE|SWP_NOZORDER);
    EnableWindow(hParent, FALSE);
    g_pfResult = -1;
    MSG msg;
    while (g_pfResult == -1 && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    EnableWindow(hParent, TRUE);
    SetForegroundWindow(hParent);
    return g_pfResult == 1;
}

/* 检查 dateStr(YYYY-MM-DD) 是否在 [today, today+maxDays] 范围内 */
static int is_date_in_range(const char *dateStr, int maxDays) {
    time_t now = time(NULL);
    struct tm tmNow;
    memcpy(&tmNow, localtime(&now), sizeof(tmNow));

    struct tm tmTarget = {0};
    if (sscanf(dateStr, "%d-%d-%d", &tmTarget.tm_year, &tmTarget.tm_mon, &tmTarget.tm_mday) != 3)
        return 0;
    tmTarget.tm_year -= 1900;
    tmTarget.tm_mon  -= 1;
    tmTarget.tm_hour = 12; /* midday to avoid DST edge issues */
    time_t tTarget = mktime(&tmTarget);
    if (tTarget == (time_t)-1) return 0;

    /* 当天 00:00 */
    tmNow.tm_hour = 0; tmNow.tm_min = 0; tmNow.tm_sec = 0;
    time_t tNow = mktime(&tmNow);

    if (tTarget < tNow) return 0; /* 不能在过去 */
    if (tTarget > tNow + maxDays * 86400LL) return 0; /* 超出最大天数 */
    return 1;
}

/* 检查医生在指定日期时段是否有排班 */
static int doctor_has_schedule(const char *docId, const char *date, const char *timeSlot) {
    ScheduleNode *sched = load_schedules_list();
    if (!sched) return 1; /* 无排班数据则放行 */
    int found = 0;
    for (ScheduleNode *cur = sched; cur; cur = cur->next) {
        if (strcmp(cur->data.doctor_id, docId) == 0 &&
            strcmp(cur->data.work_date, date) == 0 &&
            strcmp(cur->data.time_slot, timeSlot) == 0 &&
            strcmp(cur->data.status, "正常") == 0) {
            found = 1;
            break;
        }
    }
    free_schedule_list(sched);
    return found;
}

/* ─── 挂号对话框 ──────────────────────────────────────────────────── */

#define IDC_REG_DEPT    3101
#define IDC_REG_DOCTOR  3102
#define IDC_REG_DATE    3103
#define IDC_REG_TIME    3104
#define IDC_REG_OK      3105
#define IDC_REG_STATUS  3106

static int g_regResult = 0;

static LRESULT CALLBACK RegDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        int y = 15;
        CreateWindowA("STATIC", "选择科室:", WS_VISIBLE|WS_CHILD,
            20, y+2, 80, 20, hDlg, NULL, g_hInst, NULL);
        HWND hDept = CreateWindowA("COMBOBOX", "",
            WS_VISIBLE|WS_CHILD|CBS_DROPDOWNLIST|WS_VSCROLL|CBS_HASSTRINGS,
            110, y, 250, 200, hDlg, (HMENU)IDC_REG_DEPT, g_hInst, NULL);

        DepartmentNode *depts = load_departments_list();
        if (depts) {
            for (DepartmentNode *d = depts; d; d = d->next)
                SendMessageA(hDept, CB_ADDSTRING, 0, (LPARAM)d->data.name);
            SendMessage(hDept, CB_SETCURSEL, 0, 0);
            free_department_list(depts);
        }
        y += 30;

        CreateWindowA("STATIC", "选择医生:", WS_VISIBLE|WS_CHILD,
            20, y+2, 80, 20, hDlg, NULL, g_hInst, NULL);
        HWND hDoc = CreateWindowA("COMBOBOX", "",
            WS_VISIBLE|WS_CHILD|CBS_DROPDOWNLIST|WS_VSCROLL|CBS_HASSTRINGS,
            110, y, 250, 200, hDlg, (HMENU)IDC_REG_DOCTOR, g_hInst, NULL);

        DoctorNode *docs = load_doctors_list();
        if (docs) {
            for (DoctorNode *d = docs; d; d = d->next) {
                char label[150];
                snprintf(label, sizeof(label), "%s - %s", d->data.name, d->data.title);
                SendMessageA(hDoc, CB_ADDSTRING, 0, (LPARAM)label);
            }
            SendMessage(hDoc, CB_SETCURSEL, 0, 0);
            free_doctor_list(docs);
        }
        y += 30;

        CreateWindowA("STATIC", "日期:", WS_VISIBLE|WS_CHILD,
            20, y+2, 80, 20, hDlg, NULL, g_hInst, NULL);
        HWND hDate = CreateWindowA(DATETIMEPICK_CLASSA, "",
            WS_VISIBLE|WS_CHILD|DTS_SHORTDATEFORMAT,
            110, y, 150, 22, hDlg, (HMENU)IDC_REG_DATE, g_hInst, NULL);
        /* 设置可选范围：今天 ~ 今天+7天 */
        {
            SYSTEMTIME stRange[2];
            memset(&stRange, 0, sizeof(stRange));
            SYSTEMTIME stNow;
            GetLocalTime(&stNow);
            stRange[0].wYear = stNow.wYear;
            stRange[0].wMonth = stNow.wMonth;
            stRange[0].wDay = stNow.wDay;
            time_t t = time(NULL) + 7 * 86400;
            struct tm *tmMax = localtime(&t);
            stRange[1].wYear = tmMax->tm_year + 1900;
            stRange[1].wMonth = tmMax->tm_mon + 1;
            stRange[1].wDay = tmMax->tm_mday;
            SendMessage(hDate, DTM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)stRange);
        }

        CreateWindowA("STATIC", "时段:", WS_VISIBLE|WS_CHILD,
            20, y+2, 80, 20, hDlg, NULL, g_hInst, NULL);
        HWND hTime = CreateWindowA("COMBOBOX", "",
            WS_VISIBLE|WS_CHILD|CBS_DROPDOWNLIST|WS_VSCROLL|CBS_HASSTRINGS,
            110, y, 150, 100, hDlg, (HMENU)IDC_REG_TIME, g_hInst, NULL);
        SendMessageA(hTime, CB_ADDSTRING, 0, (LPARAM)"08:00-09:00");
        SendMessageA(hTime, CB_ADDSTRING, 0, (LPARAM)"09:00-10:00");
        SendMessageA(hTime, CB_ADDSTRING, 0, (LPARAM)"10:00-11:00");
        SendMessageA(hTime, CB_ADDSTRING, 0, (LPARAM)"11:00-12:00");
        SendMessageA(hTime, CB_ADDSTRING, 0, (LPARAM)"14:00-15:00");
        SendMessageA(hTime, CB_ADDSTRING, 0, (LPARAM)"15:00-16:00");
        SendMessageA(hTime, CB_ADDSTRING, 0, (LPARAM)"16:00-17:00");
        SendMessage(hTime, CB_SETCURSEL, 0, 0);
        y += 35;

        CreateWindowA("BUTTON", "确认挂号", WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON,
            50, y, 120, 30, hDlg, (HMENU)IDC_REG_OK, g_hInst, NULL);
        CreateWindowA("STATIC", "", WS_VISIBLE|WS_CHILD|SS_CENTER,
            50, y+35, 300, 20, hDlg, (HMENU)IDC_REG_STATUS, g_hInst, NULL);
        return 0;
    }

    case WM_COMMAND: {
        if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_REG_DEPT) {
            HWND hDept = GetDlgItem(hDlg, IDC_REG_DEPT);
            HWND hDoc  = GetDlgItem(hDlg, IDC_REG_DOCTOR);
            int sel = (int)SendMessage(hDept, CB_GETCURSEL, 0, 0);
            if (sel == CB_ERR) return 0;
            char deptName[100] = {0};
            SendMessageA(hDept, CB_GETLBTEXT, (WPARAM)sel, (LPARAM)deptName);

            char deptId[20] = {0};
            DepartmentNode *depts = load_departments_list();
            for (DepartmentNode *d = depts; d; d = d->next) {
                if (strcmp(d->data.name, deptName) == 0) {
                    strcpy(deptId, d->data.department_id);
                    break;
                }
            }
            free_department_list(depts);

            SendMessage(hDoc, CB_RESETCONTENT, 0, 0);
            if (strlen(deptId) > 0) {
                DoctorNode *docs = load_doctors_list();
                int idx = 0;
                for (DoctorNode *d = docs; d; d = d->next) {
                    if (strcmp(d->data.department_id, deptId) == 0) {
                        char label[150];
                        snprintf(label, sizeof(label), "%s - %s", d->data.name, d->data.title);
                        SendMessageA(hDoc, CB_ADDSTRING, 0, (LPARAM)label);
                        idx++;
                    }
                }
                free_doctor_list(docs);
            }
            SendMessage(hDoc, CB_SETCURSEL, 0, 0);
            return 0;
        }

        if (LOWORD(wParam) == IDC_REG_OK) {
            HWND hDept = GetDlgItem(hDlg, IDC_REG_DEPT);
            HWND hDoc  = GetDlgItem(hDlg, IDC_REG_DOCTOR);

            int deptSel = (int)SendMessage(hDept, CB_GETCURSEL, 0, 0);
            int docSel  = (int)SendMessage(hDoc, CB_GETCURSEL, 0, 0);
            if (deptSel == CB_ERR || docSel == CB_ERR) {
                SetDlgItemTextA(hDlg, IDC_REG_STATUS, "请选择科室和医生");
                return 0;
            }

            /* 从标签中提取科室名、医生名、时段 */
            char deptName[100] = {0};
            SendMessageA(hDept, CB_GETLBTEXT, (WPARAM)deptSel, (LPARAM)deptName);
            char docLabel[150] = {0};
            SendMessageA(hDoc, CB_GETLBTEXT, (WPARAM)docSel, (LPARAM)docLabel);
            char timeSlot[10] = {0};
            HWND hTime = GetDlgItem(hDlg, IDC_REG_TIME);
            int timeSel = (int)SendMessage(hTime, CB_GETCURSEL, 0, 0);
            SendMessageA(hTime, CB_GETLBTEXT, (WPARAM)timeSel, (LPARAM)timeSlot);

            /* ── 从日期控件获取日期 ── */
            SYSTEMTIME st;
            memset(&st, 0, sizeof(st));
            SendMessage(GetDlgItem(hDlg, IDC_REG_DATE), DTM_GETSYSTEMTIME, 0, (LPARAM)&st);
            char dateStr[20] = {0};
            snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);

            if (!is_date_in_range(dateStr, 7)) {
                SetDlgItemTextA(hDlg, IDC_REG_STATUS, "日期须为今天起 7 天内");
                return 0;
            }

            /* ── 排班校验 ── */
            /* 找医生 ID（先取 deptId 用于排班检查） */
            /* 根据科室名找科室 ID */
            char deptId[20] = {0};
            DepartmentNode *depts = load_departments_list();
            for (DepartmentNode *d = depts; d; d = d->next) {
                if (strcmp(d->data.name, deptName) == 0) {
                    strcpy(deptId, d->data.department_id);
                    break;
                }
            }
            free_department_list(depts);

            /* 找医生 ID：从 docLabel 中取名字部分 */
            char docName[100] = {0};
            char *dash = strstr(docLabel, " - ");
            if (dash) {
                size_t len = dash - docLabel;
                if (len >= sizeof(docName)) len = sizeof(docName)-1;
                memcpy(docName, docLabel, len);
                docName[len] = 0;
            } else {
                strcpy(docName, docLabel);
            }

            char docId[20] = {0};
            DoctorNode *docs = load_doctors_list();
            for (DoctorNode *d = docs; d; d = d->next) {
                if (strcmp(d->data.name, docName) == 0) {
                    strcpy(docId, d->data.doctor_id);
                    /* 更新医生繁忙度 */
                    if (d->data.busy_level < 10) d->data.busy_level++;
                    break;
                }
            }
            save_doctors_list(docs);
            free_doctor_list(docs);

            if (strlen(docId) == 0) {
                SetDlgItemTextA(hDlg, IDC_REG_STATUS, "未找到医生信息");
                return 0;
            }

            if (!doctor_has_schedule(docId, dateStr, timeSlot)) {
                SetDlgItemTextA(hDlg, IDC_REG_STATUS, "该医生此时段无排班");
                return 0;
            }

            /* 创建预约 */
            Appointment apt;
            memset(&apt, 0, sizeof(apt));
            generate_id(apt.appointment_id, sizeof(apt.appointment_id), "APT");
            strcpy(apt.patient_id, GetPatientId());
            strcpy(apt.doctor_id, docId);
            strcpy(apt.department_id, deptId);
            strcpy(apt.appointment_date, dateStr);
            strcpy(apt.appointment_time, timeSlot);
            strcpy(apt.status, "待就诊");
            get_current_time(apt.create_time, sizeof(apt.create_time));

            AppointmentNode *head = load_appointments_list();
            AppointmentNode *node = create_appointment_node(&apt);
            node->next = head;
            save_appointments_list(node);
            free_appointment_list(node);

            g_regResult = 1;
            DestroyWindow(hDlg);
            return 0;
        }
        return 0;
    }
    case WM_CLOSE:
        g_regResult = 0;
        DestroyWindow(hDlg);
        return 0;
    default:
        return DefWindowProcA(hDlg, msg, wParam, lParam);
    }
}

static int ShowRegDialog(HWND hParent) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = RegDlgProc;
    wc.hInstance = g_hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszClassName = "PatientRegDialog";
    RegisterClassA(&wc);

    HWND hDlg = CreateWindowExA(0, "PatientRegDialog", "预约挂号",
        WS_VISIBLE|WS_POPUPWINDOW|WS_CAPTION|WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 260,
        hParent, NULL, g_hInst, NULL);
    if (!hDlg) return 0;

    RECT pr, rc;
    GetWindowRect(hParent, &pr);
    GetWindowRect(hDlg, &rc);
    SetWindowPos(hDlg, NULL,
        pr.left+(pr.right-pr.left-(rc.right-rc.left))/2,
        pr.top+(pr.bottom-pr.top-(rc.bottom-rc.top))/2,
        0, 0, SWP_NOSIZE|SWP_NOZORDER);

    EnableWindow(hParent, FALSE);
    g_regResult = -1;
    MSG msg;
    while (g_regResult == -1 && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    EnableWindow(hParent, TRUE);
    SetForegroundWindow(hParent);
    return g_regResult == 1;
}

/* ─── 页面窗口过程 ─────────────────────────────────────────────────── */

static LRESULT CALLBACK PatientPageWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    int viewId = (int)GetWindowLongPtrA(hWnd, GWLP_USERDATA);

    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCT *cs = (CREATESTRUCT *)lParam;
        SetWindowLongPtrA(hWnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        return 0;
    }
    case WM_SIZE: {
        int cx = LOWORD(lParam), cy = HIWORD(lParam);
        HWND hChild = GetWindow(hWnd, GW_CHILD);
        while (hChild) {
            SetWindowPos(hChild, NULL, 0, 0, cx, cy - 40, SWP_NOZORDER | SWP_NOMOVE);
            hChild = GetWindow(hChild, GW_HWNDNEXT);
        }
        return 0;
    }
    case WM_COMMAND:
        if (HIWORD(wParam) != BN_CLICKED) return 0;

        switch (LOWORD(wParam)) {

        case 1010: { /* 挂号（从挂号页面发起） */
            if (ShowRegDialog(hWnd)) {
                MessageBoxA(hWnd, "挂号成功！请前往「预约查询」查看。", "成功", MB_OK | MB_ICONINFORMATION);
            }
            return 0;
        }

        case 1002: { /* 取消预约（预约查询页面） */
            HWND hLV = GetDlgItem(hWnd, 2001);
            int sel = ListView_GetNextItem(hLV, -1, LVNI_SELECTED);
            if (sel < 0) {
                MessageBoxA(hWnd, "请先选择一个预约", "提示", MB_OK);
                return 0;
            }
            char apptId[20] = {0};
            ListView_GetItemText(hLV, sel, 0, apptId, sizeof(apptId));
            if (MessageBoxA(hWnd, "确定要取消该预约吗？", "确认",
                            MB_YESNO | MB_ICONQUESTION) == IDYES) {
                AppointmentNode *apps = load_appointments_list();
                for (AppointmentNode *cur = apps; cur; cur = cur->next) {
                    if (strcmp(cur->data.appointment_id, apptId) == 0) {
                        strcpy(cur->data.status, "已取消");
                        break;
                    }
                }
                save_appointments_list(apps);
                free_appointment_list(apps);
                MessageBoxA(hWnd, "预约已取消", "成功", MB_OK);
                /* 刷新：重新创建页面 */
                PostMessage(GetParent(hWnd), WM_APP + 1, NAV_PATIENT_APPOINTMENT, 0);
            }
            return 0;
        }

        case 1020: { /* 编辑个人信息 */
            const char *pid = GetPatientId();
            if (strlen(pid) == 0) {
                MessageBoxA(hWnd, "未找到患者信息", "错误", MB_OK);
                return 0;
            }
            Patient *p = find_patient_by_id(pid);
            if (!p) {
                MessageBoxA(hWnd, "未找到患者信息", "错误", MB_OK);
                return 0;
            }

            char ageStr[16];
            snprintf(ageStr, sizeof(ageStr), "%d", p->age);
            PatFieldDef f[7] = {
                {"姓名", "", MAX_NAME, FALSE},
                {"性别", "", 10, FALSE},
                {"年龄", "", 16, FALSE},
                {"电话", "", 20, FALSE},
                {"地址", "", 200, FALSE},
                {"患者类型", "", 20, TRUE},
                {"治疗阶段", "", 20, TRUE},
            };
            strcpy(f[0].value, p->name);
            strcpy(f[1].value, p->gender);
            strcpy(f[2].value, ageStr);
            strcpy(f[3].value, p->phone);
            strcpy(f[4].value, p->address);
            strcpy(f[5].value, p->patient_type);
            strcpy(f[6].value, p->treatment_stage);

            if (ShowPatFieldDialog(hWnd, "编辑个人信息", f, 7)) {
                PatientNode *head = load_patients_list();
                for (PatientNode *cur = head; cur; cur = cur->next) {
                    if (strcmp(cur->data.patient_id, pid) == 0) {
                        strcpy(cur->data.name, f[0].value);
                        strcpy(cur->data.gender, f[1].value);
                        cur->data.age = atoi(f[2].value);
                        strcpy(cur->data.phone, f[3].value);
                        strcpy(cur->data.address, f[4].value);
                        break;
                    }
                }
                save_patients_list(head);
                free_patient_list(head);
                MessageBoxA(hWnd, "个人信息已更新", "成功", MB_OK);
                PostMessage(GetParent(hWnd), WM_APP + 1, NAV_PATIENT_PROFILE, 0);
            }
            return 0;
        }
        }
        return 0;

    case WM_NOTIFY: {
        if (viewId == NAV_PATIENT_DIAGNOSIS) {
            NMHDR *nm = (NMHDR *)lParam;
            if (nm->idFrom == 2002 && nm->code == LVN_ITEMCHANGED) {
                NMLISTVIEW *nmlv = (NMLISTVIEW *)lParam;
                if ((nmlv->uChanged & LVIF_STATE) &&
                    (nmlv->uNewState & LVIS_SELECTED) &&
                    !(nmlv->uOldState & LVIS_SELECTED)) {
                    char recId[20] = "";
                    ListView_GetItemText(nm->hwndFrom, nmlv->iItem, 0, recId, sizeof(recId));
                    if (recId[0] == 0) return 0;

                    MedicalRecordNode *recs = load_medical_records_list();
                    if (recs) {
                        MedicalRecordNode *cur = recs;
                        while (cur) {
                            if (strcmp(cur->data.record_id, recId) == 0) {
                                char buf[2048];
                                snprintf(buf, sizeof(buf),
                                    "诊断: %s\n\n"
                                    "医生: %s\n"
                                    "日期: %s\n"
                                    "状态: %s",
                                    cur->data.diagnosis,
                                    cur->data.doctor_id,
                                    cur->data.diagnosis_date,
                                    cur->data.status);
                                HWND hEdit = GetDlgItem(hWnd, 3000);
                                if (hEdit) SetWindowTextA(hEdit, buf);
                                break;
                            }
                            cur = cur->next;
                        }
                        free_medical_record_list(recs);
                    }
                }
            }
        }
        return 0;
    }

    default:
        return DefWindowProcA(hWnd, msg, wParam, lParam);
    }
}

/* ─── 挂号页面 ─────────────────────────────────────────────────────── */

static HWND CreateRegisterPage(HWND hParent, RECT *rc) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = PatientPageWndProc;
    wc.hInstance     = g_hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "PatientRegisterPage";
    RegisterClassA(&wc);

    HWND hPage = CreateWindowExA(0, "PatientRegisterPage", "",
        WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
        rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top,
        hParent, NULL, g_hInst, (LPVOID)(INT_PTR)NAV_PATIENT_REGISTER);
    if (!hPage) return NULL;

    int w = (rc->right - rc->left) - 40;
    int y = 20;

    CreateWindowA("STATIC", "预约挂号",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        20, y, 200, 25, hPage, NULL, g_hInst, NULL);
    y += 30;

    CreateWindowA("STATIC",
        "点击下方按钮选择科室和医生进行预约挂号",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        20, y, 400, 20, hPage, NULL, g_hInst, NULL);
    y += 35;

    CreateWindowA("BUTTON", "开始挂号",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        20, y, 120, 35, hPage, (HMENU)1010, g_hInst, NULL);

    return hPage;
}

/* ─── 预约查询页面 ─────────────────────────────────────────────────── */

static HWND CreateAppointmentPage(HWND hParent, RECT *rc) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = PatientPageWndProc;
    wc.hInstance     = g_hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "PatientApptPage";
    RegisterClassA(&wc);

    HWND hPage = CreateWindowExA(0, "PatientApptPage", "",
        WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
        rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top,
        hParent, NULL, g_hInst, (LPVOID)(INT_PTR)NAV_PATIENT_APPOINTMENT);
    if (!hPage) return NULL;

    int w = (rc->right - rc->left) - 10;
    int h = (rc->bottom - rc->top) - 10;

    HWND hLV = CreateListView(hPage, 2001, 5, 5, w, h - 50);
    AddCol(hLV, 0, "预约ID", 100);
    AddCol(hLV, 1, "医生", 80);
    AddCol(hLV, 2, "日期", 100);
    AddCol(hLV, 3, "时段", 60);
    AddCol(hLV, 4, "状态", 60);

    const char *pid = GetPatientId();
    AppointmentNode *apps = load_appointments_list();
    int row = 0;
    if (apps && strlen(pid) > 0) {
        AppointmentNode *cur = apps;
        while (cur) {
            if (strcmp(cur->data.patient_id, pid) == 0) {
                const char *items[5] = {
                    cur->data.appointment_id,
                    cur->data.doctor_id,
                    cur->data.appointment_date,
                    cur->data.appointment_time,
                    cur->data.status
                };
                AddRow(hLV, row++, 5, items);
            }
            cur = cur->next;
        }
    }
    free_appointment_list(apps);

    CreateWindowA("BUTTON", "取消预约",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        w - 100, h - 40, 90, 30,
        hPage, (HMENU)1002, g_hInst, NULL);

    return hPage;
}

/* ─── 诊断结果页面 ─────────────────────────────────────────────────── */

static HWND CreateDiagnosisPage(HWND hParent, RECT *rc) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = PatientPageWndProc;
    wc.hInstance     = g_hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "PatientDiagPage";
    RegisterClassA(&wc);

    HWND hPage = CreateWindowExA(0, "PatientDiagPage", "",
        WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
        rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top,
        hParent, NULL, g_hInst, (LPVOID)(INT_PTR)NAV_PATIENT_DIAGNOSIS);
    if (!hPage) return NULL;

    int w = (rc->right - rc->left) - 10;

    HWND hLV = CreateListView(hPage, 2002, 5, 5, w - 10, 150);
    AddCol(hLV, 0, "病历ID", 100);
    AddCol(hLV, 1, "医生", 80);
    AddCol(hLV, 2, "日期", 100);
    AddCol(hLV, 3, "状态", 60);

    const char *pid = GetPatientId();
    MedicalRecordNode *recs = load_medical_records_list();
    int row = 0;
    if (recs && strlen(pid) > 0) {
        MedicalRecordNode *cur = recs;
        while (cur) {
            if (strcmp(cur->data.patient_id, pid) == 0) {
                const char *items[4] = {
                    cur->data.record_id,
                    cur->data.doctor_id,
                    cur->data.diagnosis_date,
                    cur->data.status
                };
                AddRow(hLV, row++, 4, items);
            }
            cur = cur->next;
        }
    }
    free_medical_record_list(recs);

    CreateWindowA("STATIC", "诊断详情:",
        WS_VISIBLE | WS_CHILD, 5, 165, 100, 20,
        hPage, NULL, g_hInst, NULL);
    HWND hDiagEdit = CreateWindowA("EDIT", "",
        WS_VISIBLE | WS_CHILD | ES_MULTILINE | ES_READONLY |
        WS_BORDER | WS_VSCROLL,
        5, 190, w - 10, (rc->bottom - rc->top) - 200,
        hPage, (HMENU)3000, g_hInst, NULL);
    (void)hDiagEdit;

    return hPage;
}

/* ─── 处方查询页面 ─────────────────────────────────────────────────── */

static HWND CreatePrescriptionPage(HWND hParent, RECT *rc) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = PatientPageWndProc;
    wc.hInstance     = g_hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "PatientRxPage";
    RegisterClassA(&wc);

    HWND hPage = CreateWindowExA(0, "PatientRxPage", "",
        WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
        rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top,
        hParent, NULL, g_hInst, (LPVOID)(INT_PTR)NAV_PATIENT_PRESCRIPTION);
    if (!hPage) return NULL;

    int w = (rc->right - rc->left) - 10;

    HWND hLV = CreateListView(hPage, 2003, 5, 5, w - 10,
                              (rc->bottom - rc->top) - 15);
    AddCol(hLV, 0, "处方ID", 100);
    AddCol(hLV, 1, "药品", 160);
    AddCol(hLV, 2, "数量", 60);
    AddCol(hLV, 3, "金额", 80);
    AddCol(hLV, 4, "日期", 100);

    const char *pid = GetPatientId();
    PrescriptionNode *rxs = load_prescriptions_list();
    int row = 0;
    if (rxs && strlen(pid) > 0) {
        PrescriptionNode *cur = rxs;
        while (cur) {
            if (strcmp(cur->data.patient_id, pid) == 0) {
                char priceStr[32], qtyStr[16];
                snprintf(priceStr, sizeof(priceStr), "%.2f", cur->data.total_price);
                snprintf(qtyStr, sizeof(qtyStr), "%d", cur->data.quantity);
                Drug *drug = find_drug_by_id(cur->data.drug_id);
                const char *drugName = drug ? drug->name : cur->data.drug_id;
                const char *items[5] = {
                    cur->data.prescription_id,
                    drugName,
                    qtyStr,
                    priceStr,
                    cur->data.prescription_date
                };
                AddRow(hLV, row++, 5, items);
                if (drug) free(drug);
            }
            cur = cur->next;
        }
    }
    free_prescription_list(rxs);

    return hPage;
}

/* ─── 住院信息页面 ─────────────────────────────────────────────────── */

static HWND CreateWardPage(HWND hParent, RECT *rc) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = PatientPageWndProc;
    wc.hInstance     = g_hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "PatientWardPage";
    RegisterClassA(&wc);

    HWND hPage = CreateWindowExA(0, "PatientWardPage", "",
        WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
        rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top,
        hParent, NULL, g_hInst, (LPVOID)(INT_PTR)NAV_PATIENT_WARD);
    if (!hPage) return NULL;

    int w = (rc->right - rc->left) - 10;

    HWND hLV = CreateListView(hPage, 2004, 5, 5, w - 10,
                              (rc->bottom - rc->top) - 15);
    AddCol(hLV, 0, "病房ID", 80);
    AddCol(hLV, 1, "类型", 100);
    AddCol(hLV, 2, "总床位", 60);
    AddCol(hLV, 3, "剩余床位", 60);

    WardNode *wards = load_wards_list();
    int row = 0;
    if (wards) {
        WardNode *cur = wards;
        while (cur) {
            char totalStr[16], remainStr[16];
            snprintf(totalStr, sizeof(totalStr), "%d", cur->data.total_beds);
            snprintf(remainStr, sizeof(remainStr), "%d", cur->data.remain_beds);
            const char *items[4] = {
                cur->data.ward_id, cur->data.type, totalStr, remainStr
            };
            AddRow(hLV, row++, 4, items);
            cur = cur->next;
        }
        free_ward_list(wards);
    }
    return hPage;
}

/* ─── 治疗进度页面 ─────────────────────────────────────────────────── */

static HWND CreateProgressPage(HWND hParent, RECT *rc) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = PatientPageWndProc;
    wc.hInstance     = g_hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "PatientProgPage";
    RegisterClassA(&wc);

    HWND hPage = CreateWindowExA(0, "PatientProgPage", "",
        WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
        rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top,
        hParent, NULL, g_hInst, (LPVOID)(INT_PTR)NAV_PATIENT_PROGRESS);
    if (!hPage) return NULL;

    const char *pid = GetPatientId();
    Patient *p = NULL;
    if (strlen(pid) > 0) p = find_patient_by_id(pid);

    char info[512];
    if (p) {
        snprintf(info, sizeof(info),
            "患者: %s\n"
            "治疗阶段: %s\n"
            "紧急状态: %s\n\n"
            "下一阶段请咨询主治医生。",
            p->name,
            p->treatment_stage,
            p->is_emergency ? "是" : "否");
    } else {
        snprintf(info, sizeof(info), "未找到患者信息");
    }

    CreateWindowA("STATIC", info,
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        20, 20, 400, 200, hPage, NULL, g_hInst, NULL);

    return hPage;
}

/* ─── 个人信息页面 ─────────────────────────────────────────────────── */

static HWND CreateProfilePage(HWND hParent, RECT *rc) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = PatientPageWndProc;
    wc.hInstance     = g_hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "PatientProfilePage";
    RegisterClassA(&wc);

    HWND hPage = CreateWindowExA(0, "PatientProfilePage", "",
        WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
        rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top,
        hParent, NULL, g_hInst, (LPVOID)(INT_PTR)NAV_PATIENT_PROFILE);
    if (!hPage) return NULL;

    const char *pid = GetPatientId();
    Patient *p = (strlen(pid) > 0) ? find_patient_by_id(pid) : NULL;

    int y = 20;
    char buf[256];
    if (p) {
        snprintf(buf, sizeof(buf), "姓名: %s", p->name);
        CreateWindowA("STATIC", buf, WS_VISIBLE | WS_CHILD | SS_LEFT,
                      20, y, 300, 20, hPage, NULL, g_hInst, NULL);
        y += 28;

        snprintf(buf, sizeof(buf), "性别: %s", p->gender);
        CreateWindowA("STATIC", buf, WS_VISIBLE | WS_CHILD | SS_LEFT,
                      20, y, 300, 20, hPage, NULL, g_hInst, NULL);
        y += 28;

        snprintf(buf, sizeof(buf), "年龄: %d", p->age);
        CreateWindowA("STATIC", buf, WS_VISIBLE | WS_CHILD | SS_LEFT,
                      20, y, 300, 20, hPage, NULL, g_hInst, NULL);
        y += 28;

        snprintf(buf, sizeof(buf), "电话: %s", p->phone);
        CreateWindowA("STATIC", buf, WS_VISIBLE | WS_CHILD | SS_LEFT,
                      20, y, 300, 20, hPage, NULL, g_hInst, NULL);
        y += 28;

        snprintf(buf, sizeof(buf), "地址: %s", p->address);
        CreateWindowA("STATIC", buf, WS_VISIBLE | WS_CHILD | SS_LEFT,
                      20, y, 500, 20, hPage, NULL, g_hInst, NULL);
        y += 28;

        snprintf(buf, sizeof(buf), "患者类型: %s", p->patient_type);
        CreateWindowA("STATIC", buf, WS_VISIBLE | WS_CHILD | SS_LEFT,
                      20, y, 300, 20, hPage, NULL, g_hInst, NULL);
        y += 28;

        snprintf(buf, sizeof(buf), "治疗阶段: %s", p->treatment_stage);
        CreateWindowA("STATIC", buf, WS_VISIBLE | WS_CHILD | SS_LEFT,
                      20, y, 300, 20, hPage, NULL, g_hInst, NULL);
        y += 35;

        CreateWindowA("BUTTON", "编辑资料",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            20, y, 90, 30, hPage, (HMENU)1020, g_hInst, NULL);
    } else {
        CreateWindowA("STATIC", "未找到患者信息",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            20, 20, 200, 20, hPage, NULL, g_hInst, NULL);
    }
    return hPage;
}

/* ─── 公开接口 ─────────────────────────────────────────────────────── */

HWND CreatePatientPage(HWND hParent, int viewId, RECT *rc) {
    switch (viewId) {
    case NAV_PATIENT_REGISTER:     return CreateRegisterPage(hParent, rc);
    case NAV_PATIENT_APPOINTMENT:  return CreateAppointmentPage(hParent, rc);
    case NAV_PATIENT_DIAGNOSIS:    return CreateDiagnosisPage(hParent, rc);
    case NAV_PATIENT_PRESCRIPTION: return CreatePrescriptionPage(hParent, rc);
    case NAV_PATIENT_WARD:         return CreateWardPage(hParent, rc);
    case NAV_PATIENT_PROGRESS:     return CreateProgressPage(hParent, rc);
    case NAV_PATIENT_PROFILE:      return CreateProfilePage(hParent, rc);
    default: return NULL;
    }
}
