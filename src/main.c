/*
 * main.c — 控制台模式程序入口 / Console mode program entry point
 *
 * 系统启动流程:
 *   1. 初始化控制台 UTF-8 编码 (支持中文显示)
 *   2. 显示启动画面 (电子医疗管理系统标题)
 *   3. 初始化数据存储 (创建 data/ 目录)
 *   4. 检查数据文件完整性 (缺失/损坏提示)
 *   5. 创建默认用户 (首次运行: admin/doctor1/patient1, 密码=用户名+123)
 *   6. 迁移旧明码密码到 SHA-256 (兼容旧数据)
 *   7. 确保医生/患者档案存在 (无档案则创建默认档案)
 *   8. 迁移旧格式医生 ID → 新格式 (X0001)
 *   9. 初始化默认诊断模板 (18 个预设模板)
 *   10. 进入主循环: 未登录→显示主菜单, 已登录→路由到角色菜单
 *
 * Startup flow: UTF-8 init → splash → data dir → integrity check →
 * default users → password migration → profile ensuring → doctor ID migration →
 * default templates → main loop (main menu or role menu)
 *
 * 角色路由 / Role routing:
 *   - patient → run_patient_menu()  (8 项: 挂号/查询/病房/进度/资料/改密等)
 *   - doctor  → run_doctor_menu()   (8 项: 提醒/接诊/开药/呼叫/急诊/进度/模板等)
 *   - admin   → run_admin_menu()    (10 项: 科室/医生/患者/药品/病房/排班/分析/日志/数据等)
 */

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

/* 全局会话状态: 跟踪当前登录用户和登录状态
   Global session: tracks current logged-in user and login state */
Session global_session = {0};

/* 前向声明 / Forward declarations */
void show_main_menu(void);             /* 显示主菜单 (登录/注册/退出) */
void run_patient_menu(const User *user); /* 患者角色菜单循环 */
void run_doctor_menu(const User *user);  /* 医生角色菜单循环 */
void run_admin_menu(const User *user);   /* 管理员角色菜单循环 */

/* 检查数据文件完整性: 验证所有数据文件存在且格式正确
   首个文件 (users.txt) 缺失 → 视为全新安装 (返回 0)
   其他文件缺失或首行非 # 开头 → 发出警告
   Check data file integrity: verify all data files exist with valid format.
   Missing users.txt → fresh install (return 0).
   Other missing/corrupt files → warn and return 1. */
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
            if (i == 0) return 0;  /* users.txt 缺失 → 首次安装 / fresh install */
            if (missing == 0) {
                printf(C_YELLOW "  ⚠ 部分数据文件缺失:\n" C_RESET);
            }
            printf(C_YELLOW "    - %s\n" C_RESET, required_files[i]);
            missing++;
        } else {
            /* 快速格式检查: 首行应为 # 开头的列名注释
               Quick format check: first line should be column header starting with # */
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
    /* 步骤 1: 初始化控制台 UTF-8 编码 / Init console to UTF-8 */
    init_console_encoding();

    /* 步骤 2: 显示启动画面 / Show splash screen */
    printf("\n");
    printf(C_BOLD C_CYAN "  ╔══════════════════════════════════════════════════╗\n" C_RESET);
    printf(C_BOLD C_CYAN "  ║" C_RESET "                                                  " C_BOLD C_CYAN "║\n" C_RESET);
    printf(C_BOLD C_CYAN "  ║" C_RESET "      " C_BOLD C_WHITE "电子医疗管理系统" C_RESET "                         " C_BOLD C_CYAN "║\n" C_RESET);
    printf(C_BOLD C_CYAN "  ║" C_RESET "      " C_DIM "Electronic Medical Management System" C_RESET "        " C_BOLD C_CYAN "║\n" C_RESET);
    printf(C_BOLD C_CYAN "  ║" C_RESET "                                                  " C_BOLD C_CYAN "║\n" C_RESET);
    printf(C_BOLD C_CYAN "  ╚══════════════════════════════════════════════════╝\n" C_RESET);
    printf("\n");

    /* 步骤 3: 初始化数据存储 / Initialize data storage */
    if (init_data_storage() != SUCCESS) {
        printf("数据存储初始化失败!\n");
        return 1;
    }

    /* 步骤 4: 检查数据文件完整性 / Check data file integrity */
    check_data_files();

    /* 步骤 5-6: 加载用户列表，决定是创建默认用户还是迁移密码
       Load user list; either create defaults or migrate passwords */
    UserNode *head = load_users_list();

    if (!head) {
        /* 首次运行: 创建 3 个默认用户 (admin/doctor1/patient1)，密码使用 SHA-256 哈希
           First run: create 3 default users with SHA-256 hashed passwords */
        User default_users[3];
        const char *usernames[3] = { "admin", "doctor1", "patient1" };
        const char *plain_pwds[3] = { "admin123", "doctor123", "patient123" };
        const char *roles[3] = { ROLE_ADMIN, ROLE_DOCTOR, ROLE_PATIENT };

        for (int i = 0; i < 3; i++) {
            memset(&default_users[i], 0, sizeof(User));
            strcpy(default_users[i].username, usernames[i]);
            strcpy(default_users[i].role, roles[i]);
            /* 明文密码 → SHA-256 哈希 → 64 字符十六进制字符串 */
            uint8_t hash[SHA256_DIGEST_SIZE];
            char hex[SHA256_HEX_SIZE];
            sha256_hash((const uint8_t*)plain_pwds[i], strlen(plain_pwds[i]), hash);
            sha256_hex(hash, hex);
            strcpy(default_users[i].password, hex);
        }

        /* 构建链表并保存 / Build linked list and save */
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

        /* 创建默认医生档案 (doctor1) 和患者档案 (patient1)
           Create default doctor & patient profiles */
        ensure_doctor_profile("doctor1");
        ensure_patient_profile("patient1");
        printf(C_GREEN "  ✓ 已创建默认用户" C_RESET "  admin / doctor1 / patient1  (密码同用户名+123)\n");
    } else {
        /* 已有用户: 执行密码迁移 (明码→SHA-256) 并确保每个用户有对应档案
           Existing users: migrate passwords to SHA-256, ensure profiles exist */
        migrate_user_passwords();

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

    /* 步骤 8-9: 迁移旧医生 ID 格式 + 初始化默认模板
       Migrate old doctor ID format + init default templates */
    migrate_doctor_ids();
    ensure_default_templates();

    /* 步骤 10: 主循环 — 未登录→主菜单, 已登录→角色菜单
       Main loop: not logged-in → main menu, logged-in → role menu */
    while (1) {
        if (!global_session.logged_in) {
            show_main_menu();
        } else {
            /* 按角色路由到对应菜单 / Route to role-specific menu */
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
                /* 注册 → 成功后自动登录 / Register → auto-login on success */
                User new_user;
                int result = register_user(&new_user);
                if (result == SUCCESS) {
                    global_session.current_user = new_user;
                    global_session.logged_in = true;
                }
            } else if (choice == 1) {
                /* 登录 / Login */
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

/* 主菜单: 登录 (1) / 注册 (2) / 退出 (0)
   Main menu: Login / Register / Exit */
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

/* 患者菜单循环: 8 项功能 + 退出登录
   Patient menu loop: 8 functions + logout */
void run_patient_menu(const User *user) {
    while (1) {
        ui_clear_screen();
        printf("\n");
        ui_box_top("患 者 系 统");
        ui_user_badge(user->username, user->role);
        ui_divider();
        show_warning_banner();
        patient_main_menu(user);                    /* 输出 1-8 项患者功能菜单 */
        printf(C_BOLD C_CYAN "  ║" C_RESET "                                                    " C_BOLD C_CYAN "║\n" C_RESET);
        ui_menu_track_line();
        ui_menu_exit(0, "退出登录");
        ui_box_bottom();

        int choice = get_menu_choice(0, 7);

        switch (choice) {
            case 0:
                logout(&global_session);
                return;
            case 1: patient_register_menu(user); break;             /* 预约挂号 */
            case 2: patient_appointment_menu(user); break;           /* 挂号查询 */
            case 3: patient_query_diagnosis_menu(user); break;       /* 诊断查询 */
            case 4: patient_view_ward_menu(user); break;             /* 病房信息 */
            case 5: patient_view_treatment_progress_menu(user); break; /* 治疗进度 */
            case 6: patient_edit_profile_menu(user); break;          /* 编辑资料 */
            case 7: change_password(user); break;                    /* 修改密码 */
        }
        pause_screen();
    }
}

/* 医生菜单循环: 8 项功能 + 退出登录
   Doctor menu loop: 8 functions + logout */
void run_doctor_menu(const User *user) {
    while (1) {
        ui_clear_screen();
        printf("\n");
        ui_box_top("医 生 系 统");
        ui_user_badge(user->username, user->role);
        ui_divider();
        show_warning_banner();
        doctor_main_menu(user);                    /* 输出 1-8 项医生功能菜单 */
        printf(C_BOLD C_CYAN "  ║" C_RESET "                                                    " C_BOLD C_CYAN "║\n" C_RESET);
        ui_menu_track_line();
        ui_menu_exit(0, "退出登录");
        ui_box_bottom();

        int choice = get_menu_choice(0, 8);

        switch (choice) {
            case 0:
                logout(&global_session);
                return;
            case 1: doctor_appointment_reminder_menu(user); break;  /* 挂号提醒 */
            case 2: doctor_consultation_menu(user); break;           /* 接诊问诊 */
            case 3: doctor_prescribe_menu(user); break;              /* 开药处方 */
            case 4: doctor_ward_call_menu(user); break;              /* 病房呼叫 */
            case 5: doctor_emergency_flag_menu(user); break;         /* 急诊标记 */
            case 6: doctor_update_progress_menu(user); break;        /* 更新进度 */
            case 7: doctor_template_menu(user); break;               /* 模板管理 */
            case 8: change_password(user); break;                    /* 修改密码 */
        }
        pause_screen();
    }
}

/* 管理员菜单循环: 10 项功能 + 密码重置 + 退出登录
   Admin menu loop: 10 functions + password reset + logout */
void run_admin_menu(const User *user) {
    while (1) {
        ui_clear_screen();
        printf("\n");
        ui_box_top("管 理 员 系 统");
        ui_user_badge(user->username, user->role);
        ui_divider();
        show_warning_banner();
        admin_main_menu(user);                     /* 输出 1-10 项管理员功能菜单 */
        printf(C_BOLD C_CYAN "  ║" C_RESET "                                                    " C_BOLD C_CYAN "║\n" C_RESET);
        ui_menu_track_line();
        ui_menu_exit(0, "退出登录");
        ui_box_bottom();

        int choice = get_menu_choice(0, 10);

        switch (choice) {
            case 0:
                logout(&global_session);
                return;
            case 1: admin_department_menu(user); break;   /* 科室管理 */
            case 2: admin_doctor_menu(user); break;        /* 医生管理 */
            case 3: admin_patient_menu(user); break;       /* 患者管理 */
            case 4: admin_drug_menu(user); break;          /* 药品管理 */
            case 5: admin_ward_menu(user); break;          /* 病房管理 */
            case 6: admin_schedule_menu(user); break;      /* 排班管理 */
            case 7: admin_analysis_menu(user); break;      /* 统计分析 */
            case 8: admin_log_menu(user); break;           /* 日志查看 */
            case 9: admin_data_menu(user); break;          /* 数据管理 */
            case 10: admin_reset_password(user); break;    /* 密码重置 */
        }
        pause_screen();
    }
}
