#include "admin.h"
#include "data_storage.h"
#include "public.h"
#include "ui_utils.h"
#include "sha256.h"

static void list_departments(void) {
    DepartmentNode *head = load_departments_list();
    int count = count_department_list(head);

    if (count == 0) {
        ui_warn("暂无科室数据。");
        free_department_list(head);
        return;
    }

    const char **items = malloc(count * sizeof(const char *));
    char (*buf)[100] = malloc(count * sizeof(*buf));

    int i = 0;
    DepartmentNode *cur = head;
    while (cur) {
        snprintf(buf[i], 100, "%-10s %-16s %-16s %-16s",
                 cur->data.department_id,
                 cur->data.name,
                 cur->data.leader,
                 cur->data.phone);
        items[i] = buf[i];
        i++; cur = cur->next;
    }

    ui_search_list("科室列表(输入搜索, Esc返回)", items, count);

    free((void*)items);
    free(buf);
    free_department_list(head);
}

static void list_doctors(void) {
    DoctorNode *head = load_doctors_list();
    int count = count_doctor_list(head);

    if (count == 0) {
        ui_warn("暂无医生数据。");
        free_doctor_list(head);
        return;
    }

    const char **items = malloc(count * sizeof(const char *));
    char (*buf)[100] = malloc(count * sizeof(*buf));

    int i = 0;
    DoctorNode *cur = head;
    while (cur) {
        snprintf(buf[i], 100, "%-10s %-12s %-10s %-14s %3d",
                 cur->data.doctor_id,
                 cur->data.name,
                 cur->data.department_id[0] ? cur->data.department_id : "未分配",
                 cur->data.title,
                 cur->data.busy_level);
        items[i] = buf[i];
        i++; cur = cur->next;
    }

    ui_search_list("医生列表(输入搜索, Esc返回)", items, count);

    free((void*)items);
    free(buf);
    free_doctor_list(head);
}

static void list_patients(void) {
    PatientNode *head = load_patients_list();
    int count = count_patient_list(head);

    if (count == 0) {
        ui_warn("暂无患者数据。");
        free_patient_list(head);
        return;
    }

    const char **items = malloc(count * sizeof(const char *));
    char (*buf)[120] = malloc(count * sizeof(*buf));

    int i = 0;
    PatientNode *cur = head;
    while (cur) {
        snprintf(buf[i], 120, "%-10s %-10s %-8s %-10s %-16s %s",
                 cur->data.patient_id,
                 cur->data.name,
                 cur->data.patient_type,
                 cur->data.treatment_stage,
                 cur->data.phone,
                 cur->data.is_emergency ? "是" : "否");
        items[i] = buf[i];
        i++; cur = cur->next;
    }

    ui_search_list("患者列表(输入搜索, Esc返回)", items, count);

    free((void*)items);
    free(buf);
    free_patient_list(head);
}
static void list_drugs(void) {
    DrugNode *head = load_drugs_list();
    int count = count_drug_list(head);

    if (count == 0) {
        ui_warn("暂无药品数据。");
        free_drug_list(head);
        return;
    }

    const char **items = malloc(count * sizeof(const char *));
    char (*buf)[140] = malloc(count * sizeof(*buf));

    int i = 0;
    DrugNode *cur = head;
    while (cur) {
        snprintf(buf[i], 140, "%-10s %-14s %-8s %8.2f %4d %4d %5.0f%%",
                 cur->data.drug_id,
                 cur->data.name,
                 cur->data.category,
                 cur->data.price,
                 cur->data.stock_num,
                 cur->data.warning_line,
                 cur->data.reimbursement_ratio * 100);
        items[i] = buf[i];
        i++; cur = cur->next;
    }

    ui_search_list("药品列表(输入搜索, Esc返回)", items, count);

    free((void*)items);
    free(buf);
    free_drug_list(head);
}

static void list_wards(void) {
    WardNode *head = load_wards_list();
    int count = count_ward_list(head);

    if (count == 0) {
        ui_warn("暂无病房数据。");
        free_ward_list(head);
        return;
    }

    const char **items = malloc(count * sizeof(const char *));
    char (*buf)[100] = malloc(count * sizeof(*buf));

    int i = 0;
    WardNode *cur = head;
    while (cur) {
        snprintf(buf[i], 100, "%-10s %-15s %3d张 %3d张 %3d",
                 cur->data.ward_id,
                 cur->data.type,
                 cur->data.total_beds,
                 cur->data.remain_beds,
                 cur->data.warning_line);
        items[i] = buf[i];
        i++; cur = cur->next;
    }

    ui_search_list("病房列表(输入搜索, Esc返回)", items, count);

    free((void*)items);
    free(buf);
    free_ward_list(head);
}

void admin_main_menu(const User *current_user) {
    (void)current_user;
    ui_menu_item(1, "科室管理");
    ui_menu_item(2, "医生管理");
    ui_menu_item(3, "患者管理");
    ui_menu_item(4, "药物管理");
    ui_menu_item(5, "病房管理");
    ui_menu_item(6, "排班管理");
    ui_menu_item(7, "分析报表");
    ui_menu_item(8, "操作日志");
    ui_menu_item(9, "数据管理");
}

int admin_department_menu(const User *current_user) {
    (void)current_user;
    DepartmentNode *head = NULL;
    DepartmentNode *current = NULL;
    DepartmentNode *prev = NULL;
    DepartmentNode *new_node = NULL;
    Department dept;
    char dept_id[MAX_ID];
    char name[MAX_NAME];
    char leader[MAX_NAME];
    char phone[20];
    int choice;

    ui_header("科室管理");
    printf("1. 查看所有科室\n");
    printf("2. 新增科室\n");
    printf("3. 修改科室\n");
    printf("4. 删除科室\n");
    printf("0. 返回\n");

    choice = get_menu_choice(0, 4);
    switch (choice) {
        case 0:
            return SUCCESS;
        case 1:
            list_departments();
            return SUCCESS;
        case 2:
            printf("\n请输入科室编号(如DEP004): ");
            if (fgets(dept_id, sizeof(dept_id), stdin) == NULL) return ERROR_INVALID_INPUT;
            dept_id[strcspn(dept_id, "\n")] = 0;

            head = load_departments_list();
            current = head;
            while (current) {
                if (strcmp(current->data.department_id, dept_id) == 0) {
                    ui_err("科室编号已存在!");
                    free_department_list(head);
                    return ERROR_DUPLICATE;
                }
                current = current->next;
            }

            printf("请输入科室名称: ");
            if (fgets(name, sizeof(name), stdin) == NULL) {
                free_department_list(head);
                return ERROR_INVALID_INPUT;
            }
            name[strcspn(name, "\n")] = 0;
            if (name[0] == '\0') {
                ui_err("科室名称不能为空!");
                free_department_list(head);
                return ERROR_INVALID_INPUT;
            }

            printf("请输入负责人: ");
            if (fgets(leader, sizeof(leader), stdin) == NULL) {
                free_department_list(head);
                return ERROR_INVALID_INPUT;
            }
            leader[strcspn(leader, "\n")] = 0;
            if (leader[0] == '\0') {
                ui_err("负责人不能为空!");
                free_department_list(head);
                return ERROR_INVALID_INPUT;
            }

            printf("请输入联系电话: ");
            if (fgets(phone, sizeof(phone), stdin) == NULL) {
                free_department_list(head);
                return ERROR_INVALID_INPUT;
            }
            phone[strcspn(phone, "\n")] = 0;
            if (!is_valid_phone(phone)) {
                ui_err("电话号码无效! 需要7-15位纯数字。");
                free_department_list(head);
                return ERROR_INVALID_INPUT;
            }

            memset(&dept, 0, sizeof(dept));
            strcpy(dept.department_id, dept_id);
            strcpy(dept.name, name);
            strcpy(dept.leader, leader);
            strcpy(dept.phone, phone);

            new_node = create_department_node(&dept);
            if (!new_node) {
                free_department_list(head);
                return ERROR_FILE_IO;
            }

            if (!head) {
                head = new_node;
            } else {
                DepartmentNode *tail = head;
                while (tail->next) tail = tail->next;
                tail->next = new_node;
            }

            save_departments_list(head);
            free_department_list(head);
            append_log(current_user->username, "create", "department", dept_id, dept.name);
            printf("科室 %s 创建成功!\n", dept_id);
            return SUCCESS;
        case 3: {
            head = load_departments_list();
            int dc = count_department_list(head);
            if (dc == 0) { ui_warn("暂无科室数据。"); free_department_list(head); return SUCCESS; }

            const char **di = malloc(dc * sizeof(const char *));
            char (*db)[100] = malloc(dc * sizeof(*db));
            int di_idx = 0;
            DepartmentNode *dcu = head;
            while (dcu) {
                snprintf(db[di_idx], 100, "%s - %s", dcu->data.department_id, dcu->data.name);
                di[di_idx] = db[di_idx]; di_idx++; dcu = dcu->next;
            }

            int sel = ui_search_list("选择要修改的科室", di, dc);
            free((void*)di); free(db);
            if (sel < 0) { free_department_list(head); return SUCCESS; }

            current = head;
            for (int j = 0; j < sel; j++) current = current->next;
            ui_sub_header("修改科室");
            printf("  %-6s %s\n", "编号:", current->data.department_id);
            printf("  %-6s %s\n", "名称:", current->data.name);
            printf("  %-6s %s\n", "负责人:", current->data.leader);
            printf("  %-6s %s\n", "电话:", current->data.phone);
            ui_divider();

            printf("当前名称: %s, 输入新名称(直接回车不改): ", current->data.name);
            if (fgets(name, sizeof(name), stdin)) {
                name[strcspn(name, "\n")] = 0;
                if (name[0] != '\0') strcpy(current->data.name, name);
            }
            printf("当前负责人: %s, 输入新负责人(直接回车不改): ", current->data.leader);
            if (fgets(leader, sizeof(leader), stdin)) {
                leader[strcspn(leader, "\n")] = 0;
                if (leader[0] != '\0') strcpy(current->data.leader, leader);
            }
            printf("当前电话: %s, 输入新电话(直接回车不改): ", current->data.phone);
            if (fgets(phone, sizeof(phone), stdin)) {
                phone[strcspn(phone, "\n")] = 0;
                if (phone[0] != '\0') {
                    if (!is_valid_phone(phone)) {
                        ui_err("电话号码无效! 需要7-15位纯数字。");
                        free_department_list(head);
                        return ERROR_INVALID_INPUT;
                    }
                    strcpy(current->data.phone, phone);
                }
            }

            save_departments_list(head);
            free_department_list(head);
            append_log(current_user->username, "update", "department", current->data.department_id, current->data.name);
            printf("科室修改成功!\n");
            return SUCCESS;
        }
        case 4: {
            head = load_departments_list();
            int dc = count_department_list(head);
            if (dc == 0) { ui_warn("暂无科室数据。"); free_department_list(head); return SUCCESS; }

            const char **di = malloc(dc * sizeof(const char *));
            char (*db)[100] = malloc(dc * sizeof(*db));
            int di_idx = 0;
            DepartmentNode *dcu = head;
            while (dcu) {
                snprintf(db[di_idx], 100, "%s - %s", dcu->data.department_id, dcu->data.name);
                di[di_idx] = db[di_idx]; di_idx++; dcu = dcu->next;
            }

            int sel = ui_search_list("选择要删除的科室", di, dc);
            free((void*)di); free(db);
            if (sel < 0) { free_department_list(head); return SUCCESS; }

            current = head;
            prev = NULL;
            for (int j = 0; j < sel; j++) { prev = current; current = current->next; }

            {
                DoctorNode *doc_head = load_doctors_list();
                DoctorNode *doc_current = doc_head;
                int has_doctor = 0;
                while (doc_current) {
                    if (strcmp(doc_current->data.department_id, current->data.department_id) == 0) {
                        has_doctor = 1;
                        break;
                    }
                    doc_current = doc_current->next;
                }
                free_doctor_list(doc_head);
                if (has_doctor) {
                    printf("该科室下还有医生，无法删除! 请先将医生调离该科室。\n");
                    free_department_list(head);
                    return ERROR_PERMISSION_DENIED;
                }
            }

            if (!ui_confirm("确认删除该科室?")) {
                free_department_list(head);
                return SUCCESS;
            }

            if (!prev) {
                head = current->next;
            } else {
                prev->next = current->next;
            }
            append_log(current_user->username, "delete", "department", current->data.department_id, current->data.name);
            free(current);
            save_departments_list(head);
            free_department_list(head);
            printf("科室已删除!\n");
            return SUCCESS;
        }
        default:
            return ERROR_INVALID_INPUT;
    }
}

int admin_doctor_menu(const User *current_user) {
    (void)current_user;
    DoctorNode *head = NULL;
    DoctorNode *current = NULL;
    DoctorNode *prev = NULL;
    char doc_id[MAX_ID];
    char input[MAX_BUFFER];
    int choice;

    ui_header("医生管理");
    printf("1. 查看所有医生\n");
    printf("2. 修改医生信息\n");
    printf("3. 删除医生\n");
    printf("4. 新增医生\n");
    printf("0. 返回\n");

    choice = get_menu_choice(0, 4);
    switch (choice) {
        case 0:
            return SUCCESS;
        case 1:
            list_doctors();
            return SUCCESS;
        case 4: {
            char new_username[MAX_USERNAME];
            char new_name[MAX_NAME];
            char new_title[50];
            char new_dept[MAX_ID];
            User new_user;
            uint8_t hash_bytes[SHA256_DIGEST_SIZE];
            char hash_hex[SHA256_HEX_SIZE];

            ui_sub_header("新增医生");

            printf("请输入用户名: ");
            if (fgets(new_username, sizeof(new_username), stdin) == NULL) return ERROR_INVALID_INPUT;
            new_username[strcspn(new_username, "\n")] = 0;

            // Check username duplicate
            {
                UserNode *uh = load_users_list();
                UserNode *uc = uh;
                int dup = 0;
                while (uc) {
                    if (strcmp(uc->data.username, new_username) == 0) { dup = 1; break; }
                    uc = uc->next;
                }
                free_user_list(uh);
                if (dup) { ui_err("用户名已存在!"); return ERROR_DUPLICATE; }
            }

            printf("请输入姓名: ");
            if (fgets(new_name, sizeof(new_name), stdin) == NULL) return ERROR_INVALID_INPUT;
            new_name[strcspn(new_name, "\n")] = 0;

            // Title selection
            {
                const char *title_options[] = {
                    "主任医师", "副主任医师", "主治医师", "住院医师", "医士",
                    "主任药师", "副主任药师", "主管药师", "药师",
                    "主任护师", "副主任护师", "主管护师", "护师", "护士",
                    "教授", "副教授", "研究员", "副研究员"
                };
                int title_count = sizeof(title_options) / sizeof(title_options[0]);
                int title_sel = ui_select_list("选择职称", title_options, title_count);
                if (title_sel >= 0) strcpy(new_title, title_options[title_sel]);
                else strcpy(new_title, "医生");
            }

            // Department selection
            new_dept[0] = '\0';
            {
                DepartmentNode *dept_head = load_departments_list();
                if (dept_head) {
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
                        int sel = ui_search_list("选择所属科室", (const char **)di, dc);
                        if (sel >= 0) {
                            dp = dept_head;
                            for (int j = 0; j < sel; j++) dp = dp->next;
                            strcpy(new_dept, dp->data.department_id);
                        }
                        free((void*)di); free(db);
                    }
                }
                free_department_list(dept_head);
            }

            // Create user with default password (username+123)
            memset(&new_user, 0, sizeof(new_user));
            strcpy(new_user.username, new_username);
            strcpy(new_user.role, ROLE_DOCTOR);
            {
                char default_pwd[MAX_USERNAME + 10];
                snprintf(default_pwd, sizeof(default_pwd), "%s123", new_username);
                sha256_hash((const uint8_t*)default_pwd, strlen(default_pwd), hash_bytes);
                sha256_hex(hash_bytes, hash_hex);
                strcpy(new_user.password, hash_hex);
            }

            // Save user
            {
                UserNode *uh = load_users_list();
                UserNode *un = create_user_node(&new_user);
                if (!un) { free_user_list(uh); return ERROR_FILE_IO; }
                if (!uh) { uh = un; }
                else { UserNode *t = uh; while (t->next) t = t->next; t->next = un; }
                save_users_list(uh);
                free_user_list(uh);
            }

            // Create doctor profile
            create_doctor_profile_with_details(new_username, new_name, new_title, new_dept);

            append_log(current_user->username, "create", "doctor", new_user.username, new_name);
            printf("医生 %s (%s) 创建成功! 默认密码: %s123\n", new_name, new_username, new_username);
            return SUCCESS;
        }
        case 2: {
            head = load_doctors_list();
            int dc = count_doctor_list(head);
            if (dc == 0) { ui_warn("暂无医生数据。"); free_doctor_list(head); return SUCCESS; }

            const char **di = malloc(dc * sizeof(const char *));
            char (*db)[100] = malloc(dc * sizeof(*db));
            int di_idx = 0;
            DoctorNode *dcu = head;
            while (dcu) {
                snprintf(db[di_idx], 100, "%s - %s (%s)", dcu->data.doctor_id, dcu->data.name, dcu->data.title);
                di[di_idx] = db[di_idx]; di_idx++; dcu = dcu->next;
            }

            int sel = ui_search_list("选择要修改的医生", di, dc);
            free((void*)di); free(db);
            if (sel < 0) { free_doctor_list(head); return SUCCESS; }

            current = head;
            for (int j = 0; j < sel; j++) current = current->next;
            ui_sub_header("修改医生");
            printf("  %-6s %s\n", "编号:", current->data.doctor_id);
            printf("  %-6s %s\n", "姓名:", current->data.name);
            printf("  %-6s %s\n", "职称:", current->data.title);
            printf("  %-6s %s\n", "科室:", current->data.department_id[0] ? current->data.department_id : "未分配");
            ui_divider();

            printf("当前姓名: %s, 输入新姓名(直接回车不改): ", current->data.name);
            if (fgets(input, sizeof(input), stdin)) {
                input[strcspn(input, "\n")] = 0;
                if (input[0] != '\0') strcpy(current->data.name, input);
            }

            // 职称选择
            {
                const char *title_options[] = {
                    "主任医师", "副主任医师", "主治医师", "住院医师", "医士",
                    "主任药师", "副主任药师", "主管药师", "药师",
                    "主任护师", "副主任护师", "主管护师", "护师", "护士",
                    "教授", "副教授", "研究员", "副研究员"
                };
                int title_count = sizeof(title_options) / sizeof(title_options[0]);
                printf("当前职称: %s\n", current->data.title);
                int title_sel = ui_select_list("选择新职称", title_options, title_count);
                if (title_sel >= 0) {
                    strcpy(current->data.title, title_options[title_sel]);
                }
            }

            // 科室选择
            {
                DepartmentNode *dept_head = load_departments_list();
                if (dept_head) {
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
                        printf("当前科室: %s\n", current->data.department_id[0] ? current->data.department_id : "未分配");
                        int sel = ui_search_list("选择新科室", (const char **)di, dc);
                        if (sel >= 0) {
                            dp = dept_head;
                            for (int j = 0; j < sel; j++) dp = dp->next;
                            char old_id[MAX_ID];
                            char new_id[MAX_ID];
                            strcpy(old_id, current->data.doctor_id);
                            strcpy(current->data.department_id, dp->data.department_id);
                            generate_doctor_id(dp->data.department_id, new_id, MAX_ID);
                            if (strcmp(old_id, new_id) != 0) {
                                strcpy(current->data.doctor_id, new_id);
                                update_doctor_id_across_files(old_id, new_id);
                            }
                        }
                        free((void*)di); free(db);
                    } else {
                        ui_warn("暂无科室数据，无法修改科室。");
                    }
                } else {
                    ui_warn("暂无科室数据，无法修改科室。");
                }
                free_department_list(dept_head);
            }

            save_doctors_list(head);
            free_doctor_list(head);
            printf("医生信息修改成功!\n");
            return SUCCESS;
        }
        case 3: {
            head = load_doctors_list();
            int dc = count_doctor_list(head);
            if (dc == 0) { ui_warn("暂无医生数据。"); free_doctor_list(head); return SUCCESS; }

            const char **di = malloc(dc * sizeof(const char *));
            char (*db)[100] = malloc(dc * sizeof(*db));
            int di_idx = 0;
            DoctorNode *dcu = head;
            while (dcu) {
                snprintf(db[di_idx], 100, "%s - %s (%s)", dcu->data.doctor_id, dcu->data.name, dcu->data.title);
                di[di_idx] = db[di_idx]; di_idx++; dcu = dcu->next;
            }

            int sel = ui_search_list("选择要删除的医生", di, dc);
            free((void*)di); free(db);
            if (sel < 0) { free_doctor_list(head); return SUCCESS; }

            current = head;
            prev = NULL;
            for (int j = 0; j < sel; j++) { prev = current; current = current->next; }

            // Cascade check: verify no active appointments, records, or prescriptions
            {
                int has_active = 0;
                AppointmentNode *apt_head = load_appointments_list();
                AppointmentNode *apt_cur = apt_head;
                while (apt_cur && !has_active) {
                    if (strcmp(apt_cur->data.doctor_id, current->data.doctor_id) == 0 &&
                        strcmp(apt_cur->data.status, "已取消") != 0 &&
                        strcmp(apt_cur->data.status, "已完成") != 0 &&
                        strcmp(apt_cur->data.status, "已就诊") != 0) {
                        has_active = 1;
                    }
                    apt_cur = apt_cur->next;
                }
                free_appointment_list(apt_head);
                if (!has_active) {
                    MedicalRecordNode *rec_head = load_medical_records_list();
                    MedicalRecordNode *rec_cur = rec_head;
                    while (rec_cur && !has_active) {
                        if (strcmp(rec_cur->data.doctor_id, current->data.doctor_id) == 0) {
                            has_active = 1;
                        }
                        rec_cur = rec_cur->next;
                    }
                    free_medical_record_list(rec_head);
                }
                if (!has_active) {
                    PrescriptionNode *pre_head = load_prescriptions_list();
                    PrescriptionNode *pre_cur = pre_head;
                    while (pre_cur && !has_active) {
                        if (strcmp(pre_cur->data.doctor_id, current->data.doctor_id) == 0) {
                            has_active = 1;
                        }
                        pre_cur = pre_cur->next;
                    }
                    free_prescription_list(pre_head);
                }
                if (has_active) {
                    ui_err("该医生下有活跃的挂号/病历/处方记录，无法删除!");
                    free_doctor_list(head);
                    return ERROR_PERMISSION_DENIED;
                }
            }

            if (!ui_confirm("确认删除该医生?")) {
                free_doctor_list(head);
                return SUCCESS;
            }

            if (!prev) {
                head = current->next;
            } else {
                prev->next = current->next;
            }
            append_log(current_user->username, "delete", "doctor", current->data.doctor_id, current->data.name);
            free(current);
            save_doctors_list(head);
            free_doctor_list(head);
            printf("医生已删除!\n");
            return SUCCESS;
        }
        default:
            return ERROR_INVALID_INPUT;
    }
}

int admin_patient_menu(const User *current_user) {
    (void)current_user;
    PatientNode *head = NULL;
    PatientNode *current = NULL;
    PatientNode *prev = NULL;
    char patient_id[MAX_ID];
    char input[MAX_BUFFER];
    int int_input;
    int choice;

    ui_header("患者管理");
    printf("1. 查看所有患者\n");
    printf("2. 修改患者信息\n");
    printf("3. 删除患者\n");
    printf("4. 新增患者\n");
    printf("0. 返回\n");

    choice = get_menu_choice(0, 4);
    switch (choice) {
        case 0:
            return SUCCESS;
        case 1:
            list_patients();
            return SUCCESS;
        case 4: {
            char new_username[MAX_USERNAME];
            char new_name[MAX_NAME];
            char new_phone[20];
            char new_patient_type[20];
            User new_user;
            uint8_t hash_bytes[SHA256_DIGEST_SIZE];
            char hash_hex[SHA256_HEX_SIZE];

            ui_sub_header("新增患者");

            printf("请输入用户名: ");
            if (fgets(new_username, sizeof(new_username), stdin) == NULL) return ERROR_INVALID_INPUT;
            new_username[strcspn(new_username, "\n")] = 0;

            // Check username duplicate
            {
                UserNode *uh = load_users_list();
                UserNode *uc = uh;
                int dup = 0;
                while (uc) {
                    if (strcmp(uc->data.username, new_username) == 0) { dup = 1; break; }
                    uc = uc->next;
                }
                free_user_list(uh);
                if (dup) { ui_err("用户名已存在!"); return ERROR_DUPLICATE; }
            }

            printf("请输入姓名: ");
            if (fgets(new_name, sizeof(new_name), stdin) == NULL) return ERROR_INVALID_INPUT;
            new_name[strcspn(new_name, "\n")] = 0;

            printf("请输入电话(7-15位数字): ");
            if (fgets(new_phone, sizeof(new_phone), stdin) == NULL) return ERROR_INVALID_INPUT;
            new_phone[strcspn(new_phone, "\n")] = 0;
            if (new_phone[0] != '\0' && !is_valid_phone(new_phone)) {
                ui_err("电话号码无效!");
                return ERROR_INVALID_INPUT;
            }

            {
                const char *type_options[] = {"普通", "医保", "军人"};
                int tsel = ui_select_list("选择患者类型", type_options, 3);
                if (tsel >= 0) strcpy(new_patient_type, type_options[tsel]);
                else strcpy(new_patient_type, "普通");
            }

            // Create user
            memset(&new_user, 0, sizeof(new_user));
            strcpy(new_user.username, new_username);
            strcpy(new_user.role, ROLE_PATIENT);
            {
                char default_pwd[MAX_USERNAME + 10];
                snprintf(default_pwd, sizeof(default_pwd), "%s123", new_username);
                sha256_hash((const uint8_t*)default_pwd, strlen(default_pwd), hash_bytes);
                sha256_hex(hash_bytes, hash_hex);
                strcpy(new_user.password, hash_hex);
            }

            {
                UserNode *uh = load_users_list();
                UserNode *un = create_user_node(&new_user);
                if (!un) { free_user_list(uh); return ERROR_FILE_IO; }
                if (!uh) { uh = un; }
                else { UserNode *t = uh; while (t->next) t = t->next; t->next = un; }
                save_users_list(uh);
                free_user_list(uh);
            }

            // Create patient profile
            {
                PatientNode *ph = load_patients_list();
                Patient new_patient;
                memset(&new_patient, 0, sizeof(new_patient));
                generate_id(new_patient.patient_id, MAX_ID, "P");
                strcpy(new_patient.username, new_username);
                strcpy(new_patient.name, new_name);
                strcpy(new_patient.gender, "未知");
                strcpy(new_patient.phone, new_phone);
                strcpy(new_patient.patient_type, new_patient_type);
                strcpy(new_patient.treatment_stage, "初诊");
                new_patient.is_emergency = false;

                PatientNode *pn = create_patient_node(&new_patient);
                if (!pn) { free_patient_list(ph); return ERROR_FILE_IO; }
                if (!ph) { ph = pn; }
                else { PatientNode *t = ph; while (t->next) t = t->next; t->next = pn; }
                save_patients_list(ph);
                free_patient_list(ph);
            }

            append_log(current_user->username, "create", "patient", new_user.username, new_name);
            printf("患者 %s (%s) 创建成功! 默认密码: %s123\n", new_name, new_username, new_username);
            return SUCCESS;
        }
        case 2: {
            head = load_patients_list();
            int pc = count_patient_list(head);
            if (pc == 0) { ui_warn("暂无患者数据。"); free_patient_list(head); return SUCCESS; }

            const char **pi = malloc(pc * sizeof(const char *));
            char (*pb)[120] = malloc(pc * sizeof(*pb));
            int pi_idx = 0;
            PatientNode *pcu = head;
            while (pcu) {
                snprintf(pb[pi_idx], 120, "%s - %s (%s)", pcu->data.patient_id, pcu->data.name, pcu->data.patient_type);
                pi[pi_idx] = pb[pi_idx]; pi_idx++; pcu = pcu->next;
            }

            int sel = ui_search_list("选择要修改的患者", pi, pc);
            free((void*)pi); free(pb);
            if (sel < 0) { free_patient_list(head); return SUCCESS; }

            current = head;
            for (int j = 0; j < sel; j++) current = current->next;
            ui_sub_header("修改患者");
            printf("  %-8s %s\n", "编号:", current->data.patient_id);
            printf("  %-8s %s\n", "姓名:", current->data.name);
            printf("  %-8s %s\n", "性别:", current->data.gender);
            {
                char age_str[16];
                snprintf(age_str, sizeof(age_str), "%d", current->data.age);
                printf("  %-8s %s\n", "年龄:", age_str);
            }
            printf("  %-8s %s\n", "电话:", current->data.phone);
            printf("  %-8s %s\n", "类型:", current->data.patient_type);
            ui_divider();

            printf("当前姓名: %s, 输入新姓名(直接回车不改): ", current->data.name);
            if (fgets(input, sizeof(input), stdin)) {
                input[strcspn(input, "\n")] = 0;
                if (input[0] != '\0') strcpy(current->data.name, input);
            }

            // 性别选择
            {
                const char *gender_options[] = {"男", "女"};
                printf("当前性别: %s\n", current->data.gender);
                int gsel = ui_select_list("选择新性别", gender_options, 2);
                if (gsel >= 0) strcpy(current->data.gender, gender_options[gsel]);
            }

            printf("当前年龄: %d, 输入新年龄(-1不改): ", current->data.age);
            if (scanf("%d", &int_input) == 1) {
                if (int_input == -1) {
                    /* keep existing */
                } else if (is_valid_age(int_input)) {
                    current->data.age = int_input;
                } else {
                    ui_err("年龄必须在 0-150 之间。");
                    clear_input_buffer();
                    free_patient_list(head);
                    return ERROR_INVALID_INPUT;
                }
            }
            clear_input_buffer();

            printf("当前电话: %s, 输入新电话(直接回车不改): ", current->data.phone);
            if (fgets(input, sizeof(input), stdin)) {
                input[strcspn(input, "\n")] = 0;
                if (input[0] != '\0') {
                    if (!is_valid_phone(input)) {
                        ui_err("电话号码无效! 需要7-15位纯数字。");
                        free_patient_list(head);
                        return ERROR_INVALID_INPUT;
                    }
                    strcpy(current->data.phone, input);
                }
            }

            // 患者类型选择
            {
                const char *type_options[] = {"普通", "医保", "军人"};
                printf("当前患者类型: %s\n", current->data.patient_type);
                int tsel = ui_select_list("选择新患者类型", type_options, 3);
                if (tsel >= 0) strcpy(current->data.patient_type, type_options[tsel]);
            }

            save_patients_list(head);
            free_patient_list(head);
            printf("患者信息修改成功!\n");
            return SUCCESS;
        }
        case 3: {
            head = load_patients_list();
            int pc = count_patient_list(head);
            if (pc == 0) { ui_warn("暂无患者数据。"); free_patient_list(head); return SUCCESS; }

            const char **pi = malloc(pc * sizeof(const char *));
            char (*pb)[120] = malloc(pc * sizeof(*pb));
            int pi_idx = 0;
            PatientNode *pcu = head;
            while (pcu) {
                snprintf(pb[pi_idx], 120, "%s - %s (%s)", pcu->data.patient_id, pcu->data.name, pcu->data.patient_type);
                pi[pi_idx] = pb[pi_idx]; pi_idx++; pcu = pcu->next;
            }

            int sel = ui_search_list("选择要删除的患者", pi, pc);
            free((void*)pi); free(pb);
            if (sel < 0) { free_patient_list(head); return SUCCESS; }

            current = head;
            prev = NULL;
            for (int j = 0; j < sel; j++) { prev = current; current = current->next; }

            // Cascade check: verify no active appointment registrations
            {
                int has_appointment = 0;
                AppointmentNode *apt_head = load_appointments_list();
                AppointmentNode *apt_cur = apt_head;
                while (apt_cur && !has_appointment) {
                    if (strcmp(apt_cur->data.patient_id, current->data.patient_id) == 0 &&
                        strcmp(apt_cur->data.status, "已取消") != 0 &&
                        strcmp(apt_cur->data.status, "已完成") != 0 &&
                        strcmp(apt_cur->data.status, "已就诊") != 0 &&
                        strcmp(apt_cur->data.status, "已退号") != 0) {
                        has_appointment = 1;
                    }
                    apt_cur = apt_cur->next;
                }
                free_appointment_list(apt_head);
                if (!has_appointment) {
                    OnsiteRegistrationQueue oq = load_onsite_registration_queue();
                    OnsiteRegistrationNode *oc = oq.front;
                    while (oc && !has_appointment) {
                        if (strcmp(oc->data.patient_id, current->data.patient_id) == 0 &&
                            strcmp(oc->data.status, "已退号") != 0 &&
                            strcmp(oc->data.status, "已完成") != 0) {
                            has_appointment = 1;
                        }
                        oc = oc->next;
                    }
                    free_onsite_registration_queue(&oq);
                }
                if (has_appointment) {
                    ui_err("该患者下有活跃的挂号记录，无法删除!");
                    free_patient_list(head);
                    return ERROR_PERMISSION_DENIED;
                }
            }

            if (!ui_confirm("确认删除该患者?")) {
                free_patient_list(head);
                return SUCCESS;
            }

            if (!prev) {
                head = current->next;
            } else {
                prev->next = current->next;
            }
            append_log(current_user->username, "delete", "patient", current->data.patient_id, current->data.name);
            free(current);
            save_patients_list(head);
            free_patient_list(head);
            printf("患者已删除!\n");
            return SUCCESS;
        }
        default:
            return ERROR_INVALID_INPUT;
    }
}

int admin_drug_menu(const User *current_user) {
    DrugNode *head = NULL;
    DrugNode *current = NULL;
    DrugNode *prev = NULL;
    DrugNode *new_node = NULL;
    Drug drug;
    char drug_id[MAX_ID];
    char input[MAX_BUFFER];
    int add_stock;
    int choice;

    (void)current_user;
    ui_header("药物管理");
    printf("1. 查看药品\n");
    printf("2. 新增药品\n");
    printf("3. 修改药品\n");
    printf("4. 药品补货\n");
    printf("5. 删除药品\n");
    printf("6. 库存预警\n");
    printf("7. 批量补货\n");
    printf("0. 返回\n");

    choice = get_menu_choice(0, 7);
    switch (choice) {
        case 0:
            return SUCCESS;
        case 1:
            list_drugs();
            return SUCCESS;
        case 2:
            printf("\n请输入药品编号: ");
            if (fgets(drug_id, sizeof(drug_id), stdin) == NULL) return ERROR_INVALID_INPUT;
            drug_id[strcspn(drug_id, "\n")] = 0;
            if (drug_id[0] == '\0') {
                ui_err("药品编号不能为空!");
                return ERROR_INVALID_INPUT;
            }

            head = load_drugs_list();
            current = head;
            while (current) {
                if (strcmp(current->data.drug_id, drug_id) == 0) {
                    ui_err("药品编号已存在!");
                    free_drug_list(head);
                    return ERROR_DUPLICATE;
                }
                current = current->next;
            }

            memset(&drug, 0, sizeof(drug));
            strcpy(drug.drug_id, drug_id);
            printf("请输入药品名称: ");
            if (fgets(drug.name, sizeof(drug.name), stdin) == NULL) {
                free_drug_list(head);
                return ERROR_INVALID_INPUT;
            }
            drug.name[strcspn(drug.name, "\n")] = 0;
            if (drug.name[0] == '\0') {
                ui_err("药品名称不能为空!");
                free_drug_list(head);
                return ERROR_INVALID_INPUT;
            }

            printf("请输入单价: ");
            if (scanf("%f", &drug.price) != 1 || drug.price < 0) {
                clear_input_buffer();
                free_drug_list(head);
                return ERROR_INVALID_INPUT;
            }
            clear_input_buffer();

            printf("请输入库存数量: ");
            if (scanf("%d", &drug.stock_num) != 1 || drug.stock_num < 0) {
                clear_input_buffer();
                free_drug_list(head);
                return ERROR_INVALID_INPUT;
            }
            clear_input_buffer();

            printf("请输入预警阈值: ");
            if (scanf("%d", &drug.warning_line) != 1 || drug.warning_line < 0) {
                clear_input_buffer();
                free_drug_list(head);
                return ERROR_INVALID_INPUT;
            }
            clear_input_buffer();

            printf("是否特效药(1:是, 0:否): ");
            {
                int is_special_int;
                if (scanf("%d", &is_special_int) != 1) {
                    clear_input_buffer();
                    free_drug_list(head);
                    return ERROR_INVALID_INPUT;
                }
                drug.is_special = (is_special_int != 0);
            }
            clear_input_buffer();

            printf("请输入报销比例(0-1): ");
            if (scanf("%f", &drug.reimbursement_ratio) != 1 || drug.reimbursement_ratio < 0 || drug.reimbursement_ratio > 1) {
                clear_input_buffer();
                free_drug_list(head);
                return ERROR_INVALID_INPUT;
            }
            clear_input_buffer();

            {
                const char *cat_options[] = {"西药", "中成药", "中药饮片", "耗材", "其他"};
                int cat_sel = ui_select_list("选择药品分类", cat_options, 5);
                if (cat_sel >= 0) strcpy(drug.category, cat_options[cat_sel]);
                else strcpy(drug.category, "西药");
            }

            new_node = create_drug_node(&drug);
            if (!new_node) {
                free_drug_list(head);
                return ERROR_FILE_IO;
            }

            if (!head) {
                head = new_node;
            } else {
                DrugNode *tail = head;
                while (tail->next) tail = tail->next;
                tail->next = new_node;
            }
            save_drugs_list(head);
            free_drug_list(head);
            append_log(current_user->username, "create", "drug", drug_id, drug.name);
            printf("药品 %s 创建成功!\n", drug_id);
            return SUCCESS;
        case 3: {
            head = load_drugs_list();
            int dc = count_drug_list(head);
            if (dc == 0) { ui_warn("暂无药品数据。"); free_drug_list(head); return SUCCESS; }

            const char **di = malloc(dc * sizeof(const char *));
            char (*db)[120] = malloc(dc * sizeof(*db));
            int di_idx = 0;
            DrugNode *dcu = head;
            while (dcu) {
                snprintf(db[di_idx], 120, "%s - %s (库存:%d)", dcu->data.drug_id, dcu->data.name, dcu->data.stock_num);
                di[di_idx] = db[di_idx]; di_idx++; dcu = dcu->next;
            }

            int sel = ui_search_list("选择要修改的药品", di, dc);
            free((void*)di); free(db);
            if (sel < 0) { free_drug_list(head); return SUCCESS; }

            current = head;
            for (int j = 0; j < sel; j++) current = current->next;
            ui_sub_header("修改药品");
            printf("  %-10s %s\n", "编号:", current->data.drug_id);
            printf("  %-10s %s\n", "名称:", current->data.name);
            printf("  %-10s %.2f\n", "单价:", current->data.price);
            printf("  %-10s %d\n", "库存:", current->data.stock_num);
            printf("  %-10s %d\n", "预警阈值:", current->data.warning_line);
            printf("  %-10s %.0f%%\n", "报销率:", current->data.reimbursement_ratio * 100);
            printf("  %-10s %s\n", "分类:", current->data.category);
            ui_divider();

            printf("当前名称: %s, 输入新名称(直接回车不改): ", current->data.name);
            if (fgets(input, sizeof(input), stdin)) {
                input[strcspn(input, "\n")] = 0;
                if (input[0] != '\0') strcpy(current->data.name, input);
            }

            printf("当前单价: %.2f, 输入新单价(-1不改): ", current->data.price);
            {
                float new_price;
                if (scanf("%f", &new_price) == 1 && new_price >= 0) current->data.price = new_price;
                clear_input_buffer();
            }

            printf("当前库存: %d, 输入新库存(-1不改): ", current->data.stock_num);
            {
                int new_stock;
                if (scanf("%d", &new_stock) == 1 && new_stock >= 0) current->data.stock_num = new_stock;
                clear_input_buffer();
            }

            printf("当前预警值: %d, 输入新预警值(-1不改): ", current->data.warning_line);
            {
                int new_warning;
                if (scanf("%d", &new_warning) == 1 && new_warning >= 0) current->data.warning_line = new_warning;
                clear_input_buffer();
            }

            printf("当前报销率: %.2f, 输入新报销率(-1不改): ", current->data.reimbursement_ratio);
            {
                float new_ratio;
                if (scanf("%f", &new_ratio) == 1 && new_ratio >= 0) {
                    if (new_ratio > 1) new_ratio = 1;
                    current->data.reimbursement_ratio = new_ratio;
                }
                clear_input_buffer();
            }

            {
                const char *cat_options[] = {"西药", "中成药", "中药饮片", "耗材", "其他"};
                printf("当前分类: %s\n", current->data.category);
                int cat_sel = ui_select_list("选择新分类", cat_options, 5);
                if (cat_sel >= 0) strcpy(current->data.category, cat_options[cat_sel]);
            }

            save_drugs_list(head);
            free_drug_list(head);
            append_log(current_user->username, "update", "drug", current->data.drug_id, current->data.name);
            printf("药品信息修改成功!\n");
            return SUCCESS;
        }
        case 4: {
            head = load_drugs_list();
            int dc = count_drug_list(head);
            if (dc == 0) { ui_warn("暂无药品数据。"); free_drug_list(head); return SUCCESS; }

            const char **di = malloc(dc * sizeof(const char *));
            char (*db)[120] = malloc(dc * sizeof(*db));
            int di_idx = 0;
            DrugNode *dcu = head;
            while (dcu) {
                snprintf(db[di_idx], 120, "%s - %s (库存:%d)", dcu->data.drug_id, dcu->data.name, dcu->data.stock_num);
                di[di_idx] = db[di_idx]; di_idx++; dcu = dcu->next;
            }

            int sel = ui_search_list("选择要补货的药品", di, dc);
            free((void*)di); free(db);
            if (sel < 0) { free_drug_list(head); return SUCCESS; }

            current = head;
            for (int j = 0; j < sel; j++) current = current->next;

            printf("当前库存: %d, 请输入补货数量: ", current->data.stock_num);
            if (scanf("%d", &add_stock) != 1) {
                clear_input_buffer();
                free_drug_list(head);
                return ERROR_INVALID_INPUT;
            }
            clear_input_buffer();
            if (add_stock <= 0) { free_drug_list(head); return ERROR_INVALID_INPUT; }

            current->data.stock_num += add_stock;
            save_drugs_list(head);
            {
                char detail[60];
                snprintf(detail, sizeof(detail), "%s +%d(库存:%d)", current->data.name, add_stock, current->data.stock_num);
                append_log(current_user->username, "update", "drug", current->data.drug_id, detail);
            }
            printf("补货成功，当前库存: %d\n", current->data.stock_num);
            free_drug_list(head);
            return SUCCESS;
        }
        case 5: {
            head = load_drugs_list();
            int dc = count_drug_list(head);
            if (dc == 0) { ui_warn("暂无药品数据。"); free_drug_list(head); return SUCCESS; }

            const char **di = malloc(dc * sizeof(const char *));
            char (*db)[120] = malloc(dc * sizeof(*db));
            int di_idx = 0;
            DrugNode *dcu = head;
            while (dcu) {
                snprintf(db[di_idx], 120, "%s - %s (库存:%d)", dcu->data.drug_id, dcu->data.name, dcu->data.stock_num);
                di[di_idx] = db[di_idx]; di_idx++; dcu = dcu->next;
            }

            int sel = ui_search_list("选择要删除的药品", di, dc);
            free((void*)di); free(db);
            if (sel < 0) { free_drug_list(head); return SUCCESS; }

            current = head;
            prev = NULL;
            for (int j = 0; j < sel; j++) { prev = current; current = current->next; }

            if (!ui_confirm("确认删除该药品?")) {
                free_drug_list(head);
                return SUCCESS;
            }

            if (!prev) {
                head = current->next;
            } else {
                prev->next = current->next;
            }
            append_log(current_user->username, "delete", "drug", current->data.drug_id, current->data.name);
            free(current);
            save_drugs_list(head);
            free_drug_list(head);
            printf("药品已删除!\n");
            return SUCCESS;
        }
        case 6:
            check_drug_warning();
            return SUCCESS;
        case 7: {
            head = load_drugs_list();
            int dc = count_drug_list(head);
            if (dc == 0) { ui_warn("暂无药品数据。"); free_drug_list(head); return SUCCESS; }

            const char **di = malloc(dc * sizeof(const char *));
            char (*db)[120] = malloc(dc * sizeof(*db));
            int di_idx = 0;
            DrugNode *dcu = head;
            while (dcu) {
                snprintf(db[di_idx], 120, "%s - %s (库存:%d)", dcu->data.drug_id, dcu->data.name, dcu->data.stock_num);
                di[di_idx] = db[di_idx]; di_idx++; dcu = dcu->next;
            }

            bool *selected = calloc(dc, sizeof(bool));
            int sel_count = 0;

            while (1) {
                printf("\n当前已选 %d 种药品。\n", sel_count);
                int sel = ui_search_list("选择药品(选完后按Esc结束批量选择)", di, dc);
                if (sel < 0) { break; }
                if (!selected[sel]) {
                    selected[sel] = true;
                    sel_count++;
                    printf("已选择: %s\n", db[sel]);
                } else {
                    printf("该药品已在选择列表中。\n");
                }
            }

            if (sel_count == 0) {
                free((void*)di); free(db); free(selected); free_drug_list(head);
                return SUCCESS;
            }

            printf("请输入每种药品的补货数量: ");
            int qty;
            if (scanf("%d", &qty) != 1 || qty <= 0) {
                clear_input_buffer();
                free((void*)di); free(db); free(selected); free_drug_list(head);
                return ERROR_INVALID_INPUT;
            }
            clear_input_buffer();

            int applied = 0;
            dcu = head;
            for (int i = 0; i < dc && dcu; i++) {
                if (selected[i]) { dcu->data.stock_num += qty; applied++; }
                dcu = dcu->next;
            }

            save_drugs_list(head);
            {
                char detail[60];
                snprintf(detail, sizeof(detail), "%d种药品各+%d", applied, qty);
                append_log(current_user->username, "update", "drug", "batch", detail);
            }
            printf("批量补货完成! %d 种药品每种增加 %d。\n", applied, qty);
            free((void*)di); free(db); free(selected); free_drug_list(head);
            return SUCCESS;
        }
        default:
            return ERROR_INVALID_INPUT;
    }
}

int admin_ward_menu(const User *current_user) {
    WardNode *ward_head = NULL;
    WardNode *current_ward = NULL;
    WardNode *prev = NULL;
    WardNode *new_node = NULL;
    Ward ward;
    Patient *patient = NULL;
    char ward_id[MAX_ID];
    char patient_id[MAX_ID];
    char message[200];
    char input[MAX_BUFFER];
    int remain_beds;
    int choice;

    (void)current_user;
    ui_header("病房管理");
    printf("1. 查看病房\n");
    printf("2. 新增病房\n");
    printf("3. 修改病房\n");
    printf("4. 删除病房\n");
    printf("5. 调整剩余床位\n");
    printf("6. 发起病房呼叫\n");
    printf("7. 床位预警\n");
    printf("0. 返回\n");

    choice = get_menu_choice(0, 7);
    switch (choice) {
        case 0:
            return SUCCESS;
        case 1:
            list_wards();
            return SUCCESS;
        case 2:
            printf("\n请输入病房编号: ");
            if (fgets(ward_id, sizeof(ward_id), stdin) == NULL) return ERROR_INVALID_INPUT;
            ward_id[strcspn(ward_id, "\n")] = 0;
            if (ward_id[0] == '\0') {
                ui_err("病房编号不能为空!");
                return ERROR_INVALID_INPUT;
            }

            ward_head = load_wards_list();
            current_ward = ward_head;
            while (current_ward) {
                if (strcmp(current_ward->data.ward_id, ward_id) == 0) {
                    ui_err("病房编号已存在!");
                    free_ward_list(ward_head);
                    return ERROR_DUPLICATE;
                }
                current_ward = current_ward->next;
            }

            memset(&ward, 0, sizeof(ward));
            strcpy(ward.ward_id, ward_id);
            printf("请输入病房类型: ");
            if (fgets(ward.type, sizeof(ward.type), stdin) == NULL) {
                free_ward_list(ward_head);
                return ERROR_INVALID_INPUT;
            }
            ward.type[strcspn(ward.type, "\n")] = 0;
            if (ward.type[0] == '\0') {
                ui_err("病房类型不能为空!");
                free_ward_list(ward_head);
                return ERROR_INVALID_INPUT;
            }

            printf("请输入总床位数: ");
            if (scanf("%d", &ward.total_beds) != 1 || ward.total_beds <= 0) {
                clear_input_buffer();
                free_ward_list(ward_head);
                return ERROR_INVALID_INPUT;
            }
            clear_input_buffer();

            printf("请输入剩余床位数: ");
            if (scanf("%d", &ward.remain_beds) != 1 || ward.remain_beds < 0 || ward.remain_beds > ward.total_beds) {
                clear_input_buffer();
                free_ward_list(ward_head);
                return ERROR_INVALID_INPUT;
            }
            clear_input_buffer();

            printf("请输入预警阈值: ");
            if (scanf("%d", &ward.warning_line) != 1 || ward.warning_line < 0) {
                clear_input_buffer();
                free_ward_list(ward_head);
                return ERROR_INVALID_INPUT;
            }
            clear_input_buffer();

            new_node = create_ward_node(&ward);
            if (!new_node) {
                free_ward_list(ward_head);
                return ERROR_FILE_IO;
            }

            if (!ward_head) {
                ward_head = new_node;
            } else {
                WardNode *tail = ward_head;
                while (tail->next) tail = tail->next;
                tail->next = new_node;
            }
            save_wards_list(ward_head);
            free_ward_list(ward_head);
            append_log(current_user->username, "create", "ward", ward_id, ward.type);
            printf("病房 %s 创建成功!\n", ward_id);
            return SUCCESS;
        case 3: {
            ward_head = load_wards_list();
            int wc = count_ward_list(ward_head);
            if (wc == 0) { ui_warn("暂无病房数据。"); free_ward_list(ward_head); return SUCCESS; }

            const char **wi = malloc(wc * sizeof(const char *));
            char (*wb)[100] = malloc(wc * sizeof(*wb));
            int wi_idx = 0;
            WardNode *wcu = ward_head;
            while (wcu) {
                snprintf(wb[wi_idx], 100, "%s - %s (%d床)", wcu->data.ward_id, wcu->data.type, wcu->data.total_beds);
                wi[wi_idx] = wb[wi_idx]; wi_idx++; wcu = wcu->next;
            }

            int sel = ui_search_list("选择要修改的病房", wi, wc);
            free((void*)wi); free(wb);
            if (sel < 0) { free_ward_list(ward_head); return SUCCESS; }

            current_ward = ward_head;
            for (int j = 0; j < sel; j++) current_ward = current_ward->next;
            ui_sub_header("修改病房");
            printf("  %-10s %s\n", "编号:", current_ward->data.ward_id);
            printf("  %-10s %s\n", "类型:", current_ward->data.type);
            printf("  %-10s %d\n", "总床位:", current_ward->data.total_beds);
            printf("  %-10s %d\n", "剩余床位:", current_ward->data.remain_beds);
            printf("  %-10s %d\n", "预警阈值:", current_ward->data.warning_line);
            ui_divider();

            printf("当前类型: %s, 输入新类型(直接回车不改): ", current_ward->data.type);
            if (fgets(input, sizeof(input), stdin)) {
                input[strcspn(input, "\n")] = 0;
                if (input[0] != '\0') strcpy(current_ward->data.type, input);
            }

            printf("当前总床位: %d, 输入新总床位(-1不改): ", current_ward->data.total_beds);
            {
                int new_total;
                if (scanf("%d", &new_total) == 1 && new_total > 0) {
                    current_ward->data.total_beds = new_total;
                }
                clear_input_buffer();
            }

            printf("当前剩余床位: %d, 输入新剩余床位(-1不改): ", current_ward->data.remain_beds);
            {
                int new_remain;
                if (scanf("%d", &new_remain) == 1 && new_remain >= 0) {
                    if (new_remain <= current_ward->data.total_beds) {
                        current_ward->data.remain_beds = new_remain;
                    }
                }
                clear_input_buffer();
            }

            save_wards_list(ward_head);
            free_ward_list(ward_head);
            append_log(current_user->username, "update", "ward", current_ward->data.ward_id, current_ward->data.type);
            printf("病房信息修改成功!\n");
            return SUCCESS;
        }
        case 4: {
            ward_head = load_wards_list();
            int wc = count_ward_list(ward_head);
            if (wc == 0) { ui_warn("暂无病房数据。"); free_ward_list(ward_head); return SUCCESS; }

            const char **wi = malloc(wc * sizeof(const char *));
            char (*wb)[100] = malloc(wc * sizeof(*wb));
            int wi_idx = 0;
            WardNode *wcu = ward_head;
            while (wcu) {
                snprintf(wb[wi_idx], 100, "%s - %s (%d床)", wcu->data.ward_id, wcu->data.type, wcu->data.total_beds);
                wi[wi_idx] = wb[wi_idx]; wi_idx++; wcu = wcu->next;
            }

            int sel = ui_search_list("选择要删除的病房", wi, wc);
            free((void*)wi); free(wb);
            if (sel < 0) { free_ward_list(ward_head); return SUCCESS; }

            current_ward = ward_head;
            prev = NULL;
            for (int j = 0; j < sel; j++) { prev = current_ward; current_ward = current_ward->next; }

            if (!ui_confirm("确认删除该病房?")) {
                free_ward_list(ward_head);
                return SUCCESS;
            }

            if (!prev) {
                ward_head = current_ward->next;
            } else {
                prev->next = current_ward->next;
            }
            append_log(current_user->username, "delete", "ward", current_ward->data.ward_id, current_ward->data.type);
            free(current_ward);
            save_wards_list(ward_head);
            free_ward_list(ward_head);
            printf("病房已删除!\n");
            return SUCCESS;
        }
        case 5: {
            ward_head = load_wards_list();
            int wc = count_ward_list(ward_head);
            if (wc == 0) { ui_warn("暂无病房数据。"); free_ward_list(ward_head); return SUCCESS; }

            const char **wi = malloc(wc * sizeof(const char *));
            char (*wb)[100] = malloc(wc * sizeof(*wb));
            int wi_idx = 0;
            WardNode *wcu = ward_head;
            while (wcu) {
                snprintf(wb[wi_idx], 100, "%s - %s (%d/%d床)", wcu->data.ward_id, wcu->data.type,
                         wcu->data.remain_beds, wcu->data.total_beds);
                wi[wi_idx] = wb[wi_idx]; wi_idx++; wcu = wcu->next;
            }

            int sel = ui_search_list("选择要调整床位的病房", wi, wc);
            free((void*)wi); free(wb);
            if (sel < 0) { free_ward_list(ward_head); return SUCCESS; }

            current_ward = ward_head;
            for (int j = 0; j < sel; j++) current_ward = current_ward->next;

            printf("当前剩余床位: %d/%d, 请输入新的剩余床位数: ", current_ward->data.remain_beds, current_ward->data.total_beds);
            if (scanf("%d", &remain_beds) != 1) {
                clear_input_buffer();
                free_ward_list(ward_head);
                return ERROR_INVALID_INPUT;
            }
            clear_input_buffer();

            if (remain_beds < 0 || remain_beds > current_ward->data.total_beds) {
                ui_err("剩余床位超出范围!");
                free_ward_list(ward_head);
                return ERROR_INVALID_INPUT;
            }
            current_ward->data.remain_beds = remain_beds;
            save_wards_list(ward_head);
            {
                char detail[60];
                snprintf(detail, sizeof(detail), "%s 剩余床位:%d", current_ward->data.type, remain_beds);
                append_log(current_user->username, "update", "ward", current_ward->data.ward_id, detail);
            }
            printf("病房床位更新成功。\n");
            free_ward_list(ward_head);
            return SUCCESS;
        }
        case 6: {
            WardCallNode *call_head = load_ward_calls_list();
            WardCallNode *tail = call_head;
            WardCall call;

            ward_head = load_wards_list();
            int wc = count_ward_list(ward_head);
            if (wc == 0) { ui_warn("暂无病房数据。"); free_ward_list(ward_head); free_ward_call_list(call_head); return SUCCESS; }

            const char **wi = malloc(wc * sizeof(const char *));
            char (*wb)[100] = malloc(wc * sizeof(*wb));
            int wi_idx = 0;
            WardNode *wcu = ward_head;
            while (wcu) {
                snprintf(wb[wi_idx], 100, "%s - %s (%d/%d床)", wcu->data.ward_id, wcu->data.type,
                         wcu->data.remain_beds, wcu->data.total_beds);
                wi[wi_idx] = wb[wi_idx]; wi_idx++; wcu = wcu->next;
            }

            int sel = ui_search_list("选择病房", wi, wc);
            free((void*)wi); free(wb);
            if (sel < 0) { free_ward_list(ward_head); free_ward_call_list(call_head); return SUCCESS; }

            current_ward = ward_head;
            for (int j = 0; j < sel; j++) current_ward = current_ward->next;
            strcpy(ward_id, current_ward->data.ward_id);
            free_ward_list(ward_head);

            {
                PatientNode *ph = load_patients_list();
                int pc = count_patient_list(ph);
                if (pc == 0) { ui_warn("暂无患者。"); free_patient_list(ph); free_ward_call_list(call_head); return SUCCESS; }
                const char **pi = malloc(pc * sizeof(const char *));
                char (*pb)[90] = malloc(pc * sizeof(*pb));
                int idx = 0;
                PatientNode *pu = ph;
                while (pu) {
                    snprintf(pb[idx], 90, "%s - %s (%s)", pu->data.patient_id, pu->data.name, pu->data.patient_type);
                    pi[idx] = pb[idx]; idx++; pu = pu->next;
                }
                int sel = ui_search_list("选择患者", pi, pc);
                free((void*)pi); free(pb);
                if (sel < 0) { free_patient_list(ph); free_ward_call_list(call_head); return SUCCESS; }
                pu = ph;
                for (int j = 0; j < sel; j++) pu = pu->next;
                strcpy(patient_id, pu->data.patient_id);
                free_patient_list(ph);
            }

            patient = find_patient_by_id(patient_id);
            if (!patient) {
                free_ward_call_list(call_head);
                return ERROR_NOT_FOUND;
            }

            printf("请输入呼叫说明: ");
            if (fgets(message, sizeof(message), stdin) == NULL) {
                free(patient);
                free_ward_call_list(call_head);
                return ERROR_INVALID_INPUT;
            }
            message[strcspn(message, "\n")] = 0;

            memset(&call, 0, sizeof(call));
            generate_id(call.call_id, MAX_ID, "WC");
            strcpy(call.ward_id, ward_id);
            strcpy(call.patient_id, patient_id);
            strcpy(call.message, message);
            strcpy(call.status, "待处理");
            get_current_time(call.create_time, sizeof(call.create_time));

            {
                AppointmentNode *appointment_head = load_appointments_list();
                AppointmentNode *current_appointment = appointment_head;
                strcpy(call.department_id, "DEP001");
                while (current_appointment) {
                    if (strcmp(current_appointment->data.patient_id, patient_id) == 0) {
                        strcpy(call.department_id, current_appointment->data.department_id);
                        break;
                    }
                    current_appointment = current_appointment->next;
                }
                free_appointment_list(appointment_head);
            }

            {
                WardCallNode *new_node = create_ward_call_node(&call);
                if (!new_node) {
                    free(patient);
                    free_ward_call_list(call_head);
                    return ERROR_FILE_IO;
                }

                if (!call_head) {
                    call_head = new_node;
                } else {
                    while (tail->next) tail = tail->next;
                    tail->next = new_node;
                }
            }

            save_ward_calls_list(call_head);
            printf("病房呼叫已创建，等待对应医生处理。\n");
            free(patient);
            free_ward_call_list(call_head);
            return SUCCESS;
        }
        case 7:
            check_ward_warning();
            return SUCCESS;
        default:
            return ERROR_INVALID_INPUT;
    }
}

// Replaced by admin_analysis_menu() in analysis.c

// ======================= 排班管理 =======================

static void list_schedules(void) {
    ScheduleNode *head = load_schedules_list();
    int count = count_schedule_list(head);

    if (count == 0) {
        ui_warn("暂无排班数据。");
        free_schedule_list(head);
        return;
    }

    const char **items = malloc(count * sizeof(const char *));
    char (*buf)[100] = malloc(count * sizeof(*buf));

    int i = 0;
    ScheduleNode *cur = head;
    while (cur) {
        snprintf(buf[i], 100, "%-16s %-10s %-10s %-6s %3d %3d %-6s",
                 cur->data.schedule_id,
                 cur->data.doctor_id,
                 cur->data.work_date,
                 cur->data.time_slot,
                 cur->data.max_appt,
                 cur->data.max_onsite,
                 cur->data.status);
        items[i] = buf[i];
        i++; cur = cur->next;
    }

    ui_paginate(items, count, 15, "排班列表");

    free((void*)items);
    free(buf);
    free_schedule_list(head);
}

int admin_schedule_menu(const User *current_user) {
    (void)current_user;
    ScheduleNode *head = NULL;
    ScheduleNode *current = NULL;
    ScheduleNode *prev = NULL;
    ScheduleNode *new_node = NULL;
    Schedule schedule;
    char schedule_id[MAX_ID];
    char doctor_id[MAX_ID];
    char work_date[12];
    char time_slot[10];
    char start_date[12];
    char end_date[12];
    char input[MAX_BUFFER];
    int choice;

    ui_header("排班管理");
    printf("1. 查看所有排班\n");
    printf("2. 新增排班\n");
    printf("3. 批量排班\n");
    printf("4. 修改排班\n");
    printf("5. 删除排班\n");
    printf("6. 停诊/恢复\n");
    printf("0. 返回\n");

    choice = get_menu_choice(0, 6);
    switch (choice) {
        case 0:
            return SUCCESS;
        case 1:
            list_schedules();
            return SUCCESS;

        case 2:
            printf("\n请输入医生编号: ");
            if (fgets(doctor_id, sizeof(doctor_id), stdin) == NULL) return ERROR_INVALID_INPUT;
            doctor_id[strcspn(doctor_id, "\n")] = 0;

            printf("请输入排班日期(如 2026-05-02): ");
            if (fgets(work_date, sizeof(work_date), stdin) == NULL) return ERROR_INVALID_INPUT;
            work_date[strcspn(work_date, "\n")] = 0;

            printf("选择时段 (1.上午 2.下午 3.全天): ");
            {
                int ts;
                if (scanf("%d", &ts) != 1) { clear_input_buffer(); return ERROR_INVALID_INPUT; }
                clear_input_buffer();
                if (ts == 1) strcpy(time_slot, "上午");
                else if (ts == 2) strcpy(time_slot, "下午");
                else strcpy(time_slot, "全天");
            }

            // Check for duplicate
            head = load_schedules_list();
            current = head;
            while (current) {
                if (strcmp(current->data.doctor_id, doctor_id) == 0 &&
                    strcmp(current->data.work_date, work_date) == 0 &&
                    strcmp(current->data.time_slot, time_slot) == 0) {
                    ui_err("该医生在此时间段已有排班!");
                    free_schedule_list(head);
                    return ERROR_DUPLICATE;
                }
                current = current->next;
            }

            memset(&schedule, 0, sizeof(schedule));
            generate_id(schedule.schedule_id, MAX_ID, "SC");
            strcpy(schedule.doctor_id, doctor_id);
            strcpy(schedule.work_date, work_date);
            strcpy(schedule.time_slot, time_slot);
            schedule.max_appt = 6;
            schedule.max_onsite = 8;
            strcpy(schedule.status, "正常");

            new_node = create_schedule_node(&schedule);
            if (!new_node) { free_schedule_list(head); return ERROR_FILE_IO; }

            if (!head) {
                head = new_node;
            } else {
                ScheduleNode *tail = head;
                while (tail->next) tail = tail->next;
                tail->next = new_node;
            }

            save_schedules_list(head);
            free_schedule_list(head);
            {
                char detail[60];
                snprintf(detail, sizeof(detail), "%s %s %s", doctor_id, work_date, time_slot);
                append_log(current_user->username, "create", "schedule", schedule.schedule_id, detail);
            }
            printf("排班创建成功! 编号: %s\n", schedule.schedule_id);
            return SUCCESS;

        case 3:
            printf("\n请输入医生编号: ");
            if (fgets(doctor_id, sizeof(doctor_id), stdin) == NULL) return ERROR_INVALID_INPUT;
            doctor_id[strcspn(doctor_id, "\n")] = 0;

            printf("请输入开始日期(如 2026-05-01): ");
            if (fgets(start_date, sizeof(start_date), stdin) == NULL) return ERROR_INVALID_INPUT;
            start_date[strcspn(start_date, "\n")] = 0;

            printf("请输入结束日期(如 2026-05-07): ");
            if (fgets(end_date, sizeof(end_date), stdin) == NULL) return ERROR_INVALID_INPUT;
            end_date[strcspn(end_date, "\n")] = 0;

            printf("选择时段 (1.上午 2.下午 3.全天): ");
            {
                int ts;
                if (scanf("%d", &ts) != 1) { clear_input_buffer(); return ERROR_INVALID_INPUT; }
                clear_input_buffer();
                if (ts == 1) strcpy(time_slot, "上午");
                else if (ts == 2) strcpy(time_slot, "下午");
                else strcpy(time_slot, "全天");
            }

            head = load_schedules_list();
            {
                // Parse dates and iterate
                int sy, sm, sd, ey, em, ed;
                int created = 0;
                if (sscanf(start_date, "%d-%d-%d", &sy, &sm, &sd) != 3 ||
                    sscanf(end_date, "%d-%d-%d", &ey, &em, &ed) != 3) {
                    ui_err("日期格式无效!");
                    free_schedule_list(head);
                    return ERROR_INVALID_INPUT;
                }

                // Iterate day by day
                int cy = sy, cm = sm, cd = sd;
                while (1) {
                    // Check if past end date
                    if (cy > ey || (cy == ey && cm > em) || (cy == ey && cm == em && cd > ed))
                        break;

                    char cur_date[12];
                    snprintf(cur_date, sizeof(cur_date), "%04d-%02d-%02d", cy, cm, cd);

                    // Check for duplicate
                    ScheduleNode *dup = head;
                    int is_dup = 0;
                    while (dup) {
                        if (strcmp(dup->data.doctor_id, doctor_id) == 0 &&
                            strcmp(dup->data.work_date, cur_date) == 0 &&
                            strcmp(dup->data.time_slot, time_slot) == 0) {
                            is_dup = 1;
                            break;
                        }
                        dup = dup->next;
                    }

                    if (!is_dup) {
                        memset(&schedule, 0, sizeof(schedule));
                        generate_id(schedule.schedule_id, MAX_ID, "SC");
                        strcpy(schedule.doctor_id, doctor_id);
                        strcpy(schedule.work_date, cur_date);
                        strcpy(schedule.time_slot, time_slot);
                        schedule.max_appt = 6;
                        schedule.max_onsite = 8;
                        strcpy(schedule.status, "正常");

                        new_node = create_schedule_node(&schedule);
                        if (new_node) {
                            if (!head) {
                                head = new_node;
                            } else {
                                ScheduleNode *tail = head;
                                while (tail->next) tail = tail->next;
                                tail->next = new_node;
                            }
                            created++;
                        }
                    }

                    // Advance one day
                    cd++;
                    int days_in_month[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
                    int leap = (cm == 2 && ((cy % 4 == 0 && cy % 100 != 0) || cy % 400 == 0));
                    int maxd = days_in_month[cm] + (leap ? 1 : 0);
                    if (cd > maxd) { cd = 1; cm++; }
                    if (cm > 12) { cm = 1; cy++; }
                }

                save_schedules_list(head);
                free_schedule_list(head);
                {
                    char detail[60];
                    snprintf(detail, sizeof(detail), "%s %s~%s %s %d条", doctor_id, start_date, end_date, time_slot, created);
                    append_log(current_user->username, "create", "schedule", "batch", detail);
                }
                printf("批量排班完成，共创建 %d 条排班记录。\n", created);
            }
            return SUCCESS;

        case 4: {
            head = load_schedules_list();
            int sc = count_schedule_list(head);
            if (sc == 0) { ui_warn("暂无排班数据。"); free_schedule_list(head); return SUCCESS; }

            const char **si = malloc(sc * sizeof(const char *));
            char (*sb)[100] = malloc(sc * sizeof(*sb));
            int si_idx = 0;
            ScheduleNode *scu = head;
            while (scu) {
                snprintf(sb[si_idx], 100, "%s - %s %s %s", scu->data.schedule_id,
                         scu->data.doctor_id, scu->data.work_date, scu->data.time_slot);
                si[si_idx] = sb[si_idx]; si_idx++; scu = scu->next;
            }

            int sel = ui_search_list("选择要修改的排班", si, sc);
            free((void*)si); free(sb);
            if (sel < 0) { free_schedule_list(head); return SUCCESS; }

            current = head;
            for (int j = 0; j < sel; j++) current = current->next;

            ui_sub_header("修改排班");
            printf(" 当前日期: %s\n", current->data.work_date);
            printf(" 当前时段: %s\n", current->data.time_slot);

            printf("新预约上限(当前%d, -1不改): ", current->data.max_appt);
            {
                int v;
                if (scanf("%d", &v) == 1 && v >= 0) current->data.max_appt = v;
                clear_input_buffer();
            }
            printf("新现场上限(当前%d, -1不改): ", current->data.max_onsite);
            {
                int v;
                if (scanf("%d", &v) == 1 && v >= 0) current->data.max_onsite = v;
                clear_input_buffer();
            }

            save_schedules_list(head);
            free_schedule_list(head);
            printf("排班修改成功!\n");
            return SUCCESS;
        }

        case 5: {
            head = load_schedules_list();
            int sc = count_schedule_list(head);
            if (sc == 0) { ui_warn("暂无排班数据。"); free_schedule_list(head); return SUCCESS; }

            const char **si = malloc(sc * sizeof(const char *));
            char (*sb)[100] = malloc(sc * sizeof(*sb));
            int si_idx = 0;
            ScheduleNode *scu = head;
            while (scu) {
                snprintf(sb[si_idx], 100, "%s - %s %s %s", scu->data.schedule_id,
                         scu->data.doctor_id, scu->data.work_date, scu->data.time_slot);
                si[si_idx] = sb[si_idx]; si_idx++; scu = scu->next;
            }

            int sel = ui_search_list("选择要删除的排班", si, sc);
            free((void*)si); free(sb);
            if (sel < 0) { free_schedule_list(head); return SUCCESS; }

            current = head;
            prev = NULL;
            for (int j = 0; j < sel; j++) { prev = current; current = current->next; }

            if (!ui_confirm("确认删除该排班?")) { free_schedule_list(head); return SUCCESS; }

            if (!prev) head = current->next;
            else prev->next = current->next;
            append_log(current_user->username, "delete", "schedule", current->data.schedule_id,
                       current->data.doctor_id);
            free(current);
            save_schedules_list(head);
            free_schedule_list(head);
            printf("排班已删除!\n");
            return SUCCESS;
        }

        case 6: {
            head = load_schedules_list();
            int sc = count_schedule_list(head);
            if (sc == 0) { ui_warn("暂无排班数据。"); free_schedule_list(head); return SUCCESS; }

            const char **si = malloc(sc * sizeof(const char *));
            char (*sb)[100] = malloc(sc * sizeof(*sb));
            int si_idx = 0;
            ScheduleNode *scu = head;
            while (scu) {
                snprintf(sb[si_idx], 100, "%s - %s %s %s [%s]", scu->data.schedule_id,
                         scu->data.doctor_id, scu->data.work_date, scu->data.time_slot,
                         scu->data.status);
                si[si_idx] = sb[si_idx]; si_idx++; scu = scu->next;
            }

            int sel = ui_search_list("选择要切换状态的排班", si, sc);
            free((void*)si); free(sb);
            if (sel < 0) { free_schedule_list(head); return SUCCESS; }

            current = head;
            for (int j = 0; j < sel; j++) current = current->next;

            if (strcmp(current->data.status, "正常") == 0) {
                if (ui_confirm("确认停诊该排班?")) {
                    strcpy(current->data.status, "停诊");
                } else {
                    free_schedule_list(head);
                    return SUCCESS;
                }
            } else {
                strcpy(current->data.status, "正常");
                printf("已恢复出诊。\n");
            }

            save_schedules_list(head);
            free_schedule_list(head);
            {
                char detail[60];
                snprintf(detail, sizeof(detail), "%s %s -> %s", current->data.schedule_id, current->data.doctor_id, current->data.status);
                append_log(current_user->username, "update", "schedule", current->data.schedule_id, detail);
            }
            printf("排班状态已更新!\n");
            return SUCCESS;
        }

        default:
            return ERROR_INVALID_INPUT;
    }
}

// ======================= 操作日志 =======================

int admin_log_menu(const User *current_user) {
    (void)current_user;
    LogEntryNode *head = load_logs_list();
    int count = count_log_entry_list(head);

    ui_header("操作日志");

    if (count == 0) {
        ui_warn("暂无日志记录。");
        free_log_entry_list(head);
        return SUCCESS;
    }

    const char **items = malloc(count * sizeof(const char *));
    char (*buf)[180] = malloc(count * sizeof(*buf));

    int i = 0;
    LogEntryNode *cur = head;
    while (cur) {
        snprintf(buf[i], 180, "%-18s %-12s %-8s %-10s %-8s %s",
                 cur->data.create_time,
                 cur->data.operator_name,
                 cur->data.action,
                 cur->data.target,
                 cur->data.target_id,
                 cur->data.detail);
        items[i] = buf[i];
        i++; cur = cur->next;
    }

    ui_search_list("操作日志(输入搜索, Esc返回)", items, count);

    free((void*)items);
    free(buf);
    free_log_entry_list(head);
    return SUCCESS;
}

// ======================= 数据管理 =======================

int admin_data_menu(const User *current_user) {
    (void)current_user;
    int choice;

    ui_header("数据管理");
    printf("1. 备份数据\n");
    printf("2. 还原数据\n");
    printf("0. 返回\n");

    choice = get_menu_choice(0, 2);
    switch (choice) {
        case 0:
            return SUCCESS;
        case 1:
            if (!ui_confirm("确认备份当前所有数据?")) return SUCCESS;
            backup_data();
            append_log(current_user->username, "create", "backup", "system", "数据备份");
            return SUCCESS;
        case 2: {
            const char **backups = NULL;
            int backup_count = 0;
            list_backups(&backups, &backup_count);

            if (backup_count == 0) {
                ui_warn("暂无备份数据。");
                return SUCCESS;
            }

            int sel = ui_search_list("选择要还原的备份", backups, backup_count);
            if (sel < 0) { free_backups_list(backups, backup_count); return SUCCESS; }

            if (!ui_confirm("还原将覆盖当前所有数据，确认继续?")) {
                free_backups_list(backups, backup_count);
                return SUCCESS;
            }

            restore_data(backups[sel]);
            append_log(current_user->username, "update", "restore", "system", backups[sel]);
            free_backups_list(backups, backup_count);
            return SUCCESS;
        }
        default:
            return ERROR_INVALID_INPUT;
    }
}
