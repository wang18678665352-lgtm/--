#include "admin.h"
#include "data_storage.h"
#include "public.h"
#include "ui_utils.h"

static void list_departments(void) {
    DepartmentNode *head = load_departments_list();
    DepartmentNode *current = head;

    ui_sub_header("科室列表");
    printf("  ");
    ui_print_col("科室编号", 10);
    ui_print_col("科室名称", 15);
    ui_print_col("负责人", 15);
    ui_print_col("联系电话", 15);
    printf("\n");
    ui_divider();
    while (current) {
        printf("  ");
        ui_print_col(current->data.department_id, 10);
        ui_print_col(current->data.name, 15);
        ui_print_col(current->data.leader, 15);
        ui_print_col(current->data.phone, 15);
        printf("\n");
        current = current->next;
    }

    if (!head) {
        ui_warn("暂无科室数据。");
    }

    free_department_list(head);
}

static void list_doctors(void) {
    DoctorNode *head = load_doctors_list();
    DoctorNode *current = head;

    ui_sub_header("医生列表");
    printf("  ");
    ui_print_col("医生编号", 10);
    ui_print_col("姓名", 12);
    ui_print_col("科室", 10);
    ui_print_col("职称", 14);
    ui_print_col("工作量", 6);
    printf("\n");
    ui_divider();
    while (current) {
        printf("  ");
        ui_print_col(current->data.doctor_id, 10);
        ui_print_col(current->data.name, 12);
        ui_print_col(current->data.department_id[0] ? current->data.department_id : "未分配", 10);
        ui_print_col(current->data.title, 14);
        ui_print_col_int(current->data.busy_level, 6);
        printf("\n");
        current = current->next;
    }

    if (!head) {
        ui_warn("暂无医生数据。");
    }

    free_doctor_list(head);
}

static void list_patients(void) {
    PatientNode *head = load_patients_list();
    PatientNode *current = head;

    ui_sub_header("患者列表");
    printf("  ");
    ui_print_col("患者编号", 10);
    ui_print_col("姓名", 10);
    ui_print_col("类型", 8);
    ui_print_col("阶段", 10);
    ui_print_col("电话", 16);
    ui_print_col("紧急", 6);
    printf("\n");
    ui_divider();
    while (current) {
        printf("  ");
        ui_print_col(current->data.patient_id, 10);
        ui_print_col(current->data.name, 10);
        ui_print_col(current->data.patient_type, 8);
        ui_print_col(current->data.treatment_stage, 10);
        ui_print_col(current->data.phone, 16);
        ui_print_col(current->data.is_emergency ? "是" : "否", 6);
        printf("\n");
        current = current->next;
    }

    if (!head) {
        ui_warn("暂无患者数据。");
    }

    free_patient_list(head);
}
static void list_drugs(void) {
    DrugNode *head = load_drugs_list();
    DrugNode *current = head;

    ui_sub_header("药品列表");
    printf("  ");
    ui_print_col("药品编号", 10);
    ui_print_col("名称", 16);
    ui_print_col("单价", 10);
    ui_print_col("库存", 8);
    ui_print_col("预警值", 8);
    ui_print_col("报销率", 10);
    printf("\n");
    ui_divider();
    while (current) {
        printf("  ");
        ui_print_col(current->data.drug_id, 10);
        ui_print_col(current->data.name, 16);
        ui_print_col_float(current->data.price, 10);
        ui_print_col_int(current->data.stock_num, 8);
        ui_print_col_int(current->data.warning_line, 8);
        ui_print_col_float(current->data.reimbursement_ratio, 10);
        printf("\n");
        current = current->next;
    }

    if (!head) {
        ui_warn("暂无药品数据。");
    }

    free_drug_list(head);
}

static void list_wards(void) {
    WardNode *head = load_wards_list();
    WardNode *current = head;

    ui_sub_header("病房列表");
    printf("  ");
    ui_print_col("病房编号", 10);
    ui_print_col("类型", 15);
    ui_print_col("总床位", 10);
    ui_print_col("剩余", 8);
    ui_print_col("预警值", 8);
    printf("\n");
    ui_divider();
    while (current) {
        printf("  ");
        ui_print_col(current->data.ward_id, 10);
        ui_print_col(current->data.type, 15);
        ui_print_col_int(current->data.total_beds, 10);
        ui_print_col_int(current->data.remain_beds, 8);
        ui_print_col_int(current->data.warning_line, 8);
        printf("\n");
        current = current->next;
    }

    if (!head) {
        ui_warn("暂无病房数据。");
    }

    free_ward_list(head);
}

void admin_main_menu(const User *current_user) {
    (void)current_user;
    ui_menu_item(1, "科室管理");
    ui_menu_item(2, "医生管理");
    ui_menu_item(3, "患者管理");
    ui_menu_item(4, "药物管理");
    ui_menu_item(5, "病房管理");
    ui_menu_item(6, "报表管理");
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
            printf("科室 %s 创建成功!\n", dept_id);
            return SUCCESS;
        case 3:
            list_departments();
            printf("\n请输入要修改的科室编号: ");
            if (fgets(dept_id, sizeof(dept_id), stdin) == NULL) return ERROR_INVALID_INPUT;
            dept_id[strcspn(dept_id, "\n")] = 0;

            head = load_departments_list();
            current = head;
            while (current) {
                if (strcmp(current->data.department_id, dept_id) == 0) break;
                current = current->next;
            }
            if (!current) {
                ui_err("科室不存在!");
                free_department_list(head);
                return ERROR_NOT_FOUND;
            }

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
                if (phone[0] != '\0') strcpy(current->data.phone, phone);
            }

            save_departments_list(head);
            free_department_list(head);
            printf("科室 %s 修改成功!\n", dept_id);
            return SUCCESS;
        case 4:
            list_departments();
            printf("\n请输入要删除的科室编号: ");
            if (fgets(dept_id, sizeof(dept_id), stdin) == NULL) return ERROR_INVALID_INPUT;
            dept_id[strcspn(dept_id, "\n")] = 0;

            head = load_departments_list();
            current = head;
            prev = NULL;
            while (current) {
                if (strcmp(current->data.department_id, dept_id) == 0) break;
                prev = current;
                current = current->next;
            }
            if (!current) {
                ui_err("科室不存在!");
                free_department_list(head);
                return ERROR_NOT_FOUND;
            }

            {
                DoctorNode *doc_head = load_doctors_list();
                DoctorNode *doc_current = doc_head;
                int has_doctor = 0;
                while (doc_current) {
                    if (strcmp(doc_current->data.department_id, dept_id) == 0) {
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
            free(current);
            save_departments_list(head);
            free_department_list(head);
            printf("科室 %s 已删除!\n", dept_id);
            return SUCCESS;
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
    printf("0. 返回\n");

    choice = get_menu_choice(0, 3);
    switch (choice) {
        case 0:
            return SUCCESS;
        case 1:
            list_doctors();
            return SUCCESS;
        case 2:
            list_doctors();
            printf("\n请输入要修改的医生编号: ");
            if (fgets(doc_id, sizeof(doc_id), stdin) == NULL) return ERROR_INVALID_INPUT;
            doc_id[strcspn(doc_id, "\n")] = 0;

            head = load_doctors_list();
            current = head;
            while (current) {
                if (strcmp(current->data.doctor_id, doc_id) == 0) break;
                current = current->next;
            }
            if (!current) {
                ui_err("医生不存在!");
                free_doctor_list(head);
                return ERROR_NOT_FOUND;
            }

            printf("当前姓名: %s, 输入新姓名(直接回车不改): ", current->data.name);
            if (fgets(input, sizeof(input), stdin)) {
                input[strcspn(input, "\n")] = 0;
                if (input[0] != '\0') strcpy(current->data.name, input);
            }

            printf("当前职称: %s, 输入新职称(直接回车不改): ", current->data.title);
            if (fgets(input, sizeof(input), stdin)) {
                input[strcspn(input, "\n")] = 0;
                if (input[0] != '\0') strcpy(current->data.title, input);
            }

            printf("当前科室: %s, 输入新科室编号(直接回车不改): ",
                   current->data.department_id[0] ? current->data.department_id : "未分配");
            if (fgets(input, sizeof(input), stdin)) {
                input[strcspn(input, "\n")] = 0;
                if (input[0] != '\0') {
                    DepartmentNode *dept_head = load_departments_list();
                    DepartmentNode *dept_current = dept_head;
                    int dept_found = 0;
                    while (dept_current) {
                        if (strcmp(dept_current->data.department_id, input) == 0) {
                            dept_found = 1;
                            break;
                        }
                        dept_current = dept_current->next;
                    }
                    free_department_list(dept_head);
                    if (!dept_found) {
                        printf("科室编号 %s 不存在! 保持原科室不变。\n", input);
                    } else {
                        char old_id[MAX_ID];
                        char new_id[MAX_ID];
                        strcpy(old_id, current->data.doctor_id);
                        strcpy(current->data.department_id, input);
                        generate_doctor_id(input, new_id, MAX_ID);
                        if (strcmp(old_id, new_id) != 0) {
                            strcpy(current->data.doctor_id, new_id);
                            update_doctor_id_across_files(old_id, new_id);
                        }
                    }
                }
            }

            save_doctors_list(head);
            free_doctor_list(head);
            printf("医生信息修改成功!\n");
            return SUCCESS;
        case 3:
            list_doctors();
            printf("\n请输入要删除的医生编号: ");
            if (fgets(doc_id, sizeof(doc_id), stdin) == NULL) return ERROR_INVALID_INPUT;
            doc_id[strcspn(doc_id, "\n")] = 0;

            head = load_doctors_list();
            current = head;
            prev = NULL;
            while (current) {
                if (strcmp(current->data.doctor_id, doc_id) == 0) break;
                prev = current;
                current = current->next;
            }
            if (!current) {
                ui_err("医生不存在!");
                free_doctor_list(head);
                return ERROR_NOT_FOUND;
            }

            // Cascade check: verify no active appointments, records, or prescriptions
            {
                int has_active = 0;
                AppointmentNode *apt_head = load_appointments_list();
                AppointmentNode *apt_cur = apt_head;
                while (apt_cur && !has_active) {
                    if (strcmp(apt_cur->data.doctor_id, doc_id) == 0 &&
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
                        if (strcmp(rec_cur->data.doctor_id, doc_id) == 0) {
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
                        if (strcmp(pre_cur->data.doctor_id, doc_id) == 0) {
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
            free(current);
            save_doctors_list(head);
            free_doctor_list(head);
            printf("医生 %s 已删除!\n", doc_id);
            return SUCCESS;
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
    printf("0. 返回\n");

    choice = get_menu_choice(0, 3);
    switch (choice) {
        case 0:
            return SUCCESS;
        case 1:
            list_patients();
            return SUCCESS;
        case 2:
            list_patients();
            printf("\n请输入要修改的患者编号: ");
            if (fgets(patient_id, sizeof(patient_id), stdin) == NULL) return ERROR_INVALID_INPUT;
            patient_id[strcspn(patient_id, "\n")] = 0;

            head = load_patients_list();
            current = head;
            while (current) {
                if (strcmp(current->data.patient_id, patient_id) == 0) break;
                current = current->next;
            }
            if (!current) {
                ui_err("患者不存在!");
                free_patient_list(head);
                return ERROR_NOT_FOUND;
            }

            printf("当前姓名: %s, 输入新姓名(直接回车不改): ", current->data.name);
            if (fgets(input, sizeof(input), stdin)) {
                input[strcspn(input, "\n")] = 0;
                if (input[0] != '\0') strcpy(current->data.name, input);
            }

            printf("当前性别: %s, 输入新性别(男/女, 直接回车不改): ", current->data.gender);
            if (fgets(input, sizeof(input), stdin)) {
                input[strcspn(input, "\n")] = 0;
                if (input[0] != '\0') strcpy(current->data.gender, input);
            }

            printf("当前年龄: %d, 输入新年龄(-1不改): ", current->data.age);
            if (scanf("%d", &int_input) == 1 && int_input >= 0) {
                current->data.age = int_input;
            }
            clear_input_buffer();

            printf("当前电话: %s, 输入新电话(直接回车不改): ", current->data.phone);
            if (fgets(input, sizeof(input), stdin)) {
                input[strcspn(input, "\n")] = 0;
                if (input[0] != '\0') strcpy(current->data.phone, input);
            }

            printf("当前患者类型(%s), 输入新类型(普通/医保/军人, 直接回车不改): ", current->data.patient_type);
            if (fgets(input, sizeof(input), stdin)) {
                input[strcspn(input, "\n")] = 0;
                if (input[0] != '\0') {
                    if (strcmp(input, "普通") != 0 && strcmp(input, "医保") != 0 && strcmp(input, "军人") != 0) {
                        ui_err("无效的患者类型! 有效值: 普通/医保/军人");
                        free_patient_list(head);
                        return ERROR_INVALID_INPUT;
                    }
                    strcpy(current->data.patient_type, input);
                }
            }

            save_patients_list(head);
            free_patient_list(head);
            printf("患者信息修改成功!\n");
            return SUCCESS;
        case 3:
            list_patients();
            printf("\n请输入要删除的患者编号: ");
            if (fgets(patient_id, sizeof(patient_id), stdin) == NULL) return ERROR_INVALID_INPUT;
            patient_id[strcspn(patient_id, "\n")] = 0;

            head = load_patients_list();
            current = head;
            prev = NULL;
            while (current) {
                if (strcmp(current->data.patient_id, patient_id) == 0) break;
                prev = current;
                current = current->next;
            }
            if (!current) {
                ui_err("患者不存在!");
                free_patient_list(head);
                return ERROR_NOT_FOUND;
            }

            // Cascade check: verify no active appointment registrations
            {
                int has_appointment = 0;
                AppointmentNode *apt_head = load_appointments_list();
                AppointmentNode *apt_cur = apt_head;
                while (apt_cur && !has_appointment) {
                    if (strcmp(apt_cur->data.patient_id, patient_id) == 0 &&
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
                        if (strcmp(oc->data.patient_id, patient_id) == 0 &&
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
            free(current);
            save_patients_list(head);
            free_patient_list(head);
            printf("患者 %s 已删除!\n", patient_id);
            return SUCCESS;
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
    printf("0. 返回\n");

    choice = get_menu_choice(0, 6);
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
            printf("药品 %s 创建成功!\n", drug_id);
            return SUCCESS;
        case 3:
            list_drugs();
            printf("\n请输入要修改的药品编号: ");
            if (fgets(drug_id, sizeof(drug_id), stdin) == NULL) return ERROR_INVALID_INPUT;
            drug_id[strcspn(drug_id, "\n")] = 0;

            head = load_drugs_list();
            current = head;
            while (current) {
                if (strcmp(current->data.drug_id, drug_id) == 0) break;
                current = current->next;
            }
            if (!current) {
                ui_err("药品不存在!");
                free_drug_list(head);
                return ERROR_NOT_FOUND;
            }

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

            save_drugs_list(head);
            free_drug_list(head);
            printf("药品信息修改成功!\n");
            return SUCCESS;
        case 4:
            list_drugs();
            printf("\n请输入要补货的药品编号: ");
            if (fgets(drug_id, sizeof(drug_id), stdin) == NULL) return ERROR_INVALID_INPUT;
            drug_id[strcspn(drug_id, "\n")] = 0;

            printf("请输入补货数量: ");
            if (scanf("%d", &add_stock) != 1) {
                clear_input_buffer();
                return ERROR_INVALID_INPUT;
            }
            clear_input_buffer();
            if (add_stock <= 0) return ERROR_INVALID_INPUT;

            head = load_drugs_list();
            current = head;
            while (current) {
                if (strcmp(current->data.drug_id, drug_id) == 0) {
                    current->data.stock_num += add_stock;
                    save_drugs_list(head);
                    printf("补货成功，当前库存: %d\n", current->data.stock_num);
                    free_drug_list(head);
                    return SUCCESS;
                }
                current = current->next;
            }
            free_drug_list(head);
            return ERROR_NOT_FOUND;
        case 5:
            list_drugs();
            printf("\n请输入要删除的药品编号: ");
            if (fgets(drug_id, sizeof(drug_id), stdin) == NULL) return ERROR_INVALID_INPUT;
            drug_id[strcspn(drug_id, "\n")] = 0;

            head = load_drugs_list();
            current = head;
            prev = NULL;
            while (current) {
                if (strcmp(current->data.drug_id, drug_id) == 0) break;
                prev = current;
                current = current->next;
            }
            if (!current) {
                ui_err("药品不存在!");
                free_drug_list(head);
                return ERROR_NOT_FOUND;
            }

            if (!ui_confirm("确认删除该药品?")) {
                free_drug_list(head);
                return SUCCESS;
            }

            if (!prev) {
                head = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            save_drugs_list(head);
            free_drug_list(head);
            printf("药品 %s 已删除!\n", drug_id);
            return SUCCESS;
        case 6:
            check_drug_warning();
            return SUCCESS;
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
            printf("病房 %s 创建成功!\n", ward_id);
            return SUCCESS;
        case 3:
            list_wards();
            printf("\n请输入要修改的病房编号: ");
            if (fgets(ward_id, sizeof(ward_id), stdin) == NULL) return ERROR_INVALID_INPUT;
            ward_id[strcspn(ward_id, "\n")] = 0;

            ward_head = load_wards_list();
            current_ward = ward_head;
            while (current_ward) {
                if (strcmp(current_ward->data.ward_id, ward_id) == 0) break;
                current_ward = current_ward->next;
            }
            if (!current_ward) {
                ui_err("病房不存在!");
                free_ward_list(ward_head);
                return ERROR_NOT_FOUND;
            }

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
            printf("病房信息修改成功!\n");
            return SUCCESS;
        case 4:
            list_wards();
            printf("\n请输入要删除的病房编号: ");
            if (fgets(ward_id, sizeof(ward_id), stdin) == NULL) return ERROR_INVALID_INPUT;
            ward_id[strcspn(ward_id, "\n")] = 0;

            ward_head = load_wards_list();
            current_ward = ward_head;
            prev = NULL;
            while (current_ward) {
                if (strcmp(current_ward->data.ward_id, ward_id) == 0) break;
                prev = current_ward;
                current_ward = current_ward->next;
            }
            if (!current_ward) {
                ui_err("病房不存在!");
                free_ward_list(ward_head);
                return ERROR_NOT_FOUND;
            }

            if (!ui_confirm("确认删除该病房?")) {
                free_ward_list(ward_head);
                return SUCCESS;
            }

            if (!prev) {
                ward_head = current_ward->next;
            } else {
                prev->next = current_ward->next;
            }
            free(current_ward);
            save_wards_list(ward_head);
            free_ward_list(ward_head);
            printf("病房 %s 已删除!\n", ward_id);
            return SUCCESS;
        case 5:
            list_wards();
            printf("\n请输入病房编号: ");
            if (fgets(ward_id, sizeof(ward_id), stdin) == NULL) return ERROR_INVALID_INPUT;
            ward_id[strcspn(ward_id, "\n")] = 0;
            printf("请输入新的剩余床位数: ");
            if (scanf("%d", &remain_beds) != 1) {
                clear_input_buffer();
                return ERROR_INVALID_INPUT;
            }
            clear_input_buffer();

            ward_head = load_wards_list();
            current_ward = ward_head;
            while (current_ward) {
                if (strcmp(current_ward->data.ward_id, ward_id) == 0) {
                    if (remain_beds < 0 || remain_beds > current_ward->data.total_beds) {
                        free_ward_list(ward_head);
                        return ERROR_INVALID_INPUT;
                    }
                    current_ward->data.remain_beds = remain_beds;
                    save_wards_list(ward_head);
                    printf("病房床位更新成功。\n");
                    free_ward_list(ward_head);
                    return SUCCESS;
                }
                current_ward = current_ward->next;
            }
            free_ward_list(ward_head);
            return ERROR_NOT_FOUND;
        case 6: {
            WardCallNode *call_head = load_ward_calls_list();
            WardCallNode *tail = call_head;
            WardCall call;

            list_wards();
            printf("\n请输入病房编号: ");
            if (fgets(ward_id, sizeof(ward_id), stdin) == NULL) {
                free_ward_call_list(call_head);
                return ERROR_INVALID_INPUT;
            }
            ward_id[strcspn(ward_id, "\n")] = 0;

            printf("请输入患者编号: ");
            if (fgets(patient_id, sizeof(patient_id), stdin) == NULL) {
                free_ward_call_list(call_head);
                return ERROR_INVALID_INPUT;
            }
            patient_id[strcspn(patient_id, "\n")] = 0;

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

int admin_report_menu(const User *current_user) {
    AppointmentNode *appointment_head = load_appointments_list();
    MedicalRecordNode *record_head = load_medical_records_list();
    PrescriptionNode *prescription_head = load_prescriptions_list();
    WardCallNode *call_head = load_ward_calls_list();
    OnsiteRegistrationQueue onsite_queue = load_onsite_registration_queue();
    float total_prescription_amount = 0.0f;
    float total_reimbursement = 0.0f;
    PrescriptionNode *current_prescription = prescription_head;

    (void)current_user;

    while (current_prescription) {
        total_prescription_amount += current_prescription->data.total_price;
        Drug *drug = find_drug_by_id(current_prescription->data.drug_id);
        if (drug) {
            Patient *patient = find_patient_by_id(current_prescription->data.patient_id);
            if (patient) {
                float reimb = calculate_drug_reimbursement(drug, current_prescription->data.quantity, patient->patient_type);
                total_reimbursement += reimb;
                free(patient);
            }
            free(drug);
        }
        current_prescription = current_prescription->next;
    }

    ui_header("报表管理");
    printf("预约挂号总量: %d\n", count_appointment_list(appointment_head));
    printf("现场挂号排队人数: %d\n", onsite_queue.size);
    printf("诊疗记录数: %d\n", count_medical_record_list(record_head));
    printf("处方记录数: %d\n", count_prescription_list(prescription_head));
    printf("病房呼叫数: %d\n", count_ward_call_list(call_head));
    printf("处方总金额: %.2f\n", total_prescription_amount);
    printf("报销总金额: %.2f\n", total_reimbursement);
    printf("实际收入: %.2f\n", total_prescription_amount - total_reimbursement);

    free_appointment_list(appointment_head);
    free_medical_record_list(record_head);
    free_prescription_list(prescription_head);
    free_ward_call_list(call_head);
    free_onsite_registration_queue(&onsite_queue);
    return SUCCESS;
}
