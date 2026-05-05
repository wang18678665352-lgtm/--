/*
 * ============================================================================
 * admin.c — 管理员全实体CRUD模块 (Administrator All-Entity CRUD Module)
 * ============================================================================
 *
 * 【中文说明】
 * 本文件实现了医院管理系统(C99控制台模式)中管理员角色的全部管理功能。
 * 管理员登录后可对以下所有实体进行增删改查操作:
 *   1. 科室管理   — 查看/新增/修改/删除科室(删除前检查下属医生)
 *   2. 医生管理   — 查看/修改/删除医生(删除前检查活跃关联,修改科室时级联更新ID)
 *   3. 患者管理   — 查看/修改/删除患者(删除前检查活跃挂号)
 *   4. 药物管理   — 查看/新增/修改/补货/删除/库存预警(含报销比例设置)
 *   5. 病房管理   — 查看/新增/修改/删除/调整床位/发起呼叫/床位预警
 *   6. 排班管理   — 查看/新增排班/停诊
 *   7. 分析报表   — 各实体统计(预约/现场/病历/处方/呼叫总量,处方金额/报销/收入)
 *   8. 操作日志   — 查看系统操作日志
 *   9. 数据管理   — 备份/恢复/查看备份列表
 *  10. 修改密码
 *
 * 关键安全约束:
 *   - 删除科室前检查是否还有下属医生(有则拒绝)
 *   - 删除医生前级联检查活跃的预约/病历/处方记录
 *   - 删除患者前检查活跃的预约/现场挂号记录
 *   - 医生修改科室时自动重新生成医生ID(前缀变为新科室缩写)并同步所有文件
 *   - 所有药品补货和库存预警均有阈值保护
 *   - 备份/恢复操作记录到操作日志中
 *
 * 【English】
 * This file implements all management functions for the administrator role
 * in a C99 console-mode hospital management system.
 * After login, an admin can CRUD all entities:
 *   1. Department CRUD  — View/add/edit/delete (check subordinate doctors before delete)
 *   2. Doctor CRUD      — View/edit/delete (cascade check active relations, auto-update ID on dept change)
 *   3. Patient CRUD     — View/edit/delete (check active registrations before delete)
 *   4. Drug CRUD        — View/add/edit/restock/delete/stock warning (with reimbursement ratio)
 *   5. Ward CRUD        — View/add/edit/delete/adjust beds/initiate call/bed warning
 *   6. Schedule CRUD    — View/add schedule/cancel (停诊)
 *   7. Reports          — Entity statistics (appointments/onsite/records/prescriptions/calls, totals/reimbursement/revenue)
 *   8. Operation Logs   — View system operation logs
 *   9. Data Management  — Backup/restore/list backups
 *  10. Change Password
 *
 * Key safety constraints:
 *   - Department deletion blocked if subordinate doctors exist
 *   - Doctor deletion cascade-checks active appointments/records/prescriptions
 *   - Patient deletion checks active appointments/onsite registrations
 *   - When a doctor's department changes, the doctor ID is auto-regenerated (new dept prefix)
 *     and synchronized across all files
 *   - Drug restock and stock warnings have threshold protection
 *   - Backup/restore operations are logged
 * ============================================================================
 */

#include "admin.h"
#include "data_storage.h"
#include "public.h"
#include "ui_utils.h"

/* ============================================================================
 * 列表展示函数 (List Display Functions)
 * ============================================================================
 * 分页展示各实体的所有记录,供管理员浏览和选择操作目标。
 * Paginated display of all records for each entity, for admin browsing and selection.
 * ============================================================================
 */

/**
 * list_departments — 分页展示所有科室信息
 *                    Paginated display of all departments
 */
static void list_departments(void) {
    DepartmentNode *head = load_departments_list();
    DepartmentNode *current = head;

    ui_sub_header("科室列表");
    printf("  ");
    ui_print_col("科室编号", 10);    /* Department ID */
    ui_print_col("科室名称", 15);    /* Department Name */
    ui_print_col("负责人", 15);      /* Leader */
    ui_print_col("联系电话", 15);    /* Phone */
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

/**
 * list_doctors — 分页展示所有医生信息
 *                Paginated display of all doctors
 */
static void list_doctors(void) {
    DoctorNode *head = load_doctors_list();
    DoctorNode *current = head;

    ui_sub_header("医生列表");
    printf("  ");
    ui_print_col("医生编号", 10);    /* Doctor ID */
    ui_print_col("姓名", 12);        /* Name */
    ui_print_col("科室", 10);        /* Department */
    ui_print_col("职称", 14);        /* Title */
    ui_print_col("工作量", 6);       /* Busy Level */
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

/**
 * list_patients — 分页展示所有患者信息
 *                 Paginated display of all patients
 */
static void list_patients(void) {
    PatientNode *head = load_patients_list();
    PatientNode *current = head;

    ui_sub_header("患者列表");
    printf("  ");
    ui_print_col("患者编号", 10);    /* Patient ID */
    ui_print_col("姓名", 10);        /* Name */
    ui_print_col("类型", 8);         /* Type (普通/医保/军人) */
    ui_print_col("阶段", 10);        /* Treatment Stage */
    ui_print_col("电话", 16);        /* Phone */
    ui_print_col("紧急", 6);         /* Emergency */
    printf("\n");
    ui_divider();
    while (current) {
        printf("  ");
        ui_print_col(current->data.patient_id, 10);
        ui_print_col(current->data.name, 10);
        ui_print_col(current->data.patient_type, 8);
        ui_print_col(current->data.treatment_stage, 10);
        ui_print_col(current->data.phone, 16);
        ui_print_col(current->data.is_emergency ? "是" : "否", 6);  /* 急诊标记 Emergency flag */
        printf("\n");
        current = current->next;
    }

    if (!head) {
        ui_warn("暂无患者数据。");
    }

    free_patient_list(head);
}

/**
 * list_drugs — 分页展示所有药品信息
 *              Paginated display of all drugs
 */
static void list_drugs(void) {
    DrugNode *head = load_drugs_list();
    DrugNode *current = head;

    ui_sub_header("药品列表");
    printf("  ");
    ui_print_col("药品编号", 10);    /* Drug ID */
    ui_print_col("名称", 16);        /* Name */
    ui_print_col("单价", 10);        /* Unit Price */
    ui_print_col("库存", 8);         /* Stock */
    ui_print_col("预警值", 8);       /* Warning Line */
    ui_print_col("报销率", 10);      /* Reimbursement Ratio */
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

/**
 * list_wards — 分页展示所有病房信息
 *              Paginated display of all wards
 */
static void list_wards(void) {
    WardNode *head = load_wards_list();
    WardNode *current = head;

    ui_sub_header("病房列表");
    printf("  ");
    ui_print_col("病房编号", 10);    /* Ward ID */
    ui_print_col("类型", 15);        /* Type */
    ui_print_col("总床位", 10);      /* Total Beds */
    ui_print_col("剩余", 8);         /* Remaining */
    ui_print_col("预警值", 8);       /* Warning Line */
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

/* ============================================================================
 * 管理员主菜单 (Admin Main Menu)
 * ============================================================================
 * 定义管理员角色的10大功能入口。
 * Defines the 10 function entry points for the administrator role.
 * ============================================================================
 */

void admin_main_menu(const User *current_user) {
    (void)current_user;
    ui_menu_item(1, "科室管理");       /* Department Management */
    ui_menu_item(2, "医生管理");       /* Doctor Management */
    ui_menu_item(3, "患者管理");       /* Patient Management */
    ui_menu_item(4, "药物管理");       /* Drug Management */
    ui_menu_item(5, "病房管理");       /* Ward Management */
    ui_menu_item(6, "排班管理");       /* Schedule Management */
    ui_menu_item(7, "分析报表");       /* Reports */
    ui_menu_item(8, "操作日志");       /* Operation Logs */
    ui_menu_item(9, "数据管理");       /* Data Management */
    ui_menu_item(10, "修改密码");      /* Change Password */
}

/* ============================================================================
 * 功能1: 科室管理 (Department CRUD)
 * ============================================================================
 * 查看/新增/修改/删除科室。
 * 删除前检查冲突:如果该科室下还有医生,则拒绝删除。
 * View / add / edit / delete departments.
 * Conflict check before delete: reject if subordinate doctors exist.
 * ============================================================================
 */

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
    printf("1. 查看所有科室\n");    /* View all departments */
    printf("2. 新增科室\n");        /* Add department */
    printf("3. 修改科室\n");        /* Edit department */
    printf("4. 删除科室\n");        /* Delete department */
    printf("0. 返回\n");

    choice = get_menu_choice(0, 4);
    switch (choice) {
        case 0:
            return SUCCESS;
        case 1:
            list_departments();
            return SUCCESS;
        case 2:
            /* 新增科室 Add new department */
            printf("\n请输入科室编号(如DEP004): ");
            if (fgets(dept_id, sizeof(dept_id), stdin) == NULL) return ERROR_INVALID_INPUT;
            dept_id[strcspn(dept_id, "\n")] = 0;

            /* 检查科室编号唯一性 Check department ID uniqueness */
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

            printf("请输入负责人: ");
            if (fgets(leader, sizeof(leader), stdin) == NULL) {
                free_department_list(head);
                return ERROR_INVALID_INPUT;
            }
            leader[strcspn(leader, "\n")] = 0;

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

            /* 创建节点并追加到链表尾部 Create node and append to tail */
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
            /* 修改科室 Edit department */
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

            /* 逐一编辑字段(直接回车保持不变) Edit each field (press Enter to keep current) */
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
            /* 删除科室 Delete department */
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

            /* 级联检查:科室下是否有医生,有则禁止删除
               Cascade check: reject if any doctor belongs to this department */
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
                    printf("该科室下还有医生，无法删除! 请先将医生调离该科室。\n");  /* Has subordinate doctors, can't delete */
                    free_department_list(head);
                    return ERROR_PERMISSION_DENIED;
                }
            }

            /* 从链表中删除节点 Remove node from linked list */
            if (!prev) {
                head = current->next;       /* 删除头节点 Delete head */
            } else {
                prev->next = current->next; /* 删除中间/尾节点 Delete middle/tail */
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

/* ============================================================================
 * 功能2: 医生管理 (Doctor CRUD)
 * ============================================================================
 * 查看/修改/删除医生。
 * 修改医生科室时:验证新科室存在,自动重新生成医生ID(新科室前缀),级联更新所有文件。
 * 删除医生前:级联检查活跃的预约/病历/处方记录,有则禁止删除。
 *
 * View / edit / delete doctors.
 * When changing doctor's department: validate new department exists, auto-regenerate
 * doctor ID (new dept prefix), cascade-update across all files.
 * Before delete: cascade-check active appointments/records/prescriptions; block if any exist.
 * ============================================================================
 */

int admin_doctor_menu(const User *current_user) {
    (void)current_user;
    DoctorNode *head = NULL;
    DoctorNode *current = NULL;
    DoctorNode *prev = NULL;
    char doc_id[MAX_ID];
    char input[MAX_BUFFER];
    int choice;

    ui_header("医生管理");
    printf("1. 查看所有医生\n");    /* View all doctors */
    printf("2. 修改医生信息\n");    /* Edit doctor info */
    printf("3. 删除医生\n");        /* Delete doctor */
    printf("0. 返回\n");

    choice = get_menu_choice(0, 3);
    switch (choice) {
        case 0:
            return SUCCESS;
        case 1:
            list_doctors();
            return SUCCESS;
        case 2:
            /* 修改医生信息 Edit doctor info */
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

            /* 编辑姓名 Edit name */
            printf("当前姓名: %s, 输入新姓名(直接回车不改): ", current->data.name);
            if (fgets(input, sizeof(input), stdin)) {
                input[strcspn(input, "\n")] = 0;
                if (input[0] != '\0') strcpy(current->data.name, input);
            }

            /* 编辑职称 Edit title */
            printf("当前职称: %s, 输入新职称(直接回车不改): ", current->data.title);
            if (fgets(input, sizeof(input), stdin)) {
                input[strcspn(input, "\n")] = 0;
                if (input[0] != '\0') strcpy(current->data.title, input);
            }

            /* 编辑科室 — 如果变更,自动重新生成医生ID并级联更新所有文件
               Edit department — if changed, auto-regenerate doctor ID and cascade-update all files */
            printf("当前科室: %s, 输入新科室编号(直接回车不改): ",
                   current->data.department_id[0] ? current->data.department_id : "未分配");
            if (fgets(input, sizeof(input), stdin)) {
                input[strcspn(input, "\n")] = 0;
                if (input[0] != '\0') {
                    /* 验证新科室是否存在 Validate that the new department exists */
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
                        printf("科室编号 %s 不存在! 保持原科室不变。\n", input);  /* Dept not found, keep original */
                    } else {
                        char old_id[MAX_ID];
                        char new_id[MAX_ID];
                        strcpy(old_id, current->data.doctor_id);
                        strcpy(current->data.department_id, input);
                        /* 基于新科室生成新医生ID Generate new doctor ID based on new department */
                        generate_doctor_id(input, new_id, MAX_ID);
                        if (strcmp(old_id, new_id) != 0) {
                            strcpy(current->data.doctor_id, new_id);
                            /* 级联更新所有关联文件中的医生ID
                               Cascade-update doctor ID across all related files */
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
            /* 删除医生 Delete doctor */
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

            /* 级联检查:验证是否有关联的活跃记录(预约/病历/处方)
               Cascade check: verify no active related records (appointments / medical records / prescriptions) */
            {
                int has_active = 0;
                /* 检查活跃预约(排除已取消/已完成/已就诊) Check active appointments */
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
                /* 检查病历记录 Check medical records */
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
                /* 检查处方记录 Check prescriptions */
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
                    ui_err("该医生下有活跃的挂号/病历/处方记录，无法删除!");  /* Has active records, can't delete */
                    free_doctor_list(head);
                    return ERROR_PERMISSION_DENIED;
                }
            }

            /* 从链表中删除节点 Remove node from linked list */
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

/* ============================================================================
 * 功能3: 患者管理 (Patient CRUD)
 * ============================================================================
 * 查看/修改/删除患者。
 * 删除前检查活跃的预约和现场挂号记录。
 * View / edit / delete patients.
 * Check active appointments and onsite registrations before delete.
 * ============================================================================
 */

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
    printf("1. 查看所有患者\n");    /* View all patients */
    printf("2. 修改患者信息\n");    /* Edit patient info */
    printf("3. 删除患者\n");        /* Delete patient */
    printf("0. 返回\n");

    choice = get_menu_choice(0, 3);
    switch (choice) {
        case 0:
            return SUCCESS;
        case 1:
            list_patients();
            return SUCCESS;
        case 2:
            /* 修改患者信息 Edit patient info */
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

            /* 逐一编辑字段 Edit each field */
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

            /* 患者类型校验 Patient type validation */
            printf("当前患者类型(%s), 输入新类型(普通/医保/军人, 直接回车不改): ", current->data.patient_type);
            if (fgets(input, sizeof(input), stdin)) {
                input[strcspn(input, "\n")] = 0;
                if (input[0] != '\0') strcpy(current->data.patient_type, input);
            }

            save_patients_list(head);
            free_patient_list(head);
            printf("患者信息修改成功!\n");
            return SUCCESS;
        case 3:
            /* 删除患者 Delete patient */
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

            /* 级联检查:是否有活跃的预约/现场挂号记录
               Cascade check: verify no active appointments or onsite registrations */
            {
                int has_appointment = 0;
                /* 检查活跃的预约记录 Check active appointments */
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
                /* 检查活跃的现场挂号记录 Check active onsite registrations */
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
                    ui_err("该患者下有活跃的挂号记录，无法删除!");  /* Has active registrations, can't delete */
                    free_patient_list(head);
                    return ERROR_PERMISSION_DENIED;
                }
            }

            /* 从链表中删除节点 Remove node from linked list */
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

/* ============================================================================
 * 功能4: 药物管理 (Drug CRUD + Restock + Stock Warning)
 * ============================================================================
 * 查看/新增/修改/补货/删除药品,含库存预警功能。
 * 药品属性:编号/名称/单价/库存/预警值/是否特效药/报销比例(0-1)。
 *
 * View / add / edit / restock / delete drugs, with stock warning check.
 * Drug attributes: ID / name / unit price / stock / warning line / special / reimbursement ratio (0-1).
 * ============================================================================
 */

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
    printf("1. 查看药品\n");        /* View drugs */
    printf("2. 新增药品\n");        /* Add drug */
    printf("3. 修改药品\n");        /* Edit drug */
    printf("4. 药品补货\n");        /* Restock */
    printf("5. 删除药品\n");        /* Delete drug */
    printf("6. 库存预警\n");        /* Stock warning */
    printf("0. 返回\n");

    choice = get_menu_choice(0, 6);
    switch (choice) {
        case 0:
            return SUCCESS;
        case 1:
            list_drugs();
            return SUCCESS;
        case 2:
            /* 新增药品 Add new drug */
            printf("\n请输入药品编号: ");
            if (fgets(drug_id, sizeof(drug_id), stdin) == NULL) return ERROR_INVALID_INPUT;
            drug_id[strcspn(drug_id, "\n")] = 0;

            /* 检查药品编号唯一性 Check drug ID uniqueness */
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

            /* 输入单价 Enter unit price */
            printf("请输入单价: ");
            if (scanf("%f", &drug.price) != 1 || drug.price < 0) {
                clear_input_buffer();
                free_drug_list(head);
                return ERROR_INVALID_INPUT;
            }
            clear_input_buffer();

            /* 输入库存数量 Enter stock quantity */
            printf("请输入库存数量: ");
            if (scanf("%d", &drug.stock_num) != 1 || drug.stock_num < 0) {
                clear_input_buffer();
                free_drug_list(head);
                return ERROR_INVALID_INPUT;
            }
            clear_input_buffer();

            /* 输入预警阈值 Enter warning threshold */
            printf("请输入预警阈值: ");
            if (scanf("%d", &drug.warning_line) != 1 || drug.warning_line < 0) {
                clear_input_buffer();
                free_drug_list(head);
                return ERROR_INVALID_INPUT;
            }
            clear_input_buffer();

            /* 输入是否特效药 Enter special drug flag */
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

            /* 输入报销比例(0-1) Enter reimbursement ratio (0-1) */
            printf("请输入报销比例(0-1): ");
            if (scanf("%f", &drug.reimbursement_ratio) != 1 || drug.reimbursement_ratio < 0 || drug.reimbursement_ratio > 1) {
                clear_input_buffer();
                free_drug_list(head);
                return ERROR_INVALID_INPUT;
            }
            clear_input_buffer();

            /* 创建节点并追加到链表尾部 Create node and append to tail */
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
            /* 修改药品 Edit drug */
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

            /* 逐一编辑字段 Edit each field */
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
                    if (new_ratio > 1) new_ratio = 1;  /* 报销率上限为1 Cap reimbursement ratio at 1 */
                    current->data.reimbursement_ratio = new_ratio;
                }
                clear_input_buffer();
            }

            save_drugs_list(head);
            free_drug_list(head);
            printf("药品信息修改成功!\n");
            return SUCCESS;
        case 4:
            /* 药品补货 Restock drug */
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
            if (add_stock <= 0) return ERROR_INVALID_INPUT;  /* 补货数量必须为正 Restock amount must be positive */

            head = load_drugs_list();
            current = head;
            while (current) {
                if (strcmp(current->data.drug_id, drug_id) == 0) {
                    current->data.stock_num += add_stock;  /* 库存增加 Increase stock */
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
            /* 删除药品 Delete drug */
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

            /* 从链表中删除节点 Remove node from linked list */
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
            /* 库存预警检查 Stock warning check */
            check_drug_warning();
            return SUCCESS;
        default:
            return ERROR_INVALID_INPUT;
    }
}

/* ============================================================================
 * 功能5: 病房管理 (Ward CRUD + Bed Adjustment + Call + Warning)
 * ============================================================================
 * 查看/新增/修改/删除/调整床位/发起病房呼叫/床位预警。
 * 病房属性:编号/类型/总床位/剩余床位/预警值。剩余床位不能超过总床位。
 *
 * View / add / edit / delete / adjust beds / initiate call / bed warning.
 * Ward attributes: ID / type / total beds / remaining beds / warning line.
 * Remaining beds cannot exceed total beds.
 * ============================================================================
 */

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
    printf("1. 查看病房\n");          /* View wards */
    printf("2. 新增病房\n");          /* Add ward */
    printf("3. 修改病房\n");          /* Edit ward */
    printf("4. 删除病房\n");          /* Delete ward */
    printf("5. 调整剩余床位\n");      /* Adjust remaining beds */
    printf("6. 发起病房呼叫\n");      /* Initiate ward call */
    printf("7. 床位预警\n");          /* Bed warning */
    printf("0. 返回\n");

    choice = get_menu_choice(0, 7);
    switch (choice) {
        case 0:
            return SUCCESS;
        case 1:
            list_wards();
            return SUCCESS;
        case 2:
            /* 新增病房 Add new ward */
            printf("\n请输入病房编号: ");
            if (fgets(ward_id, sizeof(ward_id), stdin) == NULL) return ERROR_INVALID_INPUT;
            ward_id[strcspn(ward_id, "\n")] = 0;

            /* 检查病房编号唯一性 Check ward ID uniqueness */
            ward_head = load_wards_list();
            current_ward = ward_head;
            while (current_ward) {
                if (strcmp(current_ward->data.ward_id, ward_id) == 0) {
                    printf("病房编号已存在!\n");
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

            printf("请输入总床位数: ");
            if (scanf("%d", &ward.total_beds) != 1 || ward.total_beds <= 0) {
                clear_input_buffer();
                free_ward_list(ward_head);
                return ERROR_INVALID_INPUT;
            }
            clear_input_buffer();

            /* 剩余床位不能超过总床位 Remaining beds <= total beds */
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

            /* 创建节点并追加到链表尾部 Create node and append to tail */
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
            /* 修改病房 Edit ward */
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
                printf("病房不存在!\n");
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

            /* 剩余床位不能超过总床位 Remaining beds <= total beds */
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
            /* 删除病房 Delete ward */
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
                printf("病房不存在!\n");
                free_ward_list(ward_head);
                return ERROR_NOT_FOUND;
            }

            /* 从链表中删除节点 Remove node from linked list */
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
            /* 调整剩余床位 Adjust remaining beds */
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
                    /* 验证剩余床位范围: 0 <= remain_beds <= total_beds */
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
            /* 发起病房呼叫 Initiate ward call */
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

            /* 验证患者存在 Validate patient exists */
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

            /* 构建呼叫对象 Build call object */
            memset(&call, 0, sizeof(call));
            generate_id(call.call_id, MAX_ID, "WC");  /* 生成"WC"前缀的呼叫ID */
            strcpy(call.ward_id, ward_id);
            strcpy(call.patient_id, patient_id);
            strcpy(call.message, message);
            strcpy(call.status, "待处理");  /* 初始状态: 待处理 Initial status: pending */
            get_current_time(call.create_time, sizeof(call.create_time));

            /* 从预约记录中推断科室信息(默认DEP001)
               Infer department from appointment records (default DEP001) */
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

            /* 追加到病房呼叫链表 Append to ward call linked list */
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
            printf("病房呼叫已创建，等待对应医生处理。\n");  /* Ward call created, awaiting doctor processing */
            free(patient);
            free_ward_call_list(call_head);
            return SUCCESS;
        }
        case 7:
            /* 床位预警检查 Bed warning check */
            check_ward_warning();
            return SUCCESS;
        default:
            return ERROR_INVALID_INPUT;
    }
}

/* ============================================================================
 * 功能6: 分析报表 (Reports & Statistics)
 * ============================================================================
 * 汇总统计所有业务实体的关键指标:
 *   - 预约挂号总量 / 现场排队人数
 *   - 诊疗记录数 / 处方记录数 / 病房呼叫数
 *   - 处方总金额 / 报销总金额 / 实际收入(总金额 - 报销)
 *
 * Aggregate statistics for all business entities:
 *   - Total appointments / onsite queue size
 *   - Total medical records / prescriptions / ward calls
 *   - Total prescription amount / total reimbursement / actual revenue (total - reimbursement)
 * ============================================================================
 */

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

    /* 遍历所有处方,累加总金额和报销金额
       Iterate all prescriptions, accumulate total amount and reimbursement */
    while (current_prescription) {
        total_prescription_amount += current_prescription->data.total_price;
        Drug *drug = find_drug_by_id(current_prescription->data.drug_id);
        if (drug) {
            Patient *patient = find_patient_by_id(current_prescription->data.patient_id);
            if (patient) {
                /* 根据患者类型计算报销额 Calculate reimbursement based on patient type */
                float reimb = calculate_drug_reimbursement(drug, current_prescription->data.quantity, patient->patient_type);
                total_reimbursement += reimb;
                free(patient);
            }
            free(drug);
        }
        current_prescription = current_prescription->next;
    }

    ui_header("报表管理");
    printf("预约挂号总量: %d\n", count_appointment_list(appointment_head));       /* Total appointments */
    printf("现场挂号排队人数: %d\n", onsite_queue.size);                         /* Onsite queue size */
    printf("诊疗记录数: %d\n", count_medical_record_list(record_head));          /* Total medical records */
    printf("处方记录数: %d\n", count_prescription_list(prescription_head));      /* Total prescriptions */
    printf("病房呼叫数: %d\n", count_ward_call_list(call_head));                 /* Total ward calls */
    printf("处方总金额: %.2f\n", total_prescription_amount);                     /* Total prescription amount */
    printf("报销总金额: %.2f\n", total_reimbursement);                           /* Total reimbursement */
    printf("实际收入: %.2f\n", total_prescription_amount - total_reimbursement);  /* Actual revenue = total - reimbursement */

    free_appointment_list(appointment_head);
    free_medical_record_list(record_head);
    free_prescription_list(prescription_head);
    free_ward_call_list(call_head);
    free_onsite_registration_queue(&onsite_queue);
    return SUCCESS;
}

/* ============================================================================
 * 功能7: 排班管理 (Schedule Management)
 * ============================================================================
 * 查看/新增排班/停诊。
 * 排班属性:排班ID / 医生ID / 工作日期 / 时段 / 状态("正常"/"停诊") / 最大预约数/现场数。
 *
 * View / add schedule / cancel schedule (停诊).
 * Schedule attributes: schedule ID / doctor ID / work date / time slot / status / max appointments/onsite.
 * ============================================================================
 */

int admin_schedule_menu(const User *current_user) {
    (void)current_user;
    int choice;
    ScheduleNode *head, *cur, *node;
    Schedule sched;
    char id[MAX_ID], doc_id[MAX_ID], date[12], slot[10];

    ui_header("排班管理");
    printf("1. 查看排班\n");      /* View schedules */
    printf("2. 新增排班\n");      /* Add schedule */
    printf("3. 停诊\n");          /* Cancel schedule (停诊) */
    printf("0. 返回\n");
    choice = get_menu_choice(0, 3);
    switch (choice) {
    case 0: return SUCCESS;
    case 1:
        /* 查看排班 View all schedules */
        head = load_schedules_list();
        printf("\n%d 条排班记录\n", count_schedule_list(head));
        for (cur = head; cur; cur = cur->next)
            printf("  %s | %s | %s | %s | %s\n",
                   cur->data.schedule_id, cur->data.doctor_id,
                   cur->data.work_date, cur->data.time_slot, cur->data.status);
        free_schedule_list(head);
        return SUCCESS;
    case 2:
        /* 新增排班 Add schedule */
        printf("医生ID: "); if (!fgets(doc_id, sizeof(doc_id), stdin)) return ERROR_INVALID_INPUT;
        doc_id[strcspn(doc_id, "\n")] = 0;
        printf("日期(YYYY-MM-DD): "); if (!fgets(date, sizeof(date), stdin)) return ERROR_INVALID_INPUT;
        date[strcspn(date, "\n")] = 0;
        printf("时段(如08:00-09:00): "); if (!fgets(slot, sizeof(slot), stdin)) return ERROR_INVALID_INPUT;
        slot[strcspn(slot, "\n")] = 0;
        generate_id(id, sizeof(id), "SCH");  /* 生成"SCH"前缀的排班ID Generate SCH-prefixed schedule ID */
        memset(&sched, 0, sizeof(sched));
        strcpy(sched.schedule_id, id);
        strcpy(sched.doctor_id, doc_id);
        strcpy(sched.work_date, date);
        strcpy(sched.time_slot, slot);
        strcpy(sched.status, "正常");  /* 初始状态: 正常 Initial status: normal */
        sched.max_appt = 20;           /* 默认最大预约数 Default max appointments */
        sched.max_onsite = 10;         /* 默认最大现场挂号数 Default max onsite */
        head = load_schedules_list();
        node = create_schedule_node(&sched);
        node->next = head;             /* 头插法 Prepend to list */
        save_schedules_list(node);
        free_schedule_list(node);
        printf("排班 %s 创建成功\n", id);
        return SUCCESS;
    case 3:
        /* 停诊 Cancel schedule */
        head = load_schedules_list();
        printf("排班ID: "); if (!fgets(id, sizeof(id), stdin)) { free_schedule_list(head); return ERROR_INVALID_INPUT; }
        id[strcspn(id, "\n")] = 0;
        for (cur = head; cur; cur = cur->next) {
            if (strcmp(cur->data.schedule_id, id) == 0) {
                strcpy(cur->data.status, "停诊");  /* 将状态改为"停诊" Set status to cancelled */
                save_schedules_list(head);
                free_schedule_list(head);
                printf("排班 %s 已停诊\n", id);
                return SUCCESS;
            }
        }
        free_schedule_list(head);
        ui_err("未找到该排班");
        return ERROR_NOT_FOUND;
    }
    return SUCCESS;
}

/* ============================================================================
 * 功能8: 操作日志 (Operation Logs)
 * ============================================================================
 * 查看系统操作日志,包含时间、操作人、动作、目标、详情。
 * View system operation logs: timestamp, operator, action, target, details.
 * ============================================================================
 */

int admin_log_menu(const User *current_user) {
    (void)current_user;
    LogEntryNode *logs = load_logs_list();
    if (!logs) {
        ui_header("操作日志");
        printf("暂无日志记录\n");  /* No log records */
        return SUCCESS;
    }
    int total = count_log_entry_list(logs);
    ui_header("操作日志");
    printf("共 %d 条日志记录\n\n", total);
    LogEntryNode *cur = logs;
    while (cur) {
        /* 日志格式: [时间] 操作人 | 动作 目标 目标ID | 详情
           Log format: [time] operator | action target target_id | detail */
        printf("  [%s] %s | %s %s %s | %s\n",
               cur->data.create_time, cur->data.operator_name,
               cur->data.action, cur->data.target,
               cur->data.target_id, cur->data.detail);
        cur = cur->next;
    }
    free_log_entry_list(logs);
    return SUCCESS;
}

/* ============================================================================
 * 功能9: 数据管理 (Data Management — Backup / Restore)
 * ============================================================================
 * 备份/恢复/查看备份列表。
 * 备份时自动记录操作日志。恢复时列出可用备份供选择。
 *
 * Backup / restore / list backups.
 * Backup operations are automatically logged. Restore lists available backups for selection.
 * ============================================================================
 */

int admin_data_menu(const User *current_user) {
    (void)current_user;
    int choice;
    char dir_name[64];

    ui_header("数据管理");
    printf("1. 备份数据\n");          /* Backup data */
    printf("2. 恢复数据\n");          /* Restore data */
    printf("3. 查看备份列表\n");      /* View backup list */
    printf("0. 返回\n");
    choice = get_menu_choice(0, 3);
    switch (choice) {
    case 0: return SUCCESS;
    case 1:
        /* 备份数据 Backup data */
        if (backup_data() >= 0) {
            /* 备份成功后记录操作日志 Log the backup operation */
            append_log(current_user->username, "备份", "数据", "all", "手动数据备份");
            printf("数据备份完成!\n");
        } else {
            ui_err("备份失败");
        }
        return SUCCESS;
    case 2: {
        /* 恢复数据 Restore data */
        const char **names = NULL;
        int count = 0;
        list_backups(&names, &count);
        if (count == 0) {
            printf("没有找到备份\n");  /* No backups found */
            return SUCCESS;
        }
        printf("可用备份:\n");  /* Available backups */
        for (int i = 0; i < count; i++) printf("  %s\n", names[i]);
        printf("输入备份目录名: ");
        if (!fgets(dir_name, sizeof(dir_name), stdin)) {
            free_backups_list(names, count);
            return ERROR_INVALID_INPUT;
        }
        dir_name[strcspn(dir_name, "\n")] = 0;
        if (restore_data(dir_name) >= 0) {
            /* 恢复成功后记录操作日志 Log the restore operation */
            append_log(current_user->username, "恢复", "数据", "all", "数据恢复");
            printf("数据恢复完成!\n");
        } else {
            ui_err("恢复失败，请检查目录名");  /* Restore failed, check directory name */
        }
        free_backups_list(names, count);
        return SUCCESS;
    }
    case 3: {
        /* 查看备份列表 View backup list */
        const char **names = NULL;
        int count = 0;
        list_backups(&names, &count);
        if (count == 0) {
            printf("暂无备份\n");  /* No backups */
            return SUCCESS;
        }
        printf("备份列表 (%d):\n", count);
        for (int i = 0; i < count; i++) printf("  %s\n", names[i]);
        free_backups_list(names, count);
        return SUCCESS;
    }
    }
    return SUCCESS;
}
