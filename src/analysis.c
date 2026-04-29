#include "analysis.h"
#include "data_storage.h"
#include "public.h"
#include "ui_utils.h"

// =======================  辅助结构 =======================

typedef struct {
    char key[MAX_ID];
    char name[MAX_NAME];
    float amount;
    int count;
    float reimbursement;
} AccItem;

typedef struct {
    char month[8];
    int appt_count;
    int onsite_count;
} MonthAcc;

typedef struct {
    char doctor_id[MAX_ID];
    char name[MAX_NAME];
    char dept_name[MAX_NAME];
    char title[50];
    int appt_count;
    int record_count;
    int busy_level;
} DoctorLoad;

// =======================  辅助函数 =======================

static int analysis_get_month_filter(char *out_buf, size_t buf_size) {
    char input[16];

    out_buf[0] = '\0';
    printf(S_LABEL "  按月筛选? (y/n, 直接回车看全部): " C_RESET);
    fflush(stdout);
    if (fgets(input, sizeof(input), stdin) == NULL) return 0;
    input[strcspn(input, "\n")] = 0;

    if (input[0] == 'y' || input[0] == 'Y') {
        printf(S_LABEL "  输入年月 (如 2026-04): " C_RESET);
        if (fgets(out_buf, (int)buf_size, stdin) == NULL) { out_buf[0] = '\0'; return 0; }
        out_buf[strcspn(out_buf, "\n")] = 0;
    }

    return (int)strlen(out_buf);
}

static int add_or_update_item(AccItem *items, int *count, int max,
                              const char *key, const char *name,
                              float amount, int qty, float reimbursement) {
    for (int i = 0; i < *count; i++) {
        if (strcmp(items[i].key, key) == 0) {
            items[i].amount += amount;
            items[i].count += qty;
            items[i].reimbursement += reimbursement;
            return i;
        }
    }
    if (*count >= max) return -1;
    strncpy(items[*count].key, key, sizeof(items[0].key) - 1);
    items[*count].key[sizeof(items[0].key) - 1] = '\0';
    strncpy(items[*count].name, name, sizeof(items[0].name) - 1);
    items[*count].name[sizeof(items[0].name) - 1] = '\0';
    items[*count].amount = amount;
    items[*count].count = qty;
    items[*count].reimbursement = reimbursement;
    (*count)++;
    return *count - 1;
}

static int compare_amount_desc(const void *a, const void *b) {
    const AccItem *pa = (const AccItem *)a;
    const AccItem *pb = (const AccItem *)b;
    if (pb->amount > pa->amount) return 1;
    if (pb->amount < pa->amount) return -1;
    return 0;
}

static int compare_count_desc(const void *a, const void *b) {
    const AccItem *pa = (const AccItem *)a;
    const AccItem *pb = (const AccItem *)b;
    return pb->count - pa->count;
}

static int compare_doctor_load_desc(const void *a, const void *b) {
    const DoctorLoad *pa = (const DoctorLoad *)a;
    const DoctorLoad *pb = (const DoctorLoad *)b;
    return pb->appt_count - pa->appt_count;
}

static int compare_month(const void *a, const void *b) {
    return strcmp(((const MonthAcc *)a)->month, ((const MonthAcc *)b)->month);
}

static void calculate_prev_month(const char *current, char *out) {
    int year, month;
    if (sscanf(current, "%d-%d", &year, &month) != 2) {
        strcpy(out, current);
        return;
    }
    month--;
    if (month < 1) { month = 12; year--; }
    snprintf(out, 8, "%04d-%02d", year, month);
}

static bool str_starts_with(const char *str, const char *prefix) {
    size_t plen = strlen(prefix);
    return plen > 0 && strncmp(str, prefix, plen) == 0;
}

static const char* busy_level_label(int level) {
    if (level <= 3) return "较清闲";
    if (level <= 6) return "正常";
    if (level <= 8) return "较忙";
    return "繁忙";
}

// =======================  1. 经营概览 =======================

static void analysis_operations_overview(const char *month_filter) {
    size_t mlen = strlen(month_filter);
    char today_str[16] = "";
    // Get today's date (YYYY-MM-DD)
    {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        strftime(today_str, sizeof(today_str), "%Y-%m-%d", tm_info);
    }

    AppointmentNode *ap_head = load_appointments_list();
    OnsiteRegistrationQueue onsite_queue = load_onsite_registration_queue();
    WardNode *ward_head = load_wards_list();
    PrescriptionNode *pres_head = load_prescriptions_list();
    DrugNode *drug_head = load_drugs_list();

    ui_sub_header("经营概览");
    if (mlen > 0) printf(C_DIM "  (%s 数据)\n" C_RESET, month_filter);

    // --- 今日预约与挂号 ---
    int today_appt = 0, today_onsite = 0;
    {
        AppointmentNode *ap = ap_head;
        while (ap) {
            if ((mlen == 0 || str_starts_with(ap->data.create_time, month_filter)) &&
                str_starts_with(ap->data.appointment_date, today_str)) {
                today_appt++;
            }
            ap = ap->next;
        }
    }
    {
        OnsiteRegistrationNode *on = onsite_queue.front;
        while (on) {
            if ((mlen == 0 || str_starts_with(on->data.create_time, month_filter)) &&
                str_starts_with(on->data.create_time, today_str)) {
                today_onsite++;
            }
            on = on->next;
        }
    }

    printf("\n  " C_BOLD "今日门诊" C_RESET "\n");
    printf("  %-16s %d\n", "预约挂号:", today_appt);
    printf("  %-16s %d\n", "现场挂号:", today_onsite);

    // --- 今日收入 ---
    float today_revenue = 0.0f, today_reimb = 0.0f;
    {
        PrescriptionNode *pr = pres_head;
        while (pr) {
            if (str_starts_with(pr->data.prescription_date, today_str)) {
                if (mlen == 0 || str_starts_with(pr->data.prescription_date, month_filter)) {
                    today_revenue += pr->data.total_price;
                    Drug *drug = find_drug_by_id(pr->data.drug_id);
                    Patient *patient = find_patient_by_id(pr->data.patient_id);
                    if (drug && patient) {
                        today_reimb += calculate_drug_reimbursement(drug, pr->data.quantity, patient->patient_type);
                    }
                    free(drug);
                    free(patient);
                }
            }
            pr = pr->next;
        }
    }
    printf("  %-16s %.2f\n", "处方总额:", today_revenue);
    printf("  %-16s %.2f\n", "报销金额:", today_reimb);
    printf("  %-16s %.2f\n", "实际收入:", today_revenue - today_reimb);

    // --- 床位使用率 ---
    int total_beds = 0, remain_beds = 0;
    {
        WardNode *w = ward_head;
        while (w) {
            total_beds += w->data.total_beds;
            remain_beds += w->data.remain_beds;
            w = w->next;
        }
    }
    int occupied = total_beds - remain_beds;
    float usage_rate = (total_beds > 0) ? (occupied * 100.0f / total_beds) : 0.0f;

    printf("\n  " C_BOLD "床位使用" C_RESET "\n");
    printf("  %-16s %d / %d\n", "床位:", occupied, total_beds);
    if (usage_rate > 90.0f) {
        printf("  %-16s " C_BOLD C_YELLOW "%.1f%%" C_RESET "\n", "使用率:", usage_rate);
    } else {
        printf("  %-16s %.1f%%\n", "使用率:", usage_rate);
    }

    // --- 药物消耗 TOP10 ---
    printf("\n  " C_BOLD "药物消耗 TOP10" C_RESET "\n");
    {
        AccItem drugs[256];
        int drug_count = 0;

        PrescriptionNode *pr = pres_head;
        while (pr) {
            if (mlen == 0 || str_starts_with(pr->data.prescription_date, month_filter)) {
                // Resolve drug name from pre-loaded drug list
                const char *dname = pr->data.drug_id;
                DrugNode *d = drug_head;
                while (d) {
                    if (strcmp(d->data.drug_id, pr->data.drug_id) == 0) {
                        dname = d->data.name;
                        break;
                    }
                    d = d->next;
                }
                add_or_update_item(drugs, &drug_count, 256,
                                   pr->data.drug_id, dname,
                                   pr->data.total_price, pr->data.quantity, 0);
            }
            pr = pr->next;
        }

        qsort(drugs, drug_count, sizeof(AccItem), compare_count_desc);

        if (drug_count == 0) {
            printf("  " C_DIM "暂无数据" C_RESET "\n");
        } else {
            int top = drug_count > 10 ? 10 : drug_count;
            printf("  %-6s  %-18s  %6s  %8s\n", "排名", "药品名", "用量", "金额");
            for (int i = 0; i < top; i++) {
                printf("  %-4d   ", i + 1);
                ui_print_col(drugs[i].name, 18);
                printf("  %4d    %8.2f\n", drugs[i].count, drugs[i].amount);
            }
        }
    }

    free_drug_list(drug_head);
    free_prescription_list(pres_head);
    free_ward_list(ward_head);
    free_onsite_registration_queue(&onsite_queue);
    free_appointment_list(ap_head);
}

// =======================  2. 趋势分析 =======================

static void analysis_trend_trends(const char *month_filter) {
    size_t mlen = strlen(month_filter);
    AppointmentNode *ap_head = load_appointments_list();
    OnsiteRegistrationQueue onsite_queue = load_onsite_registration_queue();

    ui_sub_header("趋势分析");

    // --- 月度就诊量 ---
    {
        MonthAcc months[48];
        int month_count = 0;

        AppointmentNode *ap = ap_head;
        while (ap) {
            if (ap->data.create_time[0]) {
                char mkey[8];
                strncpy(mkey, ap->data.create_time, 7);
                mkey[7] = '\0';
                add_or_update_item((AccItem *)months, &month_count, 48, mkey, "", 0, 0, 0);
                for (int i = 0; i < month_count; i++) {
                    if (strcmp(months[i].month, mkey) == 0) {
                        months[i].appt_count++;
                        break;
                    }
                }
            }
            ap = ap->next;
        }

        OnsiteRegistrationNode *on = onsite_queue.front;
        while (on) {
            if (on->data.create_time[0]) {
                char mkey[8];
                strncpy(mkey, on->data.create_time, 7);
                mkey[7] = '\0';
                bool found = false;
                for (int i = 0; i < month_count; i++) {
                    if (strcmp(months[i].month, mkey) == 0) {
                        months[i].onsite_count++;
                        found = true;
                        break;
                    }
                }
                if (!found && month_count < 48) {
                    strncpy(months[month_count].month, mkey, 7);
                    months[month_count].month[7] = '\0';
                    months[month_count].appt_count = 0;
                    months[month_count].onsite_count = 1;
                    month_count++;
                }
            }
            on = on->next;
        }

        qsort(months, month_count, sizeof(MonthAcc), compare_month);

        printf("\n  " C_BOLD "月度就诊量" C_RESET "\n");
        printf("  %-10s %6s %6s %6s\n", "月份", "预约", "现场", "合计");
        for (int i = 0; i < month_count; i++) {
            int total = months[i].appt_count + months[i].onsite_count;
            printf("  %-10s %6d %6d %6d\n", months[i].month, months[i].appt_count, months[i].onsite_count, total);
        }
    }

    // --- 周度分布 (当有月份筛选时) ---
    if (mlen > 0) {
        int weekly[4] = {0, 0, 0, 0};

        AppointmentNode *ap = ap_head;
        while (ap) {
            if (str_starts_with(ap->data.create_time, month_filter)) {
                const char *day_str = ap->data.create_time + 8;
                int day = atoi(day_str);
                int week_idx = (day - 1) / 7;
                if (week_idx > 3) week_idx = 3;
                weekly[week_idx]++;
            }
            ap = ap->next;
        }

        OnsiteRegistrationNode *on = onsite_queue.front;
        while (on) {
            if (str_starts_with(on->data.create_time, month_filter)) {
                const char *day_str = on->data.create_time + 8;
                int day = atoi(day_str);
                int week_idx = (day - 1) / 7;
                if (week_idx > 3) week_idx = 3;
                weekly[week_idx]++;
            }
            on = on->next;
        }

        printf("\n  " C_BOLD "周度分布 (%s)" C_RESET "\n", month_filter);
        const char *week_labels[] = {"第1周(1-7日)", "第2周(8-14日)", "第3周(15-21日)", "第4周(22日+)"};
        int max_w = 0;
        for (int i = 0; i < 4; i++) if (weekly[i] > max_w) max_w = weekly[i];
        for (int i = 0; i < 4; i++) {
            printf("  %s: %3d  ", week_labels[i], weekly[i]);
            int bar = max_w > 0 ? (weekly[i] * 30 / max_w) : 0;
            for (int j = 0; j < bar && j < 30; j++) printf("*");
            printf("\n");
        }
    }

    // --- 药物消耗趋势 (有月份筛选时对比上月) ---
    if (mlen > 0) {
        char prev_month[8];
        calculate_prev_month(month_filter, prev_month);

        DrugNode *drug_head = load_drugs_list();
        PrescriptionNode *pres_head = load_prescriptions_list();

        AccItem curr_items[256], prev_items[256];
        int curr_count = 0, prev_count = 0;

        PrescriptionNode *pr = pres_head;
        while (pr) {
            DrugNode *d = drug_head;
            const char *dname = pr->data.drug_id;
            while (d) {
                if (strcmp(d->data.drug_id, pr->data.drug_id) == 0) { dname = d->data.name; break; }
                d = d->next;
            }
            if (str_starts_with(pr->data.prescription_date, month_filter)) {
                add_or_update_item(curr_items, &curr_count, 256, pr->data.drug_id, dname,
                                   pr->data.total_price, pr->data.quantity, 0);
            } else if (str_starts_with(pr->data.prescription_date, prev_month)) {
                add_or_update_item(prev_items, &prev_count, 256, pr->data.drug_id, dname,
                                   pr->data.total_price, pr->data.quantity, 0);
            }
            pr = pr->next;
        }

        qsort(curr_items, curr_count, sizeof(AccItem), compare_count_desc);

        printf("\n  " C_BOLD "药物消耗趋势 (%s vs %s)" C_RESET "\n", month_filter, prev_month);
        printf("  %-18s %8s %8s %s\n", "药品名", "当前用量", "上月用量", "变化");
        int top = curr_count > 5 ? 5 : curr_count;
        for (int i = 0; i < top; i++) {
            int prev_qty = 0;
            for (int j = 0; j < prev_count; j++) {
                if (strcmp(curr_items[i].key, prev_items[j].key) == 0) {
                    prev_qty = prev_items[j].count;
                    break;
                }
            }
            const char *arrow;
            if (prev_qty == 0 && curr_items[i].count > 0) arrow = "  NEW";
            else if (prev_qty > 0 && curr_items[i].count > prev_qty) arrow = "  ↑";
            else if (prev_qty > 0 && curr_items[i].count < prev_qty) arrow = "  ↓";
            else arrow = "  →";
            printf("  ");
            ui_print_col(curr_items[i].name, 18);
            printf("  %6d  %6d  %s\n", curr_items[i].count, prev_qty, arrow);
        }

        free_prescription_list(pres_head);
        free_drug_list(drug_head);
    }

    free_onsite_registration_queue(&onsite_queue);
    free_appointment_list(ap_head);
}

// =======================  3. 医生负载 =======================

static void analysis_doctor_load(const char *month_filter) {
    size_t mlen = strlen(month_filter);
    DoctorNode *doc_head = load_doctors_list();
    AppointmentNode *ap_head = load_appointments_list();
    MedicalRecordNode *rec_head = load_medical_records_list();
    DepartmentNode *dept_head = load_departments_list();

    ui_sub_header("医生负载");
    if (mlen > 0) printf(C_DIM "  (%s 数据)\n" C_RESET, month_filter);

    int doc_count = count_doctor_list(doc_head);
    if (doc_count == 0) {
        ui_warn("暂无医生数据。");
        free_department_list(dept_head);
        free_medical_record_list(rec_head);
        free_appointment_list(ap_head);
        free_doctor_list(doc_head);
        return;
    }

    DoctorLoad *loads = malloc(doc_count * sizeof(DoctorLoad));
    int idx = 0;
    DoctorNode *d = doc_head;
    while (d) {
        memset(&loads[idx], 0, sizeof(DoctorLoad));
        strncpy(loads[idx].doctor_id, d->data.doctor_id, sizeof(loads[idx].doctor_id) - 1);
        strncpy(loads[idx].name, d->data.name, sizeof(loads[idx].name) - 1);
        strncpy(loads[idx].title, d->data.title, sizeof(loads[idx].title) - 1);
        loads[idx].busy_level = d->data.busy_level;

        // Resolve department name
        DepartmentNode *dp = dept_head;
        while (dp) {
            if (strcmp(dp->data.department_id, d->data.department_id) == 0) {
                strncpy(loads[idx].dept_name, dp->data.name, sizeof(loads[idx].dept_name) - 1);
                break;
            }
            dp = dp->next;
        }
        if (loads[idx].dept_name[0] == '\0') {
            strncpy(loads[idx].dept_name, d->data.department_id, sizeof(loads[idx].dept_name) - 1);
        }

        idx++;
        d = d->next;
    }

    // Count appointments per doctor
    AppointmentNode *ap = ap_head;
    while (ap) {
        if (mlen == 0 || str_starts_with(ap->data.create_time, month_filter)) {
            for (int i = 0; i < doc_count; i++) {
                if (strcmp(loads[i].doctor_id, ap->data.doctor_id) == 0) {
                    loads[i].appt_count++;
                    break;
                }
            }
        }
        ap = ap->next;
    }

    // Count medical records per doctor
    MedicalRecordNode *mr = rec_head;
    while (mr) {
        if (mlen == 0 || str_starts_with(mr->data.diagnosis_date, month_filter)) {
            for (int i = 0; i < doc_count; i++) {
                if (strcmp(loads[i].doctor_id, mr->data.doctor_id) == 0) {
                    loads[i].record_count++;
                    break;
                }
            }
        }
        mr = mr->next;
    }

    qsort(loads, doc_count, sizeof(DoctorLoad), compare_doctor_load_desc);

    printf("\n  %-12s %-16s %-8s %-8s %-8s %s\n", "医生", "科室", "预约量", "病历数", "繁忙度", "状态");
    for (int i = 0; i < doc_count; i++) {
        printf("  ");
        ui_print_col(loads[i].name, 12);
        ui_print_col(loads[i].dept_name, 16);
        ui_print_col_int(loads[i].appt_count, 8);
        ui_print_col_int(loads[i].record_count, 8);
        ui_print_col_int(loads[i].busy_level, 8);
        printf(" %s\n", busy_level_label(loads[i].busy_level));
    }

    free(loads);
    free_department_list(dept_head);
    free_medical_record_list(rec_head);
    free_appointment_list(ap_head);
    free_doctor_list(doc_head);
}

// =======================  4. 患者画像 =======================

static void analysis_patient_profile(const char *month_filter) {
    size_t mlen = strlen(month_filter);
    PatientNode *pat_head = load_patients_list();
    AppointmentNode *ap_head = load_appointments_list();
    OnsiteRegistrationQueue onsite_queue = load_onsite_registration_queue();
    MedicalRecordNode *rec_head = load_medical_records_list();

    ui_sub_header("患者画像");
    if (mlen > 0) printf(C_DIM "  (%s 数据)\n" C_RESET, month_filter);

    // Collect unique active patients (from appointments and onsite)
    typedef struct {
        char patient_id[MAX_ID];
        bool is_emergency;
        char patient_type[20];
        char treatment_stage[20];
    } PatientInfo;

    PatientInfo active[1000];
    int active_count = 0;

    AppointmentNode *ap = ap_head;
    while (ap) {
        if (mlen == 0 || str_starts_with(ap->data.create_time, month_filter)) {
            bool found = false;
            for (int i = 0; i < active_count; i++) {
                if (strcmp(active[i].patient_id, ap->data.patient_id) == 0) { found = true; break; }
            }
            if (!found && active_count < 1000) {
                strncpy(active[active_count].patient_id, ap->data.patient_id, MAX_ID - 1);
                // Look up patient details
                PatientNode *p = pat_head;
                while (p) {
                    if (strcmp(p->data.patient_id, ap->data.patient_id) == 0) {
                        active[active_count].is_emergency = p->data.is_emergency;
                        strncpy(active[active_count].patient_type, p->data.patient_type, sizeof(active[0].patient_type) - 1);
                        strncpy(active[active_count].treatment_stage, p->data.treatment_stage, sizeof(active[0].treatment_stage) - 1);
                        break;
                    }
                    p = p->next;
                }
                active_count++;
            }
        }
        ap = ap->next;
    }

    // Also check onsite registrations
    OnsiteRegistrationNode *on = onsite_queue.front;
    while (on) {
        if (mlen == 0 || str_starts_with(on->data.create_time, month_filter)) {
            bool found = false;
            for (int i = 0; i < active_count; i++) {
                if (strcmp(active[i].patient_id, on->data.patient_id) == 0) { found = true; break; }
            }
            if (!found && active_count < 1000) {
                strncpy(active[active_count].patient_id, on->data.patient_id, MAX_ID - 1);
                PatientNode *p = pat_head;
                while (p) {
                    if (strcmp(p->data.patient_id, on->data.patient_id) == 0) {
                        active[active_count].is_emergency = p->data.is_emergency;
                        strncpy(active[active_count].patient_type, p->data.patient_type, sizeof(active[0].patient_type) - 1);
                        strncpy(active[active_count].treatment_stage, p->data.treatment_stage, sizeof(active[0].treatment_stage) - 1);
                        break;
                    }
                    p = p->next;
                }
                active_count++;
            }
        }
        on = on->next;
    }

    if (active_count == 0) {
        ui_warn("筛选期内无活跃患者数据。");
        free_medical_record_list(rec_head);
        free_onsite_registration_queue(&onsite_queue);
        free_appointment_list(ap_head);
        free_patient_list(pat_head);
        return;
    }

    printf("  %-20s %d\n", "活跃患者数:", active_count);

    // Patient type distribution
    int normal = 0, insured = 0, military = 0;
    int emergency = 0;
    int stage_counts[5] = {0};
    const char *stage_names[] = {"初诊", "检查中", "治疗中", "康复观察", "已出院"};

    for (int i = 0; i < active_count; i++) {
        if (active[i].is_emergency) emergency++;

        if (strcmp(active[i].patient_type, "医保") == 0) insured++;
        else if (strcmp(active[i].patient_type, "军人") == 0) military++;
        else normal++;

        for (int s = 0; s < 5; s++) {
            if (strcmp(active[i].treatment_stage, stage_names[s]) == 0) {
                stage_counts[s]++;
                break;
            }
        }
    }

    int type_total = normal + insured + military;

    printf("\n  " C_BOLD "患者类型分布" C_RESET "\n");
    printf("  %-16s %4d (%.1f%%)\n", "普通:", normal, type_total > 0 ? normal * 100.0f / type_total : 0);
    printf("  %-16s %4d (%.1f%%)\n", "医保:", insured, type_total > 0 ? insured * 100.0f / type_total : 0);
    printf("  %-16s %4d (%.1f%%)\n", "军人:", military, type_total > 0 ? military * 100.0f / type_total : 0);

    printf("\n  " C_BOLD "急诊比例" C_RESET "\n");
    printf("  %-16s %d / %d (%.1f%%)\n", "急诊:", emergency, active_count,
           active_count > 0 ? emergency * 100.0f / active_count : 0);

    printf("\n  " C_BOLD "治疗阶段分布" C_RESET "\n");
    for (int s = 0; s < 5; s++) {
        if (stage_counts[s] > 0) {
            printf("  %-16s %d\n", stage_names[s], stage_counts[s]);
        }
    }

    free_medical_record_list(rec_head);
    free_onsite_registration_queue(&onsite_queue);
    free_appointment_list(ap_head);
    free_patient_list(pat_head);
}

// =======================  5. 财务统计 =======================

static void analysis_financial_stats(const char *month_filter) {
    size_t mlen = strlen(month_filter);
    PrescriptionNode *pres_head = load_prescriptions_list();
    DoctorNode *doc_head = load_doctors_list();
    DepartmentNode *dept_head = load_departments_list();

    ui_sub_header("财务统计");
    if (mlen > 0) printf(C_DIM "  (%s 数据)\n" C_RESET, month_filter);

    // --- 全局财务汇总 ---
    float total_amount = 0.0f, total_reimb = 0.0f;
    int total_pres = 0;

    // Per-department accumulation
    AccItem dept_items[100];
    int dept_count = 0;

    // Per-month accumulation (when no filter)
    MonthAcc monthly_revenue[48];
    int month_rev_count = 0;

    PrescriptionNode *pr = pres_head;
    while (pr) {
        if (mlen == 0 || str_starts_with(pr->data.prescription_date, month_filter)) {
            total_pres++;
            total_amount += pr->data.total_price;

            // Compute reimbursement
            Drug *drug = find_drug_by_id(pr->data.drug_id);
            Patient *patient = find_patient_by_id(pr->data.patient_id);
            float reimb = 0;
            if (drug && patient) {
                reimb = calculate_drug_reimbursement(drug, pr->data.quantity, patient->patient_type);
            }
            free(drug);
            free(patient);
            total_reimb += reimb;

            // Per-department: find doctor -> department
            const char *dept_id = "";
            const char *dept_name = "";
            DoctorNode *d = doc_head;
            while (d) {
                if (strcmp(d->data.doctor_id, pr->data.doctor_id) == 0) {
                    dept_id = d->data.department_id;
                    DepartmentNode *dp = dept_head;
                    while (dp) {
                        if (strcmp(dp->data.department_id, dept_id) == 0) {
                            dept_name = dp->data.name;
                            break;
                        }
                        dp = dp->next;
                    }
                    break;
                }
                d = d->next;
            }
            if (dept_id[0] == '\0') dept_id = "未知";
            if (dept_name[0] == '\0') dept_name = dept_id;
            add_or_update_item(dept_items, &dept_count, 100, dept_id, dept_name,
                               pr->data.total_price, 1, reimb);

            // Per-month
            if (mlen == 0 && pr->data.prescription_date[0]) {
                char mkey[8];
                strncpy(mkey, pr->data.prescription_date, 7);
                mkey[7] = '\0';
                bool found = false;
                for (int i = 0; i < month_rev_count; i++) {
                    if (strcmp(monthly_revenue[i].month, mkey) == 0) {
                        monthly_revenue[i].appt_count++;
                        monthly_revenue[i].onsite_count += (int)pr->data.total_price;
                        found = true;
                        break;
                    }
                }
                if (!found && month_rev_count < 48) {
                    strncpy(monthly_revenue[month_rev_count].month, mkey, 7);
                    monthly_revenue[month_rev_count].month[7] = '\0';
                    monthly_revenue[month_rev_count].appt_count = 1;
                    monthly_revenue[month_rev_count].onsite_count = (int)pr->data.total_price;
                    month_rev_count++;
                }
            }
        }
        pr = pr->next;
    }

    printf("\n  " C_BOLD "费用汇总" C_RESET "\n");
    printf("  %-16s %d\n", "处方总数:", total_pres);
    printf("  %-16s %.2f\n", "总金额:", total_amount);
    printf("  %-16s %.2f\n", "报销金额:", total_reimb);
    printf("  %-16s %.2f\n", "实际收入:", total_amount - total_reimb);

    // --- 科室收入排名 ---
    qsort(dept_items, dept_count, sizeof(AccItem), compare_amount_desc);

    printf("\n  " C_BOLD "科室收入排名" C_RESET "\n");
    printf("  %-4s %-14s %6s %10s %10s %10s\n", "排名", "科室", "处方数", "总收入", "报销", "实际收入");
    for (int i = 0; i < dept_count; i++) {
        printf("  %-3d  ", i + 1);
        ui_print_col(dept_items[i].name, 14);
        printf(" %5d  ", dept_items[i].count);
        printf("%8.2f  ", dept_items[i].amount);
        printf("%8.2f  ", dept_items[i].reimbursement);
        printf("%8.2f\n", dept_items[i].amount - dept_items[i].reimbursement);
    }

    // --- 月度收入汇总 (无筛选时) ---
    if (mlen == 0 && month_rev_count > 0) {
        qsort(monthly_revenue, month_rev_count, sizeof(MonthAcc), compare_month);
        printf("\n  " C_BOLD "月度收入汇总" C_RESET "\n");
        printf("  %-10s %6s %10s\n", "月份", "处方数", "收入");
        for (int i = 0; i < month_rev_count; i++) {
            printf("  %-10s %6d %10.2f\n",
                   monthly_revenue[i].month,
                   monthly_revenue[i].appt_count,
                   (float)monthly_revenue[i].onsite_count);
        }
    }

    free_department_list(dept_head);
    free_doctor_list(doc_head);
    free_prescription_list(pres_head);
}

// =======================  入口菜单 =======================

int admin_analysis_menu(const User *current_user) {
    (void)current_user;
    char month_filter[8] = "";
    int mlen = analysis_get_month_filter(month_filter, sizeof(month_filter));
    (void)mlen;

    while (1) {
        ui_clear_screen();
        printf("\n");
        ui_box_top("分析报表");
        ui_divider();
        ui_menu_item(1, "经营概览");
        ui_menu_item(2, "趋势分析");
        ui_menu_item(3, "医生负载");
        ui_menu_item(4, "患者画像");
        ui_menu_item(5, "财务统计");
        ui_menu_item(6, "切换月份筛选");
        if (month_filter[0]) {
            printf(C_DIM "  当前筛选: %s\n" C_RESET, month_filter);
        }
        ui_menu_item(0, "返回");
        ui_box_bottom();

        int choice = get_menu_choice(0, 6);
        if (choice == 0) break;

        switch (choice) {
            case 1:
                analysis_operations_overview(month_filter);
                break;
            case 2:
                analysis_trend_trends(month_filter);
                break;
            case 3:
                analysis_doctor_load(month_filter);
                break;
            case 4:
                analysis_patient_profile(month_filter);
                break;
            case 5:
                analysis_financial_stats(month_filter);
                break;
            case 6: {
                char new_filter[8] = "";
                analysis_get_month_filter(new_filter, sizeof(new_filter));
                strncpy(month_filter, new_filter, sizeof(month_filter) - 1);
                month_filter[sizeof(month_filter) - 1] = '\0';
                continue;
            }
        }
        pause_screen();
    }

    return SUCCESS;
}
