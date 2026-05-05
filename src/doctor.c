/*
 * ============================================================================
 * doctor.c — 医生工作流模块 (Doctor Workflow Module)
 * ============================================================================
 *
 * 【中文说明】
 * 本文件实现了医院管理系统(C99控制台模式)中医生角色的完整业务流程。
 * 医生登录后可执行以下操作:
 *   1. 被挂号提醒        — 查看当日挂号患者列表(预约+现场排队+急诊标记)
 *   2. 接诊处理          — 按队列顺序接诊,录入诊断/治疗/检查信息,生成病历
 *   3. 开药功能           — 基于病历开具处方,含库存扣减、报销计算、库存不足回滚
 *   4. 病房被叫提醒       — 查看/处理/发起病房呼叫
 *   5. 紧急病人标识       — 标记患者为急诊,自动生成优先病房呼叫
 *   6. 治疗进度更新       — 推进或结束患者治疗阶段
 *   7. 模板管理           — 诊断/治疗/检查模板的CRUD,用于快速输入
 *
 * 关键业务约束:
 *   - 医生只能处理与自己有关系的患者(预约/现场/病历中有记录)
 *   - 库存不足时禁止开药;处方保存失败时自动回滚库存
 *   - 7天内重复药物提醒(避免重复用药)
 *   - 急诊患者享有优先队列(现场挂号自动插队)和床位优先分配
 *   - 接诊完成后自动推进患者治疗阶段,降低医生繁忙度
 *
 * 【English】
 * This file implements the complete business workflow for the doctor role
 * in a C99 console-mode hospital management system.
 * After login, a doctor can:
 *   1. Appointment Reminder — View today's patient list (appointments + queue + emergency)
 *   2. Consultation — Serve patients by queue order, record diagnosis/treatment/exam, create medical record
 *   3. Prescribing — Issue prescriptions from a medical record, with stock deduction, reimbursement calc, rollback on failure
 *   4. Ward Call — View / handle / initiate ward calls
 *   5. Emergency Flag — Mark a patient as emergency, auto-generate priority ward call
 *   6. Progress Update — Advance or complete a patient's treatment stage
 *   7. Template CRUD — Create/delete diagnosis, treatment, and exam templates for quick input
 *
 * Key business constraints:
 *   - Doctors can only operate on patients with established relationships (appointment/onsite/record)
 *   - Prescribing is blocked if stock is insufficient; stock is rolled back if prescription save fails
 *   - 7-day duplicate drug check warns about repeat prescriptions
 *   - Emergency patients get priority queue position (auto-inserted ahead) and bed priority
 *   - After consultation, the patient's treatment stage advances and doctor busy-level decreases
 * ============================================================================
 */

#include "doctor.h"
#include "public.h"
#include "ui_utils.h"
#include "login.h"

/* ============================================================================
 * 工具辅助函数 (Utility Helper Functions)
 * ============================================================================
 * 以下为医生模块内部使用的静态辅助函数,封装了常见的查找/更新/标记操作。
 * These are static helper functions used internally by the doctor module,
 * encapsulating common lookup / update / marking operations.
 * ============================================================================
 */

/**
 * print_patient_name_by_id — 根据患者ID查找并输出患者姓名
 *                          Look up patient name by patient ID
 * 遍历患者链表,找到匹配ID后复制姓名到输出缓冲区;未找到则默认"未知"。
 * Iterates the patient list; copies the name to the output buffer if found,
 * otherwise defaults to "未知" (unknown).
 */
static void print_patient_name_by_id(const char *patient_id, char *patient_name, size_t size) {
    PatientNode *patient_head = load_patients_list();
    PatientNode *current_patient = patient_head;

    strncpy(patient_name, "未知", size - 1);
    patient_name[size - 1] = '\0';

    while (current_patient) {
        if (strcmp(current_patient->data.patient_id, patient_id) == 0) {
            strncpy(patient_name, current_patient->data.name, size - 1);
            patient_name[size - 1] = '\0';
            break;
        }
        current_patient = current_patient->next;
    }

    free_patient_list(patient_head);
}

/**
 * doctor_has_patient_relation — 检查医生与患者是否存在关联关系
 *                               Check if doctor has a relationship with the patient
 * 在三个数据源中查找:预约记录、现场挂号队列、病历记录。
 * 只要在任一数据源中找到匹配的(doctor_id, patient_id)对即返回1。
 * 这是权限控制的核心——医生只能操作与自己有关联的患者。
 * Searches three data sources: appointments, onsite queue, medical records.
 * Returns 1 if a matching (doctor_id, patient_id) pair exists in any source.
 * This is the core permission control — doctors can only operate on related patients.
 */
static int doctor_has_patient_relation(const Doctor *doctor, const char *patient_id) {
    AppointmentNode *appointment_head = load_appointments_list();
    AppointmentNode *current_appointment = appointment_head;
    OnsiteRegistrationQueue onsite_queue = load_onsite_registration_queue();
    OnsiteRegistrationNode *current_onsite = onsite_queue.front;
    MedicalRecordNode *record_head = load_medical_records_list();
    MedicalRecordNode *current_record = record_head;

    /* 检查预约记录中的关联 Check appointment records */
    while (current_appointment) {
        if (strcmp(current_appointment->data.doctor_id, doctor->doctor_id) == 0 &&
            strcmp(current_appointment->data.patient_id, patient_id) == 0) {
            free_appointment_list(appointment_head);
            free_onsite_registration_queue(&onsite_queue);
            free_medical_record_list(record_head);
            return 1;
        }
        current_appointment = current_appointment->next;
    }

    /* 检查现场挂号队列中的关联 Check onsite registration queue */
    while (current_onsite) {
        if (strcmp(current_onsite->data.doctor_id, doctor->doctor_id) == 0 &&
            strcmp(current_onsite->data.patient_id, patient_id) == 0) {
            free_appointment_list(appointment_head);
            free_onsite_registration_queue(&onsite_queue);
            free_medical_record_list(record_head);
            return 1;
        }
        current_onsite = current_onsite->next;
    }

    /* 检查病历记录中的关联 Check medical records */
    while (current_record) {
        if (strcmp(current_record->data.doctor_id, doctor->doctor_id) == 0 &&
            strcmp(current_record->data.patient_id, patient_id) == 0) {
            free_appointment_list(appointment_head);
            free_onsite_registration_queue(&onsite_queue);
            free_medical_record_list(record_head);
            return 1;
        }
        current_record = current_record->next;
    }

    free_appointment_list(appointment_head);
    free_onsite_registration_queue(&onsite_queue);
    free_medical_record_list(record_head);
    return 0;
}

/**
 * adjust_doctor_busy_level — 调整医生繁忙度
 *                            Adjust the doctor's busy level by a delta value
 * 加载医生列表,找到指定医生后修改busy_level(加delta),值不低于0。
 * 接诊完成时delta=-1(降低繁忙度);新挂号时delta=+1(增加繁忙度)。
 * Loads the doctor list, adjusts busy_level by delta, clamped to >= 0.
 * Consultation completion uses delta=-1; new registration uses delta=+1.
 */
static void adjust_doctor_busy_level(const char *doctor_id, int delta) {
    DoctorNode *doctor_head = load_doctors_list();
    DoctorNode *current_doctor = doctor_head;

    while (current_doctor) {
        if (strcmp(current_doctor->data.doctor_id, doctor_id) == 0) {
            current_doctor->data.busy_level += delta;
            if (current_doctor->data.busy_level < 0) {
                current_doctor->data.busy_level = 0;  /* 繁忙度不低于0 Clamp busy_level to 0 minimum */
            }
            break;
        }
        current_doctor = current_doctor->next;
    }

    save_doctors_list(doctor_head);
    free_doctor_list(doctor_head);
}

/**
 * update_patient_stage — 更新患者治疗阶段和急诊状态
 *                        Update patient treatment stage and emergency flag
 * 可选择性更新治疗阶段(仅当new_stage非空时)和急诊标记(仅当emergency_flag>=0时)。
 * 接诊后自动调用此函数推进患者到下一阶段。
 * Optionally updates treatment stage (when new_stage is non-empty) and
 * emergency flag (when emergency_flag >= 0). Called after consultation.
 */
static int update_patient_stage(const char *patient_id, const char *new_stage, int emergency_flag) {
    PatientNode *patient_head = load_patients_list();
    PatientNode *current_patient = patient_head;

    while (current_patient) {
        if (strcmp(current_patient->data.patient_id, patient_id) == 0) {
            if (new_stage && new_stage[0] != '\0') {
                strncpy(current_patient->data.treatment_stage, new_stage, sizeof(current_patient->data.treatment_stage) - 1);
                current_patient->data.treatment_stage[sizeof(current_patient->data.treatment_stage) - 1] = '\0';
            }
            if (emergency_flag >= 0) {
                /* emergency_flag=1 标记为急诊 true=emergency, false=normal */
                current_patient->data.is_emergency = emergency_flag ? true : false;
            }
            save_patients_list(patient_head);
            free_patient_list(patient_head);
            return SUCCESS;
        }
        current_patient = current_patient->next;
    }

    free_patient_list(patient_head);
    return ERROR_NOT_FOUND;
}

/**
 * mark_registration_status — 将挂号状态标记为"已就诊"(预约)或"就诊中"(现场)
 *                            Mark appointment as "已就诊" or onsite as "就诊中"
 * 根据service_id在预约记录和现场挂号队列中查找匹配项。
 * 预约挂号的旧状态变为"已就诊",现场挂号的旧状态变为"就诊中"。
 * 同时输出关联的patient_id和department_id,供后续病历创建使用。
 * Searches both appointments and onsite queue by service_id.
 * Appointment status -> "已就诊" (visited), Onsite status -> "就诊中" (in consultation).
 * Also outputs the linked patient_id and department_id for subsequent record creation.
 */
static int mark_registration_status(const Doctor *doctor, const char *service_id, char *patient_id, char *department_id) {
    AppointmentNode *appointment_head = load_appointments_list();
    AppointmentNode *current_appointment = appointment_head;

    /* 首先在预约挂号记录中查找 First search in appointment records */
    while (current_appointment) {
        if (strcmp(current_appointment->data.appointment_id, service_id) == 0 &&
            strcmp(current_appointment->data.doctor_id, doctor->doctor_id) == 0) {
            strcpy(patient_id, current_appointment->data.patient_id);
            strcpy(department_id, current_appointment->data.department_id);
            strcpy(current_appointment->data.status, "已就诊");  /* 标记为已就诊 Mark as visited */
            save_appointments_list(appointment_head);
            free_appointment_list(appointment_head);
            return SUCCESS;
        }
        current_appointment = current_appointment->next;
    }
    free_appointment_list(appointment_head);

    /* 然后在现场挂号队列中查找 Then search in onsite registration queue */
    {
        OnsiteRegistrationQueue onsite_queue = load_onsite_registration_queue();
        OnsiteRegistrationNode *current_onsite = onsite_queue.front;

        while (current_onsite) {
            if (strcmp(current_onsite->data.onsite_id, service_id) == 0 &&
                strcmp(current_onsite->data.doctor_id, doctor->doctor_id) == 0) {
                strcpy(patient_id, current_onsite->data.patient_id);
                strcpy(department_id, current_onsite->data.department_id);
                strcpy(current_onsite->data.status, "就诊中");  /* 标记为就诊中 Mark as in-consultation */
                save_onsite_registration_queue(&onsite_queue);
                free_onsite_registration_queue(&onsite_queue);
                return SUCCESS;
            }
            current_onsite = current_onsite->next;
        }

        free_onsite_registration_queue(&onsite_queue);
    }

    return ERROR_NOT_FOUND;
}

/**
 * upsert_medical_record — 创建或更新病历记录 (Upsert Medical Record)
 *                          Create a new medical record or update an existing one
 * 如果已存在同一医生的同一服务单号的病历,则更新诊断和状态;
 * 否则创建新病历记录,自动生成MR前缀的ID,记录当前时间为诊断日期。
 * "诊断日期"每次更新都会刷新,确保记录时间是最新的。
 * If a record exists for the same doctor + service_id, update diagnosis & status;
 * otherwise create a new record with auto-generated MR-prefixed ID and current time.
 * The diagnosis_date is refreshed on every update to reflect latest consultation time.
 */
static int upsert_medical_record(const Doctor *doctor, const char *patient_id, const char *service_id, const char *diagnosis_text, const char *status_text, char *record_id_out) {
    MedicalRecordNode *record_head = load_medical_records_list();
    MedicalRecordNode *current_record = record_head;
    MedicalRecord new_record;

    /* 查找已存在的病历记录 — 存在则更新 Search for existing record — update if found */
    while (current_record) {
        if (strcmp(current_record->data.doctor_id, doctor->doctor_id) == 0 &&
            strcmp(current_record->data.appointment_id, service_id) == 0) {
            strncpy(current_record->data.diagnosis, diagnosis_text, sizeof(current_record->data.diagnosis) - 1);
            current_record->data.diagnosis[sizeof(current_record->data.diagnosis) - 1] = '\0';
            strncpy(current_record->data.status, status_text, sizeof(current_record->data.status) - 1);
            current_record->data.status[sizeof(current_record->data.status) - 1] = '\0';
            get_current_time(current_record->data.diagnosis_date, sizeof(current_record->data.diagnosis_date));
            if (record_id_out) {
                strcpy(record_id_out, current_record->data.record_id);
            }
            save_medical_records_list(record_head);
            free_medical_record_list(record_head);
            return SUCCESS;
        }
        current_record = current_record->next;
    }

    /* 不存在 — 创建新病历 Not found — create a new medical record */
    memset(&new_record, 0, sizeof(new_record));
    generate_id(new_record.record_id, MAX_ID, "MR");  /* 生成"MR"前缀的病历ID Generate MR-prefixed record ID */
    strcpy(new_record.patient_id, patient_id);
    strcpy(new_record.doctor_id, doctor->doctor_id);
    strcpy(new_record.appointment_id, service_id);
    strncpy(new_record.diagnosis, diagnosis_text, sizeof(new_record.diagnosis) - 1);
    strncpy(new_record.status, status_text, sizeof(new_record.status) - 1);
    get_current_time(new_record.diagnosis_date, sizeof(new_record.diagnosis_date));

    /* 创建链表节点并追加到尾部 Create node and append to tail */
    {
        MedicalRecordNode *new_node = create_medical_record_node(&new_record);
        if (!new_node) {
            free_medical_record_list(record_head);
            return ERROR_FILE_IO;
        }

        if (!record_head) {
            record_head = new_node;
        } else {
            MedicalRecordNode *tail = record_head;
            while (tail->next) {
                tail = tail->next;
            }
            tail->next = new_node;
        }
    }

    if (record_id_out) {
        strcpy(record_id_out, new_record.record_id);
    }
    save_medical_records_list(record_head);
    free_medical_record_list(record_head);
    return SUCCESS;
}

/* ============================================================================
 * 挂号展示 (Registration Display)
 * ============================================================================
 * 显示当前医生所有待处理的患者列表,包括预约挂号和现场排队。
 * Show all pending patients for the current doctor: appointments and onsite queue.
 * ============================================================================
 */

/**
 * show_current_doctor_registrations — 展示当前医生的所有挂号记录(分页)
 *                                     Display all registrations for the current doctor (paginated)
 * 包括三部分:
 *   1. 急诊患者横幅 — 红色高亮显示待接诊的急诊患者数量
 *   2. 预约挂号列表 — 分页显示所有预约记录(ID/日期/时间/患者/科室/状态)
 *   3. 现场挂号列表 — 分页显示所有现场排队记录(ID/患者/排号/科室/状态)
 * Includes three sections:
 *   1. Emergency banner — red highlight showing count of emergency patients awaiting
 *   2. Appointment list — paginated display of all appointments
 *   3. Onsite queue list — paginated display of all onsite queue entries
 */
static void show_current_doctor_registrations(const Doctor *doctor) {
    AppointmentNode *appointment_head = load_appointments_list();
    OnsiteRegistrationQueue onsite_queue = load_onsite_registration_queue();
    int found_appt = 0;
    int found_onsite = 0;

    /* ===== 急诊患者横幅 Emergency patient banner ===== */
    /* 遍历现场挂号队列,统计该医生名下待就诊/呼叫中的急诊患者数量 */
    {
        PatientNode *ph = load_patients_list();
        OnsiteRegistrationNode *on = onsite_queue.front;
        int emergency_count = 0;
        while (on) {
            if (strcmp(on->data.doctor_id, doctor->doctor_id) == 0 &&
                (strcmp(on->data.status, "待就诊") == 0 || strcmp(on->data.status, "呼叫中") == 0)) {
                PatientNode *pn = ph;
                while (pn) {
                    if (strcmp(pn->data.patient_id, on->data.patient_id) == 0 && pn->data.is_emergency) {
                        emergency_count++;
                        break;
                    }
                    pn = pn->next;
                }
            }
            on = on->next;
        }
        if (emergency_count > 0) {
            /* 红色背景白色文字醒目提示 Red background white text for urgent notice */
            printf("\n" BG_RED C_WHITE "  ⚠ 急诊患者 %d 人待接诊! " C_RESET "\n\n", emergency_count);
        }
        if (ph) free_patient_list(ph);
    }

    /* ===== 预约挂号列表 (分页) Appointment list (paginated) ===== */
    {
        /* 第一遍遍历:统计匹配的预约数 First pass: count matching appointments */
        int match = 0;
        AppointmentNode *ap = appointment_head;
        while (ap) {
            if (strcmp(ap->data.doctor_id, doctor->doctor_id) == 0) match++;
            ap = ap->next;
        }

        if (match > 0) {
            /* 构建分页显示的字符串数组 Build string arrays for paginated display */
            const char **ai = malloc(match * sizeof(const char *));
            char (*ab)[90] = malloc(match * sizeof(*ab));
            int idx = 0;
            ap = appointment_head;
            while (ap && idx < match) {
                if (strcmp(ap->data.doctor_id, doctor->doctor_id) == 0) {
                    char pn[MAX_NAME];
                    print_patient_name_by_id(ap->data.patient_id, pn, sizeof(pn));
                    snprintf(ab[idx], 90, "%-16s %-12s %-6s %-12s %-10s %-8s",
                             ap->data.appointment_id, ap->data.appointment_date,
                             ap->data.appointment_time, pn,
                             ap->data.department_id, ap->data.status);
                    ai[idx] = ab[idx];
                    idx++;
                    found_appt++;
                }
                ap = ap->next;
            }
            ui_paginate(ai, match, 15, "预约挂号");  /* 每页15行 15 rows per page */
            free((void*)ai); free(ab);
        }
    }

    if (!found_appt) {
        printf("暂无预约挂号记录。\n");  /* No appointment records */
    }

    /* ===== 现场挂号列表 (分页) Onsite registration list (paginated) ===== */
    {
        int match = 0;
        OnsiteRegistrationNode *on = onsite_queue.front;
        while (on) {
            if (strcmp(on->data.doctor_id, doctor->doctor_id) == 0) match++;
            on = on->next;
        }

        if (match > 0) {
            const char **oi = malloc(match * sizeof(const char *));
            char (*ob)[80] = malloc(match * sizeof(*ob));
            int idx = 0;
            on = onsite_queue.front;
            while (on && idx < match) {
                if (strcmp(on->data.doctor_id, doctor->doctor_id) == 0) {
                    char pn[MAX_NAME];
                    print_patient_name_by_id(on->data.patient_id, pn, sizeof(pn));
                    snprintf(ob[idx], 80, "%-16s %-10s 排%03d  %-10s %-10s",
                             on->data.onsite_id, pn, on->data.queue_number,
                             on->data.department_id, on->data.status);
                    oi[idx] = ob[idx];
                    idx++;
                    found_onsite++;
                }
                on = on->next;
            }
            ui_paginate(oi, match, 15, "现场挂号(排队)");
            free((void*)oi); free(ob);
        }
    }

    if (!found_onsite) {
        printf("暂无现场排队记录。\n");  /* No onsite queue records */
    }

    free_appointment_list(appointment_head);
    free_onsite_registration_queue(&onsite_queue);
}

/* ============================================================================
 * 医生主菜单 (Doctor Main Menu)
 * ============================================================================
 * 定义医生角色的7大功能入口。
 * Defines the 7 function entry points for the doctor role.
 * ============================================================================
 */

void doctor_main_menu(const User *current_user) {
    (void)current_user;
    ui_menu_item(1, "被挂号提醒");      /* Appointment reminder */
    ui_menu_item(2, "接诊处理");        /* Consultation */
    ui_menu_item(3, "开药功能");        /* Prescribing */
    ui_menu_item(4, "病房被叫提醒");    /* Ward call alert */
    ui_menu_item(5, "紧急病人标识");    /* Emergency patient flag */
    ui_menu_item(6, "治疗进度更新");    /* Treatment progress update */
    ui_menu_item(7, "模板管理");        /* Template management */
    ui_menu_item(8, "修改密码");        /* Change password */
}

/* ============================================================================
 * 功能1: 被挂号提醒 (Appointment Reminder)
 * ============================================================================
 * 显示医生的基本信息(姓名/职称/繁忙度)、当日排班和所有挂号记录。
 * Show doctor's basic info (name/title/busy_level), today's schedule, and all registrations.
 * ============================================================================
 */

int doctor_appointment_reminder_menu(const User *current_user) {
    Doctor *current_doctor = find_doctor_by_username(current_user->username);

    if (!current_doctor) {
        ui_err("医生信息不存在!");  /* Doctor info not found */
        return ERROR_NOT_FOUND;
    }

    ui_header("被挂号提醒");
    printf("医生: %s [%s]\n", current_doctor->name, current_doctor->title);
    /* 繁忙度分级显示: <=2 较空闲, <=5 适中, >5 较忙 */
    printf("当前繁忙度: %d", current_doctor->busy_level);
    if (current_doctor->busy_level <= 2) {
        printf(" (较空闲)\n");       /* Relatively free */
    } else if (current_doctor->busy_level <= 5) {
        printf(" (适中)\n");         /* Moderate */
    } else {
        printf(" (较忙)\n");         /* Busy */
    }

    /* 显示当日排班 Show today's schedule */
    {
        char today_str[12];
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        strftime(today_str, sizeof(today_str), "%Y-%m-%d", tm_info);

        ScheduleNode *sh = load_schedules_list();
        ScheduleNode *sc = sh;
        int found = 0;
        while (sc) {
            if (strcmp(sc->data.doctor_id, current_doctor->doctor_id) == 0 &&
                strcmp(sc->data.work_date, today_str) == 0) {
                if (!found) {
                    printf(C_DIM "  今日排班: " C_RESET);  /* Today's schedule label */
                    found = 1;
                }
                printf(C_DIM "%s [%s]  " C_RESET, sc->data.time_slot, sc->data.status);
            }
            sc = sc->next;
        }
        if (found) {
            printf("\n");
        } else {
            printf(C_DIM "  今日无排班或未设置\n" C_RESET);  /* No schedule today */
        }
        free_schedule_list(sh);
    }

    show_current_doctor_registrations(current_doctor);
    free(current_doctor);
    return SUCCESS;
}

/* ============================================================================
 * 功能2: 接诊处理 (Patient Consultation)
 * ============================================================================
 * 核心接诊流程:
 *   1. 显示当前医生的挂号列表和叫号状态
 *   2. 建议按队列顺序接诊,急诊患者优先提示
 *   3. 医生选择接诊单号(预约或现场)
 *   4. 依次录入诊断、治疗建议、检查项目(支持模板快捷输入)
 *   5. 生成/更新病历,推进患者治疗阶段,降低医生繁忙度
 * Core consultation flow:
 *   1. Show doctor's registration list and current serving number
 *   2. Suggest serving by queue order, with emergency priority alerts
 *   3. Doctor selects a service ID (appointment or onsite)
 *   4. Input diagnosis, treatment advice, and exam items (with template support)
 *   5. Create/update medical record, advance patient stage, decrease busy level
 * ============================================================================
 */

int doctor_consultation_menu(const User *current_user) {
    Doctor *current_doctor = find_doctor_by_username(current_user->username);
    char service_id[MAX_ID];
    char patient_id[MAX_ID];
    char department_id[MAX_ID];
    char diagnosis[200];
    char treatment_advice[120];
    char exam_items[120];
    char final_text[500];
    char record_id[MAX_ID];
    Patient *patient = NULL;
    int result;

    if (!current_doctor) {
        ui_err("医生信息不存在!");
        return ERROR_NOT_FOUND;
    }

    ui_header("接诊处理");
    printf("医生: %s [%s]\n", current_doctor->name, current_doctor->title);
    show_current_doctor_registrations(current_doctor);

    /* 显示当前叫号 Display current serving number */
    {
        OnsiteRegistrationQueue tmp_queue = load_onsite_registration_queue();
        OnsiteRegistrationNode *tmp_node = tmp_queue.front;
        int serving_num = 0;
        while (tmp_node) {
            if (strcmp(tmp_node->data.doctor_id, current_doctor->doctor_id) == 0 &&
                strcmp(tmp_node->data.status, "就诊中") == 0) {
                serving_num = tmp_node->data.queue_number;  /* 正在就诊的号码 Currently serving */
                break;
            }
            tmp_node = tmp_node->next;
        }
        free_onsite_registration_queue(&tmp_queue);
        if (serving_num > 0) {
            printf("  当前叫号: %d 号\n", serving_num);
        } else {
            printf("  当前叫号: 暂无\n");  /* No one being served */
        }
    }

    /* 建议按队列顺序接诊:查找当前医生最早的"排队中"现场患者
       Suggest serving by queue order: find the earliest "排队中" onsite patient */
    {
        OnsiteRegistrationQueue tmp_queue = load_onsite_registration_queue();
        OnsiteRegistrationNode *tmp_node = tmp_queue.front;
        int has_pending = 0;
        int has_emergency = 0;
        while (tmp_node) {
            if (strcmp(tmp_node->data.doctor_id, current_doctor->doctor_id) == 0 &&
                strcmp(tmp_node->data.status, "排队中") == 0) {
                if (!has_pending) {
                    printf("\n建议接诊(队列顺序): %s (排队号:%d)\n",
                           tmp_node->data.onsite_id, tmp_node->data.queue_number);
                    has_pending = 1;
                }
                /* 检查该患者是否为急诊 — 急诊优先处理 Check if this patient is emergency */
                Patient *p = find_patient_by_id(tmp_node->data.patient_id);
                if (p && p->is_emergency) {
                    printf(C_BOLD BG_RED "  !!! 急诊患者: %s (%s) !!!" C_RESET "\n", p->name, p->patient_id);
                    has_emergency = 1;
                    free(p);
                    break; /* 急诊患者在最前面则高亮后退出 emergency patient at front — highlight and stop */
                }
                if (p) free(p);
                if (has_pending) break; /* 只显示前几个 only show first few */
            }
            tmp_node = tmp_node->next;
        }
        if (!has_pending) {
            printf("\n当前无排队中的现场患者。\n");  /* No patients in queue */
        } else if (has_emergency) {
            printf(C_BOLD C_RED "  ⚠ 请优先处理急诊患者!" C_RESET "\n");  /* Please handle emergency patients first */
        }
        free_onsite_registration_queue(&tmp_queue);
    }

    /* 构建当前医生接诊候选列表 Build candidate list of service IDs for this doctor */
    {
        const char *svc_list[500];
        int svc_count = 0;
        /* 收集"已预约"状态的预约单号 Collect "已预约" appointment IDs */
        {
            AppointmentNode *ah = load_appointments_list();
            AppointmentNode *ap = ah;
            while (ap && svc_count < 200) {
                if (strcmp(ap->data.doctor_id, current_doctor->doctor_id) == 0 &&
                    strcmp(ap->data.status, "已预约") == 0) {
                    svc_list[svc_count++] = ap->data.appointment_id;
                }
                ap = ap->next;
            }
            free_appointment_list(ah);
        }
        /* 收集"排队中"状态的现场挂号单号 Collect "排队中" onsite IDs */
        {
            OnsiteRegistrationQueue oq = load_onsite_registration_queue();
            OnsiteRegistrationNode *on = oq.front;
            while (on && svc_count < 500) {
                if (strcmp(on->data.doctor_id, current_doctor->doctor_id) == 0 &&
                    strcmp(on->data.status, "排队中") == 0) {
                    svc_list[svc_count++] = on->data.onsite_id;
                }
                on = on->next;
            }
            free_onsite_registration_queue(&oq);
        }

        printf("\n请输入要接诊的单号（或回车选择）: ");
        int idx = smart_id_lookup("接诊单号", svc_list, svc_count, service_id, sizeof(service_id));
        if (idx != 1 || service_id[0] == 0) {
            free(current_doctor);
            return ERROR_INVALID_INPUT;
        }
    }

    /* 标记挂号状态并获取患者/科室信息 Mark registration status and get patient/department info */
    memset(patient_id, 0, sizeof(patient_id));
    memset(department_id, 0, sizeof(department_id));
    result = mark_registration_status(current_doctor, service_id, patient_id, department_id);
    if (result != SUCCESS) {
        ui_err("未找到该接诊单号，或该单号不属于当前医生。");
        free(current_doctor);
        return result;
    }

    patient = find_patient_by_id(patient_id);
    if (!patient) {
        free(current_doctor);
        return ERROR_NOT_FOUND;
    }

    /* 依次录入诊断、治疗建议、检查项目(均支持模板快捷输入)
       Sequentially input diagnosis, treatment advice, exam items (all support template quick input) */
    printf("\n");
    if (quick_template_input("诊断", "诊断结果", diagnosis, sizeof(diagnosis)) != 1 || diagnosis[0] == 0) {
        free(patient);
        free(current_doctor);
        return ERROR_INVALID_INPUT;
    }

    printf("\n");
    if (quick_template_input("治疗", "治疗建议", treatment_advice, sizeof(treatment_advice)) != 1 || treatment_advice[0] == 0) {
        free(patient);
        free(current_doctor);
        return ERROR_INVALID_INPUT;
    }

    printf("\n");
    if (quick_template_input("检查", "检查项目", exam_items, sizeof(exam_items)) != 1 || exam_items[0] == 0) {
        free(patient);
        free(current_doctor);
        return ERROR_INVALID_INPUT;
    }
    exam_items[strcspn(exam_items, "\n")] = 0;

    /* 拼接完整的诊疗文本,生成/更新病历 Concatenate full diagnosis text, upsert medical record */
    snprintf(final_text, sizeof(final_text), "诊断:%s | 建议:%s | 检查:%s", diagnosis, treatment_advice, exam_items);
    result = upsert_medical_record(current_doctor, patient_id, service_id, final_text, "检查中", record_id);
    if (result != SUCCESS) {
        free(patient);
        free(current_doctor);
        return result;
    }

    /* 接诊后:推进患者治疗阶段 + 降低医生繁忙度
       After consultation: advance patient stage + decrease doctor busy level */
    update_patient_stage(patient_id, get_next_stage(patient->treatment_stage), patient->is_emergency ? 1 : 0);
    adjust_doctor_busy_level(current_doctor->doctor_id, -1);  /* 完成接诊后减少繁忙度 Decrease busy level after serving */

    printf("\n接诊完成!\n");
    printf("患者: %s\n", patient->name);
    printf("病历编号: %s\n", record_id);
    printf("当前阶段: %s\n", get_next_stage(patient->treatment_stage));

    free(patient);
    free(current_doctor);
    return SUCCESS;
}

/* ============================================================================
 * 功能3: 开药功能 (Prescription / Drug Prescribing)
 * ============================================================================
 * 基于已有病历开具处方的完整流程:
 *   1. 选择病历记录(分页列表)
 *   2. 药物推荐 — 基于患者历史用药去重推荐
 *   3. 选择药品 + 输入数量
 *   4. 库存校验 + 7日重复药物风险提示
 *   5. 事务性保存:先扣库存(防超卖),再保存处方;任一步失败则回滚库存
 *   6. 计算报销金额(根据患者类型和药品报销比例)
 *   7. 库存预警检查
 * Complete prescribing flow based on existing medical record:
 *   1. Select a medical record (paginated list)
 *   2. Drug recommendation — deduplicated from patient's historical prescriptions
 *   3. Choose drug + input quantity
 *   4. Stock validation + 7-day duplicate drug risk warning
 *   5. Transactional save: deduct stock first (prevent oversell), then save Rx; rollback on failure
 *   6. Calculate reimbursement (based on patient type and drug reimbursement ratio)
 *   7. Stock warning check
 * ============================================================================
 */

int doctor_prescribe_menu(const User *current_user) {
    Doctor *current_doctor = find_doctor_by_username(current_user->username);
    MedicalRecordNode *record_head = NULL;
    MedicalRecordNode *current_record = NULL;
    DrugNode *drug_head = NULL;
    DrugNode *current_drug = NULL;
    PrescriptionNode *prescription_head = NULL;
    Prescription new_prescription;
    char record_id[MAX_ID];
    char drug_id[MAX_ID];
    int quantity;

    if (!current_doctor) {
        ui_err("医生信息不存在!");
        return ERROR_NOT_FOUND;
    }

    ui_header("开药功能");

    /* ===== 病历列表 (分页) Medical record list (paginated) ===== */
    record_head = load_medical_records_list();
    {
        MedicalRecordNode *mr = record_head;
        int match = 0;
        while (mr) {
            if (strcmp(mr->data.doctor_id, current_doctor->doctor_id) == 0) match++;
            mr = mr->next;
        }
        if (match > 0) {
            const char **ri = malloc(match * sizeof(const char *));
            char (*rb)[80] = malloc(match * sizeof(*rb));
            int idx = 0;
            mr = record_head;
            while (mr && idx < match) {
                if (strcmp(mr->data.doctor_id, current_doctor->doctor_id) == 0) {
                    snprintf(rb[idx], 80, "%-16s %-14s %-20s %-10s",
                             mr->data.record_id, mr->data.patient_id,
                             mr->data.diagnosis_date, mr->data.status);
                    ri[idx] = rb[idx]; idx++; mr = mr->next;
                } else {
                    mr = mr->next;
                }
            }
            ui_paginate(ri, match, 15, "病历列表");
            free((void*)ri); free(rb);
        }
    }

    /* 构建病历候选列表 Build medical record candidate list for smart selection */
    {
        const char *rec_list[500];
        int rec_count = 0;
        MedicalRecordNode *rc = record_head;
        while (rc && rec_count < 500) {
            if (strcmp(rc->data.doctor_id, current_doctor->doctor_id) == 0) {
                rec_list[rec_count++] = rc->data.record_id;
            }
            rc = rc->next;
        }
        printf("\n");
        if (smart_id_lookup("病历编号 (或回车选择)", rec_list, rec_count, record_id, sizeof(record_id)) != 1 || record_id[0] == 0) {
            free_medical_record_list(record_head);
            free(current_doctor);
            return ERROR_INVALID_INPUT;
        }
    }

    /* 查找选定的病历记录 Find the selected medical record */
    current_record = record_head;
    while (current_record) {
        if (strcmp(current_record->data.record_id, record_id) == 0 &&
            strcmp(current_record->data.doctor_id, current_doctor->doctor_id) == 0) {
            break;
        }
        current_record = current_record->next;
    }

    if (!current_record) {
        ui_err("病历不存在或不属于当前医生。");  /* Record not found or doesn't belong to this doctor */
        free_medical_record_list(record_head);
        free(current_doctor);
        return ERROR_NOT_FOUND;
    }

    /* ===== 药物推荐:根据患者历史用药推荐 (去重) =====
             Drug recommendation: based on patient's historical prescriptions (deduplicated) */
    {
        PrescriptionNode *all_pres = load_prescriptions_list();
        PrescriptionNode *p_iter = all_pres;
        int rec_count = 0;
        char rec_drugs[100][MAX_ID];  /* 最多推荐100种药物 Max 100 recommended drugs */
        int i;

        memset(rec_drugs, 0, sizeof(rec_drugs));
        while (p_iter) {
            if (strcmp(p_iter->data.patient_id, current_record->data.patient_id) == 0) {
                /* 去重检查:避免重复推荐同一药物 Dedup check: avoid recommending same drug twice */
                int dup = 0;
                for (i = 0; i < rec_count; i++) {
                    if (strcmp(rec_drugs[i], p_iter->data.drug_id) == 0) { dup = 1; break; }
                }
                if (!dup && rec_count < 100) {
                    strcpy(rec_drugs[rec_count], p_iter->data.drug_id);
                    rec_count++;
                }
            }
            p_iter = p_iter->next;
        }
        free_prescription_list(all_pres);

        if (rec_count > 0) {
            ui_sub_header("推荐药品 (基于历史用药)");
            printf("  ");
            ui_print_col("药品编号", 10);
            ui_print_col("药品名称", 16);
            ui_print_col("单价", 10);
            ui_print_col("库存", 8);
            printf("\n");
            ui_divider();
            for (i = 0; i < rec_count; i++) {
                Drug *drug = find_drug_by_id(rec_drugs[i]);
                if (drug) {
                    printf("  ");
                    ui_print_col(drug->drug_id, 10);
                    ui_print_col(drug->name, 16);
                    ui_print_col_float(drug->price, 10);
                    ui_print_col_int(drug->stock_num, 8);
                    /* 库存紧张警告 Low stock warning */
                    if (drug->stock_num <= drug->warning_line) printf(" (库存紧张)");
                    printf("\n");
                    free(drug);
                }
            }
        }
    }

    /* ===== 所有可用药品列表 (分页) All available drugs (paginated) ===== */
    drug_head = load_drugs_list();
    {
        int dc = count_drug_list(drug_head);
        if (dc > 0) {
            const char **di = malloc(dc * sizeof(const char *));
            char (*db)[100] = malloc(dc * sizeof(*db));
            int idx = 0;
            DrugNode *du = drug_head;
            while (du) {
                snprintf(db[idx], 100, "%-10s %-16s %8.2f %4d  %5.0f%%",
                         du->data.drug_id, du->data.name, du->data.price,
                         du->data.stock_num, du->data.reimbursement_ratio * 100);
                di[idx] = db[idx]; idx++; du = du->next;
            }
            ui_paginate(di, dc, 15, "所有可用药品");
            free((void*)di); free(db);
        }
    }

    /* 智能药品输入:支持缩写自动匹配 Smart drug input: supports abbreviation auto-completion */
    printf("\n");
    {
        int res = smart_drug_input("药品编号 (输缩写自动匹配)", drug_id, sizeof(drug_id));
        if (res != 1 || drug_id[0] == 0) {
            free_drug_list(drug_head);
            free_medical_record_list(record_head);
            free(current_doctor);
            return ERROR_INVALID_INPUT;
        }
    }

    printf("请输入数量: ");
    if (scanf("%d", &quantity) != 1) {
        clear_input_buffer();
        free_drug_list(drug_head);
        free_medical_record_list(record_head);
        free(current_doctor);
        return ERROR_INVALID_INPUT;
    }
    clear_input_buffer();

    /* 查找选定的药品 Find the selected drug */
    current_drug = drug_head;
    while (current_drug) {
        if (strcmp(current_drug->data.drug_id, drug_id) == 0) {
            break;
        }
        current_drug = current_drug->next;
    }

    if (!current_drug) {
        ui_err("药品不存在。");  /* Drug not found */
        free_drug_list(drug_head);
        free_medical_record_list(record_head);
        free(current_doctor);
        return ERROR_NOT_FOUND;
    }

    /* 库存校验:数量必须>0且不超过当前库存 Stock validation: quantity must be > 0 and <= current stock */
    if (quantity <= 0 || current_drug->data.stock_num < quantity) {
        printf("库存不足或数量非法，当前库存: %d\n", current_drug->data.stock_num);
        free_drug_list(drug_head);
        free_medical_record_list(record_head);
        free(current_doctor);
        return ERROR_INVALID_INPUT;
    }

    /* 7天内重复药物风险检查 7-day duplicate prescription risk check */
    if (is_duplicate_prescription_risk(current_record->data.patient_id, drug_id)) {
        int confirm_choice;
        printf("检测到 7 天内存在相同药物记录，是否仍继续开药？\n");  /* Duplicate in 7 days, continue? */
        printf("1. 继续\n");
        printf("0. 取消\n");
        confirm_choice = get_menu_choice(0, 1);
        if (confirm_choice != 1) {
            free_drug_list(drug_head);
            free_medical_record_list(record_head);
            free(current_doctor);
            return SUCCESS;
        }
    }

    /* 构建新处方对象 Build new prescription object */
    memset(&new_prescription, 0, sizeof(new_prescription));
    generate_id(new_prescription.prescription_id, MAX_ID, "PR");  /* 生成"PR"前缀的处方ID */
    strcpy(new_prescription.record_id, current_record->data.record_id);
    strcpy(new_prescription.patient_id, current_record->data.patient_id);
    strcpy(new_prescription.doctor_id, current_doctor->doctor_id);
    strcpy(new_prescription.drug_id, current_drug->data.drug_id);
    new_prescription.quantity = quantity;
    new_prescription.total_price = current_drug->data.price * quantity;  /* 总价 = 单价 x 数量 */
    get_current_time(new_prescription.prescription_date, sizeof(new_prescription.prescription_date));

    /* 在处方链表中追加新节点 Append new node to prescription linked list */
    prescription_head = load_prescriptions_list();
    {
        PrescriptionNode *new_node = create_prescription_node(&new_prescription);
        if (!new_node) {
            free_prescription_list(prescription_head);
            free_drug_list(drug_head);
            free_medical_record_list(record_head);
            free(current_doctor);
            return ERROR_FILE_IO;
        }

        if (!prescription_head) {
            prescription_head = new_node;
        } else {
            PrescriptionNode *tail = prescription_head;
            while (tail->next) {
                tail = tail->next;
            }
            tail->next = new_node;
        }
    }
    /*
     * 事务性保存 (Transactional Save):
     *   先保存药品(扣库存) -> 再保存处方
     *   若任一步失败,执行回滚以保证数据一致性
     *   Save drugs first (deduct stock) -> then save prescription.
     *   Rollback on any failure to ensure data consistency.
     */
    current_drug->data.stock_num -= quantity;  /* 扣减库存 Deduct stock */
    if (save_drugs_list(drug_head) != SUCCESS) {
        /* 库存更新失败,回滚库存 Rollback stock on failure */
        ui_err("库存更新失败，处方未保存!");
        current_drug->data.stock_num += quantity;
        free_prescription_list(prescription_head);
        free_drug_list(drug_head);
        free_medical_record_list(record_head);
        return ERROR_FILE_IO;
    }
    if (save_prescriptions_list(prescription_head) != SUCCESS) {
        /* 处方保存失败,回滚库存 Rollback stock on prescription save failure */
        ui_err("处方保存失败，回滚库存!");
        current_drug->data.stock_num += quantity;
        save_drugs_list(drug_head);  /* 恢复库存 Restore stock */
        free_prescription_list(prescription_head);
        free_drug_list(drug_head);
        free_medical_record_list(record_head);
        return ERROR_FILE_IO;
    }
    free_prescription_list(prescription_head);

    /* 更新病历状态为"治疗中" Update medical record status to "治疗中" (under treatment) */
    strcpy(current_record->data.status, "治疗中");
    save_medical_records_list(record_head);

    /* 计算报销金额,推进患者治疗阶段 Calculate reimbursement, advance patient stage */
    {
        Patient *patient = find_patient_by_id(current_record->data.patient_id);
        float reimbursement = 0.0f;
        float self_pay = new_prescription.total_price;

        if (patient) {
            /* 根据患者类型(普通/医保/军人)和药品报销比例计算报销额
               Calculate reimbursement based on patient type (normal/insurance/military) and drug ratio */
            reimbursement = calculate_drug_reimbursement(&current_drug->data, quantity, patient->patient_type);
            self_pay = new_prescription.total_price - reimbursement;  /* 自费 = 总价 - 报销 */
            if (self_pay < 0.0f) {
                self_pay = 0.0f;
            }
            update_patient_stage(patient->patient_id, "治疗中", patient->is_emergency ? 1 : 0);
            printf("\n处方开具成功!\n");
            printf("处方编号: %s\n", new_prescription.prescription_id);
            printf("药品: %s\n", current_drug->data.name);
            printf("数量: %d\n", quantity);
            printf("总金额: %.2f\n", new_prescription.total_price);
            printf("报销金额: %.2f\n", reimbursement);   /* Reimbursement amount */
            printf("自费金额: %.2f\n", self_pay);         /* Self-pay amount */
            free(patient);
        } else {
            printf("\n处方开具成功!\n");
            printf("处方编号: %s\n", new_prescription.prescription_id);
        }
    }

    /* 库存预警检查:低于预警线时发出警告 Stock warning: alert when below warning threshold */
    if (current_drug->data.stock_num <= current_drug->data.warning_line) {
        printf("警告: 药品 %s 已达到库存预警线。\n", current_drug->data.name);
    }

    free_drug_list(drug_head);
    free_medical_record_list(record_head);
    free(current_doctor);
    return SUCCESS;
}

/* ============================================================================
 * 功能4: 病房被叫提醒 (Ward Call Processing)
 * ============================================================================
 * 医生查看本科室相关的病房呼叫记录,支持:
 *   1. 处理呼叫 — 将"待处理"状态的呼叫标记为"已处理"
 *   2. 发起呼叫 — 医生主动发起病房呼叫(选择病房+患者+说明)
 * Ward call records for the doctor's department, supporting:
 *   1. Handle calls — mark "待处理" (pending) calls as "已处理" (handled)
 *   2. Initiate calls — doctor initiates a ward call (select ward + patient + message)
 * ============================================================================
 */

int doctor_ward_call_menu(const User *current_user) {
    Doctor *current_doctor = find_doctor_by_username(current_user->username);
    WardCallNode *call_head = NULL;
    int found = 0;

    if (!current_doctor) {
        ui_err("医生信息不存在!");
        return ERROR_NOT_FOUND;
    }

    ui_header("病房被叫提醒");
    call_head = load_ward_calls_list();

    /* ===== 分页显示该医生所在科室的病房呼叫记录 =====
             Paginated display of ward call records for this doctor's department */
    {
        WardCallNode *wc = call_head;
        int match = 0;
        while (wc) {
            /* 按科室过滤:只显示医生所属科室的呼叫 Filter by department: only show calls in doctor's department */
            if (strcmp(wc->data.department_id, current_doctor->department_id) == 0) match++;
            wc = wc->next;
        }
        if (match > 0) {
            const char **ci = malloc(match * sizeof(const char *));
            char (*cb)[120] = malloc(match * sizeof(*cb));
            int idx = 0;
            wc = call_head;
            while (wc && idx < match) {
                if (strcmp(wc->data.department_id, current_doctor->department_id) == 0) {
                    char pn[MAX_NAME];
                    print_patient_name_by_id(wc->data.patient_id, pn, sizeof(pn));
                    snprintf(cb[idx], 120, "%-16s %-10s %-10s %-12s %-20s",
                             wc->data.call_id, wc->data.ward_id, pn,
                             wc->data.status, wc->data.create_time);
                    ci[idx] = cb[idx]; idx++;
                    found++;
                }
                wc = wc->next;
            }
            ui_paginate(ci, match, 15, "病房呼叫记录");
            /* 当条目数<=15时,分页后额外显示每条呼叫的消息内容
               When items <= 15, also print message content after pagination */
            if (match <= 15) {
                wc = call_head;
                while (wc) {
                    if (strcmp(wc->data.department_id, current_doctor->department_id) == 0) {
                        printf(C_DIM "  %s: %s" C_RESET "\n", wc->data.call_id, wc->data.message);
                    }
                    wc = wc->next;
                }
            }
            free((void*)ci); free(cb);
        }
    }

    if (!found) {
        ui_warn("当前无病房呼叫记录。");  /* No ward call records */
    }

    printf("\n请选择操作:\n");
    printf("1. 处理呼叫\n");           /* Handle call */
    printf("2. 发起病房呼叫(紧急情况)\n"); /* Initiate ward call (emergency) */
    printf("0. 返回\n");

    {
        int action = get_menu_choice(0, 2);
        if (action == 0) {
            free_ward_call_list(call_head);
            free(current_doctor);
            return SUCCESS;
        }

        if (action == 1) {
            /* 处理呼叫:列出"待处理"状态的呼叫供选择 Handle call: list "待处理" calls for selection */
            {
                int match = 0;
                WardCallNode *wc = call_head;
                while (wc) {
                    if (strcmp(wc->data.department_id, current_doctor->department_id) == 0 &&
                        strcmp(wc->data.status, "待处理") == 0) match++;
                    wc = wc->next;
                }
                if (match == 0) {
                    ui_warn("暂无待处理的呼叫。");  /* No pending calls */
                } else {
                    const char **ci = malloc(match * sizeof(const char *));
                    char (*cb)[120] = malloc(match * sizeof(*cb));
                    int idx = 0;
                    wc = call_head;
                    while (wc && idx < match) {
                        if (strcmp(wc->data.department_id, current_doctor->department_id) == 0 &&
                            strcmp(wc->data.status, "待处理") == 0) {
                            char pn[MAX_NAME];
                            print_patient_name_by_id(wc->data.patient_id, pn, sizeof(pn));
                            snprintf(cb[idx], 120, "%s - %s (%s)", wc->data.call_id, pn, wc->data.message);
                            ci[idx] = cb[idx]; idx++;
                        }
                        wc = wc->next;
                    }
                    int sel = ui_search_list("选择要处理的呼叫", ci, match);
                    free((void*)ci); free(cb);
                    if (sel >= 0) {
                        /* 定位到选中的呼叫,标记为"已处理" Locate selected call, mark as "已处理" */
                        wc = call_head;
                        for (int j = 0; j < sel; j++) wc = wc->next;
                        strcpy(wc->data.status, "已处理");
                        save_ward_calls_list(call_head);
                        ui_ok("病房呼叫已处理。");  /* Ward call handled */
                    }
                }
            }
        } else if (action == 2) {
            /* 发起病房呼叫:医生主动创建呼叫记录
               Initiate ward call: doctor creates a new call record */
            char ward_id[MAX_ID];
            char patient_id[MAX_ID];
            char msg[200];
            WardCall new_call;

            ui_sub_header("发起病房呼叫");

            /* 选择病房 Select ward */
            {
                WardNode *wh = load_wards_list();
                int wc = count_ward_list(wh);
                if (wc == 0) { ui_warn("暂无病房数据。"); free_ward_list(wh); free_ward_call_list(call_head); free(current_doctor); return SUCCESS; }

                const char **wi = malloc(wc * sizeof(const char *));
                char (*wb)[80] = malloc(wc * sizeof(*wb));
                int idx = 0;
                WardNode *wu = wh;
                while (wu) {
                    snprintf(wb[idx], 80, "%s - %s (剩余:%d)", wu->data.ward_id, wu->data.type, wu->data.remain_beds);
                    wi[idx] = wb[idx]; idx++; wu = wu->next;
                }
                int sel = ui_search_list("选择病房", wi, wc);
                free((void*)wi); free(wb);
                if (sel < 0) { free_ward_list(wh); free_ward_call_list(call_head); free(current_doctor); return SUCCESS; }
                wu = wh;
                for (int j = 0; j < sel; j++) wu = wu->next;
                strcpy(ward_id, wu->data.ward_id);
                free_ward_list(wh);
            }

            /* 选择患者 Select patient */
            {
                PatientNode *ph = load_patients_list();
                int pc = count_patient_list(ph);
                if (pc == 0) { free_patient_list(ph); free_ward_call_list(call_head); free(current_doctor); return SUCCESS; }

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
                if (sel < 0) { free_patient_list(ph); free_ward_call_list(call_head); free(current_doctor); return SUCCESS; }
                pu = ph;
                for (int j = 0; j < sel; j++) pu = pu->next;
                strcpy(patient_id, pu->data.patient_id);
                free_patient_list(ph);
            }

            printf("请输入呼叫说明: ");  /* Enter call description */
            if (fgets(msg, sizeof(msg), stdin) == NULL) {
                free_ward_call_list(call_head);
                free(current_doctor);
                return ERROR_INVALID_INPUT;
            }
            msg[strcspn(msg, "\n")] = 0;

            /* 构建新病房呼叫对象 Build new ward call object */
            memset(&new_call, 0, sizeof(new_call));
            generate_id(new_call.call_id, MAX_ID, "WC");  /* 生成"WC"前缀的呼叫ID */
            strcpy(new_call.ward_id, ward_id);
            strcpy(new_call.department_id, current_doctor->department_id);
            strcpy(new_call.patient_id, patient_id);
            strncpy(new_call.message, msg, sizeof(new_call.message) - 1);
            strcpy(new_call.status, "待处理");  /* 初始状态为"待处理" Initial status: pending */
            get_current_time(new_call.create_time, sizeof(new_call.create_time));

            /* 追加到病房呼叫链表 Append to ward call linked list */
            {
                WardCallNode *new_node = create_ward_call_node(&new_call);
                if (!new_node) {
                    free_ward_call_list(call_head);
                    free(current_doctor);
                    return ERROR_FILE_IO;
                }
                if (!call_head) {
                    call_head = new_node;
                } else {
                    WardCallNode *tail = call_head;
                    while (tail->next) { tail = tail->next; }
                    tail->next = new_node;
                }
            }
            save_ward_calls_list(call_head);
            ui_ok("病房呼叫已发起，等待处理。");  /* Ward call initiated, awaiting processing */
        }
    }

    free_ward_call_list(call_head);
    free(current_doctor);
    return SUCCESS;
}

/* ============================================================================
 * 功能5: 紧急病人标识 (Emergency Patient Flagging)
 * ============================================================================
 * 医生将患者标记为急诊状态:
 *   1. 验证患者属于当前医生的负责范围
 *   2. 更新患者is_emergency=true
 *   3. 自动生成病房呼叫记录(如不存在),请求优先安排病房
 * Emergency patient flagging:
 *   1. Verify the patient is within the doctor's responsibility
 *   2. Set patient is_emergency = true
 *   3. Auto-generate ward call record (if not existing) for priority bed assignment
 * ============================================================================
 */

int doctor_emergency_flag_menu(const User *current_user) {
    Doctor *current_doctor = find_doctor_by_username(current_user->username);
    char patient_id[MAX_ID];
    Patient *patient = NULL;

    if (!current_doctor) {
        ui_err("医生信息不存在!");
        return ERROR_NOT_FOUND;
    }

    ui_header("紧急病人标识");
    printf("\n");
    /* 智能患者输入:只显示与当前医生有关系的患者
       Smart patient input: only show patients related to current doctor */
    if (smart_patient_input(current_doctor->doctor_id, "患者编号", patient_id, sizeof(patient_id)) != 1 || patient_id[0] == 0) {
        free(current_doctor);
        return ERROR_INVALID_INPUT;
    }

    /* 权限检查:患者必须与当前医生有关联 Permission check: patient must be related to doctor */
    if (!doctor_has_patient_relation(current_doctor, patient_id)) {
        ui_err("该患者不属于当前医生负责范围。");
        free(current_doctor);
        return ERROR_PERMISSION_DENIED;
    }

    patient = find_patient_by_id(patient_id);
    if (!patient) {
        free(current_doctor);
        return ERROR_NOT_FOUND;
    }

    /* 更新患者为急诊状态 Update patient to emergency status */
    update_patient_stage(patient_id, patient->treatment_stage, 1);

    /* 自动生成病房呼叫:如果不存在该患者的活跃呼叫,则创建
       Auto-generate ward call: if no active call exists for this patient, create one */
    {
        WardCallNode *call_head = load_ward_calls_list();
        WardCallNode *current_call = call_head;
        int exists = 0;

        /* 检查是否已有该患者+科室的非"已处理"呼叫
           Check if non-handled call already exists for this patient + department */
        while (current_call) {
            if (strcmp(current_call->data.patient_id, patient_id) == 0 &&
                strcmp(current_call->data.department_id, current_doctor->department_id) == 0 &&
                strcmp(current_call->data.status, "已处理") != 0) {
                exists = 1;
                break;
            }
            current_call = current_call->next;
        }

        if (!exists) {
            WardCall call;
            memset(&call, 0, sizeof(call));
            generate_id(call.call_id, MAX_ID, "WC");
            strcpy(call.ward_id, "待分配");  /* 病房待分配 Ward pending assignment */
            strcpy(call.department_id, current_doctor->department_id);
            strcpy(call.patient_id, patient_id);
            strcpy(call.message, "紧急病人请求优先安排病房与处理。");  /* Priority ward assignment requested */
            strcpy(call.status, "待处理");
            get_current_time(call.create_time, sizeof(call.create_time));

            /* 追加到病房呼叫链表 Append to ward call linked list */
            {
                WardCallNode *new_node = create_ward_call_node(&call);
                if (!new_node) {
                    free_ward_call_list(call_head);
                    free(patient);
                    free(current_doctor);
                    return ERROR_FILE_IO;
                }

                if (!call_head) {
                    call_head = new_node;
                } else {
                    WardCallNode *tail = call_head;
                    while (tail->next) {
                        tail = tail->next;
                    }
                    tail->next = new_node;
                }
            }
            save_ward_calls_list(call_head);
        }

        free_ward_call_list(call_head);
    }

    printf("已将患者 %s 标记为紧急病人，并同步生成病房优先处理提醒。\n", patient->name);
    free(patient);
    free(current_doctor);
    return SUCCESS;
}

/* ============================================================================
 * 功能6: 治疗进度更新 (Treatment Progress Update)
 * ============================================================================
 * 医生推进患者的治疗阶段:
 *   1. 查看当前阶段和系统推荐的下一阶段
 *   2. 选择"推进到下一阶段"或"标记为已出院"
 *   3. 同步更新患者的treatment_stage和相关病历的状态
 * Treatment progress update by the doctor:
 *   1. View current stage and the system-recommended next stage
 *   2. Choose "advance to next stage" or "mark as discharged"
 *   3. Synchronize updates to patient's treatment_stage and related medical record status
 * ============================================================================
 */

int doctor_update_progress_menu(const User *current_user) {
    Doctor *current_doctor = find_doctor_by_username(current_user->username);
    char patient_id[MAX_ID];
    PatientNode *patient_head = NULL;
    PatientNode *current_patient = NULL;
    MedicalRecordNode *record_head = NULL;
    MedicalRecordNode *current_record = NULL;
    const char *next_stage;
    int choice;

    if (!current_doctor) {
        ui_err("医生信息不存在!");
        return ERROR_NOT_FOUND;
    }

    ui_header("治疗进度更新");
    printf("\n");
    if (smart_patient_input(current_doctor->doctor_id, "患者编号", patient_id, sizeof(patient_id)) != 1 || patient_id[0] == 0) {
        free(current_doctor);
        return ERROR_INVALID_INPUT;
    }

    /* 权限检查 Permission check */
    if (!doctor_has_patient_relation(current_doctor, patient_id)) {
        ui_err("该患者不属于当前医生负责范围。");
        free(current_doctor);
        return ERROR_PERMISSION_DENIED;
    }

    patient_head = load_patients_list();
    current_patient = patient_head;
    while (current_patient) {
        if (strcmp(current_patient->data.patient_id, patient_id) == 0) {
            break;
        }
        current_patient = current_patient->next;
    }

    if (!current_patient) {
        free_patient_list(patient_head);
        free(current_doctor);
        return ERROR_NOT_FOUND;
    }

    /* 获取系统推荐的下一阶段 Get next stage from treatment pipeline */
    next_stage = get_next_stage(current_patient->data.treatment_stage);
    printf("当前阶段: %s\n", current_patient->data.treatment_stage);
    printf("推荐下一阶段: %s\n", next_stage);           /* Recommended next stage */
    printf("1. 推进到下一阶段\n");                      /* Advance to next stage */
    printf("2. 标记为已出院\n");                        /* Mark as discharged */
    printf("0. 返回\n");

    choice = get_menu_choice(0, 2);
    if (choice == 0) {
        free_patient_list(patient_head);
        free(current_doctor);
        return SUCCESS;
    }

    /* 更新患者治疗阶段 Update patient's treatment stage */
    if (choice == 1) {
        strncpy(current_patient->data.treatment_stage, next_stage, sizeof(current_patient->data.treatment_stage) - 1);
        current_patient->data.treatment_stage[sizeof(current_patient->data.treatment_stage) - 1] = '\0';
    } else {
        /* 标记为已出院:同时清除急诊标记 Mark as discharged: also clear emergency flag */
        strcpy(current_patient->data.treatment_stage, "已出院");
        current_patient->data.is_emergency = false;
    }
    save_patients_list(patient_head);
    free_patient_list(patient_head);

    /* 同步更新相关病历记录的状态 Sync update related medical record status */
    record_head = load_medical_records_list();
    current_record = record_head;
    while (current_record) {
        if (strcmp(current_record->data.patient_id, patient_id) == 0 &&
            strcmp(current_record->data.doctor_id, current_doctor->doctor_id) == 0) {
            if (choice == 1) {
                strncpy(current_record->data.status, next_stage, sizeof(current_record->data.status) - 1);
                current_record->data.status[sizeof(current_record->data.status) - 1] = '\0';
            } else {
                strcpy(current_record->data.status, "已完成");  /* Record status: completed */
            }
        }
        current_record = current_record->next;
    }
    save_medical_records_list(record_head);
    free_medical_record_list(record_head);

    ui_ok("治疗进度更新成功。");
    free(current_doctor);
    return SUCCESS;
}

/* ============================================================================
 * 功能7: 模板管理 (Template CRUD Management)
 * ============================================================================
 * 医生管理自己的诊断/治疗/检查快捷模板:
 *   1. 按类别(诊断/治疗/检查)分页浏览现有模板
 *   2. 新增模板 — 选择类别,输入快捷名称和模板内容,自动生成ID
 *   3. 删除模板 — 从列表中选择删除
 * Template CRUD for diagnosis / treatment / exam quick-input templates:
 *   1. Browse existing templates by category (paginated)
 *   2. Create template — choose category, input shortcut name and text, auto-gen ID
 *   3. Delete template — select from list
 * ============================================================================
 */

int doctor_template_menu(const User *current_user) {
    Doctor *current_doctor = find_doctor_by_username(current_user->username);
    if (!current_doctor) { ui_err("医生信息不存在!"); return ERROR_NOT_FOUND; }
    free(current_doctor);

    TemplateNode *head = load_templates_list();
    ui_header("模板管理");

    /* ===== 模板列表 (按类别分页) Template list (paginated by category) ===== */
    {
        /* 内嵌函数:统计指定类别的模板数量 Nested function: count templates by category */
        int cat_match(const char *cat) {
            TemplateNode *c = head; int n = 0;
            while (c) { if (strcmp(c->data.category, cat) == 0) n++; c = c->next; }
            return n;
        }
        /* 内嵌函数:显示指定类别的模板(分页) Nested function: show templates by category (paginated) */
        void show_cat(const char *cat_label) {
            int m = cat_match(cat_label);
            if (m == 0) return;
            const char **ti = malloc(m * sizeof(const char *));
            char (*tb)[120] = malloc(m * sizeof(*tb));
            int idx = 0;
            TemplateNode *c = head;
            while (c && idx < m) {
                if (strcmp(c->data.category, cat_label) == 0) {
                    snprintf(tb[idx], 120, "%-16s %s", c->data.shortcut, c->data.text);
                    ti[idx] = tb[idx]; idx++;
                }
                c = c->next;
            }
            char title[64]; snprintf(title, sizeof(title), "%s", cat_label);
            ui_paginate(ti, m, 15, title);
            free((void*)ti); free(tb);
        }
        /* 按三个类别依次显示 Show by three categories */
        show_cat("诊断");   /* Diagnosis */
        show_cat("治疗");   /* Treatment */
        show_cat("检查");   /* Examination */
    }

    ui_divider();
    ui_menu_item(1, "新增模板");   /* Add template */
    ui_menu_item(2, "删除模板");   /* Delete template */
    ui_menu_exit(0, "返回");

    int choice = get_menu_choice(0, 2);
    if (choice == 0) { free_template_list(head); return SUCCESS; }

    if (choice == 1) {
        /* 新增模板 Create new template */
        MedicalTemplate new_tmpl;
        memset(&new_tmpl, 0, sizeof(new_tmpl));

        ui_sub_header("新增模板");
        printf("  类别: 1.诊断  2.治疗  3.检查\n");  /* Category: 1.Diagnosis 2.Treatment 3.Examination */
        printf(S_LABEL "  请选择: " C_RESET);
        int cat;
        if (scanf("%d", &cat) != 1) { clear_input_buffer(); free_template_list(head); return ERROR_INVALID_INPUT; }
        clear_input_buffer();
        if (cat == 1) strcpy(new_tmpl.category, "诊断");
        else if (cat == 2) strcpy(new_tmpl.category, "治疗");
        else strcpy(new_tmpl.category, "检查");

        printf(S_LABEL "  快捷名称: " C_RESET);  /* Shortcut name */
        if (fgets(new_tmpl.shortcut, sizeof(new_tmpl.shortcut), stdin) == NULL)
            { free_template_list(head); return ERROR_INVALID_INPUT; }
        new_tmpl.shortcut[strcspn(new_tmpl.shortcut, "\n")] = 0;

        printf(S_LABEL "  模板内容: " C_RESET);  /* Template content */
        if (fgets(new_tmpl.text, sizeof(new_tmpl.text), stdin) == NULL)
            { free_template_list(head); return ERROR_INVALID_INPUT; }
        new_tmpl.text[strcspn(new_tmpl.text, "\n")] = 0;

        /* 自动生成模板ID(T001, T002, ...) Auto-generate template ID (T001, T002, ...) */
        int max_id = 0;
        {
            TemplateNode *cur = head;
            while (cur) {
                int id = (int)strtol(cur->data.template_id + 1, NULL, 10);  /* 跳过前缀'S',解析数字 Skip prefix, parse number */
                if (id > max_id) max_id = id;
                cur = cur->next;
            }
        }
        snprintf(new_tmpl.template_id, sizeof(new_tmpl.template_id), "T%03d", max_id + 1);

        TemplateNode *node = create_template_node(&new_tmpl);
        if (!node) { free_template_list(head); return ERROR_FILE_IO; }
        if (!head) head = node;
        else { TemplateNode *t = head; while (t->next) t = t->next; t->next = node; }
        save_templates_list(head);
        ui_ok("模板已添加!");  /* Template added */
    } else if (choice == 2) {
        /* 删除模板 Delete template */
        int tc = 0;
        TemplateNode *tn = head;
        while (tn) { tc++; tn = tn->next; }
        if (tc == 0) { ui_warn("暂无模板。"); free_template_list(head); return SUCCESS; }

        const char **ti = malloc(tc * sizeof(const char *));
        char (*tb)[120] = malloc(tc * sizeof(*tb));
        int idx = 0;
        tn = head;
        while (tn) {
            snprintf(tb[idx], 120, "%s - %s: %s", tn->data.template_id, tn->data.shortcut, tn->data.text);
            ti[idx] = tb[idx]; idx++; tn = tn->next;
        }
        int sel = ui_search_list("选择要删除的模板", ti, tc);
        free((void*)ti); free(tb);
        if (sel < 0) { free_template_list(head); return SUCCESS; }

        /* 链表删除操作 Linked list deletion */
        TemplateNode *prev = NULL, *cur = head;
        for (int j = 0; j < sel; j++) { prev = cur; cur = cur->next; }
        if (!prev) head = cur->next;       /* 删除头节点 Delete head node */
        else prev->next = cur->next;       /* 删除中间/尾节点 Delete middle/tail node */
        free(cur);
        save_templates_list(head);
        ui_ok("模板已删除!");  /* Template deleted */
    }

    free_template_list(head);
    return SUCCESS;
}
