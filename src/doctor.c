#include "doctor.h"
#include "public.h"
#include "ui_utils.h"

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

static int doctor_has_patient_relation(const Doctor *doctor, const char *patient_id) {
    AppointmentNode *appointment_head = load_appointments_list();
    AppointmentNode *current_appointment = appointment_head;
    OnsiteRegistrationQueue onsite_queue = load_onsite_registration_queue();
    OnsiteRegistrationNode *current_onsite = onsite_queue.front;
    MedicalRecordNode *record_head = load_medical_records_list();
    MedicalRecordNode *current_record = record_head;

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

static void adjust_doctor_busy_level(const char *doctor_id, int delta) {
    DoctorNode *doctor_head = load_doctors_list();
    DoctorNode *current_doctor = doctor_head;

    while (current_doctor) {
        if (strcmp(current_doctor->data.doctor_id, doctor_id) == 0) {
            current_doctor->data.busy_level += delta;
            if (current_doctor->data.busy_level < 0) {
                current_doctor->data.busy_level = 0;
            }
            break;
        }
        current_doctor = current_doctor->next;
    }

    save_doctors_list(doctor_head);
    free_doctor_list(doctor_head);
}

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

static int mark_registration_status(const Doctor *doctor, const char *service_id, char *patient_id, char *department_id) {
    AppointmentNode *appointment_head = load_appointments_list();
    AppointmentNode *current_appointment = appointment_head;

    while (current_appointment) {
        if (strcmp(current_appointment->data.appointment_id, service_id) == 0 &&
            strcmp(current_appointment->data.doctor_id, doctor->doctor_id) == 0) {
            strcpy(patient_id, current_appointment->data.patient_id);
            strcpy(department_id, current_appointment->data.department_id);
            strcpy(current_appointment->data.status, "已就诊");
            save_appointments_list(appointment_head);
            free_appointment_list(appointment_head);
            return SUCCESS;
        }
        current_appointment = current_appointment->next;
    }
    free_appointment_list(appointment_head);

    {
        OnsiteRegistrationQueue onsite_queue = load_onsite_registration_queue();
        OnsiteRegistrationNode *current_onsite = onsite_queue.front;

        while (current_onsite) {
            if (strcmp(current_onsite->data.onsite_id, service_id) == 0 &&
                strcmp(current_onsite->data.doctor_id, doctor->doctor_id) == 0) {
                strcpy(patient_id, current_onsite->data.patient_id);
                strcpy(department_id, current_onsite->data.department_id);
                strcpy(current_onsite->data.status, "就诊中");
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

static int upsert_medical_record(const Doctor *doctor, const char *patient_id, const char *service_id, const char *diagnosis_text, const char *status_text, char *record_id_out) {
    MedicalRecordNode *record_head = load_medical_records_list();
    MedicalRecordNode *current_record = record_head;
    MedicalRecord new_record;

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

    memset(&new_record, 0, sizeof(new_record));
    generate_id(new_record.record_id, MAX_ID, "MR");
    strcpy(new_record.patient_id, patient_id);
    strcpy(new_record.doctor_id, doctor->doctor_id);
    strcpy(new_record.appointment_id, service_id);
    strncpy(new_record.diagnosis, diagnosis_text, sizeof(new_record.diagnosis) - 1);
    strncpy(new_record.status, status_text, sizeof(new_record.status) - 1);
    get_current_time(new_record.diagnosis_date, sizeof(new_record.diagnosis_date));

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

static void show_current_doctor_registrations(const Doctor *doctor) {
    AppointmentNode *appointment_head = load_appointments_list();
    AppointmentNode *current_appointment = appointment_head;
    OnsiteRegistrationQueue onsite_queue = load_onsite_registration_queue();
    OnsiteRegistrationNode *current_onsite = onsite_queue.front;
    int found_appt = 0;
    int found_onsite = 0;

    { // paginated: 预约挂号
        int ac = count_appointment_list(appointment_head);
        // first pass: count matching
        int match = 0;
        AppointmentNode *ap = appointment_head;
        while (ap) {
            if (strcmp(ap->data.doctor_id, doctor->doctor_id) == 0) match++;
            ap = ap->next;
        }

        if (match > 0) {
            const char **ai = malloc(match * sizeof(const char *));
            char (*ab)[90] = malloc(match * sizeof(*ab));
            int idx = 0;
            ap = appointment_head;
            while (ap && idx < match) {
                if (strcmp(ap->data.doctor_id, doctor->doctor_id) == 0) {
                    char pn[MAX_NAME];
                    print_patient_name_by_id(ap->data.patient_id, pn, sizeof(pn));
                    snprintf(ab[idx], 90, "%-16s %-12s %-6s %-10s %-10s %-8s",
                             ap->data.appointment_id, ap->data.appointment_date,
                             ap->data.appointment_time, pn,
                             ap->data.department_id, ap->data.status);
                    ai[idx] = ab[idx];
                    idx++;
                    found_appt++;
                }
                ap = ap->next;
            }
            ui_paginate(ai, match, 15, "预约挂号");
            free((void*)ai); free(ab);
        }
    }

    if (!found_appt) {
        printf("暂无预约挂号记录。\n");
    }

    { // paginated: 现场挂号
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
        printf("暂无现场排队记录。\n");
    }

    free_appointment_list(appointment_head);
    free_onsite_registration_queue(&onsite_queue);
}

void doctor_main_menu(const User *current_user) {
    (void)current_user;
    ui_menu_item(1, "被挂号提醒");
    ui_menu_item(2, "接诊处理");
    ui_menu_item(3, "开药功能");
    ui_menu_item(4, "病房被叫提醒");
    ui_menu_item(5, "紧急病人标识");
    ui_menu_item(6, "治疗进度更新");
    ui_menu_item(7, "模板管理");
}

int doctor_appointment_reminder_menu(const User *current_user) {
    Doctor *current_doctor = find_doctor_by_username(current_user->username);

    if (!current_doctor) {
        ui_err("医生信息不存在!");
        return ERROR_NOT_FOUND;
    }

    ui_header("被挂号提醒");
    printf("医生: %s [%s]\n", current_doctor->name, current_doctor->title);
    printf("当前繁忙度: %d", current_doctor->busy_level);
    if (current_doctor->busy_level <= 2) {
        printf(" (较空闲)\n");
    } else if (current_doctor->busy_level <= 5) {
        printf(" (适中)\n");
    } else {
        printf(" (较忙)\n");
    }
    show_current_doctor_registrations(current_doctor);
    free(current_doctor);
    return SUCCESS;
}

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

    // 建议按队列顺序接诊：查找当前医生最早的"排队中"现场患者
    {
        OnsiteRegistrationQueue tmp_queue = load_onsite_registration_queue();
        OnsiteRegistrationNode *tmp_node = tmp_queue.front;
        int has_pending = 0;
        while (tmp_node) {
            if (strcmp(tmp_node->data.doctor_id, current_doctor->doctor_id) == 0 &&
                strcmp(tmp_node->data.status, "排队中") == 0) {
                if (!has_pending) {
                    printf("\n建议接诊(队列顺序): %s (排队号:%d)\n",
                           tmp_node->data.onsite_id, tmp_node->data.queue_number);
                    has_pending = 1;
                }
                break; // 只显示队首第一个
            }
            tmp_node = tmp_node->next;
        }
        if (!has_pending) {
            printf("\n当前无排队中的现场患者。\n");
        }
        free_onsite_registration_queue(&tmp_queue);
    }

    // 构建当前医生接诊候选列表
    {
        const char *svc_list[500];
        int svc_count = 0;
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

    snprintf(final_text, sizeof(final_text), "诊断:%s | 建议:%s | 检查:%s", diagnosis, treatment_advice, exam_items);
    result = upsert_medical_record(current_doctor, patient_id, service_id, final_text, "检查中", record_id);
    if (result != SUCCESS) {
        free(patient);
        free(current_doctor);
        return result;
    }

    update_patient_stage(patient_id, get_next_stage(patient->treatment_stage), patient->is_emergency ? 1 : 0);
    adjust_doctor_busy_level(current_doctor->doctor_id, -1);

    printf("\n接诊完成!\n");
    printf("患者: %s\n", patient->name);
    printf("病历编号: %s\n", record_id);
    printf("当前阶段: %s\n", get_next_stage(patient->treatment_stage));

    free(patient);
    free(current_doctor);
    return SUCCESS;
}

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

    record_head = load_medical_records_list();
    {
        int rc = count_medical_record_list(record_head);
        MedicalRecordNode *mr = record_head;
        // first pass: count matching
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

    // 构建病历候选列表
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

    current_record = record_head;
    while (current_record) {
        if (strcmp(current_record->data.record_id, record_id) == 0 &&
            strcmp(current_record->data.doctor_id, current_doctor->doctor_id) == 0) {
            break;
        }
        current_record = current_record->next;
    }

    if (!current_record) {
        ui_err("病历不存在或不属于当前医生。");
        free_medical_record_list(record_head);
        free(current_doctor);
        return ERROR_NOT_FOUND;
    }

    // ===== 药物推荐：根据患者历史用药推荐 =====
    {
        PrescriptionNode *all_pres = load_prescriptions_list();
        PrescriptionNode *p_iter = all_pres;
        int rec_count = 0;
        char rec_drugs[100][MAX_ID];
        int i;

        memset(rec_drugs, 0, sizeof(rec_drugs));
        while (p_iter) {
            if (strcmp(p_iter->data.patient_id, current_record->data.patient_id) == 0) {
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
                    if (drug->stock_num <= drug->warning_line) printf(" (库存紧张)");
                    printf("\n");
                    free(drug);
                }
            }
        }
    }

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

    current_drug = drug_head;
    while (current_drug) {
        if (strcmp(current_drug->data.drug_id, drug_id) == 0) {
            break;
        }
        current_drug = current_drug->next;
    }

    if (!current_drug) {
        ui_err("药品不存在。");
        free_drug_list(drug_head);
        free_medical_record_list(record_head);
        free(current_doctor);
        return ERROR_NOT_FOUND;
    }

    if (quantity <= 0 || current_drug->data.stock_num < quantity) {
        printf("库存不足或数量非法，当前库存: %d\n", current_drug->data.stock_num);
        free_drug_list(drug_head);
        free_medical_record_list(record_head);
        free(current_doctor);
        return ERROR_INVALID_INPUT;
    }

    if (is_duplicate_prescription_risk(current_record->data.patient_id, drug_id)) {
        int confirm_choice;
        printf("检测到 7 天内存在相同药物记录，是否仍继续开药？\n");
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

    memset(&new_prescription, 0, sizeof(new_prescription));
    generate_id(new_prescription.prescription_id, MAX_ID, "PR");
    strcpy(new_prescription.record_id, current_record->data.record_id);
    strcpy(new_prescription.patient_id, current_record->data.patient_id);
    strcpy(new_prescription.doctor_id, current_doctor->doctor_id);
    strcpy(new_prescription.drug_id, current_drug->data.drug_id);
    new_prescription.quantity = quantity;
    new_prescription.total_price = current_drug->data.price * quantity;
    get_current_time(new_prescription.prescription_date, sizeof(new_prescription.prescription_date));

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
    save_prescriptions_list(prescription_head);
    free_prescription_list(prescription_head);

    current_drug->data.stock_num -= quantity;
    save_drugs_list(drug_head);

    strcpy(current_record->data.status, "治疗中");
    save_medical_records_list(record_head);

    {
        Patient *patient = find_patient_by_id(current_record->data.patient_id);
        float reimbursement = 0.0f;
        float self_pay = new_prescription.total_price;

        if (patient) {
            reimbursement = calculate_drug_reimbursement(&current_drug->data, quantity, patient->patient_type);
            self_pay = new_prescription.total_price - reimbursement;
            if (self_pay < 0.0f) {
                self_pay = 0.0f;
            }
            update_patient_stage(patient->patient_id, "治疗中", patient->is_emergency ? 1 : 0);
            printf("\n处方开具成功!\n");
            printf("处方编号: %s\n", new_prescription.prescription_id);
            printf("药品: %s\n", current_drug->data.name);
            printf("数量: %d\n", quantity);
            printf("总金额: %.2f\n", new_prescription.total_price);
            printf("报销金额: %.2f\n", reimbursement);
            printf("自费金额: %.2f\n", self_pay);
            free(patient);
        } else {
            printf("\n处方开具成功!\n");
            printf("处方编号: %s\n", new_prescription.prescription_id);
        }
    }

    if (current_drug->data.stock_num <= current_drug->data.warning_line) {
        printf("警告: 药品 %s 已达到库存预警线。\n", current_drug->data.name);
    }

    free_drug_list(drug_head);
    free_medical_record_list(record_head);
    free(current_doctor);
    return SUCCESS;
}

int doctor_ward_call_menu(const User *current_user) {
    Doctor *current_doctor = find_doctor_by_username(current_user->username);
    WardCallNode *call_head = NULL;
    WardCallNode *current_call = NULL;
    char call_id[MAX_ID];
    int found = 0;

    if (!current_doctor) {
        ui_err("医生信息不存在!");
        return ERROR_NOT_FOUND;
    }

    ui_header("病房被叫提醒");
    call_head = load_ward_calls_list();

    {
        WardCallNode *wc = call_head;
        int match = 0;
        while (wc) {
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
            // Print messages separately after pagination
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
        ui_warn("当前无病房呼叫记录。");
    }

    printf("\n请选择操作:\n");
    printf("1. 处理呼叫\n");
    printf("2. 发起病房呼叫(紧急情况)\n");
    printf("0. 返回\n");

    {
        int action = get_menu_choice(0, 2);
        if (action == 0) {
            free_ward_call_list(call_head);
            free(current_doctor);
            return SUCCESS;
        }

        if (action == 1) {
            {
                int match = 0;
                WardCallNode *wc = call_head;
                while (wc) {
                    if (strcmp(wc->data.department_id, current_doctor->department_id) == 0 &&
                        strcmp(wc->data.status, "待处理") == 0) match++;
                    wc = wc->next;
                }
                if (match == 0) {
                    ui_warn("暂无待处理的呼叫。");
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
                        wc = call_head;
                        for (int j = 0; j < sel; j++) wc = wc->next;
                        strcpy(wc->data.status, "已处理");
                        save_ward_calls_list(call_head);
                        ui_ok("病房呼叫已处理。");
                    }
                }
            }
        } else if (action == 2) {
            char ward_id[MAX_ID];
            char patient_id[MAX_ID];
            char msg[200];
            WardCall new_call;

            ui_sub_header("发起病房呼叫");

            // 选择病房
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

            // 选择患者
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

            printf("请输入呼叫说明: ");
            if (fgets(msg, sizeof(msg), stdin) == NULL) {
                free_ward_call_list(call_head);
                free(current_doctor);
                return ERROR_INVALID_INPUT;
            }
            msg[strcspn(msg, "\n")] = 0;

            memset(&new_call, 0, sizeof(new_call));
            generate_id(new_call.call_id, MAX_ID, "WC");
            strcpy(new_call.ward_id, ward_id);
            strcpy(new_call.department_id, current_doctor->department_id);
            strcpy(new_call.patient_id, patient_id);
            strncpy(new_call.message, msg, sizeof(new_call.message) - 1);
            strcpy(new_call.status, "待处理");
            get_current_time(new_call.create_time, sizeof(new_call.create_time));

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
            ui_ok("病房呼叫已发起，等待处理。");
        }
    }

    free_ward_call_list(call_head);
    free(current_doctor);
    return SUCCESS;
}

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
    if (smart_patient_input(current_doctor->doctor_id, "患者编号", patient_id, sizeof(patient_id)) != 1 || patient_id[0] == 0) {
        free(current_doctor);
        return ERROR_INVALID_INPUT;
    }

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

    update_patient_stage(patient_id, patient->treatment_stage, 1);

    {
        WardCallNode *call_head = load_ward_calls_list();
        WardCallNode *current_call = call_head;
        int exists = 0;

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
            strcpy(call.ward_id, "待分配");
            strcpy(call.department_id, current_doctor->department_id);
            strcpy(call.patient_id, patient_id);
            strcpy(call.message, "紧急病人请求优先安排病房与处理。");
            strcpy(call.status, "待处理");
            get_current_time(call.create_time, sizeof(call.create_time));

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

    next_stage = get_next_stage(current_patient->data.treatment_stage);
    printf("当前阶段: %s\n", current_patient->data.treatment_stage);
    printf("推荐下一阶段: %s\n", next_stage);
    printf("1. 推进到下一阶段\n");
    printf("2. 标记为已出院\n");
    printf("0. 返回\n");

    choice = get_menu_choice(0, 2);
    if (choice == 0) {
        free_patient_list(patient_head);
        free(current_doctor);
        return SUCCESS;
    }

    if (choice == 1) {
        strncpy(current_patient->data.treatment_stage, next_stage, sizeof(current_patient->data.treatment_stage) - 1);
        current_patient->data.treatment_stage[sizeof(current_patient->data.treatment_stage) - 1] = '\0';
    } else {
        strcpy(current_patient->data.treatment_stage, "已出院");
        current_patient->data.is_emergency = false;
    }
    save_patients_list(patient_head);
    free_patient_list(patient_head);

    record_head = load_medical_records_list();
    current_record = record_head;
    while (current_record) {
        if (strcmp(current_record->data.patient_id, patient_id) == 0 &&
            strcmp(current_record->data.doctor_id, current_doctor->doctor_id) == 0) {
            if (choice == 1) {
                strncpy(current_record->data.status, next_stage, sizeof(current_record->data.status) - 1);
                current_record->data.status[sizeof(current_record->data.status) - 1] = '\0';
            } else {
                strcpy(current_record->data.status, "已完成");
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

int doctor_template_menu(const User *current_user) {
    Doctor *current_doctor = find_doctor_by_username(current_user->username);
    if (!current_doctor) { ui_err("医生信息不存在!"); return ERROR_NOT_FOUND; }
    free(current_doctor);

    TemplateNode *head = load_templates_list();
    ui_header("模板管理");

    // ---- 模板列表 paginated ----
    {
        int cat_match(const char *cat) {
            TemplateNode *c = head; int n = 0;
            while (c) { if (strcmp(c->data.category, cat) == 0) n++; c = c->next; }
            return n;
        }
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
        show_cat("诊断");
        show_cat("治疗");
        show_cat("检查");
    }

    ui_divider();
    ui_menu_item(1, "新增模板");
    ui_menu_item(2, "删除模板");
    ui_menu_exit(0, "返回");

    int choice = get_menu_choice(0, 2);
    if (choice == 0) { free_template_list(head); return SUCCESS; }

    if (choice == 1) {
        MedicalTemplate new_tmpl;
        memset(&new_tmpl, 0, sizeof(new_tmpl));

        ui_sub_header("新增模板");
        printf("  类别: 1.诊断  2.治疗  3.检查\n");
        printf(S_LABEL "  请选择: " C_RESET);
        int cat;
        if (scanf("%d", &cat) != 1) { clear_input_buffer(); free_template_list(head); return ERROR_INVALID_INPUT; }
        clear_input_buffer();
        if (cat == 1) strcpy(new_tmpl.category, "诊断");
        else if (cat == 2) strcpy(new_tmpl.category, "治疗");
        else strcpy(new_tmpl.category, "检查");

        printf(S_LABEL "  快捷名称: " C_RESET);
        if (fgets(new_tmpl.shortcut, sizeof(new_tmpl.shortcut), stdin) == NULL)
            { free_template_list(head); return ERROR_INVALID_INPUT; }
        new_tmpl.shortcut[strcspn(new_tmpl.shortcut, "\n")] = 0;

        printf(S_LABEL "  模板内容: " C_RESET);
        if (fgets(new_tmpl.text, sizeof(new_tmpl.text), stdin) == NULL)
            { free_template_list(head); return ERROR_INVALID_INPUT; }
        new_tmpl.text[strcspn(new_tmpl.text, "\n")] = 0;

        int max_id = 0;
        {
            TemplateNode *cur = head;
            while (cur) {
                int id = (int)strtol(cur->data.template_id + 1, NULL, 10);
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
        ui_ok("模板已添加!");
    } else if (choice == 2) {
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

        TemplateNode *prev = NULL, *cur = head;
        for (int j = 0; j < sel; j++) { prev = cur; cur = cur->next; }
        if (!prev) head = cur->next;
        else prev->next = cur->next;
        free(cur);
        save_templates_list(head);
        ui_ok("模板已删除!");
    }

    free_template_list(head);
    return SUCCESS;
}
