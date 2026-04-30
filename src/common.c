#include "common.h"
#include "ui_utils.h"

#ifdef _WIN32
#include <windows.h>
#include <locale.h>
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

char* read_input_line(char *buf, size_t size) {
    if (!buf || size == 0) return NULL;
    if (fgets(buf, (int)size, stdin) == NULL) return NULL;
    buf[strcspn(buf, "\n")] = '\0';

#ifdef _WIN32
    UINT acp = GetACP();
    if (acp != CP_UTF8) {
        int wlen = MultiByteToWideChar(acp, 0, buf, -1, NULL, 0);
        if (wlen > 0) {
            wchar_t *wide = (wchar_t*)malloc(wlen * sizeof(wchar_t));
            if (wide) {
                MultiByteToWideChar(acp, 0, buf, -1, wide, wlen);
                int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
                if (utf8_len > 0 && (size_t)utf8_len <= size)
                    WideCharToMultiByte(CP_UTF8, 0, wide, -1, buf, utf8_len, NULL, NULL);
                free(wide);
            }
        }
    }
#endif
    return buf;
}

void init_console_encoding(void) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, ".UTF-8");
#endif
    ui_init_ansi();
}

void clear_input_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void pause_screen(void) {
    printf("\n按回车键继续...");
    getchar();
}

#ifndef _WIN32
static int _getch(void) {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}
#endif

int get_menu_choice(int min, int max) {
    // Get cached menu items
    int   item_nums[MAX_MENU_ITEMS];
    char  item_texts[MAX_MENU_ITEMS][60];
    bool  item_exit[MAX_MENU_ITEMS];
    int   item_count = ui_menu_cache_fill(item_nums, item_texts, item_exit, MAX_MENU_ITEMS);

    int current_index = 0;
    int current;
    
    if (item_count > 0) {
        current = item_nums[0];
        // If min was specified as > 0 and exists in array, try to select it
        if (min >= 1) {
            for (int i = 0; i < item_count; i++) {
                if (item_nums[i] == min) {
                    current_index = i;
                    current = item_nums[i];
                    break;
                }
            }
        }
    } else {
        current = (min >= 1) ? min : (min + 1);
        if (current < min) current = min;
        if (current > max) current = max;
    }

    if (item_count > 0) {
        printf("\n请选择 (\xe2\x86\x91\xe2\x86\x93 切换, 回车确认): ");
        printf(C_BOLD C_CYAN "\xe2\x86\x92 " C_RESET);
        printf(C_BOLD C_WHITE "\033[7m %s \033[0m" C_RESET, item_texts[current_index]);
    } else {
        printf("\n请选择 (\xe2\x86\x91\xe2\x86\x93 切换, 回车确认): ");
        printf("\033[7m %d \033[0m", current);
    }
    fflush(stdout);

    int prev_index = current_index;
    int prev_current = current;

    while (1) {
        int ch = _getch();

        if (ch == 0xE0 || ch == 0x00) {
            // Windows arrow key prefix
            ch = _getch();
            if (ch == 72) {
                if (item_count > 0) {
                    current_index--;
                    if (current_index < 0) current_index = item_count - 1;
                    current = item_nums[current_index];
                } else {
                    current--;
                    if (current < min) current = max;
                }
            } else if (ch == 80) {
                if (item_count > 0) {
                    current_index++;
                    if (current_index >= item_count) current_index = 0;
                    current = item_nums[current_index];
                } else {
                    current++;
                    if (current > max) current = min;
                }
            } else {
                continue;
            }
        }
        else if (ch == 0x1B) {
            // Unix/ANSI ESC [ A/B sequence
            ch = _getch();
            if (ch == '[') {
                ch = _getch();
                if (ch == 'A') {
                    if (item_count > 0) {
                        current_index--;
                        if (current_index < 0) current_index = item_count - 1;
                        current = item_nums[current_index];
                    } else {
                        current--;
                        if (current < min) current = max;
                    }
                } else if (ch == 'B') {
                    if (item_count > 0) {
                        current_index++;
                        if (current_index >= item_count) current_index = 0;
                        current = item_nums[current_index];
                    } else {
                        current++;
                        if (current > max) current = min;
                    }
                } else {
                    continue;
                }
            } else {
                continue;
            }
        }
        else if (ch == '\r' || ch == '\n') {
            printf("\n");
            return current;
        } else if (ch >= '0' && ch <= '9') {
            int num = ch - '0';
            if (num >= min && num <= max) {
                printf("\n");
                return num;
            }
            continue;
        } else {
            continue;
        }

        if (item_count > 0) {
            if (current_index == prev_index) continue;

            // In-place highlighting using DECSC/DECRC cursor save/restore.
            // Save cursor at prompt, move to items to redraw, restore back.
            {
                int total = ui_menu_get_saved_total();
                int old_idx = prev_index;
                int new_idx = current_index;
                int old_off = ui_menu_get_item_offset(old_idx);
                int new_off = ui_menu_get_item_offset(new_idx);

                // Save cursor position (at prompt)
                printf("\0337");

                // Un-highlight old item: move up from prompt to old item line
                printf("\033[%dA", total - old_off + 1);
                printf("\r\033[K");
                if (item_exit[old_idx]) {
                    printf("  " C_DIM "%d. %s" C_RESET "\n", item_nums[old_idx], item_texts[old_idx]);
                } else {
                    printf("  " C_BOLD C_YELLOW "%d." C_RESET " %s\n", item_nums[old_idx], item_texts[old_idx]);
                }

                // Restore cursor to prompt
                printf("\0338");

                // Highlight new item: move up from prompt to new item line
                printf("\033[%dA", total - new_off + 1);
                printf("\r\033[K");
                printf("  " BG_CYAN C_WHITE C_BOLD "%d. %s" C_RESET "\n", item_nums[new_idx], item_texts[new_idx]);

                // Restore cursor to prompt
                printf("\0338");
            }

            prev_index = current_index;
        } else {
            if (current == prev_current) continue;
            prev_current = current;
        }

        printf("\r\033[K请选择 (\xe2\x86\x91\xe2\x86\x93 切换, 回车确认): ");
        if (item_count > 0) {
            printf(C_BOLD C_CYAN "\xe2\x86\x92 " C_RESET);
            printf(C_BOLD C_WHITE "\033[7m %s \033[0m" C_RESET, item_texts[current_index]);
        } else {
            printf("\033[7m %d \033[0m", current);
        }
        fflush(stdout);
    }
}

void get_current_time(char *buffer, int buffer_size) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", tm_info);
}

void generate_id(char *buffer, int buffer_size, const char *prefix) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    static time_t last_sec = 0;
    static int counter = 0;

    if (now != last_sec) {
        last_sec = now;
        counter = 0;
    }
    int seq = counter++;
    snprintf(buffer, buffer_size, "%s%04d%02d%02d%02d%02d%02d%03d",
             prefix,
             tm_info->tm_year + 1900,
             tm_info->tm_mon + 1,
             tm_info->tm_mday,
             tm_info->tm_hour,
             tm_info->tm_min,
             tm_info->tm_sec,
             seq % 1000);
}
