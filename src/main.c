#include "common.h"
#include "login.h"
#include "patient.h"
#include "doctor.h"
#include "admin.h"
#include "analysis.h"
#include "public.h"
#include "sha256.h"
#include "data_storage.h"
#include "ui_utils.h"

// Global session
Session global_session = {0};

void show_main_menu(void);
void run_patient_menu(const User *user);
void run_doctor_menu(const User *user);
void run_admin_menu(const User *user);

static int check_data_files(void) {
    const char *required_files[] = {
        USERS_FILE, PATIENTS_FILE, DOCTORS_FILE, DEPARTMENTS_FILE,
        DRUGS_FILE, WARDS_FILE, APPOINTMENTS_FILE, ONSITE_REGISTRATIONS_FILE,
        WARD_CALLS_FILE, MEDICAL_RECORDS_FILE, PRESCRIPTIONS_FILE,
        TEMPLATES_FILE, SCHEDULES_FILE, LOGS_FILE
    };
    int file_count = sizeof(required_files) / sizeof(required_files[0]);
    int missing = 0;

    for (int i = 0; i < file_count; i++) {
        FILE *fp = fopen(required_files[i], "r");
        if (!fp) {
            if (i == 0) return 0; // users.txt missing - fresh install
            if (missing == 0) {
                printf(C_YELLOW "  ⚠ 部分数据文件缺失:\n" C_RESET);
            }
            printf(C_YELLOW "    - %s\n" C_RESET, required_files[i]);
            missing++;
        } else {
            // Quick format check: first line should be a header starting with #
            char first[64];
            if (!fgets(first, sizeof(first), fp) || first[0] != '#') {
                if (missing == 0) {
                    printf(C_YELLOW "  ⚠ 部分数据文件可能已损坏:\n" C_RESET);
                }
                printf(C_YELLOW "    - %s (格式异常)\n" C_RESET, required_files[i]);
                missing++;
            }
            fclose(fp);
        }
    }

    if (missing > 0) {
        printf("\n" C_YELLOW "  ⚠ 建议使用数据管理中的备份还原功能恢复数据。\n" C_RESET);
        printf(C_YELLOW "  ⚠ 或删除 data/users.txt 后重启以重新初始化。\n\n" C_RESET);
        printf(C_CYAN "  按回车键继续..." C_RESET);
        clear_input_buffer();
        getchar();
    }
    return 1;
}

int main(void) {
    // Initialize console encoding for UTF-8
    init_console_encoding();

    printf("\n");
    printf(C_BOLD C_CYAN "  ╔══════════════════════════════════════════════════╗\n" C_RESET);
    printf(C_BOLD C_CYAN "  ║" C_RESET "                                                  " C_BOLD C_CYAN "║\n" C_RESET);
    printf(C_BOLD C_CYAN "  ║" C_RESET "      " C_BOLD C_WHITE "电子医疗管理系统" C_RESET "                         " C_BOLD C_CYAN "║\n" C_RESET);
    printf(C_BOLD C_CYAN "  ║" C_RESET "      " C_DIM "Electronic Medical Management System" C_RESET "        " C_BOLD C_CYAN "║\n" C_RESET);
    printf(C_BOLD C_CYAN "  ║" C_RESET "                                                  " C_BOLD C_CYAN "║\n" C_RESET);
    printf(C_BOLD C_CYAN "  ╚══════════════════════════════════════════════════╝\n" C_RESET);
    printf("\n");

    // Initialize data storage
    if (init_data_storage() != SUCCESS) {
        printf("数据存储初始化失败!\n");
        return 1;
    }

    // Check data file integrity
    check_data_files();

    // Create default admin user if not exists
    UserNode *head = load_users_list();
    
    if (!head) {
        // Create default users
        User default_users[3];
        const char *usernames[3] = { "admin", "doctor1", "patient1" };
        const char *plain_pwds[3] = { "admin123", "doctor123", "patient123" };
        const char *roles[3] = { ROLE_ADMIN, ROLE_DOCTOR, ROLE_PATIENT };

        for (int i = 0; i < 3; i++) {
            memset(&default_users[i], 0, sizeof(User));
            strcpy(default_users[i].username, usernames[i]);
            strcpy(default_users[i].role, roles[i]);
            uint8_t hash[SHA256_DIGEST_SIZE];
            char hex[SHA256_HEX_SIZE];
            sha256_hash((const uint8_t*)plain_pwds[i], strlen(plain_pwds[i]), hash);
            sha256_hex(hash, hex);
            strcpy(default_users[i].password, hex);
        }
        
        // Create linked list from default users
        UserNode *default_head = NULL;
        UserNode *tail = NULL;
        
        for (int i = 0; i < 3; i++) {
            UserNode *node = create_user_node(&default_users[i]);
            if (!node) {
                free_user_list(default_head);
                return 1;
            }
            
            if (!default_head) {
                default_head = node;
                tail = node;
            } else {
                tail->next = node;
                tail = node;
            }
        }
        
        save_users_list(default_head);
        free_user_list(default_head);
        
        ensure_doctor_profile("doctor1");
        ensure_patient_profile("patient1");
        printf(C_GREEN "  ✓ 已创建默认用户" C_RESET "  admin / doctor1 / patient1  (密码同用户名+123)\n");
    } else {
        // Migrate existing plaintext passwords to hashed
        migrate_user_passwords();

        // Check existing users
        UserNode *current = head;
        while (current) {
            if (strcmp(current->data.role, ROLE_PATIENT) == 0) {
                ensure_patient_profile(current->data.username);
            } else if (strcmp(current->data.role, ROLE_DOCTOR) == 0) {
                ensure_doctor_profile(current->data.username);
            }
            current = current->next;
        }
        
        free_user_list(head);
    }
    
    migrate_doctor_ids();
    ensure_default_templates();
    
    while (1) {
        if (!global_session.logged_in) {
            show_main_menu();
        } else {
            // Run menu based on role
            if (strcmp(global_session.current_user.role, ROLE_PATIENT) == 0) {
                run_patient_menu(&global_session.current_user);
            } else if (strcmp(global_session.current_user.role, ROLE_DOCTOR) == 0) {
                run_doctor_menu(&global_session.current_user);
            } else if (strcmp(global_session.current_user.role, ROLE_ADMIN) == 0) {
                run_admin_menu(&global_session.current_user);
            }
            if (!global_session.logged_in) continue;
        }
        
        if (!global_session.logged_in) {
            int choice = get_menu_choice(0, 2);
            if (choice == 0) {
                printf("\n" C_BOLD C_CYAN "  ════ 感谢使用，再见! ════" C_RESET "\n\n");
                break;
            }
            
            if (choice == 2) {
                // Register new user - auto go to login after success
                User new_user;
                int result = register_user(&new_user);
                if (result == SUCCESS) {
                    // Auto login after registration
                    global_session.current_user = new_user;
                    global_session.logged_in = true;
                }
            } else if (choice == 1) {
                // Login
                User logged_user;
                int result = login(&logged_user);
                if (result == SUCCESS) {
                    global_session.current_user = logged_user;
                    global_session.logged_in = true;
                }
            }
        }
    }
    
    return 0;
}

void show_main_menu(void) {
    ui_clear_screen();
    printf("\n");
    ui_box_top("主 菜 单");
    printf(C_BOLD C_CYAN "  ║" C_RESET "                                                    " C_BOLD C_CYAN "║\n" C_RESET);
    ui_menu_track_line();
    ui_menu_item(1, "登录系统");
    ui_menu_item(2, "注册用户");
    printf(C_BOLD C_CYAN "  ║" C_RESET "                                                    " C_BOLD C_CYAN "║\n" C_RESET);
    ui_menu_track_line();
    ui_menu_exit(0, "退出程序");
    ui_box_bottom();
}

void run_patient_menu(const User *user) {
    while (1) {
        ui_clear_screen();
        printf("\n");
        ui_box_top("患 者 系 统");
        ui_user_badge(user->username, user->role);
        ui_divider();
        show_warning_banner();
        patient_main_menu(user);
        printf(C_BOLD C_CYAN "  ║" C_RESET "                                                    " C_BOLD C_CYAN "║\n" C_RESET);
        ui_menu_track_line();
        ui_menu_exit(0, "退出登录");
        ui_box_bottom();
        
        int choice = get_menu_choice(0, 7);

        switch (choice) {
            case 0:
                logout(&global_session);
                return;
            case 1: patient_register_menu(user); break;
            case 2: patient_appointment_menu(user); break;
            case 3: patient_query_diagnosis_menu(user); break;
            case 4: patient_view_ward_menu(user); break;
            case 5: patient_view_treatment_progress_menu(user); break;
            case 6: patient_edit_profile_menu(user); break;
            case 7: change_password(user); break;
        }
        pause_screen();
    }
}

void run_doctor_menu(const User *user) {
    while (1) {
        ui_clear_screen();
        printf("\n");
        ui_box_top("医 生 系 统");
        ui_user_badge(user->username, user->role);
        ui_divider();
        show_warning_banner();
        doctor_main_menu(user);
        printf(C_BOLD C_CYAN "  ║" C_RESET "                                                    " C_BOLD C_CYAN "║\n" C_RESET);
        ui_menu_track_line();
        ui_menu_exit(0, "退出登录");
        ui_box_bottom();
        
        int choice = get_menu_choice(0, 8);

        switch (choice) {
            case 0:
                logout(&global_session);
                return;
            case 1: doctor_appointment_reminder_menu(user); break;
            case 2: doctor_consultation_menu(user); break;
            case 3: doctor_prescribe_menu(user); break;
            case 4: doctor_ward_call_menu(user); break;
            case 5: doctor_emergency_flag_menu(user); break;
            case 6: doctor_update_progress_menu(user); break;
            case 7: doctor_template_menu(user); break;
            case 8: change_password(user); break;
        }
        pause_screen();
    }
}

void run_admin_menu(const User *user) {
    while (1) {
        ui_clear_screen();
        printf("\n");
        ui_box_top("管 理 员 系 统");
        ui_user_badge(user->username, user->role);
        ui_divider();
        show_warning_banner();
        admin_main_menu(user);
        printf(C_BOLD C_CYAN "  ║" C_RESET "                                                    " C_BOLD C_CYAN "║\n" C_RESET);
        ui_menu_track_line();
        ui_menu_exit(0, "退出登录");
        ui_box_bottom();
        
        int choice = get_menu_choice(0, 10);

        switch (choice) {
            case 0:
                logout(&global_session);
                return;
            case 1: admin_department_menu(user); break;
            case 2: admin_doctor_menu(user); break;
            case 3: admin_patient_menu(user); break;
            case 4: admin_drug_menu(user); break;
            case 5: admin_ward_menu(user); break;
            case 6: admin_analysis_menu(user); break;
            case 7: admin_schedule_menu(user); break;
            case 8: admin_log_menu(user); break;
            case 9: admin_data_menu(user); break;
            case 10: admin_reset_password(user); break;
        }
        pause_screen();
    }
}

