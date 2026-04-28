#ifndef UI_UTILS_H
#define UI_UTILS_H

#include "common.h"

// =======================  ANSI 颜色定义 =======================
#define C_RESET    "\033[0m"
#define C_BOLD     "\033[1m"
#define C_DIM      "\033[2m"
#define C_RED      "\033[31m"
#define C_GREEN    "\033[32m"
#define C_YELLOW   "\033[33m"
#define C_BLUE     "\033[34m"
#define C_MAGENTA  "\033[35m"
#define C_CYAN     "\033[36m"
#define C_WHITE    "\033[97m"

#define BG_RED     "\033[41m"
#define BG_GREEN   "\033[42m"
#define BG_YELLOW  "\033[43m"
#define BG_BLUE    "\033[44m"
#define BG_CYAN    "\033[46m"
#define BG_WHITE   "\033[107m"

// =======================  组合样式 =======================
#define S_TITLE    C_BOLD C_CYAN
#define S_SUCCESS  C_BOLD C_GREEN
#define S_ERROR    C_BOLD C_RED
#define S_WARNING  C_BOLD C_YELLOW
#define S_INFO     C_BOLD C_WHITE
#define S_LABEL    C_CYAN
#define S_VALUE    C_WHITE

// =======================  菜单项缓存（用于方向键高亮） =======================
void ui_menu_cache_init(void);
int  ui_menu_cache_fill(int *nums, char texts[][60], bool *is_exit, int max);

// =======================  UI 辅助函数 =======================
void ui_init_ansi(void);
void ui_clear_screen(void);
void ui_line(int width, const char *ch);
void ui_box_top(const char *title);
void ui_box_bottom(void);
void ui_header(const char *title);
void ui_sub_header(const char *title);
void ui_divider(void);
void ui_ok(const char *msg);
void ui_err(const char *msg);
void ui_warn(const char *msg);
void ui_info(const char *label, const char *value);
void ui_menu_item(int num, const char *text);
void ui_menu_exit(int num, const char *text);
void ui_user_badge(const char *name, const char *role);
void ui_step(int step, const char *desc);

// =======================  确认对话框 =======================
bool ui_confirm(const char *prompt);

// =======================  表格列打印（显示宽度对齐） =======================
int  utf8_display_width(const char *s);
void ui_print_col(const char *s, int width);
void ui_print_col_int(int val, int width);
void ui_print_col_float(float val, int width);

// =======================  智能ID输入 =======================
int  smart_id_lookup(const char *prompt, const char *id_list[], int count, char *output, int out_size);
int  smart_patient_input(const char *doctor_id, const char *prompt, char *output, int out_size);
int  smart_drug_input(const char *prompt, char *output, int out_size);

// =======================  模板快捷输入 =======================
int  quick_template_input(const char *category, const char *prompt, char *output, int out_size);

#endif
