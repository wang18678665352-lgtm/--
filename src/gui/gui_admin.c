#include "gui_admin.h"
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

static void ClearLV(HWND hLV) {
    ListView_DeleteAllItems(hLV);
}

/* ─── 字段编辑对话框 ───────────────────────────────────────────────── */

#define FD_FIELDS_MAX  16

typedef struct {
    char label[32];
    char value[256];
    int  max_len;
    BOOL read_only;
} FieldDef;

static int g_fdResult = 0; /* -1=running, 0=cancel, 1=ok */

static LRESULT CALLBACK FieldDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCT *cs = (CREATESTRUCT *)lParam;
        FieldDef *fields = (FieldDef *)cs->lpCreateParams;
        SetWindowLongPtrA(hDlg, GWLP_USERDATA, (LONG_PTR)fields);

        int count = 0;
        while (count < FD_FIELDS_MAX && fields[count].label[0]) count++;

        RECT rc;
        GetClientRect(hDlg, &rc);
        int w = rc.right - rc.left;
        int y = 15;

        for (int i = 0; i < count; i++) {
            CreateWindowA("STATIC", fields[i].label,
                WS_VISIBLE | WS_CHILD | SS_RIGHT,
                10, y + 2, 80, 20, hDlg, NULL, g_hInst, NULL);
            DWORD style = WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL;
            if (fields[i].read_only) style |= ES_READONLY;
            HWND hEdit = CreateWindowA("EDIT", fields[i].value, style,
                100, y, w - 120, 22, hDlg, (HMENU)(1000 + i), g_hInst, NULL);
            SendMessageA(hEdit, EM_SETLIMITTEXT, fields[i].max_len - 1, 0);
            y += 30;
        }

        y += 10;
        CreateWindowA("BUTTON", "确定", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            w / 2 - 95, y, 80, 28, hDlg, (HMENU)1, g_hInst, NULL);
        CreateWindowA("BUTTON", "取消", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            w / 2 + 15, y, 80, 28, hDlg, (HMENU)2, g_hInst, NULL);
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == 1) {
            FieldDef *fields = (FieldDef *)GetWindowLongPtrA(hDlg, GWLP_USERDATA);
            if (fields) {
                for (int i = 0; i < FD_FIELDS_MAX && fields[i].label[0]; i++)
                    GetDlgItemTextA(hDlg, 1000 + i, fields[i].value, fields[i].max_len);
            }
            g_fdResult = 1;
            DestroyWindow(hDlg);
        } else if (id == 2) {
            g_fdResult = 0;
            DestroyWindow(hDlg);
        }
        return 0;
    }
    case WM_CLOSE:
        g_fdResult = 0;
        DestroyWindow(hDlg);
        return 0;
    default:
        return DefWindowProcA(hDlg, msg, wParam, lParam);
    }
}

static int ShowFieldDialog(HWND hParent, const char *title,
                           FieldDef *fields, int field_count) {
    if (field_count <= 0 || field_count > FD_FIELDS_MAX) return 0;
    fields[field_count].label[0] = 0;

    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = FieldDlgProc;
    wc.hInstance     = g_hInst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "FieldDialog";
    RegisterClassA(&wc);

    int w = 400, h = 50 + field_count * 30 + 55;

    HWND hDlg = CreateWindowExA(0, "FieldDialog", title,
        WS_VISIBLE | WS_POPUPWINDOW | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, w, h,
        hParent, NULL, g_hInst, (LPVOID)fields);
    if (!hDlg) return 0;

    RECT pr, rc;
    GetWindowRect(hParent, &pr);
    GetWindowRect(hDlg, &rc);
    SetWindowPos(hDlg, NULL,
        pr.left + (pr.right - pr.left - (rc.right - rc.left)) / 2,
        pr.top + (pr.bottom - pr.top - (rc.bottom - rc.top)) / 2,
        0, 0, SWP_NOSIZE | SWP_NOZORDER);

    EnableWindow(hParent, FALSE);
    g_fdResult = -1;
    MSG msg;
    while (g_fdResult == -1 && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    EnableWindow(hParent, TRUE);
    SetForegroundWindow(hParent);

    return g_fdResult == 1;
}

/* ─── 刷新各页面 ListView ────────────────────────────────────────── */

static void PopulateDeptList(HWND hLV) {
    ClearLV(hLV);
    DepartmentNode *list = load_departments_list();
    int row = 0;
    for (DepartmentNode *cur = list; cur; cur = cur->next) {
        const char *items[4] = { cur->data.department_id, cur->data.name,
                                 cur->data.leader, cur->data.phone };
        AddRow(hLV, row++, 4, items);
    }
    free_department_list(list);
}

static void PopulateDocList(HWND hLV) {
    ClearLV(hLV);
    DoctorNode *list = load_doctors_list();
    int row = 0;
    for (DoctorNode *cur = list; cur; cur = cur->next) {
        char busy[16];
        snprintf(busy, sizeof(busy), "%d", cur->data.busy_level);
        const char *items[5] = { cur->data.doctor_id, cur->data.name,
                                 cur->data.department_id, cur->data.title, busy };
        AddRow(hLV, row++, 5, items);
    }
    free_doctor_list(list);
}

static void PopulatePatList(HWND hLV) {
    ClearLV(hLV);
    PatientNode *list = load_patients_list();
    int row = 0;
    for (PatientNode *cur = list; cur; cur = cur->next) {
        char age[16];
        snprintf(age, sizeof(age), "%d", cur->data.age);
        const char *items[7] = { cur->data.patient_id, cur->data.name,
            cur->data.gender, age, cur->data.phone,
            cur->data.patient_type, cur->data.treatment_stage };
        AddRow(hLV, row++, 7, items);
    }
    free_patient_list(list);
}

static void PopulateDrugList(HWND hLV) {
    ClearLV(hLV);
    DrugNode *list = load_drugs_list();
    int row = 0;
    for (DrugNode *cur = list; cur; cur = cur->next) {
        char price[16], stock[16], warn[16], ratio[16];
        snprintf(price, sizeof(price), "%.2f", cur->data.price);
        snprintf(stock, sizeof(stock), "%d", cur->data.stock_num);
        snprintf(warn, sizeof(warn), "%d", cur->data.warning_line);
        snprintf(ratio, sizeof(ratio), "%.0f%%", cur->data.reimbursement_ratio * 100);
        const char *items[7] = { cur->data.drug_id, cur->data.name,
            price, stock, warn, ratio, cur->data.category };
        AddRow(hLV, row++, 7, items);
    }
    free_drug_list(list);
}

static void PopulateWardList(HWND hLV) {
    ClearLV(hLV);
    WardNode *list = load_wards_list();
    int row = 0;
    for (WardNode *cur = list; cur; cur = cur->next) {
        char total[16], remain[16], warn[16];
        snprintf(total, sizeof(total), "%d", cur->data.total_beds);
        snprintf(remain, sizeof(remain), "%d", cur->data.remain_beds);
        snprintf(warn, sizeof(warn), "%d", cur->data.warning_line);
        const char *items[5] = { cur->data.ward_id, cur->data.type,
                                 total, remain, warn };
        AddRow(hLV, row++, 5, items);
    }
    free_ward_list(list);
}

static void PopulateSchedList(HWND hLV) {
    ClearLV(hLV);
    ScheduleNode *list = load_schedules_list();
    int row = 0;
    for (ScheduleNode *cur = list; cur; cur = cur->next) {
        const char *items[5] = { cur->data.schedule_id, cur->data.doctor_id,
            cur->data.work_date, cur->data.time_slot, cur->data.status };
        AddRow(hLV, row++, 5, items);
    }
    free_schedule_list(list);
}

/* ─── 读取选中行数据到 FieldDef 数组 ─────────────────────────────── */

static void GetSelectedRow(HWND hLV, int col_count, FieldDef *fields) {
    int sel = ListView_GetNextItem(hLV, -1, LVNI_SELECTED);
    if (sel < 0) return;
    for (int c = 0; c < col_count && c < FD_FIELDS_MAX; c++) {
        ListView_GetItemText(hLV, sel, c, fields[c].value, fields[c].max_len);
    }
}

/* ─── 通用管理页面基类 ─────────────────────────────────────────────── */

static LRESULT CALLBACK AdminPageWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    int viewId = (int)GetWindowLongPtrA(hWnd, GWLP_USERDATA);

    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCT *cs = (CREATESTRUCT *)lParam;
        SetWindowLongPtrA(hWnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        return 0;
    }
    case WM_SIZE:
        return 0;

    case WM_COMMAND: {
        int cmd = LOWORD(wParam);
        HWND hLV;
        int sel;

        switch (viewId) {

        /* ── 科室管理 ─────────────────────────────────────────── */
        case NAV_ADMIN_DEPT:
            hLV = GetDlgItem(hWnd, 4001);
            switch (cmd) {
            case 4101: { /* 新增 */
                FieldDef f[4] = {
                    {"名称", "", MAX_NAME, FALSE},
                    {"负责人", "", MAX_NAME, FALSE},
                    {"电话", "", 20, FALSE},
                };
                if (ShowFieldDialog(hWnd, "新增科室", f, 3)) {
                    Department d;
                    generate_id(d.department_id, sizeof(d.department_id), "DEP");
                    strcpy(d.name, f[0].value);
                    strcpy(d.leader, f[1].value);
                    strcpy(d.phone, f[2].value);
                    DepartmentNode *head = load_departments_list();
                    DepartmentNode *node = create_department_node(&d);
                    node->next = head;
                    save_departments_list(node);
                    free_department_list(node);
                    PopulateDeptList(hLV);
                }
                return 0;
            }
            case 4102: { /* 编辑 */
                sel = ListView_GetNextItem(hLV, -1, LVNI_SELECTED);
                if (sel < 0) { MessageBoxA(hWnd, "请先选择一行", "提示", MB_OK); return 0; }
                FieldDef f[4] = {
                    {"名称", "", MAX_NAME, FALSE},
                    {"负责人", "", MAX_NAME, FALSE},
                    {"电话", "", 20, FALSE},
                };
                GetSelectedRow(hLV, 4, f);
                if (ShowFieldDialog(hWnd, "编辑科室", f, 3)) {
                    DepartmentNode *head = load_departments_list();
                    DepartmentNode *cur = head;
                    char targetId[32];
                    ListView_GetItemText(hLV, sel, 0, targetId, sizeof(targetId));
                    while (cur) {
                        if (strcmp(cur->data.department_id, targetId) == 0) {
                            strcpy(cur->data.name, f[0].value);
                            strcpy(cur->data.leader, f[1].value);
                            strcpy(cur->data.phone, f[2].value);
                            break;
                        }
                        cur = cur->next;
                    }
                    save_departments_list(head);
                    free_department_list(head);
                    PopulateDeptList(hLV);
                }
                return 0;
            }
            case 4103: { /* 删除 */
                sel = ListView_GetNextItem(hLV, -1, LVNI_SELECTED);
                if (sel < 0) { MessageBoxA(hWnd, "请先选择一行", "提示", MB_OK); return 0; }
                if (MessageBoxA(hWnd, "确定要删除该科室吗？", "确认", MB_YESNO | MB_ICONQUESTION) != IDYES) return 0;
                char targetId[32];
                ListView_GetItemText(hLV, sel, 0, targetId, sizeof(targetId));
                DepartmentNode *head = load_departments_list();
                DepartmentNode *prev = NULL, *cur = head;
                while (cur) {
                    if (strcmp(cur->data.department_id, targetId) == 0) {
                        if (prev) prev->next = cur->next;
                        else head = cur->next;
                        free(cur);
                        break;
                    }
                    prev = cur;
                    cur = cur->next;
                }
                save_departments_list(head);
                free_department_list(head);
                PopulateDeptList(hLV);
                return 0;
            }
            }
            return 0;

        /* ── 医生管理 ─────────────────────────────────────────── */
        case NAV_ADMIN_DOCTOR:
            hLV = GetDlgItem(hWnd, 4002);
            switch (cmd) {
            case 4201: { /* 新增 */
                FieldDef f[4] = {
                    {"姓名", "", MAX_NAME, FALSE},
                    {"科室", "", MAX_ID, FALSE},
                    {"职称", "", 50, FALSE},
                    {"繁忙度", "", 16, FALSE},
                };
                if (ShowFieldDialog(hWnd, "新增医生", f, 4)) {
                    Doctor d;
                    generate_id(d.doctor_id, sizeof(d.doctor_id), "DOC");
                    strcpy(d.name, f[0].value);
                    strcpy(d.department_id, f[1].value);
                    strcpy(d.title, f[2].value);
                    d.busy_level = atoi(f[3].value);
                    DoctorNode *head = load_doctors_list();
                    DoctorNode *node = create_doctor_node(&d);
                    node->next = head;
                    save_doctors_list(node);
                    free_doctor_list(node);
                    PopulateDocList(hLV);
                }
                return 0;
            }
            case 4202: { /* 编辑 */
                sel = ListView_GetNextItem(hLV, -1, LVNI_SELECTED);
                if (sel < 0) { MessageBoxA(hWnd, "请先选择一行", "提示", MB_OK); return 0; }
                FieldDef f[4] = {
                    {"姓名", "", MAX_NAME, FALSE},
                    {"科室", "", MAX_ID, FALSE},
                    {"职称", "", 50, FALSE},
                    {"繁忙度", "", 16, FALSE},
                };
                GetSelectedRow(hLV, 5, f);
                if (ShowFieldDialog(hWnd, "编辑医生", f, 4)) {
                    char targetId[32];
                    ListView_GetItemText(hLV, sel, 0, targetId, sizeof(targetId));
                    DoctorNode *head = load_doctors_list();
                    for (DoctorNode *cur = head; cur; cur = cur->next) {
                        if (strcmp(cur->data.doctor_id, targetId) == 0) {
                            strcpy(cur->data.name, f[0].value);
                            strcpy(cur->data.department_id, f[1].value);
                            strcpy(cur->data.title, f[2].value);
                            cur->data.busy_level = atoi(f[3].value);
                            break;
                        }
                    }
                    save_doctors_list(head);
                    free_doctor_list(head);
                    PopulateDocList(hLV);
                }
                return 0;
            }
            case 4203: { /* 删除 */
                sel = ListView_GetNextItem(hLV, -1, LVNI_SELECTED);
                if (sel < 0) { MessageBoxA(hWnd, "请先选择一行", "提示", MB_OK); return 0; }
                if (MessageBoxA(hWnd, "确定要删除该医生吗？", "确认", MB_YESNO | MB_ICONQUESTION) != IDYES) return 0;
                char targetId[32];
                ListView_GetItemText(hLV, sel, 0, targetId, sizeof(targetId));
                DoctorNode *head = load_doctors_list();
                DoctorNode *prev = NULL, *cur = head;
                while (cur) {
                    if (strcmp(cur->data.doctor_id, targetId) == 0) {
                        if (prev) prev->next = cur->next;
                        else head = cur->next;
                        free(cur);
                        break;
                    }
                    prev = cur;
                    cur = cur->next;
                }
                save_doctors_list(head);
                free_doctor_list(head);
                PopulateDocList(hLV);
                return 0;
            }
            }
            return 0;

        /* ── 患者管理 ─────────────────────────────────────────── */
        case NAV_ADMIN_PATIENT:
            hLV = GetDlgItem(hWnd, 4003);
            switch (cmd) {
            case 4301: { /* 新增 */
                FieldDef f[7] = {
                    {"用户名", "", MAX_USERNAME, FALSE},
                    {"姓名", "", MAX_NAME, FALSE},
                    {"性别", "", 10, FALSE},
                    {"年龄", "", 16, FALSE},
                    {"电话", "", 20, FALSE},
                    {"患者类型", "", 20, FALSE},
                    {"治疗阶段", "", 20, FALSE},
                };
                if (ShowFieldDialog(hWnd, "新增患者", f, 7)) {
                    Patient p;
                    generate_id(p.patient_id, sizeof(p.patient_id), "PAT");
                    strcpy(p.username, f[0].value);
                    strcpy(p.name, f[1].value);
                    strcpy(p.gender, f[2].value);
                    p.age = atoi(f[3].value);
                    strcpy(p.phone, f[4].value);
                    strcpy(p.patient_type, f[5].value);
                    strcpy(p.treatment_stage, f[6].value);
                    p.is_emergency = 0;
                    PatientNode *head = load_patients_list();
                    PatientNode *node = create_patient_node(&p);
                    node->next = head;
                    save_patients_list(node);
                    free_patient_list(node);
                    PopulatePatList(hLV);
                }
                return 0;
            }
            case 4302: { /* 编辑 */
                sel = ListView_GetNextItem(hLV, -1, LVNI_SELECTED);
                if (sel < 0) { MessageBoxA(hWnd, "请先选择一行", "提示", MB_OK); return 0; }
                FieldDef f[7] = {
                    {"用户名", "", MAX_USERNAME, FALSE},
                    {"姓名", "", MAX_NAME, FALSE},
                    {"性别", "", 10, FALSE},
                    {"年龄", "", 16, FALSE},
                    {"电话", "", 20, FALSE},
                    {"患者类型", "", 20, FALSE},
                    {"治疗阶段", "", 20, FALSE},
                };
                /* 跳过第 0 列(ID)，从第 1 列开始读 */
                sel = ListView_GetNextItem(hLV, -1, LVNI_SELECTED);
                char idBuf[MAX_ID] = {0};
                ListView_GetItemText(hLV, sel, 0, idBuf, sizeof(idBuf));
                ListView_GetItemText(hLV, sel, 1, f[0].value, f[0].max_len);
                ListView_GetItemText(hLV, sel, 2, f[1].value, f[1].max_len);
                ListView_GetItemText(hLV, sel, 3, f[2].value, f[2].max_len);
                ListView_GetItemText(hLV, sel, 4, f[3].value, f[3].max_len);
                ListView_GetItemText(hLV, sel, 5, f[4].value, f[4].max_len);
                ListView_GetItemText(hLV, sel, 6, f[5].value, f[5].max_len);
                /* treatment_stage is col 6, we skip "年龄" remapping — keep it simple */
                if (ShowFieldDialog(hWnd, "编辑患者", f, 7)) {
                    PatientNode *head = load_patients_list();
                    for (PatientNode *cur = head; cur; cur = cur->next) {
                        if (strcmp(cur->data.patient_id, idBuf) == 0) {
                            strcpy(cur->data.username, f[0].value);
                            strcpy(cur->data.name, f[1].value);
                            strcpy(cur->data.gender, f[2].value);
                            cur->data.age = atoi(f[3].value);
                            strcpy(cur->data.phone, f[4].value);
                            strcpy(cur->data.patient_type, f[5].value);
                            strcpy(cur->data.treatment_stage, f[6].value);
                            break;
                        }
                    }
                    save_patients_list(head);
                    free_patient_list(head);
                    PopulatePatList(hLV);
                }
                return 0;
            }
            case 4303: { /* 删除 */
                sel = ListView_GetNextItem(hLV, -1, LVNI_SELECTED);
                if (sel < 0) { MessageBoxA(hWnd, "请先选择一行", "提示", MB_OK); return 0; }
                if (MessageBoxA(hWnd, "确定要删除该患者吗？", "确认", MB_YESNO | MB_ICONQUESTION) != IDYES) return 0;
                char targetId[32];
                ListView_GetItemText(hLV, sel, 0, targetId, sizeof(targetId));
                PatientNode *head = load_patients_list();
                PatientNode *prev = NULL, *cur = head;
                while (cur) {
                    if (strcmp(cur->data.patient_id, targetId) == 0) {
                        if (prev) prev->next = cur->next;
                        else head = cur->next;
                        free(cur);
                        break;
                    }
                    prev = cur;
                    cur = cur->next;
                }
                save_patients_list(head);
                free_patient_list(head);
                PopulatePatList(hLV);
                return 0;
            }
            case 4304: /* 刷新 */
                PopulatePatList(hLV);
                return 0;
            }
            return 0;

        /* ── 药品管理 ─────────────────────────────────────────── */
        case NAV_ADMIN_DRUG:
            hLV = GetDlgItem(hWnd, 4004);
            switch (cmd) {
            case 4401: { /* 新增 */
                FieldDef f[6] = {
                    {"名称", "", MAX_NAME, FALSE},
                    {"价格", "", 16, FALSE},
                    {"库存", "", 16, FALSE},
                    {"预警线", "", 16, FALSE},
                    {"报销比例%", "", 16, FALSE},
                    {"分类", "", 20, FALSE},
                };
                if (ShowFieldDialog(hWnd, "新增药品", f, 6)) {
                    Drug d;
                    generate_id(d.drug_id, sizeof(d.drug_id), "DRG");
                    strcpy(d.name, f[0].value);
                    d.price = (float)atof(f[1].value);
                    d.stock_num = atoi(f[2].value);
                    d.warning_line = atoi(f[3].value);
                    d.reimbursement_ratio = (float)atof(f[4].value) / 100.0f;
                    strcpy(d.category, f[5].value);
                    d.is_special = 0;
                    DrugNode *head = load_drugs_list();
                    DrugNode *node = create_drug_node(&d);
                    node->next = head;
                    save_drugs_list(node);
                    free_drug_list(node);
                    PopulateDrugList(hLV);
                }
                return 0;
            }
            case 4402: { /* 编辑 */
                sel = ListView_GetNextItem(hLV, -1, LVNI_SELECTED);
                if (sel < 0) { MessageBoxA(hWnd, "请先选择一行", "提示", MB_OK); return 0; }
                FieldDef f[6] = {
                    {"名称", "", MAX_NAME, FALSE},
                    {"价格", "", 16, FALSE},
                    {"库存", "", 16, FALSE},
                    {"预警线", "", 16, FALSE},
                    {"报销比例%", "", 16, FALSE},
                    {"分类", "", 20, FALSE},
                };
                char idBuf[MAX_ID] = {0};
                ListView_GetItemText(hLV, sel, 0, idBuf, sizeof(idBuf));
                ListView_GetItemText(hLV, sel, 1, f[0].value, f[0].max_len);
                ListView_GetItemText(hLV, sel, 2, f[1].value, f[1].max_len);
                ListView_GetItemText(hLV, sel, 3, f[2].value, f[2].max_len);
                ListView_GetItemText(hLV, sel, 4, f[3].value, f[3].max_len);
                /* 报销比例列显示的是 "XX%"，需要去掉 % */
                {
                    char ratioCol[32];
                    ListView_GetItemText(hLV, sel, 5, ratioCol, sizeof(ratioCol));
                    char *pct = strchr(ratioCol, '%');
                    if (pct) *pct = 0;
                    strcpy(f[4].value, ratioCol);
                }
                ListView_GetItemText(hLV, sel, 6, f[5].value, f[5].max_len);
                if (ShowFieldDialog(hWnd, "编辑药品", f, 6)) {
                    DrugNode *head = load_drugs_list();
                    for (DrugNode *cur = head; cur; cur = cur->next) {
                        if (strcmp(cur->data.drug_id, idBuf) == 0) {
                            strcpy(cur->data.name, f[0].value);
                            cur->data.price = (float)atof(f[1].value);
                            cur->data.stock_num = atoi(f[2].value);
                            cur->data.warning_line = atoi(f[3].value);
                            cur->data.reimbursement_ratio = (float)atof(f[4].value) / 100.0f;
                            strcpy(cur->data.category, f[5].value);
                            break;
                        }
                    }
                    save_drugs_list(head);
                    free_drug_list(head);
                    PopulateDrugList(hLV);
                }
                return 0;
            }
            case 4403: { /* 删除 */
                sel = ListView_GetNextItem(hLV, -1, LVNI_SELECTED);
                if (sel < 0) { MessageBoxA(hWnd, "请先选择一行", "提示", MB_OK); return 0; }
                if (MessageBoxA(hWnd, "确定要删除该药品吗？", "确认", MB_YESNO | MB_ICONQUESTION) != IDYES) return 0;
                char targetId[32];
                ListView_GetItemText(hLV, sel, 0, targetId, sizeof(targetId));
                DrugNode *head = load_drugs_list();
                DrugNode *prev = NULL, *cur = head;
                while (cur) {
                    if (strcmp(cur->data.drug_id, targetId) == 0) {
                        if (prev) prev->next = cur->next;
                        else head = cur->next;
                        free(cur);
                        break;
                    }
                    prev = cur;
                    cur = cur->next;
                }
                save_drugs_list(head);
                free_drug_list(head);
                PopulateDrugList(hLV);
                return 0;
            }
            case 4404: { /* 补货 */
                sel = ListView_GetNextItem(hLV, -1, LVNI_SELECTED);
                if (sel < 0) { MessageBoxA(hWnd, "请先选择一行", "提示", MB_OK); return 0; }
                char targetId[32];
                ListView_GetItemText(hLV, sel, 0, targetId, sizeof(targetId));
                FieldDef f[1] = {{"增加数量", "", 16, FALSE}};
                if (ShowFieldDialog(hWnd, "补货", f, 1)) {
                    int add = atoi(f[0].value);
                    if (add <= 0) { MessageBoxA(hWnd, "数量必须大于 0", "错误", MB_OK); return 0; }
                    DrugNode *head = load_drugs_list();
                    for (DrugNode *cur = head; cur; cur = cur->next) {
                        if (strcmp(cur->data.drug_id, targetId) == 0) {
                            cur->data.stock_num += add;
                            break;
                        }
                    }
                    save_drugs_list(head);
                    free_drug_list(head);
                    PopulateDrugList(hLV);
                }
                return 0;
            }
            }
            return 0;

        /* ── 病房管理 ─────────────────────────────────────────── */
        case NAV_ADMIN_WARD:
            hLV = GetDlgItem(hWnd, 4005);
            switch (cmd) {
            case 4501: { /* 新增 */
                FieldDef f[4] = {
                    {"类型", "", 50, FALSE},
                    {"总床位", "", 16, FALSE},
                    {"剩余床位", "", 16, FALSE},
                    {"预警线", "", 16, FALSE},
                };
                if (ShowFieldDialog(hWnd, "新增病房", f, 4)) {
                    Ward w;
                    generate_id(w.ward_id, sizeof(w.ward_id), "WRD");
                    strcpy(w.type, f[0].value);
                    w.total_beds = atoi(f[1].value);
                    w.remain_beds = atoi(f[2].value);
                    w.warning_line = atoi(f[3].value);
                    WardNode *head = load_wards_list();
                    WardNode *node = create_ward_node(&w);
                    node->next = head;
                    save_wards_list(node);
                    free_ward_list(node);
                    PopulateWardList(hLV);
                }
                return 0;
            }
            case 4502: { /* 编辑 */
                sel = ListView_GetNextItem(hLV, -1, LVNI_SELECTED);
                if (sel < 0) { MessageBoxA(hWnd, "请先选择一行", "提示", MB_OK); return 0; }
                FieldDef f[4] = {
                    {"类型", "", 50, FALSE},
                    {"总床位", "", 16, FALSE},
                    {"剩余床位", "", 16, FALSE},
                    {"预警线", "", 16, FALSE},
                };
                char idBuf[MAX_ID] = {0};
                ListView_GetItemText(hLV, sel, 0, idBuf, sizeof(idBuf));
                ListView_GetItemText(hLV, sel, 1, f[0].value, f[0].max_len);
                ListView_GetItemText(hLV, sel, 2, f[1].value, f[1].max_len);
                ListView_GetItemText(hLV, sel, 3, f[2].value, f[2].max_len);
                ListView_GetItemText(hLV, sel, 4, f[3].value, f[3].max_len);
                if (ShowFieldDialog(hWnd, "编辑病房", f, 4)) {
                    WardNode *head = load_wards_list();
                    for (WardNode *cur = head; cur; cur = cur->next) {
                        if (strcmp(cur->data.ward_id, idBuf) == 0) {
                            strcpy(cur->data.type, f[0].value);
                            cur->data.total_beds = atoi(f[1].value);
                            cur->data.remain_beds = atoi(f[2].value);
                            cur->data.warning_line = atoi(f[3].value);
                            break;
                        }
                    }
                    save_wards_list(head);
                    free_ward_list(head);
                    PopulateWardList(hLV);
                }
                return 0;
            }
            case 4503: { /* 删除 */
                sel = ListView_GetNextItem(hLV, -1, LVNI_SELECTED);
                if (sel < 0) { MessageBoxA(hWnd, "请先选择一行", "提示", MB_OK); return 0; }
                if (MessageBoxA(hWnd, "确定要删除该病房吗？", "确认", MB_YESNO | MB_ICONQUESTION) != IDYES) return 0;
                char targetId[32];
                ListView_GetItemText(hLV, sel, 0, targetId, sizeof(targetId));
                WardNode *head = load_wards_list();
                WardNode *prev = NULL, *cur = head;
                while (cur) {
                    if (strcmp(cur->data.ward_id, targetId) == 0) {
                        if (prev) prev->next = cur->next;
                        else head = cur->next;
                        free(cur);
                        break;
                    }
                    prev = cur;
                    cur = cur->next;
                }
                save_wards_list(head);
                free_ward_list(head);
                PopulateWardList(hLV);
                return 0;
            }
            }
            return 0;

        /* ── 排班管理 ─────────────────────────────────────────── */
        case NAV_ADMIN_SCHEDULE:
            hLV = GetDlgItem(hWnd, 4006);
            switch (cmd) {
            case 4601: { /* 生成排班 */
                FieldDef f[3] = {
                    {"医生ID", "", MAX_ID, FALSE},
                    {"日期", "", 12, FALSE},
                    {"时段", "", 10, FALSE},
                };
                if (ShowFieldDialog(hWnd, "生成排班", f, 3)) {
                    Schedule s;
                    generate_id(s.schedule_id, sizeof(s.schedule_id), "SCH");
                    strcpy(s.doctor_id, f[0].value);
                    strcpy(s.work_date, f[1].value);
                    strcpy(s.time_slot, f[2].value);
                    strcpy(s.status, "正常");
                    s.max_appt = 20;
                    s.max_onsite = 10;
                    ScheduleNode *head = load_schedules_list();
                    ScheduleNode *node = create_schedule_node(&s);
                    node->next = head;
                    save_schedules_list(node);
                    free_schedule_list(node);
                    PopulateSchedList(hLV);
                }
                return 0;
            }
            case 4602: { /* 停诊 */
                sel = ListView_GetNextItem(hLV, -1, LVNI_SELECTED);
                if (sel < 0) { MessageBoxA(hWnd, "请先选择一行", "提示", MB_OK); return 0; }
                char targetId[32];
                ListView_GetItemText(hLV, sel, 0, targetId, sizeof(targetId));
                ScheduleNode *head = load_schedules_list();
                for (ScheduleNode *cur = head; cur; cur = cur->next) {
                    if (strcmp(cur->data.schedule_id, targetId) == 0) {
                        strcpy(cur->data.status, "停诊");
                        break;
                    }
                }
                save_schedules_list(head);
                free_schedule_list(head);
                PopulateSchedList(hLV);
                return 0;
            }
            }
            return 0;
        }
        return 0;
    }

    default:
        return DefWindowProcA(hWnd, msg, wParam, lParam);
    }
}

/* ─── 各页面创建函数 ─────────────────────────────────────────────── */

static HWND CreateDeptPage(HWND hParent, RECT *rc) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = AdminPageWndProc;
    wc.hInstance     = g_hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "AdminDeptPage";
    RegisterClassA(&wc);

    HWND hPage = CreateWindowExA(0, "AdminDeptPage", "",
        WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
        rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top,
        hParent, NULL, g_hInst, (LPVOID)(INT_PTR)NAV_ADMIN_DEPT);
    if (!hPage) return NULL;

    int w = (rc->right - rc->left) - 20;
    int h = (rc->bottom - rc->top) - 60;

    HWND hLV = CreateListView(hPage, 4001, 10, 10, w, h);
    AddCol(hLV, 0, "科室ID", 80);
    AddCol(hLV, 1, "名称", 120);
    AddCol(hLV, 2, "负责人", 100);
    AddCol(hLV, 3, "电话", 100);
    PopulateDeptList(hLV);

    int by = rc->bottom - rc->top - 45;
    CreateWindowA("BUTTON", "新增", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                  10, by, 70, 30, hPage, (HMENU)4101, g_hInst, NULL);
    CreateWindowA("BUTTON", "编辑", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                  90, by, 70, 30, hPage, (HMENU)4102, g_hInst, NULL);
    CreateWindowA("BUTTON", "删除", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                  170, by, 70, 30, hPage, (HMENU)4103, g_hInst, NULL);

    return hPage;
}

static HWND CreateDoctorMgmtPage(HWND hParent, RECT *rc) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = AdminPageWndProc;
    wc.hInstance     = g_hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "AdminDocPage";
    RegisterClassA(&wc);

    HWND hPage = CreateWindowExA(0, "AdminDocPage", "",
        WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
        rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top,
        hParent, NULL, g_hInst, (LPVOID)(INT_PTR)NAV_ADMIN_DOCTOR);
    if (!hPage) return NULL;

    int w = (rc->right - rc->left) - 20;
    int h = (rc->bottom - rc->top) - 60;

    HWND hLV = CreateListView(hPage, 4002, 10, 10, w, h);
    AddCol(hLV, 0, "医生ID", 80);
    AddCol(hLV, 1, "姓名", 80);
    AddCol(hLV, 2, "科室", 80);
    AddCol(hLV, 3, "职称", 80);
    AddCol(hLV, 4, "繁忙度", 60);
    PopulateDocList(hLV);

    int by = rc->bottom - rc->top - 45;
    CreateWindowA("BUTTON", "新增", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                  10, by, 70, 30, hPage, (HMENU)4201, g_hInst, NULL);
    CreateWindowA("BUTTON", "编辑", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                  90, by, 70, 30, hPage, (HMENU)4202, g_hInst, NULL);
    CreateWindowA("BUTTON", "删除", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                  170, by, 70, 30, hPage, (HMENU)4203, g_hInst, NULL);

    return hPage;
}

static HWND CreatePatientMgmtPage(HWND hParent, RECT *rc) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = AdminPageWndProc;
    wc.hInstance     = g_hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "AdminPatPage";
    RegisterClassA(&wc);

    HWND hPage = CreateWindowExA(0, "AdminPatPage", "",
        WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
        rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top,
        hParent, NULL, g_hInst, (LPVOID)(INT_PTR)NAV_ADMIN_PATIENT);
    if (!hPage) return NULL;

    int w = (rc->right - rc->left) - 20;
    int h = (rc->bottom - rc->top) - 60;

    HWND hLV = CreateListView(hPage, 4003, 10, 10, w, h);
    AddCol(hLV, 0, "患者ID", 80);
    AddCol(hLV, 1, "姓名", 80);
    AddCol(hLV, 2, "性别", 40);
    AddCol(hLV, 3, "年龄", 40);
    AddCol(hLV, 4, "电话", 100);
    AddCol(hLV, 5, "类型", 60);
    AddCol(hLV, 6, "阶段", 80);
    PopulatePatList(hLV);

    int by = rc->bottom - rc->top - 45;
    CreateWindowA("BUTTON", "新增", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                  10, by, 70, 30, hPage, (HMENU)4301, g_hInst, NULL);
    CreateWindowA("BUTTON", "编辑", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                  90, by, 70, 30, hPage, (HMENU)4302, g_hInst, NULL);
    CreateWindowA("BUTTON", "删除", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                  170, by, 70, 30, hPage, (HMENU)4303, g_hInst, NULL);
    CreateWindowA("BUTTON", "刷新", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                  250, by, 70, 30, hPage, (HMENU)4304, g_hInst, NULL);

    return hPage;
}

static HWND CreateDrugPage(HWND hParent, RECT *rc) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = AdminPageWndProc;
    wc.hInstance     = g_hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "AdminDrugPage";
    RegisterClassA(&wc);

    HWND hPage = CreateWindowExA(0, "AdminDrugPage", "",
        WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
        rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top,
        hParent, NULL, g_hInst, (LPVOID)(INT_PTR)NAV_ADMIN_DRUG);
    if (!hPage) return NULL;

    int w = (rc->right - rc->left) - 20;
    int h = (rc->bottom - rc->top) - 60;

    HWND hLV = CreateListView(hPage, 4004, 10, 10, w, h);
    AddCol(hLV, 0, "药品ID", 80);
    AddCol(hLV, 1, "名称", 120);
    AddCol(hLV, 2, "价格", 60);
    AddCol(hLV, 3, "库存", 50);
    AddCol(hLV, 4, "预警线", 50);
    AddCol(hLV, 5, "报销比例", 70);
    AddCol(hLV, 6, "分类", 60);
    PopulateDrugList(hLV);

    int by = rc->bottom - rc->top - 45;
    CreateWindowA("BUTTON", "新增", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                  10, by, 70, 30, hPage, (HMENU)4401, g_hInst, NULL);
    CreateWindowA("BUTTON", "编辑", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                  90, by, 70, 30, hPage, (HMENU)4402, g_hInst, NULL);
    CreateWindowA("BUTTON", "删除", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                  170, by, 70, 30, hPage, (HMENU)4403, g_hInst, NULL);
    CreateWindowA("BUTTON", "补货", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                  250, by, 70, 30, hPage, (HMENU)4404, g_hInst, NULL);

    return hPage;
}

static HWND CreateWardPage(HWND hParent, RECT *rc) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = AdminPageWndProc;
    wc.hInstance     = g_hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "AdminWardPage";
    RegisterClassA(&wc);

    HWND hPage = CreateWindowExA(0, "AdminWardPage", "",
        WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
        rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top,
        hParent, NULL, g_hInst, (LPVOID)(INT_PTR)NAV_ADMIN_WARD);
    if (!hPage) return NULL;

    int w = (rc->right - rc->left) - 20;
    int h = (rc->bottom - rc->top) - 60;

    HWND hLV = CreateListView(hPage, 4005, 10, 10, w, h);
    AddCol(hLV, 0, "病房ID", 80);
    AddCol(hLV, 1, "类型", 100);
    AddCol(hLV, 2, "总床位", 60);
    AddCol(hLV, 3, "剩余床位", 60);
    AddCol(hLV, 4, "预警线", 60);
    PopulateWardList(hLV);

    int by = rc->bottom - rc->top - 45;
    CreateWindowA("BUTTON", "新增", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                  10, by, 70, 30, hPage, (HMENU)4501, g_hInst, NULL);
    CreateWindowA("BUTTON", "编辑", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                  90, by, 70, 30, hPage, (HMENU)4502, g_hInst, NULL);
    CreateWindowA("BUTTON", "删除", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                  170, by, 70, 30, hPage, (HMENU)4503, g_hInst, NULL);

    return hPage;
}

static HWND CreateSchedulePage(HWND hParent, RECT *rc) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = AdminPageWndProc;
    wc.hInstance     = g_hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "AdminSchedPage";
    RegisterClassA(&wc);

    HWND hPage = CreateWindowExA(0, "AdminSchedPage", "",
        WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
        rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top,
        hParent, NULL, g_hInst, (LPVOID)(INT_PTR)NAV_ADMIN_SCHEDULE);
    if (!hPage) return NULL;

    int w = (rc->right - rc->left) - 20;
    int h = (rc->bottom - rc->top) - 60;

    HWND hLV = CreateListView(hPage, 4006, 10, 10, w, h);
    AddCol(hLV, 0, "排班ID", 100);
    AddCol(hLV, 1, "医生ID", 80);
    AddCol(hLV, 2, "日期", 100);
    AddCol(hLV, 3, "时段", 60);
    AddCol(hLV, 4, "状态", 60);
    PopulateSchedList(hLV);

    int by = rc->bottom - rc->top - 45;
    CreateWindowA("BUTTON", "生成排班", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                  10, by, 90, 30, hPage, (HMENU)4601, g_hInst, NULL);
    CreateWindowA("BUTTON", "停诊", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                  110, by, 70, 30, hPage, (HMENU)4602, g_hInst, NULL);

    return hPage;
}

/* ─── 操作日志页面 ─────────────────────────────────────────────────── */

static HWND CreateLogPage(HWND hParent, RECT *rc) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = AdminPageWndProc;
    wc.hInstance     = g_hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "AdminLogPage";
    RegisterClassA(&wc);

    HWND hPage = CreateWindowExA(0, "AdminLogPage", "",
        WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
        rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top,
        hParent, NULL, g_hInst, (LPVOID)(INT_PTR)NAV_ADMIN_LOG);
    if (!hPage) return NULL;

    int w = (rc->right - rc->left) - 20;
    int h = (rc->bottom - rc->top) - 20;

    HWND hLV = CreateListView(hPage, 4007, 10, 10, w, h);
    AddCol(hLV, 0, "日志ID", 80);
    AddCol(hLV, 1, "操作人", 80);
    AddCol(hLV, 2, "操作", 60);
    AddCol(hLV, 3, "对象", 60);
    AddCol(hLV, 4, "对象ID", 80);
    AddCol(hLV, 5, "详情", 200);
    AddCol(hLV, 6, "时间", 120);

    LogEntryNode *logs = load_logs_list();
    int row = 0;
    if (logs) {
        LogEntryNode *cur = logs;
        while (cur) {
            const char *items[7] = {
                cur->data.log_id, cur->data.operator_name,
                cur->data.action, cur->data.target,
                cur->data.target_id, cur->data.detail, cur->data.create_time
            };
            AddRow(hLV, row++, 7, items);
            cur = cur->next;
        }
        free_log_entry_list(logs);
    }

    return hPage;
}

/* ─── 数据管理页面 ─────────────────────────────────────────────────── */

static HWND CreateDataPage(HWND hParent, RECT *rc) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = AdminPageWndProc;
    wc.hInstance     = g_hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "AdminDataPage";
    RegisterClassA(&wc);

    HWND hPage = CreateWindowExA(0, "AdminDataPage", "",
        WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
        rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top,
        hParent, NULL, g_hInst, (LPVOID)(INT_PTR)NAV_ADMIN_DATA);
    if (!hPage) return NULL;

    CreateWindowA("STATIC", "数据备份与恢复",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        20, 20, 200, 25, hPage, NULL, g_hInst, NULL);

    CreateWindowA("BUTTON", "备份数据",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        20, 60, 120, 35, hPage, (HMENU)4801, g_hInst, NULL);

    CreateWindowA("BUTTON", "恢复数据",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        160, 60, 120, 35, hPage, (HMENU)4802, g_hInst, NULL);

    CreateWindowA("STATIC", "备份操作会将 data/ 目录下所有数据打包备份。",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        20, 110, 400, 20, hPage, NULL, g_hInst, NULL);

    return hPage;
}

/* ─── 报表统计页面 ─────────────────────────────────────────────────── */

static HWND CreateAnalysisPage(HWND hParent, RECT *rc) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = AdminPageWndProc;
    wc.hInstance     = g_hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "AdminAnalPage";
    RegisterClassA(&wc);

    HWND hPage = CreateWindowExA(0, "AdminAnalPage", "",
        WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
        rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top,
        hParent, NULL, g_hInst, (LPVOID)(INT_PTR)NAV_ADMIN_ANALYSIS);
    if (!hPage) return NULL;

    HWND hTab = CreateWindowA(WC_TABCONTROLA, "",
        WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
        10, 10, (rc->right - rc->left) - 20, (rc->bottom - rc->top) - 20,
        hPage, NULL, g_hInst, NULL);

    TC_ITEMA tci = {0};
    tci.mask = TCIF_TEXT;
    tci.pszText = "概览";
    TabCtrl_InsertItem(hTab, 0, &tci);
    tci.pszText = "医生负载";
    TabCtrl_InsertItem(hTab, 1, &tci);
    tci.pszText = "财务统计";
    TabCtrl_InsertItem(hTab, 2, &tci);

    CreateWindowA("STATIC", "报表功能开发中，请使用 CLI 版查看详细报表。",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        30, 50, 300, 100, hPage, NULL, g_hInst, NULL);

    return hPage;
}

/* ─── 公开接口 ─────────────────────────────────────────────────────── */

HWND CreateAdminPage(HWND hParent, int viewId, RECT *rc) {
    switch (viewId) {
    case NAV_ADMIN_DEPT:     return CreateDeptPage(hParent, rc);
    case NAV_ADMIN_DOCTOR:   return CreateDoctorMgmtPage(hParent, rc);
    case NAV_ADMIN_PATIENT:  return CreatePatientMgmtPage(hParent, rc);
    case NAV_ADMIN_DRUG:     return CreateDrugPage(hParent, rc);
    case NAV_ADMIN_WARD:     return CreateWardPage(hParent, rc);
    case NAV_ADMIN_SCHEDULE: return CreateSchedulePage(hParent, rc);
    case NAV_ADMIN_LOG:      return CreateLogPage(hParent, rc);
    case NAV_ADMIN_DATA:     return CreateDataPage(hParent, rc);
    case NAV_ADMIN_ANALYSIS: return CreateAnalysisPage(hParent, rc);
    default: return NULL;
    }
}
