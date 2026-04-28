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

void init_console_encoding(void) {
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
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

static void render_menu_item(int num, const char *text, bool is_exit) {
    printf("  ");
    if (is_exit) {
        printf(C_DIM "%d." C_RESET " %s\n", num, text);
    } else {
        printf(C_BOLD C_YELLOW "%d." C_RESET " %s\n", num, text);
    }
}

int get_menu_choice(int min, int max) {
    // Get cached menu items
    int   item_nums[MAX_MENU_ITEMS];
    char  item_texts[MAX_MENU_ITEMS][60];
    bool  item_exit[MAX_MENU_ITEMS];
    int   item_count = ui_menu_cache_fill(item_nums, item_texts, item_exit, MAX_MENU_ITEMS);

    int current = (min >= 1) ? min : (min + 1);
    if (current < min) current = min;
    if (current > max) current = max;

    printf("\n请选择 (\xe2\x86\x91\xe2\x86\x93 切换, 回车确认): ");
    printf("\033[7m %d \033[0m", current);
    fflush(stdout);

    int prev = current;

    while (1) {
        int ch = _getch();

        if (ch == 0xE0 || ch == 0x00) {
            // Windows arrow key prefix
            ch = _getch();
            if (ch == 72) {
                current--;
                if (current < min) current = max;
            } else if (ch == 80) {
                current++;
                if (current > max) current = min;
            } else {
                continue;
            }
        }
#ifndef _WIN32
        else if (ch == 0x1B) {
            // Unix ESC [ A/B sequence
            ch = _getch();
            if (ch == '[') {
                ch = _getch();
                if (ch == 'A') {
                    current--;
                    if (current < min) current = max;
                } else if (ch == 'B') {
                    current++;
                    if (current > max) current = min;
                } else {
                    continue;
                }
            } else {
                continue;
            }
        }
#endif
        else if (ch == '\r' || ch == '\n') {
            printf("\n");
            return current;
        } else if (ch >= '0' && ch <= '9') {
            int num = ch - '0';
            if (num >= min && num <= max) {
                // Direct digit selection: redraw final state before returning
                if (item_count > 0 && num != prev) {
                    printf("\0337\033[%dA", item_count + 2);
                    for (int i = 0; i < item_count; i++) {
                        printf("\r\033[K");
                        if (item_nums[i] == num)
                            printf(BG_CYAN "  " C_BOLD C_YELLOW "%d." C_RESET BG_CYAN " %s" C_RESET "\n", item_nums[i], item_texts[i]);
                        else
                            render_menu_item(item_nums[i], item_texts[i], item_exit[i]);
                    }
                    printf("\0338");
                }
                printf("\n");
                return num;
            }
            continue;
        } else {
            continue;
        }

        if (current == prev) continue;

        // Redraw menu items with highlight using cursor save/restore
        if (item_count > 0) {
            printf("\0337");                              // save cursor (at prompt)
            printf("\033[%dA", item_count + 2);           // move up to first item
            for (int i = 0; i < item_count; i++) {
                printf("\r\033[K");                       // clear line
                if (item_nums[i] == current)
                    printf(BG_CYAN "  " C_BOLD C_YELLOW "%d." C_RESET BG_CYAN " %s" C_RESET "\n", item_nums[i], item_texts[i]);
                else
                    render_menu_item(item_nums[i], item_texts[i], item_exit[i]);
            }
            printf("\0338");                              // restore to prompt
        }

        // Update prompt
        printf("\r\033[K请选择 (\xe2\x86\x91\xe2\x86\x93 切换, 回车确认): ");
        printf("\033[7m %d \033[0m", current);
        fflush(stdout);
        prev = current;
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
