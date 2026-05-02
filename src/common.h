#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

// Role definitions
#define ROLE_ADMIN  "admin"
#define ROLE_DOCTOR "doctor"
#define ROLE_PATIENT "patient"

// Max buffer sizes
#define MAX_USERNAME 50
#define MAX_PASSWORD 65
#define MAX_NAME 100
#define MAX_ID 20
#define MAX_BUFFER 256
#define MAX_MENU_ITEMS 30

// Status codes (must undef before define to avoid conflict with winerror.h)
#ifndef SUCCESS
#define SUCCESS 0
#endif
#undef ERROR_INVALID_INPUT
#undef ERROR_NOT_FOUND
#undef ERROR_DUPLICATE
#undef ERROR_PERMISSION_DENIED
#undef ERROR_FILE_IO
#define ERROR_INVALID_INPUT -1
#define ERROR_NOT_FOUND -2
#define ERROR_DUPLICATE -3
#define ERROR_PERMISSION_DENIED -4
#define ERROR_FILE_IO -5

// User structure
typedef struct {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    char role[20];
} User;

// Session structure
typedef struct {
    User current_user;
    bool logged_in;
} Session;

// Function declarations
void init_console_encoding(void);
void clear_input_buffer(void);
void pause_screen(void);
int get_menu_choice(int min, int max);

// Utility functions
void get_current_time(char *buffer, int buffer_size);
void generate_id(char *buffer, int buffer_size, const char *prefix);

// Read a line from stdin and convert to UTF-8 (cross-platform safe for search)
char* read_input_line(char *buf, size_t size);

#endif // COMMON_H
