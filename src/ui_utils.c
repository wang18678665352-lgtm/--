#include "ui_utils.h"
#include "data_storage.h"

#ifdef _WIN32
#include <windows.h>
#endif

// =======================  行跟踪（原位高亮用） =======================
static int   g_line_counter      = 0;        // 当前菜单段落累计行数
static int   g_item_offsets[MAX_MENU_ITEMS]; // 每个菜单项的行号（从段落起点计）
static int   g_saved_total       = 0;        // ui_menu_cache_fill 时固化的总行数
static int   g_saved_offsets[MAX_MENU_ITEMS];// ui_menu_cache_fill 时固化的偏移
static int   g_saved_count       = 0;

int utf8_display_width(const char *s) {
    int w = 0;
    if (!s) return 0;
    while (*s) {
        unsigned char c = (unsigned char)*s;
        if (c <= 0x7F) { w += 1; s += 1; }
        else if (c >= 0xE0) { w += 2; s += 3; }
        else if (c >= 0xC0) { w += 1; s += 2; }
        else { w += 1; s += 1; }
    }
    return w;
}

void ui_init_ansi(void) {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        if (GetConsoleMode(hOut, &mode)) {
            mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, mode);
        }
    }
#endif
}

void ui_clear_screen(void) {
    printf(C_RESET "\033[2J\033[H");
}

void ui_line(int width, const char *ch) {
    if (width <= 0) width = 60;
    printf(C_RESET);
    for (int i = 0; i < width; i++) {
        printf("%s", ch && ch[0] ? ch : "\xe2\x94\x80");
    }
    printf("\n");
}

void ui_box_top(const char *title) {
    ui_menu_cache_init();  // fresh cache for each menu
    int len = title ? utf8_display_width(title) : 0;
    int total = 58;
    int left = (total - len) / 2;
    int right = total - len - left;

    printf(C_BOLD C_CYAN);
    printf("\xe2\x95\x94");
    for (int i = 0; i < left; i++) printf("\xe2\x95\x90");
    if (title && len > 0) {
        printf(" ");
        printf(C_WHITE);
        printf("%s", title);
        printf(C_BOLD C_CYAN);
        printf(" ");
    }
    for (int i = 0; i < right; i++) printf("\xe2\x95\x90");
    printf("\xe2\x95\x97");
    printf(C_RESET "\n");
    g_line_counter++;
}

void ui_print_col(const char *s, int width) {
    int w = utf8_display_width(s);
    printf("%s", s);
    for (int i = w; i < width; i++) printf(" ");
    printf(" ");
}

void ui_print_col_int(int val, int width) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", val);
    ui_print_col(buf, width);
}

void ui_print_col_float(float val, int width) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.2f", val);
    ui_print_col(buf, width);
}

static int str_contains_icase(const char *haystack, const char *needle) {
    if (!haystack || !needle || !*needle) return 0;
    const char *h = haystack;
    while (*h) {
        const char *n = needle;
        const char *start = h;
        while (*n && *start) {
            char hc = *start, nc = *n;
            if (hc >= 'A' && hc <= 'Z') hc += 32;
            if (nc >= 'A' && nc <= 'Z') nc += 32;
            if (hc != nc) break;
            start++; n++;
        }
        if (!*n) return 1;
        h++;
    }
    return 0;
}

int smart_id_lookup(const char *prompt, const char *id_list[], int count, char *output, int out_size) {
    if (count <= 0) {
        char input[64];
        printf(S_LABEL "  %s: " C_RESET, prompt);
        if (read_input_line(input, sizeof(input)) == NULL) return -1;
        if (input[0] == 0) return -1;
        strncpy(output, input, out_size - 1);
        output[out_size - 1] = 0;
        return 1;
    }

    int idx = ui_search_list(prompt, id_list, count);
    if (idx < 0) return -1;
    strncpy(output, id_list[idx], out_size - 1);
    output[out_size - 1] = 0;
    return 1;
}

int smart_patient_input(const char *doctor_id, const char *prompt, char *output, int out_size) {
    const char *candidates[500];
    int count = 0;

    {
        AppointmentNode *ah = load_appointments_list();
        AppointmentNode *ac = ah;
        while (ac && count < 150) {
            if (strcmp(ac->data.doctor_id, doctor_id) == 0) {
                int dup = 0;
                int j;
                for (j = 0; j < count; j++)
                    if (strcmp(candidates[j], ac->data.patient_id) == 0) { dup = 1; break; }
                if (!dup) candidates[count++] = ac->data.patient_id;
            }
            ac = ac->next;
        }
        free_appointment_list(ah);
    }
    {
        OnsiteRegistrationQueue oq = load_onsite_registration_queue();
        OnsiteRegistrationNode *oc = oq.front;
        while (oc && count < 250) {
            if (strcmp(oc->data.doctor_id, doctor_id) == 0) {
                int dup = 0;
                int j;
                for (j = 0; j < count; j++)
                    if (strcmp(candidates[j], oc->data.patient_id) == 0) { dup = 1; break; }
                if (!dup) candidates[count++] = oc->data.patient_id;
            }
            oc = oc->next;
        }
        free_onsite_registration_queue(&oq);
    }

    return smart_id_lookup(prompt, candidates, count, output, out_size);
}

int smart_drug_input(const char *prompt, char *output, int out_size) {
    DrugNode *dh = load_drugs_list();
    DrugNode *dc = dh;
    char *drug_list[300];
    char  drug_buf[300][MAX_ID + MAX_NAME + 4];
    int count = 0;

    while (dc && count < 200) {
        snprintf(drug_buf[count], sizeof(drug_buf[0]), "%s (%s)", dc->data.drug_id, dc->data.name);
        drug_list[count] = drug_buf[count];
        count++;
        dc = dc->next;
    }
    free_drug_list(dh);

    int result = smart_id_lookup(prompt, (const char **)drug_list, count, output, out_size);

    if (result == 1) {
        char *paren = strchr(output, ' ');
        if (paren) *paren = 0;
    }
    return result;
}

int quick_template_input(const char *category, const char *prompt, char *output, int out_size) {
    TemplateNode *head = load_templates_list();
    TemplateNode *cur = head;

    const char *tlist[200];
    char ttext[200][600];
    int tcount = 0;

    while (cur && tcount < 100) {
        if (strcmp(cur->data.category, category) == 0) {
            snprintf(ttext[tcount], sizeof(ttext[0]), "%s | %s",
                     cur->data.shortcut, cur->data.text);
            tlist[tcount] = ttext[tcount];
            tcount++;
        }
        cur = cur->next;
    }
    free_template_list(head);

    if (tcount == 0) {
        printf(S_LABEL "  %s" C_RESET " (直接输入): ", prompt);
        if (read_input_line(output, out_size) == NULL) return -1;
        if (output[0] == 0) return -1;
        return 1;
    }

    printf("\n" S_LABEL "  %s" C_RESET "\n", prompt);
    printf(C_DIM "  [直接输入] / [#n=模板] / [v=查看全部模板]" C_RESET "\n");

    if (read_input_line(output, out_size) == NULL) return -1;

    if (output[0] == 0) return -1;

    if (strcmp(output, "v") == 0 || strcmp(output, "V") == 0) {
        printf("\n" S_LABEL "  %s 模板列表:" C_RESET "\n", category);
        ui_divider();
        int i;
        for (i = 0; i < tcount; i++) {
            printf("  " C_BOLD C_YELLOW "%2d." C_RESET " %s\n", i + 1, tlist[i]);
        }
        printf(S_LABEL "  请输入内容或选序号: " C_RESET);
        if (read_input_line(output, out_size) == NULL) return -1;
        if (output[0] == 0) return -1;
    }

    // Handle "#n" template syntax
    if (output[0] == '#') {
        char *num_str = output + 1;
        int is_digits = 1;
        for (char *p = num_str; *p; p++) {
            if (*p < '0' || *p > '9') { is_digits = 0; break; }
        }
        if (is_digits && num_str[0] != '\0') {
            int idx = atoi(num_str) - 1;
            if (idx >= 0 && idx < tcount) {
                TemplateNode *th = load_templates_list();
                TemplateNode *tc = th;
                int cnt = 0;
                while (tc) {
                    if (strcmp(tc->data.category, category) == 0) {
                        if (cnt == idx) {
                            strncpy(output, tc->data.text, out_size - 1);
                            output[out_size - 1] = '\0';
                            free_template_list(th);
                            return 1;
                        }
                        cnt++;
                    }
                    tc = tc->next;
                }
                free_template_list(th);
            }
        }
        return 1;
    }

    {
        int is_all_digits = 1;
        char *p = output;
        while (*p) { if (*p < '0' || *p > '9') { is_all_digits = 0; break; } p++; }
        if (is_all_digits && output[0]) {
            int idx = atoi(output) - 1;
            if (idx >= 0 && idx < tcount) {
                TemplateNode *th = load_templates_list();
                TemplateNode *tc = th;
                int cnt = 0;
                while (tc) {
                    if (strcmp(tc->data.category, category) == 0) {
                        if (cnt == idx) {
                            strncpy(output, tc->data.text, out_size - 1);
                            output[out_size - 1] = 0;
                            free_template_list(th);
                            return 1;
                        }
                        cnt++;
                    }
                    tc = tc->next;
                }
                free_template_list(th);
            }
        }
    }

    return 1;
}

void ui_box_bottom(void) {
    printf(C_BOLD C_CYAN);
    printf("\xe2\x95\x9a");
    for (int i = 0; i < 58; i++) printf("\xe2\x95\x90");
    printf("\xe2\x95\x9d");
    printf(C_RESET "\n");
    g_line_counter++;
}

void ui_header(const char *title) {
    printf("\n");
    printf(C_BOLD C_CYAN);
    printf("  \xe2\x94\x81\xe2\x94\x81\xe2\x94\x81  ");
    printf(C_WHITE);
    printf("%s", title);
    printf(C_BOLD C_CYAN);
    printf("  \xe2\x94\x81\xe2\x94\x81\xe2\x94\x81");
    printf(C_RESET "\n");
    g_line_counter += 2;  // blank line + title line
}

void ui_sub_header(const char *title) {
    printf("\n");
    printf(S_LABEL);
    printf("  \xe2\x96\xb8 ");
    printf(C_RESET);
    printf(C_BOLD);
    printf("%s", title);
    printf(C_RESET "\n");
    g_line_counter += 2;  // blank line + title line
}

void ui_divider(void) {
    printf(C_DIM);
    printf("  ");
    for (int i = 0; i < 58; i++) printf("\xe2\x94\x80");
    printf(C_RESET "\n");
    g_line_counter++;
}

void ui_ok(const char *msg) {
    printf(S_SUCCESS);
    printf("  \xe2\x9c\x93 %s", msg);
    printf(C_RESET "\n");
}

void ui_err(const char *msg) {
    printf(S_ERROR);
    printf("  \xe2\x9c\x97 %s", msg);
    printf(C_RESET "\n");
}

void ui_warn(const char *msg) {
    printf(S_WARNING);
    printf("  \xe2\x9a\xa0 %s", msg);
    printf(C_RESET "\n");
}

void ui_info(const char *label, const char *value) {
    int vw = utf8_display_width(label);
    int pad = 20 - vw;
    if (pad < 1) pad = 1;

    printf("  ");
    printf(S_LABEL);
    printf("%s", label);
    printf(C_RESET);
    for (int i = 0; i < pad; i++) printf(" ");
    printf("%s", value ? value : "");
    printf("\n");
}

// =======================  菜单项缓存（用于方向键高亮） =======================
static int   g_menu_nums[MAX_MENU_ITEMS];
static char  g_menu_texts[MAX_MENU_ITEMS][60];
static bool  g_menu_exit[MAX_MENU_ITEMS];
static int   g_menu_count = 0;

void ui_menu_cache_init(void) {
    g_menu_count = 0;
    g_line_counter = 0;
}

int ui_menu_cache_fill(int *nums, char texts[][60], bool *is_exit, int max) {
    int n = (g_menu_count < max) ? g_menu_count : max;
    for (int i = 0; i < n; i++) {
        nums[i] = g_menu_nums[i];
        strncpy(texts[i], g_menu_texts[i], 59);
        texts[i][59] = 0;
        is_exit[i] = g_menu_exit[i];
    }

    // Save line-tracking snapshot before reset
    g_saved_total = g_line_counter;
    for (int i = 0; i < n && i < MAX_MENU_ITEMS; i++) {
        g_saved_offsets[i] = g_item_offsets[i];
    }
    g_saved_count = n;

    ui_menu_cache_init();  // reset for next menu
    return n;
}

void ui_menu_item(int num, const char *text) {
    if (g_menu_count < MAX_MENU_ITEMS) {
        g_item_offsets[g_menu_count] = g_line_counter;
        g_menu_nums[g_menu_count] = num;
        strncpy(g_menu_texts[g_menu_count], text, 59);
        g_menu_texts[g_menu_count][59] = 0;
        g_menu_exit[g_menu_count] = false;
        g_menu_count++;
    }
    printf("  ");
    printf(C_BOLD C_YELLOW);
    printf("%d.", num);
    printf(C_RESET);
    printf(" %s\n", text);
    g_line_counter++;
}

void ui_menu_exit(int num, const char *text) {
    if (g_menu_count < MAX_MENU_ITEMS) {
        g_item_offsets[g_menu_count] = g_line_counter;
        g_menu_nums[g_menu_count] = num;
        strncpy(g_menu_texts[g_menu_count], text, 59);
        g_menu_texts[g_menu_count][59] = 0;
        g_menu_exit[g_menu_count] = true;
        g_menu_count++;
    }
    printf("  ");
    printf(C_DIM);
    printf("%d.", num);
    printf(" %s", text);
    printf(C_RESET "\n");
    g_line_counter++;
}

bool ui_confirm(const char *prompt) {
    printf("\n" S_WARNING "  ? %s" C_RESET " [" C_BOLD C_GREEN "Y" C_RESET "/" C_DIM "n" C_RESET "]: ", prompt ? prompt : "确认执行");
    fflush(stdout);
    int c = getchar();
    if (c != '\n' && c != '\r') clear_input_buffer();
    return (c == '\n' || c == '\r' || c == 'y' || c == 'Y');
}

void ui_user_badge(const char *name, const char *role) {
    const char *role_label = "";
    if (strcmp(role, "admin") == 0) role_label = "\xe7\xae\xa1 \xe7\x90\x86 \xe5\x91\x98";
    else if (strcmp(role, "doctor") == 0) role_label = "\xe5\x8c\xbb    \xe7\x94\x9f";
    else if (strcmp(role, "patient") == 0) role_label = "\xe6\x82\xa3    \xe8\x80\x85";

    printf(C_BOLD C_CYAN);
    printf("  \xf0\x9f\x91\xa4 ");
    printf(C_WHITE);
    printf("%s", name);
    printf(C_RESET);
    printf("  ");
    printf(BG_CYAN C_BOLD);
    printf(" %s ", role_label);
    printf(C_RESET "\n");
    g_line_counter++;
}

// =======================  菜单行跟踪接口 =======================
void ui_menu_track_line(void) {
    g_line_counter++;
}

int ui_menu_get_saved_total(void) {
    return g_saved_total;
}

int ui_menu_get_item_offset(int idx) {
    if (idx >= 0 && idx < g_saved_count) return g_saved_offsets[idx];
    return 0;
}

// =======================  分页与搜索 =======================
void ui_paginate(const char **items, int count, int page_size, const char *title) {
    if (count <= 0) {
        ui_warn("暂无数据。");
        return;
    }

    if (page_size <= 0) page_size = 15;

    int total_pages = (count + page_size - 1) / page_size;
    int page = 0;

    while (page < total_pages) {
        if (title) {
            char buf[128];
            snprintf(buf, sizeof(buf), "%s (共%d条)", title, count);
            ui_sub_header(buf);
        }

        int start = page * page_size;
        int end = start + page_size;
        if (end > count) end = count;

        for (int i = start; i < end; i++) {
            printf("  " C_BOLD C_YELLOW "%d." C_RESET " %s\n", i + 1, items[i]);
        }

        if (total_pages > 1) {
            printf(C_DIM "  ─── 第 %d/%d 页", page + 1, total_pages);
            if (page < total_pages - 1) {
                printf("，回车继续");
            }
            printf("，q 返回，s 搜索" C_RESET "\n");

            int c = getchar();
            if (c != '\n') clear_input_buffer();

            if (c == 'q' || c == 'Q') break;

            if (c == 's' || c == 'S') {
                char keyword[64];
                printf(S_LABEL "  输入关键字: " C_RESET);
                if (read_input_line(keyword, sizeof(keyword)) == NULL) continue;
                if (keyword[0] == '\0') continue;

                int match_idx[count];
                int match_cnt = 0;
                for (int i = 0; i < count; i++) {
                    if (str_contains_icase(items[i], keyword)) {
                        match_idx[match_cnt++] = i;
                    }
                }

                if (match_cnt == 0) {
                    ui_warn("未匹配到结果。");
                    continue;
                }

                const char *filtered[match_cnt];
                for (int i = 0; i < match_cnt; i++) {
                    filtered[i] = items[match_idx[i]];
                }

                char search_title[128];
                if (title) {
                    snprintf(search_title, sizeof(search_title), "%s 搜索结果", title);
                } else {
                    snprintf(search_title, sizeof(search_title), "搜索结果");
                }
                ui_paginate(filtered, match_cnt, page_size, search_title);
                continue;
            }

            if (c != '\n' && page >= total_pages - 1) break;
        }

        page++;
    }
}

int ui_search_list(const char *prompt, const char **items, int count) {
    if (count <= 0) {
        ui_warn("暂无数据。");
        return -1;
    }

    char input[64];
    int last_matches[500];
    int last_mcount = 0;

    while (1) {
        printf(S_LABEL "  %s" C_RESET " (" C_DIM "输入关键字搜索, 序号直接选, q取消" C_RESET "): ",
               prompt ? prompt : "搜索");
        if (read_input_line(input, sizeof(input)) == NULL) return -1;

        if (strcmp(input, "q") == 0 || strcmp(input, "Q") == 0) return -1;
        if (input[0] == '\0') continue;

        // Try direct number selection
        {
            int is_digits = 1;
            for (char *p = input; *p; p++) {
                if (*p < '0' || *p > '9') { is_digits = 0; break; }
            }
            if (is_digits) {
                int max_idx = (last_mcount > 0) ? last_mcount : count;
                int idx = atoi(input) - 1;
                if (idx >= 0 && idx < max_idx) {
                    if (last_mcount > 0) return last_matches[idx];
                    return idx;
                }
                printf(S_ERROR "  序号超出范围 (1-%d)!" C_RESET "\n", max_idx);
                continue;
            }
        }

        // Substring search
        int matches[500];
        int mcount = 0;
        for (int i = 0; i < count && mcount < 500; i++) {
            if (str_contains_icase(items[i], input)) {
                matches[mcount++] = i;
            }
        }

        if (mcount == 0) {
            ui_err("未匹配到任何结果。");
            continue;
        }

        if (mcount == 1) {
            printf(S_SUCCESS "  %s" C_RESET "\n", items[matches[0]]);
            return matches[0];
        }

        // Store matches for subsequent number selection
        memcpy(last_matches, matches, mcount * sizeof(int));
        last_mcount = mcount;

        // Show results (first 20)
        printf("\n" S_LABEL "  %s 匹配到 %d 项:" C_RESET "\n", prompt ? prompt : "搜索结果", mcount);
        ui_divider();
        int show_count = mcount > 20 ? 20 : mcount;
        for (int i = 0; i < show_count; i++) {
            printf("  " C_BOLD C_YELLOW "%2d." C_RESET " %s\n", i + 1, items[matches[i]]);
        }
        if (mcount > 20) {
            printf(C_DIM "  ... 还有 %d 项, 输入更精确的关键字缩小范围" C_RESET "\n", mcount - 20);
        }
        // Loop back: user can enter number to select, or keyword to refine
    }
}
