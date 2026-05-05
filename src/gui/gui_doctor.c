/*
 * gui_doctor.c — Win32 GUI 医生界面实现 / Win32 GUI doctor page implementation
 *
 * 实现医生角色的所有 GUI 页面 (7 个页面 + 开药对话框):
 *   - 待接诊 (CreateReminderPage) — 显示当前医生所有"待就诊"预约, 选中后跳转接诊
 *   - 接诊 (CreateConsultationPage) — 选择患者→填写诊断/治疗建议→保存病历→更新预约状态
 *   - 病房呼叫 (CreateWardCallPage) — 查看所有病房呼叫, 标记"已响应"
 *   - 紧急标记 (CreateEmergencyPage) — 切换患者紧急状态
 *   - 进度更新 (CreateProgressPage) — 推进患者治疗阶段 (初始→诊治中→康复中→已完成)
 *   - 病历模板 (CreateTemplatePage) — 管理病历模板的 CRUD (快捷码/分类/内容)
 *   - 开药 (CreatePrescribePage) — 从病历列表选择→打开开药对话框→购物车模式添加药品
 *
 * 开药对话框 (DrugDispenseDlgProc): 搜索/筛选药品→添加到购物车→调整数量→确认开药
 * (逐药创建处方 + 扣减库存 + 记录审计日志)。
 *
 * 每个页面使用独立窗口类 + WndProc, 通过 CreateDoctorPage() 工厂函数按 viewId 路由。
 * 待接诊→接诊 通过静态变量 g_pendingApptId 传递选中的预约 ID。
 *
 * Implements all 7 doctor-role GUI pages with modal drug dispensing dialog,
 * cart-style prescription creation with stock deduction, medical record
 * lifecycle management, and template CRUD operations.
 */

#include "gui_doctor.h"
#include "gui_main.h"
#include "../data_storage.h"
#include "../public.h"

/* ─── ListView 工具函数 / ListView Utility Functions ────────────────── */
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

/* 获取当前医生 ID (通过用户名匹配, 返回静态缓冲区无需释放)
   Get current doctor ID by matching username, returns static buffer */
static const char* GetDoctorId(void) {
    static char id[MAX_ID] = "";
    DoctorNode *head = load_doctors_list();
    if (head) {
        DoctorNode *cur = head;
        while (cur) {
            if (strcmp(cur->data.username, g_currentUser.username) == 0) {
                strcpy(id, cur->data.doctor_id);
                free_doctor_list(head);
                return id;
            }
            cur = cur->next;
        }
        free_doctor_list(head);
    }
    id[0] = 0;
    return id;
}

/* 获取选中的 ListView 指定列文本 (无选中时返回空串)
   Get text from selected ListView row at specified column, or empty */
static void GetSelectedItemText(HWND hLV, int col, char *buf, int size) {
    int sel = ListView_GetNextItem(hLV, -1, LVNI_SELECTED);
    if (sel >= 0) {
        ListView_GetItemText(hLV, sel, col, buf, size);
    } else {
        buf[0] = 0;
    }
}

/* ─── 基类 WndProc (给无按钮的页面使用) / Base WndProc ─────────────── */
LRESULT CALLBACK DoctorPageWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCT *cs = (CREATESTRUCT *)lParam;
        SetWindowLongPtrA(hWnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        return 0;
    }
    case WM_SIZE: {
        int h = HIWORD(lParam);
        HWND hChild = GetWindow(hWnd, GW_CHILD);
        while (hChild) {
            SetWindowPos(hChild, NULL, 0, 0,
                         LOWORD(lParam), h - 40, SWP_NOZORDER | SWP_NOMOVE);
            hChild = GetWindow(hChild, GW_HWNDNEXT);
        }
        return 0;
    }
    case WM_COMMAND:
        return 0;
    default:
        return DefWindowProcA(hWnd, msg, wParam, lParam);
    }
}

/* ─── 开药对话框 (模态) / Drug Dispensing Dialog (Modal) ───────────── */

/* 接诊上下文 (传递给开药对话框: 病历ID/患者ID/医生ID)
   Consultation context passed to drug dispense dialog */
typedef struct {
    char record_id[MAX_ID];
    char patient_id[MAX_ID];
    char doctor_id[MAX_ID];
} ConsultData;

/* 购物车条目 / Shopping cart item */
#define MAX_CART_ITEMS 30
typedef struct {
    char drug_id[MAX_ID];
    char drug_name[MAX_NAME];
    int  quantity;
    float price;
    float total;
} CartItem;

static CartItem g_cart[MAX_CART_ITEMS];
static int      g_cartCount = 0;
static int      g_dispenseResult = -1;   /* -1=运行中, 1=确认, 0=取消 / running, confirmed, cancelled */
static ConsultData g_consultCtx;          /* 接诊上下文 (跨回调共享) / consultation context shared across callbacks */

/* 开药对话框控件 ID / Drug dispense dialog control IDs */
#define IDC_RX_SEARCH     3701
#define IDC_RX_DRUG_LIST  3702
#define IDC_RX_QUANTITY   3703
#define IDC_RX_ADD_BTN    3704
#define IDC_RX_CART_LIST  3705
#define IDC_RX_REMOVE_BTN 3706
#define IDC_RX_TOTAL      3707
#define IDC_RX_CONFIRM    3708
#define IDC_RX_CANCEL     3709

static void RefreshDrugList(HWND hDlg, const char *filter) {
    /* 刷新药品列表 (可按名称过滤) / Refresh drug list, optionally filter by name */
    HWND hLV = GetDlgItem(hDlg, IDC_RX_DRUG_LIST);
    if (!hLV) return;
    ListView_DeleteAllItems(hLV);
    DrugNode *list = load_drugs_list();
    int row = 0;
    for (DrugNode *cur = list; cur; cur = cur->next) {
        if (filter && filter[0] && strstr(cur->data.name, filter) == NULL)
            continue;
        char price[16], stock[16];
        snprintf(price, sizeof(price), "%.2f", cur->data.price);
        snprintf(stock, sizeof(stock), "%d", cur->data.stock_num);
        const char *type = cur->data.is_special ? "处方药" : "非处方药";
        const char *items[6] = {
            cur->data.drug_id, cur->data.name, price, stock, type, cur->data.category
        };
        AddRow(hLV, row++, 6, items);
    }
    free_drug_list(list);
}

static void RefreshCartList(HWND hDlg) {
    /* 刷新购物车列表 + 更新总计金额 / Refresh cart list + update grand total */
    HWND hLV = GetDlgItem(hDlg, IDC_RX_CART_LIST);
    if (!hLV) return;
    ListView_DeleteAllItems(hLV);
    float grandTotal = 0;
    for (int i = 0; i < g_cartCount; i++) {
        char qty[16], price[16], total[16];
        snprintf(qty, sizeof(qty), "%d", g_cart[i].quantity);
        snprintf(price, sizeof(price), "%.2f", g_cart[i].price);
        snprintf(total, sizeof(total), "%.2f", g_cart[i].total);
        const char *items[4] = { g_cart[i].drug_name, qty, price, total };
        AddRow(hLV, i, 4, items);
        grandTotal += g_cart[i].total;
    }
    char buf[64];
    snprintf(buf, sizeof(buf), "总计: %.2f 元", grandTotal);
    SetDlgItemTextA(hDlg, IDC_RX_TOTAL, buf);
}

static LRESULT CALLBACK DrugDispenseDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCT *cs = (CREATESTRUCT *)lParam;
        if (cs->lpCreateParams)
            memcpy(&g_consultCtx, cs->lpCreateParams, sizeof(ConsultData));
        g_cartCount = 0;

        int w = 660, x = 10;
        int y = 10;

        CreateWindowA("STATIC", "药品搜索:", WS_VISIBLE | WS_CHILD | SS_LEFT,
                      x, y, 70, 20, hDlg, NULL, g_hInst, NULL);
        CreateWindowA("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                      x + 75, y, 200, 22, hDlg, (HMENU)IDC_RX_SEARCH, g_hInst, NULL);
        y += 32;

        CreateWindowA("STATIC", "药品列表:", WS_VISIBLE | WS_CHILD | SS_LEFT,
                      x, y, 200, 20, hDlg, NULL, g_hInst, NULL);
        y += 20;

        HWND hDrugLV = CreateListView(hDlg, IDC_RX_DRUG_LIST, x, y, w - 20, 180);
        AddCol(hDrugLV, 0, "药品ID", 70);
        AddCol(hDrugLV, 1, "名称", 150);
        AddCol(hDrugLV, 2, "单价", 70);
        AddCol(hDrugLV, 3, "库存", 60);
        AddCol(hDrugLV, 4, "类型", 70);
        AddCol(hDrugLV, 5, "分类", 70);
        y += 188;

        CreateWindowA("STATIC", "数量:", WS_VISIBLE | WS_CHILD | SS_LEFT,
                      x, y, 40, 22, hDlg, NULL, g_hInst, NULL);
        CreateWindowA("EDIT", "1", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
                      x + 45, y, 50, 22, hDlg, (HMENU)IDC_RX_QUANTITY, g_hInst, NULL);
        CreateWindowA("BUTTON", "添加到清单", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                      x + 105, y - 2, 100, 26, hDlg, (HMENU)IDC_RX_ADD_BTN, g_hInst, NULL);
        CreateWindowA("BUTTON", "移除选中", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                      x + 215, y - 2, 90, 26, hDlg, (HMENU)IDC_RX_REMOVE_BTN, g_hInst, NULL);
        y += 32;

        CreateWindowA("STATIC", "已选药品:", WS_VISIBLE | WS_CHILD | SS_LEFT,
                      x, y, 200, 20, hDlg, NULL, g_hInst, NULL);
        y += 20;

        HWND hCartLV = CreateListView(hDlg, IDC_RX_CART_LIST, x, y, w - 20, 100);
        AddCol(hCartLV, 0, "药品名称", 150);
        AddCol(hCartLV, 1, "数量", 60);
        AddCol(hCartLV, 2, "单价", 80);
        AddCol(hCartLV, 3, "小计", 80);
        y += 108;

        CreateWindowA("STATIC", "总计: 0.00 元", WS_VISIBLE | WS_CHILD | SS_LEFT,
                      x, y, 200, 22, hDlg, (HMENU)IDC_RX_TOTAL, g_hInst, NULL);

        CreateWindowA("BUTTON", "确认开药", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                      w - 220, y, 100, 28, hDlg, (HMENU)IDC_RX_CONFIRM, g_hInst, NULL);
        CreateWindowA("BUTTON", "不开药", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                      w - 110, y, 100, 28, hDlg, (HMENU)IDC_RX_CANCEL, g_hInst, NULL);

        RefreshDrugList(hDlg, NULL);
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);

        if (id == IDC_RX_SEARCH && code == EN_CHANGE) {
            char filter[100] = "";
            GetWindowTextA((HWND)lParam, filter, sizeof(filter));
            RefreshDrugList(hDlg, filter);
            return 0;
        }

        if (id == IDC_RX_ADD_BTN && code == BN_CLICKED) {
            HWND hDrugLV = GetDlgItem(hDlg, IDC_RX_DRUG_LIST);
            int sel = ListView_GetNextItem(hDrugLV, -1, LVNI_SELECTED);
            if (sel < 0) {
                MessageBoxA(hDlg, "请先选择一种药品", "提示", MB_OK | MB_ICONINFORMATION);
                return 0;
            }
            char drugId[MAX_ID] = "", qtyText[16] = "";
            ListView_GetItemText(hDrugLV, sel, 0, drugId, sizeof(drugId));
            GetDlgItemTextA(hDlg, IDC_RX_QUANTITY, qtyText, sizeof(qtyText));
            int qty = atoi(qtyText);
            if (qty <= 0) {
                MessageBoxA(hDlg, "请输入有效数量", "提示", MB_OK | MB_ICONINFORMATION);
                return 0;
            }

            Drug *drug = find_drug_by_id(drugId);
            if (!drug) {
                MessageBoxA(hDlg, "未找到该药品信息", "错误", MB_OK | MB_ICONERROR);
                return 0;
            }
            if (qty > drug->stock_num) {
                char msg[100];
                snprintf(msg, sizeof(msg), "库存不足，当前库存: %d", drug->stock_num);
                MessageBoxA(hDlg, msg, "提示", MB_OK | MB_ICONINFORMATION);
                free(drug);
                return 0;
            }

            /* 检查购物车中是否已有该药品 → 累加数量 / Check if drug already in cart → add qty */
            int found = -1;
            for (int i = 0; i < g_cartCount; i++) {
                if (strcmp(g_cart[i].drug_id, drugId) == 0) {
                    found = i;
                    break;
                }
            }
            if (found >= 0) {
                int newQty = g_cart[found].quantity + qty;
                if (newQty > drug->stock_num) {
                    char msg[100];
                    snprintf(msg, sizeof(msg), "累计数量超库存，当前库存: %d，已在清单: %d",
                             drug->stock_num, g_cart[found].quantity);
                    MessageBoxA(hDlg, msg, "提示", MB_OK | MB_ICONINFORMATION);
                    free(drug);
                    return 0;
                }
                g_cart[found].quantity = newQty;
                g_cart[found].total = g_cart[found].price * newQty;
            } else if (g_cartCount < MAX_CART_ITEMS) {
                CartItem *ci = &g_cart[g_cartCount++];
                strcpy(ci->drug_id, drugId);
                strcpy(ci->drug_name, drug->name);
                ci->quantity = qty;
                ci->price = drug->price;
                ci->total = drug->price * qty;
            }

            free(drug);
            RefreshCartList(hDlg);

            SetDlgItemTextA(hDlg, IDC_RX_QUANTITY, "1");
            return 0;
        }

        if (id == IDC_RX_REMOVE_BTN && code == BN_CLICKED) {
            HWND hCartLV = GetDlgItem(hDlg, IDC_RX_CART_LIST);
            int sel = ListView_GetNextItem(hCartLV, -1, LVNI_SELECTED);
            if (sel < 0) return 0;
            for (int i = sel; i < g_cartCount - 1; i++)
                g_cart[i] = g_cart[i + 1];
            g_cartCount--;
            RefreshCartList(hDlg);
            return 0;
        }

        if (id == IDC_RX_CONFIRM && code == BN_CLICKED) {
            /* 确认开药: 逐药创建处方 → 扣减库存 → 记录审计日志
               Confirm: create prescriptions → deduct stock → audit log */
            if (g_cartCount == 0) {
                MessageBoxA(hDlg, "请至少添加一种药品", "提示", MB_OK | MB_ICONINFORMATION);
                return 0;
            }

            PrescriptionNode *head = load_prescriptions_list();
            for (int i = 0; i < g_cartCount; i++) {
                Prescription rx;
                memset(&rx, 0, sizeof(rx));
                generate_id(rx.prescription_id, sizeof(rx.prescription_id), "PR");
                strcpy(rx.record_id, g_consultCtx.record_id);
                strcpy(rx.patient_id, g_consultCtx.patient_id);
                strcpy(rx.doctor_id, g_consultCtx.doctor_id);
                strcpy(rx.drug_id, g_cart[i].drug_id);
                rx.quantity = g_cart[i].quantity;
                rx.total_price = g_cart[i].total;
                get_current_time(rx.prescription_date, sizeof(rx.prescription_date));

                PrescriptionNode *node = create_prescription_node(&rx);
                if (node) {
                    node->next = head;
                    head = node;
                }
            }
            save_prescriptions_list(head);
            free_prescription_list(head);

            /* 扣减库存 */
            DrugNode *drugs = load_drugs_list();
            if (drugs) {
                for (int i = 0; i < g_cartCount; i++) {
                    DrugNode *cur = drugs;
                    while (cur) {
                        if (strcmp(cur->data.drug_id, g_cart[i].drug_id) == 0) {
                            cur->data.stock_num -= g_cart[i].quantity;
                            if (cur->data.stock_num < 0) cur->data.stock_num = 0;
                            break;
                        }
                        cur = cur->next;
                    }
                }
                save_drugs_list(drugs);
                free_drug_list(drugs);
            }

            char summary[200] = "";
            for (int i = 0; i < g_cartCount && i < 5; i++) {
                char tmp[60];
                snprintf(tmp, sizeof(tmp), "%s%sx%d", i > 0 ? ", " : "",
                         g_cart[i].drug_name, g_cart[i].quantity);
                strncat(summary, tmp, sizeof(summary) - strlen(summary) - 1);
            }
            if (g_cartCount > 5) strncat(summary, " ...", sizeof(summary) - strlen(summary) - 1);

            append_log(g_currentUser.username, "开药", "prescription",
                       g_consultCtx.record_id, summary);

            g_dispenseResult = 1;
            DestroyWindow(hDlg);
            return 0;
        }

        if (id == IDC_RX_CANCEL && code == BN_CLICKED) {
            g_dispenseResult = 0;
            DestroyWindow(hDlg);
            return 0;
        }
        return 0;
    }
    case WM_CLOSE:
        g_dispenseResult = 0;
        DestroyWindow(hDlg);
        return 0;
    default:
        return DefWindowProcA(hDlg, msg, wParam, lParam);
    }
}

static int ShowDrugDispenseDialog(HWND hParent, ConsultData *data) {
    memcpy(&g_consultCtx, data, sizeof(ConsultData));
    g_cartCount = 0;

    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = DrugDispenseDlgProc;
    wc.hInstance     = g_hInst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "DrugDispenseDialog";
    RegisterClassA(&wc);

    HWND hDlg = CreateWindowExA(0, "DrugDispenseDialog", "开药",
        WS_VISIBLE | WS_POPUPWINDOW | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 680, 450,
        hParent, NULL, g_hInst, (LPVOID)data);
    if (!hDlg) return 0;

    RECT pr, rc;
    GetWindowRect(hParent, &pr);
    GetWindowRect(hDlg, &rc);
    SetWindowPos(hDlg, NULL,
        pr.left + (pr.right - pr.left - (rc.right - rc.left)) / 2,
        pr.top + (pr.bottom - pr.top - (rc.bottom - rc.top)) / 2,
        0, 0, SWP_NOSIZE | SWP_NOZORDER);

    EnableWindow(hParent, FALSE);
    g_dispenseResult = -1;
    MSG msg;
    while (g_dispenseResult == -1 && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    EnableWindow(hParent, TRUE);
    SetForegroundWindow(hParent);

    return g_dispenseResult == 1;
}

/* ─── 待接诊页面 / Pending Consultation Page ────────────────────────── */

/* 用于在待接诊→接诊间传递选中的预约 ID
   Bridges selected appointment ID from reminder page to consultation page */
static char g_pendingApptId[MAX_ID] = "";

static LRESULT CALLBACK ReminderPageWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCT *cs = (CREATESTRUCT *)lParam;
        SetWindowLongPtrA(hWnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        return 0;
    }
    case WM_SIZE: {
        int w = LOWORD(lParam), h = HIWORD(lParam);
        HWND hLV = GetDlgItem(hWnd, 3001);
        if (hLV) SetWindowPos(hLV, NULL, 5, 5, w - 10, h - 50, SWP_NOZORDER);
        HWND hBtn = GetDlgItem(hWnd, 3101);
        if (hBtn) SetWindowPos(hBtn, NULL, w - 120, h - 40, 110, 30, SWP_NOZORDER);
        return 0;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == 3101) {
            HWND hLV = GetDlgItem(hWnd, 3001);
            if (!hLV) return 0;
            char apptId[MAX_ID] = "";
            GetSelectedItemText(hLV, 0, apptId, sizeof(apptId));
            if (apptId[0] == 0) {
                MessageBoxA(GetParent(hWnd), "请先选择一个待接诊患者", "提示", MB_OK | MB_ICONINFORMATION);
                return 0;
            }
            strcpy(g_pendingApptId, apptId);
            SwitchView(GetParent(hWnd), NAV_DOCTOR_CONSULTATION);
        }
        return 0;
    }
    default:
        return DefWindowProcA(hWnd, msg, wParam, lParam);
    }
}

static HWND CreateReminderPage(HWND hParent, RECT *rc) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = ReminderPageWndProc;
    wc.hInstance     = g_hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "DocRemPage";
    RegisterClassA(&wc);

    HWND hPage = CreateWindowExA(0, "DocRemPage", "",
        WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
        rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top,
        hParent, NULL, g_hInst, (LPVOID)(INT_PTR)NAV_DOCTOR_REMINDER);
    if (!hPage) return NULL;

    int w = (rc->right - rc->left) - 10;
    int h = (rc->bottom - rc->top) - 10;

    HWND hLV = CreateListView(hPage, 3001, 5, 5, w - 10, h - 50);
    AddCol(hLV, 0, "预约ID", 100);
    AddCol(hLV, 1, "患者ID", 80);
    AddCol(hLV, 2, "日期", 100);
    AddCol(hLV, 3, "时段", 60);
    AddCol(hLV, 4, "状态", 60);

    const char *did = GetDoctorId();
    AppointmentNode *apps = load_appointments_list();
    int row = 0;
    if (apps && strlen(did) > 0) {
        AppointmentNode *cur = apps;
        while (cur) {
            if (strcmp(cur->data.doctor_id, did) == 0 &&
                strcmp(cur->data.status, "待就诊") == 0) {
                const char *items[5] = {
                    cur->data.appointment_id, cur->data.patient_id,
                    cur->data.appointment_date, cur->data.appointment_time,
                    cur->data.status
                };
                AddRow(hLV, row++, 5, items);
            }
            cur = cur->next;
        }
    }
    if (apps) free_appointment_list(apps);

    char info[64];
    snprintf(info, sizeof(info), "待接诊: %d 人", row);
    CreateWindowA("STATIC", info, WS_VISIBLE | WS_CHILD | SS_LEFT,
                  5, h - 40, 200, 25, hPage, NULL, g_hInst, NULL);

    CreateWindowA("BUTTON", "接诊选中患者",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        w - 120, h - 40, 110, 30,
        hPage, (HMENU)3101, g_hInst, NULL);

    return hPage;
}

/* ─── 接诊页面 / Consultation Page ───────────────────────────────────── */

/* ConsultationPageWndProc: 选择待就诊患者 → 填写诊断+治疗建议 →
   保存病历 → 更新预约状态为"已就诊" → 推进患者治疗阶段 → 可选开药 */

static LRESULT CALLBACK ConsultationPageWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCT *cs = (CREATESTRUCT *)lParam;
        SetWindowLongPtrA(hWnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        return 0;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == 3201) {
            const char *did = GetDoctorId();
            /* 保存诊断: 创建病历→更新预约→推进治疗阶段→询问开药
               Save: create medical record → update appointment → advance stage → offer Rx */
            if (strlen(did) == 0) {
                MessageBoxA(GetParent(hWnd), "未找到医生信息", "错误", MB_OK | MB_ICONERROR);
                return 0;
            }

            HWND hLV = GetDlgItem(hWnd, 3002);
            HWND hDiag = GetDlgItem(hWnd, 3202);
            HWND hAdvice = GetDlgItem(hWnd, 3203);
            if (!hLV || !hDiag || !hAdvice) return 0;

            char apptId[MAX_ID] = "";
            GetSelectedItemText(hLV, 0, apptId, sizeof(apptId));
            if (apptId[0] == 0) {
                MessageBoxA(GetParent(hWnd), "请先选择一个患者", "提示", MB_OK | MB_ICONINFORMATION);
                return 0;
            }

            char diagnosis[500] = "", advice[500] = "";
            GetWindowTextA(hDiag, diagnosis, sizeof(diagnosis));
            GetWindowTextA(hAdvice, advice, sizeof(advice));

            if (diagnosis[0] == 0) {
                MessageBoxA(GetParent(hWnd), "请输入诊断内容", "提示", MB_OK | MB_ICONINFORMATION);
                return 0;
            }

            /* 加载预约 */
            AppointmentNode *apps = load_appointments_list();
            Appointment *appt = NULL;
            if (apps) {
                AppointmentNode *cur = apps;
                while (cur) {
                    if (strcmp(cur->data.appointment_id, apptId) == 0) {
                        appt = &cur->data;
                        break;
                    }
                    cur = cur->next;
                }
            }
            if (!appt) {
                if (apps) free_appointment_list(apps);
                MessageBoxA(GetParent(hWnd), "未找到预约记录", "错误", MB_OK | MB_ICONERROR);
                return 0;
            }

            /* 创建就诊记录 */
            MedicalRecord rec;
            memset(&rec, 0, sizeof(rec));
            generate_id(rec.record_id, sizeof(rec.record_id), "MR");
            strcpy(rec.patient_id, appt->patient_id);
            strcpy(rec.doctor_id, did);
            strcpy(rec.appointment_id, apptId);
            snprintf(rec.diagnosis, sizeof(rec.diagnosis), "%s | 治疗建议: %s", diagnosis, advice);
            get_current_time(rec.diagnosis_date, sizeof(rec.diagnosis_date));
            strcpy(rec.status, "已就诊");

            /* 在释放 apps 之前保存 patient_id (appt 指针指向 apps 内部数据)
               Save patient_id before freeing apps (appt points into apps data) */
            char savedPatientId[MAX_ID];
            strcpy(savedPatientId, appt->patient_id);

            MedicalRecordNode *recs = load_medical_records_list();
            MedicalRecordNode *recNode = create_medical_record_node(&rec);
            if (recNode) {
                recNode->next = recs;
                save_medical_records_list(recNode);
                /* recNode->next 指向 recs，一并释放 */
                free_medical_record_list(recNode);
            } else if (recs) {
                free_medical_record_list(recs);
            }
            /* recs 已通过上方的分支释放，不再重复释放 */

            /* 更新预约状态 */
            strcpy(appt->status, "已就诊");
            save_appointments_list(apps);
            free_appointment_list(apps);

            /* 更新患者治疗阶段（此时 appt 已失效，用 savedPatientId） */
            PatientNode *pts = load_patients_list();
            if (pts) {
                PatientNode *cur = pts;
                while (cur) {
                    if (strcmp(cur->data.patient_id, savedPatientId) == 0) {
                        const char *next = get_next_stage(cur->data.treatment_stage);
                        if (next) strcpy(cur->data.treatment_stage, next);
                        break;
                    }
                    cur = cur->next;
                }
                save_patients_list(pts);
                free_patient_list(pts);
            }

            append_log(g_currentUser.username, "接诊", "appointment", apptId, diagnosis);

            MessageBoxA(GetParent(hWnd), "诊断已保存", "成功", MB_OK | MB_ICONINFORMATION);

            if (MessageBoxA(GetParent(hWnd), "是否需要开药？", "开药",
                            MB_YESNO | MB_ICONQUESTION) == IDYES) {
                ConsultData rxData;
                memset(&rxData, 0, sizeof(rxData));
                strcpy(rxData.record_id, rec.record_id);
                strcpy(rxData.patient_id, savedPatientId);
                strcpy(rxData.doctor_id, did);
                ShowDrugDispenseDialog(GetParent(hWnd), &rxData);
            }

            PostMessage(GetParent(hWnd), WM_APP_REFRESH, NAV_DOCTOR_CONSULTATION, 0);
        }
        return 0;
    }
    default:
        return DefWindowProcA(hWnd, msg, wParam, lParam);
    }
}

static HWND CreateConsultationPage(HWND hParent, RECT *rc) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = ConsultationPageWndProc;
    wc.hInstance     = g_hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "DocConsPage";
    RegisterClassA(&wc);

    HWND hPage = CreateWindowExA(0, "DocConsPage", "",
        WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
        rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top,
        hParent, NULL, g_hInst, (LPVOID)(INT_PTR)NAV_DOCTOR_CONSULTATION);
    if (!hPage) return NULL;

    int w = (rc->right - rc->left) - 20;
    int y = 10;

    CreateWindowA("STATIC", "选择待就诊患者:",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        10, y, 300, 20, hPage, NULL, g_hInst, NULL);
    y += 25;

    HWND hLV = CreateListView(hPage, 3002, 10, y, w, 120);
    AddCol(hLV, 0, "预约ID", 100);
    AddCol(hLV, 1, "患者ID", 100);
    AddCol(hLV, 2, "日期", 100);
    AddCol(hLV, 3, "时段", 60);
    AddCol(hLV, 4, "状态", 80);

    const char *did = GetDoctorId();
    AppointmentNode *apps = load_appointments_list();
    int row = 0, preSelectRow = -1;
    if (apps && strlen(did) > 0) {
        int idx = 0;
        AppointmentNode *cur = apps;
        while (cur) {
            if (strcmp(cur->data.doctor_id, did) == 0 &&
                strcmp(cur->data.status, "待就诊") == 0) {
                const char *items[5] = {
                    cur->data.appointment_id, cur->data.patient_id,
                    cur->data.appointment_date, cur->data.appointment_time,
                    cur->data.status
                };
                AddRow(hLV, row++, 5, items);
                if (g_pendingApptId[0] &&
                    strcmp(cur->data.appointment_id, g_pendingApptId) == 0) {
                    preSelectRow = idx;
                }
                idx++;
            }
            cur = cur->next;
        }
    }
    if (apps) free_appointment_list(apps);
    if (preSelectRow >= 0)
        ListView_SetItemState(hLV, preSelectRow, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    g_pendingApptId[0] = 0;

    y += 130;

    CreateWindowA("STATIC", "诊断:",
        WS_VISIBLE | WS_CHILD | SS_LEFT, 10, y, 100, 20,
        hPage, NULL, g_hInst, NULL);
    y += 22;

    CreateWindowA("EDIT", "",
        WS_VISIBLE | WS_CHILD | ES_MULTILINE | WS_BORDER | WS_VSCROLL,
        10, y, w, 80, hPage, (HMENU)3202, g_hInst, NULL);
    y += 90;

    CreateWindowA("STATIC", "治疗建议:",
        WS_VISIBLE | WS_CHILD | SS_LEFT, 10, y, 100, 20,
        hPage, NULL, g_hInst, NULL);
    y += 22;

    CreateWindowA("EDIT", "",
        WS_VISIBLE | WS_CHILD | ES_MULTILINE | WS_BORDER | WS_VSCROLL,
        10, y, w, 80, hPage, (HMENU)3203, g_hInst, NULL);
    y += 90;

    CreateWindowA("BUTTON", "保存诊断并标记已就诊",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        10, y, 160, 30, hPage, (HMENU)3201, g_hInst, NULL);

    return hPage;
}



/* ─── 病房呼叫页面 / Ward Call Page ──────────────────────────────────── */

static LRESULT CALLBACK WardCallPageWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCT *cs = (CREATESTRUCT *)lParam;
        SetWindowLongPtrA(hWnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        return 0;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == 3401) {
            HWND hLV = GetDlgItem(hWnd, 3004);
            if (!hLV) return 0;
            char callId[MAX_ID] = "";
            GetSelectedItemText(hLV, 0, callId, sizeof(callId));
            if (callId[0] == 0) {
                MessageBoxA(GetParent(hWnd), "请先选择一个呼叫", "提示", MB_OK | MB_ICONINFORMATION);
                return 0;
            }

            WardCallNode *calls = load_ward_calls_list();
            if (calls) {
                WardCallNode *cur = calls;
                while (cur) {
                    if (strcmp(cur->data.call_id, callId) == 0) {
                        strcpy(cur->data.status, "已响应");
                        break;
                    }
                    cur = cur->next;
                }
                save_ward_calls_list(calls);
                free_ward_call_list(calls);
            }
            append_log(g_currentUser.username, "响应呼叫", "ward_call", callId, "");
            MessageBoxA(GetParent(hWnd), "已标记为已响应", "成功", MB_OK | MB_ICONINFORMATION);
            PostMessage(GetParent(hWnd), WM_APP_REFRESH, NAV_DOCTOR_WARD_CALL, 0);
        }
        return 0;
    }
    default:
        return DefWindowProcA(hWnd, msg, wParam, lParam);
    }
}

static HWND CreateWardCallPage(HWND hParent, RECT *rc) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = WardCallPageWndProc;
    wc.hInstance     = g_hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "DocWCnPage";
    RegisterClassA(&wc);

    HWND hPage = CreateWindowExA(0, "DocWCnPage", "",
        WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
        rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top,
        hParent, NULL, g_hInst, (LPVOID)(INT_PTR)NAV_DOCTOR_WARD_CALL);
    if (!hPage) return NULL;

    int w = (rc->right - rc->left) - 10;
    int h = (rc->bottom - rc->top) - 10;

    HWND hLV = CreateListView(hPage, 3004, 5, 5, w - 10, h - 50);
    AddCol(hLV, 0, "呼叫ID", 80);
    AddCol(hLV, 1, "病房", 80);
    AddCol(hLV, 2, "患者ID", 80);
    AddCol(hLV, 3, "消息", 200);
    AddCol(hLV, 4, "状态", 60);

    WardCallNode *calls = load_ward_calls_list();
    int row = 0;
    if (calls) {
        WardCallNode *cur = calls;
        while (cur) {
            const char *items[5] = {
                cur->data.call_id, cur->data.ward_id,
                cur->data.patient_id, cur->data.message, cur->data.status
            };
            AddRow(hLV, row++, 5, items);
            cur = cur->next;
        }
        free_ward_call_list(calls);
    }

    CreateWindowA("BUTTON", "标记已响应",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        w - 100, h - 40, 90, 30, hPage, (HMENU)3401, g_hInst, NULL);

    return hPage;
}

/* ─── 紧急标记页面 / Emergency Flag Page ─────────────────────────────── */

static LRESULT CALLBACK EmergencyPageWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCT *cs = (CREATESTRUCT *)lParam;
        SetWindowLongPtrA(hWnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        return 0;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == 3501) {
            HWND hLV = GetDlgItem(hWnd, 3005);
            if (!hLV) return 0;
            char patientId[MAX_ID] = "";
            GetSelectedItemText(hLV, 0, patientId, sizeof(patientId));
            if (patientId[0] == 0) {
                MessageBoxA(GetParent(hWnd), "请先选择一个患者", "提示", MB_OK | MB_ICONINFORMATION);
                return 0;
            }

            PatientNode *pts = load_patients_list();
            if (pts) {
                PatientNode *cur = pts;
                while (cur) {
                    if (strcmp(cur->data.patient_id, patientId) == 0) {
                        cur->data.is_emergency = !cur->data.is_emergency;
                        break;
                    }
                    cur = cur->next;
                }
                save_patients_list(pts);
                free_patient_list(pts);
            }
            append_log(g_currentUser.username, "紧急标记", "patient", patientId, "");
            PostMessage(GetParent(hWnd), WM_APP_REFRESH, NAV_DOCTOR_EMERGENCY, 0);
        }
        return 0;
    }
    default:
        return DefWindowProcA(hWnd, msg, wParam, lParam);
    }
}

static HWND CreateEmergencyPage(HWND hParent, RECT *rc) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = EmergencyPageWndProc;
    wc.hInstance     = g_hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "DocEmgPage";
    RegisterClassA(&wc);

    HWND hPage = CreateWindowExA(0, "DocEmgPage", "",
        WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
        rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top,
        hParent, NULL, g_hInst, (LPVOID)(INT_PTR)NAV_DOCTOR_EMERGENCY);
    if (!hPage) return NULL;

    int w = (rc->right - rc->left) - 40;

    CreateWindowA("STATIC", "选择患者后点击按钮切换紧急状态",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        20, 20, 400, 20, hPage, NULL, g_hInst, NULL);

    HWND hLV = CreateListView(hPage, 3005, 20, 50, w, 200);
    AddCol(hLV, 0, "患者ID", 80);
    AddCol(hLV, 1, "姓名", 100);
    AddCol(hLV, 2, "紧急", 50);

    PatientNode *pts = load_patients_list();
    int row = 0;
    if (pts) {
        PatientNode *cur = pts;
        while (cur) {
            const char *items[3] = {
                cur->data.patient_id, cur->data.name,
                cur->data.is_emergency ? "是" : "否"
            };
            AddRow(hLV, row++, 3, items);
            cur = cur->next;
        }
        free_patient_list(pts);
    }

    CreateWindowA("BUTTON", "标记/取消紧急",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        20, 260, 120, 30, hPage, (HMENU)3501, g_hInst, NULL);

    return hPage;
}

/* ─── 进度更新页面 / Progress Update Page ────────────────────────────── */

static LRESULT CALLBACK ProgressPageWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCT *cs = (CREATESTRUCT *)lParam;
        SetWindowLongPtrA(hWnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        return 0;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == 3601) {
            HWND hLV = GetDlgItem(hWnd, 3006);
            if (!hLV) return 0;
            char patientId[MAX_ID] = "";
            GetSelectedItemText(hLV, 0, patientId, sizeof(patientId));
            if (patientId[0] == 0) {
                MessageBoxA(GetParent(hWnd), "请先选择一个患者", "提示", MB_OK | MB_ICONINFORMATION);
                return 0;
            }

            PatientNode *pts = load_patients_list();
            if (pts) {
                PatientNode *cur = pts;
                while (cur) {
                    if (strcmp(cur->data.patient_id, patientId) == 0) {
                        const char *next = get_next_stage(cur->data.treatment_stage);
                        if (!next) {
                            MessageBoxA(GetParent(hWnd), "该患者已处于最终治疗阶段", "提示", MB_OK | MB_ICONINFORMATION);
                        } else {
                            char msg[200];
                            snprintf(msg, sizeof(msg), "治疗阶段: %s → %s",
                                     cur->data.treatment_stage, next);
                            strcpy(cur->data.treatment_stage, next);
                            save_patients_list(pts);
                            append_log(g_currentUser.username, "更新阶段", "patient", patientId, next);
                            MessageBoxA(GetParent(hWnd), msg, "阶段已更新", MB_OK | MB_ICONINFORMATION);
                        }
                        break;
                    }
                    cur = cur->next;
                }
                free_patient_list(pts);
            }
            PostMessage(GetParent(hWnd), WM_APP_REFRESH, NAV_DOCTOR_PROGRESS, 0);
        }
        return 0;
    }
    default:
        return DefWindowProcA(hWnd, msg, wParam, lParam);
    }
}

static HWND CreateProgressPage(HWND hParent, RECT *rc) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = ProgressPageWndProc;
    wc.hInstance     = g_hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "DocProgPage";
    RegisterClassA(&wc);

    HWND hPage = CreateWindowExA(0, "DocProgPage", "",
        WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
        rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top,
        hParent, NULL, g_hInst, (LPVOID)(INT_PTR)NAV_DOCTOR_PROGRESS);
    if (!hPage) return NULL;

    int w = (rc->right - rc->left) - 40;

    CreateWindowA("STATIC", "选择患者后点击按钮推进治疗阶段",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        20, 20, 300, 20, hPage, NULL, g_hInst, NULL);

    HWND hLV = CreateListView(hPage, 3006, 20, 50, w, 200);
    AddCol(hLV, 0, "患者ID", 80);
    AddCol(hLV, 1, "姓名", 100);
    AddCol(hLV, 2, "当前阶段", 120);

    PatientNode *pts = load_patients_list();
    int row = 0;
    if (pts) {
        PatientNode *cur = pts;
        while (cur) {
            const char *items[3] = {
                cur->data.patient_id, cur->data.name, cur->data.treatment_stage
            };
            AddRow(hLV, row++, 3, items);
            cur = cur->next;
        }
        free_patient_list(pts);
    }

    CreateWindowA("BUTTON", "更新选中患者阶段",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        20, 260, 150, 30, hPage, (HMENU)3601, g_hInst, NULL);

    return hPage;
}

/* ─── 病历模板页面 / Medical Template Page ──────────────────────────── */

/* TemplatePageWndProc: 模板 CRUD (新增/修改/删除/刷新)
   模板编辑通过 TemplateEditDlgProc 模态对话框完成 */

static MedicalTemplate g_editTmpl;
static int g_tmplResult = -1;

static void RefreshTmplList(HWND hPage) {
    HWND hLV = GetDlgItem(hPage, 3007);
    if (!hLV) return;
    ListView_DeleteAllItems(hLV);
    TemplateNode *tmpls = load_templates_list();
    int row = 0;
    if (tmpls) {
        TemplateNode *cur = tmpls;
        while (cur) {
            const char *items[4] = {
                cur->data.template_id, cur->data.category,
                cur->data.shortcut, cur->data.text
            };
            AddRow(hLV, row++, 4, items);
            cur = cur->next;
        }
        free_template_list(tmpls);
    }
}

static LRESULT CALLBACK TemplateEditDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        MedicalTemplate *tmpl = (MedicalTemplate *)((CREATESTRUCT *)lParam)->lpCreateParams;
        if (tmpl) memcpy(&g_editTmpl, tmpl, sizeof(MedicalTemplate));

        int w = 500, x = 15, y = 15;

        CreateWindowA("STATIC", "分类:", WS_VISIBLE | WS_CHILD | SS_LEFT,
                      x, y, 60, 20, hDlg, NULL, g_hInst, NULL);
        CreateWindowA("EDIT", g_editTmpl.category,
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            x + 70, y - 2, 200, 22, hDlg, (HMENU)100, g_hInst, NULL);
        y += 35;

        CreateWindowA("STATIC", "快捷码:", WS_VISIBLE | WS_CHILD | SS_LEFT,
                      x, y, 60, 20, hDlg, NULL, g_hInst, NULL);
        CreateWindowA("EDIT", g_editTmpl.shortcut,
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            x + 70, y - 2, 200, 22, hDlg, (HMENU)101, g_hInst, NULL);
        y += 35;

        CreateWindowA("STATIC", "内容:", WS_VISIBLE | WS_CHILD | SS_LEFT,
                      x, y, 60, 20, hDlg, NULL, g_hInst, NULL);
        CreateWindowA("EDIT", g_editTmpl.text,
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
            x + 70, y - 2, w - 100, 150, hDlg, (HMENU)102, g_hInst, NULL);
        y += 165;

        CreateWindowA("BUTTON", "确定", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                      w - 210, y, 90, 28, hDlg, (HMENU)1, g_hInst, NULL);
        CreateWindowA("BUTTON", "取消", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                      w - 110, y, 90, 28, hDlg, (HMENU)2, g_hInst, NULL);
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == 1) {
            GetDlgItemTextA(hDlg, 100, g_editTmpl.category, sizeof(g_editTmpl.category));
            GetDlgItemTextA(hDlg, 101, g_editTmpl.shortcut, sizeof(g_editTmpl.shortcut));
            GetDlgItemTextA(hDlg, 102, g_editTmpl.text, sizeof(g_editTmpl.text));

            if (g_editTmpl.category[0] == 0 || g_editTmpl.text[0] == 0) {
                MessageBoxA(hDlg, "分类和内容不能为空", "提示", MB_OK | MB_ICONINFORMATION);
                return 0;
            }
            g_tmplResult = 1;
            DestroyWindow(hDlg);
            return 0;
        }
        if (id == 2) {
            g_tmplResult = 0;
            DestroyWindow(hDlg);
            return 0;
        }
        return 0;
    }
    case WM_CLOSE:
        g_tmplResult = 0;
        DestroyWindow(hDlg);
        return 0;
    default:
        return DefWindowProcA(hDlg, msg, wParam, lParam);
    }
}

static int ShowTemplateEditDialog(HWND hParent, MedicalTemplate *tmpl, const char *title) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = TemplateEditDlgProc;
    wc.hInstance     = g_hInst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "TmplEditDialog";
    RegisterClassA(&wc);

    int w = 520, h = 270;
    HWND hDlg = CreateWindowExA(0, "TmplEditDialog", title,
        WS_VISIBLE | WS_POPUPWINDOW | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, w, h,
        hParent, NULL, g_hInst, (LPVOID)tmpl);
    if (!hDlg) return 0;

    RECT pr, rc;
    GetWindowRect(hParent, &pr);
    GetWindowRect(hDlg, &rc);
    SetWindowPos(hDlg, NULL,
        pr.left + (pr.right - pr.left - (rc.right - rc.left)) / 2,
        pr.top + (pr.bottom - pr.top - (rc.bottom - rc.top)) / 2,
        0, 0, SWP_NOSIZE | SWP_NOZORDER);

    EnableWindow(hParent, FALSE);
    g_tmplResult = -1;
    MSG msg;
    while (g_tmplResult == -1 && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    EnableWindow(hParent, TRUE);
    SetForegroundWindow(hParent);

    return g_tmplResult == 1;
}

static LRESULT CALLBACK TemplatePageWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCT *cs = (CREATESTRUCT *)lParam;
        SetWindowLongPtrA(hWnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        return 0;
    }
    case WM_SIZE: {
        int cx = LOWORD(lParam), cy = HIWORD(lParam);
        HWND hLV = GetDlgItem(hWnd, 3007);
        if (hLV) SetWindowPos(hLV, NULL, 0, 0, cx, cy - 50, SWP_NOZORDER);
        return 0;
    }
    case WM_COMMAND: {
        int cmd = LOWORD(wParam);
        if (HIWORD(wParam) != BN_CLICKED) return 0;

        HWND hLV = GetDlgItem(hWnd, 3007);

        if (cmd == 3604) { /* 刷新 */
            RefreshTmplList(hWnd);
            return 0;
        }

        if (cmd == 3601) { /* 新增 */
            MedicalTemplate tmpl;
            memset(&tmpl, 0, sizeof(tmpl));
            if (ShowTemplateEditDialog(hWnd, &tmpl, "新增模板")) {
                generate_id(tmpl.template_id, sizeof(tmpl.template_id), "T");
                TemplateNode *head = load_templates_list();
                TemplateNode *node = create_template_node(&tmpl);
                if (node) {
                    node->next = head;
                    save_templates_list(node);
                    free_template_list(node);
                } else if (head) {
                    free_template_list(head);
                }
                RefreshTmplList(hWnd);
            }
            return 0;
        }

        if (cmd == 3602) { /* 修改 */
            if (!hLV) return 0;
            int sel = ListView_GetNextItem(hLV, -1, LVNI_SELECTED);
            if (sel < 0) {
                MessageBoxA(GetParent(hWnd), "请先选择一个模板", "提示", MB_OK);
                return 0;
            }
            char tmplId[MAX_ID] = "";
            ListView_GetItemText(hLV, sel, 0, tmplId, sizeof(tmplId));

            TemplateNode *head = load_templates_list();
            TemplateNode *cur = head;
            MedicalTemplate found;
            memset(&found, 0, sizeof(found));
            while (cur) {
                if (strcmp(cur->data.template_id, tmplId) == 0) {
                    memcpy(&found, &cur->data, sizeof(MedicalTemplate));
                    break;
                }
                cur = cur->next;
            }

            int result = 0;
            if (found.template_id[0]) {
                if (ShowTemplateEditDialog(hWnd, &found, "修改模板")) {
                    /* Update in-place */
                    cur = head;
                    while (cur) {
                        if (strcmp(cur->data.template_id, tmplId) == 0) {
                            memcpy(&cur->data, &found, sizeof(MedicalTemplate));
                            break;
                        }
                        cur = cur->next;
                    }
                    save_templates_list(head);
                    result = 1;
                }
            } else {
                MessageBoxA(GetParent(hWnd), "未找到该模板", "错误", MB_OK | MB_ICONERROR);
            }
            free_template_list(head);
            if (result) RefreshTmplList(hWnd);
            return 0;
        }

        if (cmd == 3603) { /* 删除 */
            if (!hLV) return 0;
            int sel = ListView_GetNextItem(hLV, -1, LVNI_SELECTED);
            if (sel < 0) {
                MessageBoxA(GetParent(hWnd), "请先选择一个模板", "提示", MB_OK);
                return 0;
            }
            char tmplId[MAX_ID] = "";
            ListView_GetItemText(hLV, sel, 0, tmplId, sizeof(tmplId));

            if (MessageBoxA(GetParent(hWnd), "确定要删除该模板吗？", "确认删除",
                            MB_YESNO | MB_ICONQUESTION) != IDYES)
                return 0;

            TemplateNode *head = load_templates_list();
            TemplateNode *prev = NULL, *cur = head;
            while (cur) {
                if (strcmp(cur->data.template_id, tmplId) == 0) {
                    if (prev)
                        prev->next = cur->next;
                    else
                        head = cur->next;
                    free(cur);
                    break;
                }
                prev = cur;
                cur = cur->next;
            }
            save_templates_list(head);
            free_template_list(head);
            RefreshTmplList(hWnd);
            return 0;
        }

        return 0;
    }
    default:
        return DefWindowProcA(hWnd, msg, wParam, lParam);
    }
}

static HWND CreateTemplatePage(HWND hParent, RECT *rc) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = TemplatePageWndProc;
    wc.hInstance     = g_hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "DocTmplPage";
    RegisterClassA(&wc);

    HWND hPage = CreateWindowExA(0, "DocTmplPage", "",
        WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
        rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top,
        hParent, NULL, g_hInst, (LPVOID)(INT_PTR)NAV_DOCTOR_TEMPLATE);
    if (!hPage) return NULL;

    int w = (rc->right - rc->left) - 10;
    int h = (rc->bottom - rc->top) - 10;

    HWND hLV = CreateListView(hPage, 3007, 5, 5, w - 10, h - 95);
    AddCol(hLV, 0, "模板ID", 80);
    AddCol(hLV, 1, "分类", 80);
    AddCol(hLV, 2, "快捷码", 80);
    AddCol(hLV, 3, "内容", 300);

    int btnY = h - 40;
    int btnW = 80;
    CreateWindowA("BUTTON", "新增", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                  5, btnY, btnW, 30, hPage, (HMENU)3601, g_hInst, NULL);
    CreateWindowA("BUTTON", "修改", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                  5 + btnW + 10, btnY, btnW, 30, hPage, (HMENU)3602, g_hInst, NULL);
    CreateWindowA("BUTTON", "删除", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                  5 + (btnW + 10) * 2, btnY, btnW, 30, hPage, (HMENU)3603, g_hInst, NULL);
    CreateWindowA("BUTTON", "刷新", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                  5 + (btnW + 10) * 3, btnY, btnW, 30, hPage, (HMENU)3604, g_hInst, NULL);

    RefreshTmplList(hPage);
    return hPage;
}

/* ─── 开药页面 / Prescribe Page ──────────────────────────────────────── */

/* PrescribePageWndProc: 列出当前医生的所有病历 → 选择后打开开药对话框
   Lists all medical records for current doctor → open drug dispense dialog */

static void RefreshPrescribeList(HWND hLV) {
    ListView_DeleteAllItems(hLV);
    const char *did = GetDoctorId();
    if (!did || !did[0]) return;

    MedicalRecordNode *records = load_medical_records_list();
    if (!records) return;

    int row = 0;
    for (MedicalRecordNode *mr = records; mr; mr = mr->next) {
        if (strcmp(mr->data.doctor_id, did) != 0) continue;

        char pn[50] = "未知";
        PatientNode *pts = load_patients_list();
        if (pts) {
            for (PatientNode *p = pts; p; p = p->next) {
                if (strcmp(p->data.patient_id, mr->data.patient_id) == 0) {
                    strncpy(pn, p->data.name, sizeof(pn) - 1);
                    pn[sizeof(pn) - 1] = 0;
                    break;
                }
            }
            free_patient_list(pts);
        }

        const char *items[5] = {
            mr->data.record_id, mr->data.patient_id, pn,
            mr->data.diagnosis_date, mr->data.status
        };
        AddRow(hLV, row++, 5, items);
    }
    free_medical_record_list(records);
}

static LRESULT CALLBACK PrescribePageWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        SetWindowLongPtrA(hWnd, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCT *)lParam)->lpCreateParams);
        return 0;
    }
    case WM_SIZE: {
        int w = LOWORD(lParam) - 10;
        int h = HIWORD(lParam) - 10;
        HWND hLV = GetDlgItem(hWnd, 3801);
        if (hLV) SetWindowPos(hLV, NULL, 5, 5, w - 10, h - 50, SWP_NOZORDER);
        HWND hBtn = GetDlgItem(hWnd, 3802);
        if (hBtn) SetWindowPos(hBtn, NULL, 5, h - 40, 100, 30, SWP_NOZORDER);
        return 0;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == 3802) {
            HWND hLV = GetDlgItem(hWnd, 3801);
            if (!hLV) return 0;
            char recordId[MAX_ID] = "";
            GetSelectedItemText(hLV, 0, recordId, sizeof(recordId));
            if (recordId[0] == 0) {
                MessageBoxA(GetParent(hWnd), "请先选择一个病历", "提示", MB_OK | MB_ICONINFORMATION);
                return 0;
            }

            const char *did = GetDoctorId();
            if (!did || !did[0]) {
                MessageBoxA(GetParent(hWnd), "无法获取医生信息", "错误", MB_OK | MB_ICONERROR);
                return 0;
            }

            MedicalRecordNode *records = load_medical_records_list();
            char patientId[MAX_ID] = "";
            int found = 0;
            if (records) {
                for (MedicalRecordNode *mr = records; mr; mr = mr->next) {
                    if (strcmp(mr->data.record_id, recordId) == 0 &&
                        strcmp(mr->data.doctor_id, did) == 0) {
                        strcpy(patientId, mr->data.patient_id);
                        found = 1;
                        break;
                    }
                }
                free_medical_record_list(records);
            }

            if (!found) {
                MessageBoxA(GetParent(hWnd), "病历不存在或不属于当前医生", "错误", MB_OK | MB_ICONERROR);
                return 0;
            }

            ConsultData rxData;
            memset(&rxData, 0, sizeof(rxData));
            strcpy(rxData.record_id, recordId);
            strcpy(rxData.patient_id, patientId);
            strcpy(rxData.doctor_id, did);
            ShowDrugDispenseDialog(GetParent(hWnd), &rxData);

            PostMessage(GetParent(hWnd), WM_APP_REFRESH, NAV_DOCTOR_PRESCRIBE, 0);
        }
        return 0;
    }
    default:
        return DefWindowProcA(hWnd, msg, wParam, lParam);
    }
}

static HWND CreatePrescribePage(HWND hParent, RECT *rc) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = PrescribePageWndProc;
    wc.hInstance     = g_hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "DocRxPage";
    RegisterClassA(&wc);

    HWND hPage = CreateWindowExA(0, "DocRxPage", "",
        WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
        rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top,
        hParent, NULL, g_hInst, (LPVOID)(INT_PTR)NAV_DOCTOR_PRESCRIBE);
    if (!hPage) return NULL;

    int w = (rc->right - rc->left) - 10;
    int h = (rc->bottom - rc->top) - 10;

    HWND hLV = CreateListView(hPage, 3801, 5, 5, w - 10, h - 50);
    AddCol(hLV, 0, "病历ID", 100);
    AddCol(hLV, 1, "患者ID", 100);
    AddCol(hLV, 2, "患者姓名", 120);
    AddCol(hLV, 3, "诊断日期", 100);
    AddCol(hLV, 4, "状态", 80);

    CreateWindowA("BUTTON", "开药",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        5, h - 40, 100, 30, hPage, (HMENU)3802, g_hInst, NULL);

    RefreshPrescribeList(hLV);
    return hPage;
}

/* ─── 公开接口 / Public Interface ────────────────────────────────────── */

/* CreateDoctorPage — 工厂函数, 按 viewId 路由到各页面创建函数
   Factory function routing viewId to the appropriate page creator */

HWND CreateDoctorPage(HWND hParent, int viewId, RECT *rc) {
    switch (viewId) {
    case NAV_DOCTOR_REMINDER:     return CreateReminderPage(hParent, rc);
    case NAV_DOCTOR_CONSULTATION: return CreateConsultationPage(hParent, rc);
    case NAV_DOCTOR_WARD_CALL:    return CreateWardCallPage(hParent, rc);
    case NAV_DOCTOR_EMERGENCY:    return CreateEmergencyPage(hParent, rc);
    case NAV_DOCTOR_PROGRESS:     return CreateProgressPage(hParent, rc);
    case NAV_DOCTOR_TEMPLATE:     return CreateTemplatePage(hParent, rc);
    case NAV_DOCTOR_PRESCRIBE:    return CreatePrescribePage(hParent, rc);
    default: return NULL;
    }
}
