#include "login.h"
#include "data_storage.h"
#include "ui_utils.h"
#include "sha256.h"
#include <ctype.h>
#include <stdbool.h>

int register_user(User *new_user) {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    char confirm_password[MAX_PASSWORD];
    char role[20];
    
    printf("\n");
    ui_box_top("用 户 注 册");
    ui_menu_item(1, "注册管理员");
    ui_menu_item(2, "注册医生");
    ui_menu_item(3, "注册患者");
    printf(C_BOLD C_CYAN "  ║" C_RESET "                                                    " C_BOLD C_CYAN "║\n" C_RESET);
    ui_menu_track_line();
    ui_menu_exit(0, "返回");
    ui_box_bottom();

    int role_choice = get_menu_choice(0, 3);
    if (role_choice == 0) return ERROR_INVALID_INPUT;
    
    switch (role_choice) {
        case 1: strcpy(role, ROLE_ADMIN); break;
        case 2: strcpy(role, ROLE_DOCTOR); break;
        case 3: strcpy(role, ROLE_PATIENT); break;
        default: return ERROR_INVALID_INPUT;
    }
    
    ui_sub_header("用户信息");
    printf(S_LABEL "  请输入用户名: " C_RESET);
    if (fgets(username, MAX_USERNAME, stdin) == NULL) return ERROR_INVALID_INPUT;
    username[strcspn(username, "\n")] = 0;
    
    if (strlen(username) == 0) {
        ui_err("用户名不能为空!");
        return ERROR_INVALID_INPUT;
    }
    
    printf(S_LABEL "  请输入密码: " C_RESET);
    if (fgets(password, MAX_PASSWORD, stdin) == NULL) return ERROR_INVALID_INPUT;
    password[strcspn(password, "\n")] = 0;
    
    if (strlen(password) == 0) {
        ui_err("密码不能为空!");
        return ERROR_INVALID_INPUT;
    }
    
    printf(S_LABEL "  请再次输入密码: " C_RESET);
    if (fgets(confirm_password, MAX_PASSWORD, stdin) == NULL) return ERROR_INVALID_INPUT;
    confirm_password[strcspn(confirm_password, "\n")] = 0;
    
    if (strcmp(password, confirm_password) != 0) {
        ui_err("两次输入的密码不一致!");
        return ERROR_INVALID_INPUT;
    }
    
    UserNode *head = load_users_list();
    
    UserNode *current = head;
    while (current) {
        if (strcmp(current->data.username, username) == 0) {
            free_user_list(head);
            ui_err("用户名已存在!");
            return ERROR_DUPLICATE;
        }
        current = current->next;
    }
    
    User user;
    uint8_t hash_bytes[SHA256_DIGEST_SIZE];
    char hash_hex[SHA256_HEX_SIZE];
    sha256_hash((const uint8_t*)password, strlen(password), hash_bytes);
    sha256_hex(hash_bytes, hash_hex);
    strcpy(user.username, username);
    strcpy(user.password, hash_hex);
    strcpy(user.role, role);
    
    UserNode *new_node = create_user_node(&user);
    if (!new_node) {
        free_user_list(head);
        ui_err("内存分配失败!");
        return ERROR_FILE_IO;
    }
    
    if (!head) {
        head = new_node;
    } else {
        current = head;
        while (current->next) {
            current = current->next;
        }
        current->next = new_node;
    }
    
    int result = save_users_list(head);
    free_user_list(head);
    
    if (result != SUCCESS) {
        ui_err("保存用户数据失败!");
        return result;
    }
    
    ui_ok("注册成功!");
    ui_info("用户名", username);
    ui_info("角色", role);
    
    if (strcmp(role, ROLE_PATIENT) == 0) {
        ensure_patient_profile(username);
    } else if (strcmp(role, ROLE_DOCTOR) == 0) {
        char doctor_name[MAX_NAME];
        char doctor_title[50];
        char doctor_dept[MAX_ID];

        ui_header("医生信息完善");

        printf(S_LABEL "  请输入医生姓名: " C_RESET);
        if (fgets(doctor_name, sizeof(doctor_name), stdin) == NULL) {
            doctor_name[0] = '\0';
        } else {
            doctor_name[strcspn(doctor_name, "\n")] = 0;
        }
        if (doctor_name[0] == '\0') {
            strcpy(doctor_name, username);
        }

        printf(S_LABEL "  请输入职称(如主任医师): " C_RESET);
        if (fgets(doctor_title, sizeof(doctor_title), stdin) == NULL) {
            doctor_title[0] = '\0';
        } else {
            doctor_title[strcspn(doctor_title, "\n")] = 0;
        }
        if (doctor_title[0] == '\0') {
            strcpy(doctor_title, "医生");
        }

        printf("\n");
        DepartmentNode *dept_head = load_departments_list();
        if (dept_head) {
            ui_menu_item(1, "选择科室");
            ui_menu_item(2, "暂不选择");
            int dept_choice = get_menu_choice(1, 2);
            if (dept_choice == 1) {
                int dc = count_department_list(dept_head);
                if (dc > 0) {
                    const char **di = malloc(dc * sizeof(const char *));
                    char (*db)[70] = malloc(dc * sizeof(*db));
                    int idx = 0;
                    DepartmentNode *dp = dept_head;
                    while (dp) {
                        snprintf(db[idx], 70, "%s - %s", dp->data.department_id, dp->data.name);
                        di[idx] = db[idx]; idx++; dp = dp->next;
                    }
                    int sel = ui_search_list("选择科室", di, dc);
                    free((void*)di); free(db);
                    if (sel >= 0) {
                        dp = dept_head;
                        for (int j = 0; j < sel; j++) dp = dp->next;
                        strcpy(doctor_dept, dp->data.department_id);
                    } else {
                        doctor_dept[0] = '\0';
                    }
                } else {
                    ui_warn("暂无科室数据，请联系管理员添加。");
                    doctor_dept[0] = '\0';
                }
            } else {
                doctor_dept[0] = '\0';
            }
        } else {
            ui_warn("暂无科室数据，请联系管理员添加。");
            doctor_dept[0] = '\0';
        }
        free_department_list(dept_head);

        create_doctor_profile_with_details(username, doctor_name, doctor_title, doctor_dept);
    }
    
    if (new_user) *new_user = user;
    return SUCCESS;
}

int login(User *logged_user) {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    char role[20];
    
    printf("\n");
    ui_box_top("登 录 系 统");
    ui_menu_item(1, "管理员登录");
    ui_menu_item(2, "医生登录");
    ui_menu_item(3, "患者登录");
    printf(C_BOLD C_CYAN "  ║" C_RESET "                                                    " C_BOLD C_CYAN "║\n" C_RESET);
    ui_menu_track_line();
    ui_menu_exit(0, "返回");
    ui_box_bottom();

    int role_choice = get_menu_choice(0, 3);
    if (role_choice == 0) return ERROR_INVALID_INPUT;
    
    switch (role_choice) {
        case 1: strcpy(role, ROLE_ADMIN); break;
        case 2: strcpy(role, ROLE_DOCTOR); break;
        case 3: strcpy(role, ROLE_PATIENT); break;
        default: return ERROR_INVALID_INPUT;
    }
    
    ui_sub_header("请输入登录信息");
    printf(S_LABEL "  用户名: " C_RESET);
    if (fgets(username, MAX_USERNAME, stdin) == NULL) return ERROR_INVALID_INPUT;
    username[strcspn(username, "\n")] = 0;
    
    printf(S_LABEL "  密码: " C_RESET);
    if (fgets(password, MAX_PASSWORD, stdin) == NULL) return ERROR_INVALID_INPUT;
    password[strcspn(password, "\n")] = 0;
    
    UserNode *head = load_users_list();

    if (!head) {
        ui_err("加载用户数据失败!");
        return ERROR_FILE_IO;
    }

    uint8_t hash_bytes[SHA256_DIGEST_SIZE];
    char hash_hex[SHA256_HEX_SIZE];
    sha256_hash((const uint8_t*)password, strlen(password), hash_bytes);
    sha256_hex(hash_bytes, hash_hex);

    UserNode *current = head;
    while (current) {
        if (strcmp(current->data.username, username) == 0 &&
            strcmp(current->data.password, hash_hex) == 0 &&
            strcmp(current->data.role, role) == 0) {
            *logged_user = current->data;
            free_user_list(head);
            ui_ok("登录成功!");
            return SUCCESS;
        }
        current = current->next;
    }
    
    free_user_list(head);
    ui_err("用户名、密码或角色类型错误!");
    return ERROR_NOT_FOUND;
}

void logout(Session *session) {
    if (session && session->logged_in) {
        printf(C_DIM "  ← 用户 %s 已退出登录" C_RESET "\n", session->current_user.username);
        session->logged_in = false;
        memset(&session->current_user, 0, sizeof(User));
    }
}

bool has_permission(const User *user, const char *required_role) {
    if (!user || !required_role) return false;
    
    if (strcmp(user->role, ROLE_ADMIN) == 0) {
        return true;
    }
    
    return strcmp(user->role, required_role) == 0;
}

int migrate_user_passwords(void) {
    UserNode *head = load_users_list();
    if (!head) return SUCCESS;
    UserNode *cur = head;
    int changed = 0;
    while (cur) {
        size_t len = strlen(cur->data.password);
        int needs_hash = 0;
        if (len != 64) {
            needs_hash = 1;
        } else {
            for (size_t i = 0; i < len; i++) {
                if (!isxdigit((unsigned char)cur->data.password[i])) {
                    needs_hash = 1;
                    break;
                }
            }
        }
        if (needs_hash) {
            uint8_t hash[SHA256_DIGEST_SIZE];
            char hex[SHA256_HEX_SIZE];
            sha256_hash((const uint8_t*)cur->data.password, len, hash);
            sha256_hex(hash, hex);
            strcpy(cur->data.password, hex);
            changed = 1;
        }
        cur = cur->next;
    }
    int result = changed ? save_users_list(head) : SUCCESS;
    free_user_list(head);
    return result;
}
