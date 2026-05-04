#include "patient.h"
#include "public.h"
#include "ui_utils.h"
#include "login.h"

// 预约挂号常量
#define APPOINTMENT_SLOTS_PER_HALF_DAY 6
#define ONSITE_SLOTS_PER_DAY 8

static int is_closed_status(const char *status) {
    return strcmp(status, "已取消") == 0 || strcmp(status, "已完成") == 0 || strcmp(status, "已就诊") == 0 || strcmp(status, "已退号") == 0;
}

static void get_date_after(int days, char *buffer, int size) {
    time_t now = time(NULL);
    struct tm tm_info;
    memcpy(&tm_info, localtime(&now), sizeof(tm_info));
    tm_info.tm_mday += days;
    mktime(&tm_info);
    strftime(buffer, size, "%Y-%m-%d", &tm_info);
}

static int select_date_option(char *selected_date, int size) {
    char dates[7][20];
    char day_label[7][20];
    int i;

    ui_header("选择日期");
    for (i = 0; i < 7; i++) {
        get_date_after(i, dates[i], sizeof(dates[i]));
        if (i == 0) {
            snprintf(day_label[i], sizeof(day_label[i]), "  今天");
        } else if (i == 1) {
            snprintf(day_label[i], sizeof(day_label[i]), "  明天");
        } else {
            snprintf(day_label[i], sizeof(day_label[i]), "  ");
        }
        ui_menu_item(i + 1, dates[i]);
        printf("     %s\n", day_label[i]);
        ui_menu_track_line();
    }
    ui_divider();
    ui_menu_exit(0, "返回");

    int choice = get_menu_choice(0, 7);
    if (choice == 0) {
        return ERROR_INVALID_INPUT;
    }
    strncpy(selected_date, dates[choice - 1], size - 1);
    selected_date[size - 1] = '\0';
    return SUCCESS;
}

static int count_appointments_for_slot(const char *doctor_id, const char *date, const char *time_slot) {
    AppointmentNode *head = load_appointments_list();
    AppointmentNode *current = head;
    int count = 0;

    while (current) {
        if (strcmp(current->data.doctor_id, doctor_id) == 0 &&
            strcmp(current->data.appointment_date, date) == 0 &&
            strcmp(current->data.appointment_time, time_slot) == 0 &&
            strcmp(current->data.status, "已取消") != 0 &&
            strcmp(current->data.status, "已就诊") != 0) {
            count++;
        }
        current = current->next;
    }
    free_appointment_list(head);
    return count;
}

static int count_onsite_today(const char *doctor_id) {
    OnsiteRegistrationQueue queue = load_onsite_registration_queue();
    OnsiteRegistrationNode *current = queue.front;
    int count = 0;

    while (current) {
        if (strcmp(current->data.doctor_id, doctor_id) == 0 &&
            strcmp(current->data.status, "已完成") != 0 &&
            strcmp(current->data.status, "已退号") != 0) {
            count++;
        }
        current = current->next;
    }
    free_onsite_registration_queue(&queue);
    return count;
}

static int select_department(const char *prompt, char *department_id_out) {
    DepartmentNode *dept_head = load_departments_list();
    int count = count_department_list(dept_head);
    if (count == 0) {
        ui_warn("暂无科室信息，请联系管理员添加。");
        free_department_list(dept_head);
        return ERROR_NOT_FOUND;
    }

    ui_header(prompt);
    const char **di = malloc(count * sizeof(const char *));
    char (*db)[70] = malloc(count * sizeof(*db));
    int idx = 0;
    DepartmentNode *dp = dept_head;
    while (dp) {
        snprintf(db[idx], 70, "%s - %s (%s)", dp->data.department_id, dp->data.name, dp->data.leader);
        di[idx] = db[idx]; idx++; dp = dp->next;
    }

    int sel = ui_search_list(prompt, di, count);
    free((void*)di); free(db);
    if (sel < 0) { free_department_list(dept_head); return ERROR_INVALID_INPUT; }

    dp = dept_head;
    for (int j = 0; j < sel; j++) dp = dp->next;
    strcpy(department_id_out, dp->data.department_id);
    free_department_list(dept_head);
    return SUCCESS;
}

static int load_current_patient(const User *current_user, Patient *current_patient) {
    PatientNode *patient_head = load_patients_list();
    PatientNode *current_patient_node = patient_head;

    while (current_patient_node) {
        if (strcmp(current_patient_node->data.username, current_user->username) == 0) {
            *current_patient = current_patient_node->data;
            free_patient_list(patient_head);
            return SUCCESS;
        }
        current_patient_node = current_patient_node->next;
    }

    free_patient_list(patient_head);
    ui_err("患者信息不存在!");
    return ERROR_NOT_FOUND;
}

static int has_registration_conflict(const char *patient_id, const char *doctor_id, const char *department_id) {
    AppointmentNode *appointment_head = load_appointments_list();
    AppointmentNode *current_appointment = appointment_head;
    OnsiteRegistrationQueue onsite_queue = load_onsite_registration_queue();
    OnsiteRegistrationNode *current_onsite = onsite_queue.front;

    while (current_appointment) {
        if (strcmp(current_appointment->data.patient_id, patient_id) == 0 &&
            strcmp(current_appointment->data.doctor_id, doctor_id) == 0 &&
            strcmp(current_appointment->data.department_id, department_id) == 0 &&
            !is_closed_status(current_appointment->data.status)) {
            free_appointment_list(appointment_head);
            free_onsite_registration_queue(&onsite_queue);
            return 1;
        }
        current_appointment = current_appointment->next;
    }

    while (current_onsite) {
        if (strcmp(current_onsite->data.patient_id, patient_id) == 0 &&
            strcmp(current_onsite->data.doctor_id, doctor_id) == 0 &&
            strcmp(current_onsite->data.department_id, department_id) == 0 &&
            !is_closed_status(current_onsite->data.status)) {
            free_appointment_list(appointment_head);
            free_onsite_registration_queue(&onsite_queue);
            return 1;
        }
        current_onsite = current_onsite->next;
    }

    free_appointment_list(appointment_head);
    free_onsite_registration_queue(&onsite_queue);
    return 0;
}

static void print_doctor_name_by_id(const char *doctor_id, char *doctor_name, size_t size) {
    DoctorNode *doctor_head = load_doctors_list();
    DoctorNode *current_doc = doctor_head;

    strncpy(doctor_name, "未知", size - 1);
    doctor_name[size - 1] = '\0';

    while (current_doc) {
        if (strcmp(current_doc->data.doctor_id, doctor_id) == 0) {
            strncpy(doctor_name, current_doc->data.name, size - 1);
            doctor_name[size - 1] = '\0';
            break;
        }
        current_doc = current_doc->next;
    }

    free_doctor_list(doctor_head);
}

static int patient_standard_register_menu(const User *current_user) {
    char department_id[MAX_ID];
    char selected_date[20];
    Doctor selected_doc;
    Patient current_patient;
    Appointment new_apt;
    AppointmentNode *apt_head = NULL;
    AppointmentNode *new_apt_node = NULL;
    AppointmentNode *tail = NULL;
    DoctorNode *doctor_head = NULL;
    DoctorNode *current_doc = NULL;
    int result;
    char time_slot[10];
    DoctorNode *dept_doctors = NULL;
    DoctorNode *doc_iter = NULL;
    int has_doctors = 0;

    printf("\n");
    ui_header("预约挂号");
    printf(C_DIM "  提示: 可提前7天预约就诊日期" C_RESET "\n");

    // Step 1: 选择日期
    result = select_date_option(selected_date, sizeof(selected_date));
    if (result != SUCCESS) {
        return result;
    }

    // Step 2: 选择科室
    result = select_department("选择科室", department_id);
    if (result != SUCCESS) {
        return result;
    }

    // Step 3: 显示该科室下当天有排班的医生及其剩余预约号 (paginated)
    ui_sub_header("可选医生 (当日有排班)");
    ui_info("日期", selected_date);
    printf("\n");

    dept_doctors = load_doctors_list();
    {
        // first pass: count & compute (only doctors with schedule on selected date)
        int match = 0;
        DoctorNode *dd = dept_doctors;
        while (dd) {
            if (strcmp(dd->data.department_id, department_id) == 0 &&
                has_doctor_schedule(dd->data.doctor_id, selected_date)) match++;
            dd = dd->next;
        }
        if (match > 0) {
            const char **di = malloc(match * sizeof(const char *));
            char (*db)[80] = malloc(match * sizeof(*db));
            int idx = 0;
            dd = dept_doctors;
            while (dd && idx < match) {
                if (strcmp(dd->data.department_id, department_id) == 0 &&
                    has_doctor_schedule(dd->data.doctor_id, selected_date)) {
                    int mc = count_appointments_for_slot(dd->data.doctor_id, selected_date, "上午");
                    int ac = count_appointments_for_slot(dd->data.doctor_id, selected_date, "下午");
                    int mr = APPOINTMENT_SLOTS_PER_HALF_DAY - mc; if (mr < 0) mr = 0;
                    int ar = APPOINTMENT_SLOTS_PER_HALF_DAY - ac; if (ar < 0) ar = 0;
                    snprintf(db[idx], 80, "%-10s %-15s %-10s 上午%d 下午%d busy:%d",
                             dd->data.doctor_id, dd->data.name,
                             dd->data.title[0] ? dd->data.title : "未设置",
                             mr, ar, dd->data.busy_level);
                    di[idx] = db[idx]; idx++; has_doctors = 1;
                }
                dd = dd->next;
            }
            ui_paginate(di, match, 15, "可选医生 (有排班)");
            free((void*)di); free(db);
        }
    }
    free_doctor_list(dept_doctors);

    if (!has_doctors) {
        ui_warn("该科室在所选日期暂无出诊医生。");
        return ERROR_NOT_FOUND;
    }

    // Step 4: 选择医生 (by search, only with schedule)
    {
        int match = 0;
        DoctorNode *dd;
        dept_doctors = load_doctors_list();
        dd = dept_doctors;
        while (dd) {
            if (strcmp(dd->data.department_id, department_id) == 0 &&
                has_doctor_schedule(dd->data.doctor_id, selected_date)) match++;
            dd = dd->next;
        }
        if (match == 0) { free_doctor_list(dept_doctors); return ERROR_NOT_FOUND; }

        const char **di = malloc(match * sizeof(const char *));
        char (*db)[80] = malloc(match * sizeof(*db));
        int idx = 0;
        dd = dept_doctors;
        while (dd && idx < match) {
            if (strcmp(dd->data.department_id, department_id) == 0 &&
                has_doctor_schedule(dd->data.doctor_id, selected_date)) {
                snprintf(db[idx], 80, "%s - %s (%s)", dd->data.doctor_id, dd->data.name,
                         dd->data.title[0] ? dd->data.title : "医生");
                di[idx] = db[idx]; idx++;
            }
            dd = dd->next;
        }
        int sel = ui_search_list("选择医生", di, match);
        free((void*)di); free(db);
        if (sel < 0) { free_doctor_list(dept_doctors); return ERROR_INVALID_INPUT; }

        dd = dept_doctors;
        for (int j = 0; j < sel; j++) {
            while (dd && !(strcmp(dd->data.department_id, department_id) == 0 &&
                   has_doctor_schedule(dd->data.doctor_id, selected_date))) dd = dd->next;
            if (dd) dd = dd->next;
        }
        // Go back to the selected one
        dd = dept_doctors;
        for (int j = 0; j <= sel; ) {
            if (strcmp(dd->data.department_id, department_id) == 0 &&
                has_doctor_schedule(dd->data.doctor_id, selected_date)) {
                if (j == sel) break;
                j++;
            }
            dd = dd->next;
        }
        selected_doc = dd->data;
        free_doctor_list(dept_doctors);
    }

    // Step 5: 选择时间段
    ui_sub_header("选择时间段");
    ui_menu_item(1, "上午 (08:00-12:00)");
    ui_menu_item(2, "下午 (14:00-18:00)");
    ui_divider();
    ui_menu_exit(0, "返回");
    int slot_choice = get_menu_choice(0, 2);
    if (slot_choice == 0) {
        return ERROR_INVALID_INPUT;
    }
    strcpy(time_slot, (slot_choice == 1) ? "上午" : "下午");

    // 检查该时段是否还有剩余号
    int current_count = count_appointments_for_slot(selected_doc.doctor_id, selected_date, time_slot);
    if (current_count >= APPOINTMENT_SLOTS_PER_HALF_DAY) {
        ui_warn("该时段预约号已满，请选择其他时段。");
        return ERROR_DUPLICATE;
    }

    result = load_current_patient(current_user, &current_patient);
    if (result != SUCCESS) {
        return result;
    }

    if (has_registration_conflict(current_patient.patient_id, selected_doc.doctor_id, department_id)) {
        ui_warn("您已存在该医生的有效预约或现场排队记录，请勿重复挂号。");
        return ERROR_DUPLICATE;
    }

    // 确认并创建
    memset(&new_apt, 0, sizeof(new_apt));
    generate_id(new_apt.appointment_id, MAX_ID, "APT");
    strcpy(new_apt.patient_id, current_patient.patient_id);
    strcpy(new_apt.doctor_id, selected_doc.doctor_id);
    strcpy(new_apt.department_id, department_id);
    strcpy(new_apt.appointment_date, selected_date);
    strcpy(new_apt.appointment_time, time_slot);
    strcpy(new_apt.status, "已预约");
    get_current_time(new_apt.create_time, 30);

    apt_head = load_appointments_list();
    new_apt_node = create_appointment_node(&new_apt);
    if (!new_apt_node) {
        free_appointment_list(apt_head);
        ui_err("内存分配失败!");
        return ERROR_FILE_IO;
    }

    if (!apt_head) {
        apt_head = new_apt_node;
    } else {
        tail = apt_head;
        while (tail->next) {
            tail = tail->next;
        }
        tail->next = new_apt_node;
    }

    result = save_appointments_list(apt_head);
    free_appointment_list(apt_head);
    if (result != SUCCESS) {
        return result;
    }

    doctor_head = load_doctors_list();
    current_doc = doctor_head;
    while (current_doc) {
        if (strcmp(current_doc->data.doctor_id, selected_doc.doctor_id) == 0) {
            current_doc->data.busy_level++;
            break;
        }
        current_doc = current_doc->next;
    }
    save_doctors_list(doctor_head);
    free_doctor_list(doctor_head);

    printf("\n");
    ui_ok("预约挂号成功!");
    ui_divider();
    ui_info("预约单号", new_apt.appointment_id);
    ui_info("患者", current_patient.name);
    ui_info("科室", department_id);
    ui_info("医生", selected_doc.name);
    ui_info("就诊日期", selected_date);
    ui_info("就诊时段", time_slot);
    ui_info("状态", new_apt.status);

    return SUCCESS;
}

static int patient_onsite_register_menu(const User *current_user) {
    char department_id[MAX_ID];
    Doctor selected_doc;
    Patient current_patient;
    OnsiteRegistration registration;
    OnsiteRegistrationQueue queue;
    DoctorNode *dept_doctors = NULL;
    DoctorNode *doc_iter = NULL;
    int result;
    int has_doctors = 0;

    printf("\n");
    ui_header("现场挂号 (今日)");
    printf(C_DIM "  提示: 现场挂号仅限当天，挂号后自动进入候诊队列。" C_RESET "\n\n");

    // Step 1: 选择科室
    result = select_department("今日可选科室", department_id);
    if (result != SUCCESS) {
        return result;
    }

    // Get today's date for schedule checking
    char today_str[12];
    {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        strftime(today_str, sizeof(today_str), "%Y-%m-%d", tm_info);
    }

    // Step 2: 显示该科室下今日有排班的医生及其剩余现场号 (paginated)
    ui_sub_header("今日值班医生 (有排班)");

    dept_doctors = load_doctors_list();
    {
        int match = 0;
        DoctorNode *dd = dept_doctors;
        while (dd) {
            if (strcmp(dd->data.department_id, department_id) == 0 &&
                has_doctor_schedule(dd->data.doctor_id, today_str)) match++;
            dd = dd->next;
        }
        if (match > 0) {
            const char **di = malloc(match * sizeof(const char *));
            char (*db)[80] = malloc(match * sizeof(*db));
            int idx = 0;
            dd = dept_doctors;
            while (dd && idx < match) {
                if (strcmp(dd->data.department_id, department_id) == 0 &&
                    has_doctor_schedule(dd->data.doctor_id, today_str)) {
                    int oc = count_onsite_today(dd->data.doctor_id);
                    int or_ = ONSITE_SLOTS_PER_DAY - oc;
                    if (or_ < 0) or_ = 0;
                    snprintf(db[idx], 80, "%-10s %-15s %-10s 余%d 排%d busy:%d",
                             dd->data.doctor_id, dd->data.name,
                             dd->data.title[0] ? dd->data.title : "未设置",
                             or_, oc, dd->data.busy_level);
                    di[idx] = db[idx]; idx++; has_doctors = 1;
                }
                dd = dd->next;
            }
            ui_paginate(di, match, 15, "今日值班医生 (有排班)");
            free((void*)di); free(db);
        }
    }
    free_doctor_list(dept_doctors);

    if (!has_doctors) {
        ui_warn("该科室今日无出诊医生。");
        return ERROR_NOT_FOUND;
    }

    // Step 3: 选择医生 (by search, only with schedule)
    {
        int match = 0;
        DoctorNode *dd;
        dept_doctors = load_doctors_list();
        dd = dept_doctors;
        while (dd) {
            if (strcmp(dd->data.department_id, department_id) == 0 &&
                has_doctor_schedule(dd->data.doctor_id, today_str)) match++;
            dd = dd->next;
        }
        if (match == 0) { free_doctor_list(dept_doctors); return ERROR_NOT_FOUND; }

        const char **di = malloc(match * sizeof(const char *));
        char (*db)[80] = malloc(match * sizeof(*db));
        int idx = 0;
        dd = dept_doctors;
        while (dd && idx < match) {
            if (strcmp(dd->data.department_id, department_id) == 0 &&
                has_doctor_schedule(dd->data.doctor_id, today_str)) {
                snprintf(db[idx], 80, "%s - %s (%s)", dd->data.doctor_id, dd->data.name,
                         dd->data.title[0] ? dd->data.title : "医生");
                di[idx] = db[idx]; idx++;
            }
            dd = dd->next;
        }
        int sel = ui_search_list("选择医生", di, match);
        free((void*)di); free(db);
        if (sel < 0) { free_doctor_list(dept_doctors); return ERROR_INVALID_INPUT; }

        dd = dept_doctors;
        for (int j = 0; j <= sel; ) {
            if (strcmp(dd->data.department_id, department_id) == 0 &&
                has_doctor_schedule(dd->data.doctor_id, today_str)) {
                if (j == sel) break;
                j++;
            }
            dd = dd->next;
        }
        selected_doc = dd->data;
        free_doctor_list(dept_doctors);
    }

    result = load_current_patient(current_user, &current_patient);
    if (result != SUCCESS) {
        return result;
    }

    if (has_registration_conflict(current_patient.patient_id, selected_doc.doctor_id, department_id)) {
        ui_warn("您已存在该医生的有效预约或现场排队记录，请勿重复挂号。");
        return ERROR_DUPLICATE;
    }

    memset(&registration, 0, sizeof(registration));
    generate_id(registration.onsite_id, MAX_ID, "OS");
    strcpy(registration.patient_id, current_patient.patient_id);
    strcpy(registration.doctor_id, selected_doc.doctor_id);
    strcpy(registration.department_id, department_id);
    registration.queue_number = get_next_onsite_queue_number(selected_doc.doctor_id, department_id);
    strcpy(registration.status, "排队中");
    get_current_time(registration.create_time, 30);

    queue = load_onsite_registration_queue();
    result = enqueue_onsite_registration(&queue, &registration, current_patient.is_emergency);
    if (result != SUCCESS) {
        free_onsite_registration_queue(&queue);
        ui_err("现场挂号入队失败!");
        return result;
    }

    result = save_onsite_registration_queue(&queue);
    free_onsite_registration_queue(&queue);
    if (result != SUCCESS) {
        return result;
    }

    {
        DoctorNode *doctor_head = load_doctors_list();
        DoctorNode *current_doc = doctor_head;
        while (current_doc) {
            if (strcmp(current_doc->data.doctor_id, selected_doc.doctor_id) == 0) {
                current_doc->data.busy_level++;
                break;
            }
            current_doc = current_doc->next;
        }
        save_doctors_list(doctor_head);
        free_doctor_list(doctor_head);

        // Emergency patients: check bed availability, defer actual assignment to admission
        if (current_patient.is_emergency) {
            WardNode *ward_head = load_wards_list();
            WardNode *ward_node = ward_head;
            int has_bed = 0;
            while (ward_node) {
                if (ward_node->data.remain_beds > 0) {
                    has_bed = 1;
                    break;
                }
                ward_node = ward_node->next;
            }
            if (has_bed) {
                printf(C_YELLOW "  ⚠ 急诊患者，入院时将优先分配床位。\n" C_RESET);
            } else {
                printf(C_RED "  ⚠ 急诊患者，但暂无可用床位!\n" C_RESET);
            }
            free_ward_list(ward_head);
        }
    }

    printf("\n");
    ui_ok("现场挂号成功，已进入候诊队列!");
    ui_divider();
    ui_info("现场单号", registration.onsite_id);
    ui_info("患者", current_patient.name);
    ui_info("科室", department_id);
    ui_info("医生", selected_doc.name);
    {
        char qno[16];
        snprintf(qno, sizeof(qno), "%d", registration.queue_number);
        ui_info("当前排队号", qno);
    }
    ui_info("状态", registration.status);

    return SUCCESS;
}

void show_available_departments(void) {
    DepartmentNode *head = load_departments_list();
    int count = count_department_list(head);

    if (count == 0) {
        printf("\n");
        ui_warn("暂无科室信息，请联系管理员添加。");
        free_department_list(head);
        return;
    }

    const char **items = malloc(count * sizeof(const char *));
    char (*buf)[70] = malloc(count * sizeof(*buf));
    int i = 0;
    DepartmentNode *cur = head;
    while (cur) {
        snprintf(buf[i], 70, "%-10s %-20s %-15s",
                 cur->data.department_id, cur->data.name, cur->data.leader);
        items[i] = buf[i]; i++; cur = cur->next;
    }

    ui_paginate(items, count, 15, "可选科室");
    free((void*)items); free(buf);
    free_department_list(head);
}

void show_doctors_by_department(const char *department_id) {
    DoctorNode *head = load_doctors_list();

    // first pass: count matching
    int match = 0;
    DoctorNode *cur = head;
    while (cur) {
        if (strcmp(cur->data.department_id, department_id) == 0) match++;
        cur = cur->next;
    }

    if (match == 0) {
        ui_warn("该科室暂无医生。");
        free_doctor_list(head);
        return;
    }

    const char **items = malloc(match * sizeof(const char *));
    char (*buf)[70] = malloc(match * sizeof(*buf));
    int idx = 0;
    cur = head;
    while (cur && idx < match) {
        if (strcmp(cur->data.department_id, department_id) == 0) {
            snprintf(buf[idx], 70, "%-10s %-15s %-10s %3d",
                     cur->data.doctor_id, cur->data.name,
                     cur->data.title[0] ? cur->data.title : "未设置",
                     cur->data.busy_level);
            items[idx] = buf[idx]; idx++;
        }
        cur = cur->next;
    }

    ui_paginate(items, match, 15, "可选医生");
    free((void*)items); free(buf);
    free_doctor_list(head);
}

void patient_main_menu(const User *current_user) {
    (void)current_user;
    ui_menu_item(1, "挂号就诊");
    ui_menu_item(2, "挂号状态查询");
    ui_menu_item(3, "诊断结果查询");
    ui_menu_item(4, "住院信息查看");
    ui_menu_item(5, "治疗进度查看");
    ui_menu_item(6, "修改个人信息");
    ui_menu_item(7, "修改密码");
}

int patient_register_menu(const User *current_user) {
    ui_header("挂号功能");

    if (ensure_patient_profile(current_user->username) != SUCCESS) {
        ui_err("患者信息初始化失败!");
        return ERROR_FILE_IO;
    }

    ui_menu_item(1, "预约挂号 (可提前7天预约)");
    ui_menu_item(2, "现场挂号 (当日排队)");
    ui_divider();
    ui_menu_exit(0, "返回");

    switch (get_menu_choice(0, 2)) {
        case 0:
            return SUCCESS;
        case 1:
            return patient_standard_register_menu(current_user);
        case 2:
            return patient_onsite_register_menu(current_user);
        default:
            ui_err("输入无效!");
            return ERROR_INVALID_INPUT;
    }
}

int patient_appointment_menu(const User *current_user) {
    PatientNode *patient_head = NULL;
    PatientNode *current_patient_node = NULL;
    AppointmentNode *apt_head = NULL;
    AppointmentNode *current_apt = NULL;
    OnsiteRegistrationQueue onsite_queue;
    OnsiteRegistrationNode *current_onsite = NULL;
    char patient_id_copy[MAX_ID];
    int patient_found = 0;
    int found = 0;
    char cancel_id[MAX_ID];
    int cancelled = 0;

    ui_header("挂号状态查询");

    if (ensure_patient_profile(current_user->username) != SUCCESS) {
        ui_err("患者信息初始化失败!");
        return ERROR_FILE_IO;
    }

    patient_head = load_patients_list();
    current_patient_node = patient_head;
    while (current_patient_node) {
        if (strcmp(current_patient_node->data.username, current_user->username) == 0) {
            patient_found = 1;
            strcpy(patient_id_copy, current_patient_node->data.patient_id);
            break;
        }
        current_patient_node = current_patient_node->next;
    }

    if (!patient_found) {
        free_patient_list(patient_head);
        ui_err("患者信息不存在!");
        return ERROR_NOT_FOUND;
    }
    free_patient_list(patient_head);

    ui_sub_header("我的挂号记录");

    apt_head = load_appointments_list();
    onsite_queue = load_onsite_registration_queue();

    // ----- 预约挂号记录 (paginated) -----
    {
        int match = 0;
        AppointmentNode *ap = apt_head;
        while (ap) {
            if (strcmp(ap->data.patient_id, patient_id_copy) == 0) match++;
            ap = ap->next;
        }
        if (match > 0) {
            const char **ai = malloc(match * sizeof(const char *));
            char (*ab)[90] = malloc(match * sizeof(*ab));
            int idx = 0;
            ap = apt_head;
            while (ap && idx < match) {
                if (strcmp(ap->data.patient_id, patient_id_copy) == 0) {
                    char dn[MAX_NAME];
                    print_doctor_name_by_id(ap->data.doctor_id, dn, sizeof(dn));
                    const char *act = (strcmp(ap->data.status, "已预约") == 0) ? " [可取消]" : " -";
                    snprintf(ab[idx], 90, "%-16s %-12s %-8s %-12s %-10s%s",
                             ap->data.appointment_id, ap->data.appointment_date,
                             ap->data.appointment_time, dn, ap->data.status, act);
                    ai[idx] = ab[idx]; idx++; found++;
                }
                ap = ap->next;
            }
            ui_paginate(ai, match, 15, "预约挂号记录");
            free((void*)ai); free(ab);
        }
    }

    // ----- 现场挂号记录 (paginated) with queue position -----
    {
        int match = 0;
        OnsiteRegistrationNode *on = onsite_queue.front;
        while (on) {
            if (strcmp(on->data.patient_id, patient_id_copy) == 0) match++;
            on = on->next;
        }
        if (match > 0) {
            const char **oi = malloc(match * sizeof(const char *));
            char (*ob)[110] = malloc(match * sizeof(*ob));
            int idx = 0;
            on = onsite_queue.front;
            while (on && idx < match) {
                if (strcmp(on->data.patient_id, patient_id_copy) == 0) {
                    char dn[MAX_NAME];
                    char qno[10];
                    print_doctor_name_by_id(on->data.doctor_id, dn, sizeof(dn));
                    snprintf(qno, sizeof(qno), "%d", on->data.queue_number);
                    const char *act = (strcmp(on->data.status, "排队中") == 0) ? " [可退号]" : " -";

                    // Calculate queue position: how many pending before this patient
                    char pos_str[20] = "";
                    if (strcmp(on->data.status, "排队中") == 0) {
                        int ahead = 0;
                        OnsiteRegistrationNode *tmp = onsite_queue.front;
                        while (tmp) {
                            if (strcmp(tmp->data.doctor_id, on->data.doctor_id) == 0 &&
                                strcmp(tmp->data.department_id, on->data.department_id) == 0 &&
                                strcmp(tmp->data.status, "排队中") == 0 &&
                                tmp->data.queue_number < on->data.queue_number) {
                                ahead++;
                            }
                            tmp = tmp->next;
                        }
                        snprintf(pos_str, sizeof(pos_str), "前面%d人", ahead);
                    }

                    snprintf(ob[idx], 110, "%-16s %-12s %-10s %-10s %-8s%s",
                             on->data.onsite_id, dn, qno, on->data.status, pos_str, act);
                    oi[idx] = ob[idx]; idx++; found++;
                }
                on = on->next;
            }
            ui_paginate(oi, match, 15, "现场挂号记录");
            free((void*)oi); free(ob);
        }
    }

    if (found == 0) {
        ui_warn("暂无挂号记录。");
    }

    // ----- 取消/退号 -----
    printf("\n" S_LABEL "  输入单号可取消预约或现场退号，输入0返回: " C_RESET);
    if (fgets(cancel_id, sizeof(cancel_id), stdin) == NULL) {
        free_appointment_list(apt_head);
        free_onsite_registration_queue(&onsite_queue);
        return ERROR_INVALID_INPUT;
    }
    cancel_id[strcspn(cancel_id, "\n")] = 0;

    if (strcmp(cancel_id, "0") != 0 && cancel_id[0] != '\0') {
        // 先在预约列表中查找
        current_apt = apt_head;
        while (current_apt) {
            if (strcmp(current_apt->data.appointment_id, cancel_id) == 0 &&
                strcmp(current_apt->data.patient_id, patient_id_copy) == 0) {
                if (strcmp(current_apt->data.status, "已预约") == 0) {
                    strcpy(current_apt->data.status, "已取消");
                    save_appointments_list(apt_head);
                    {
                        DoctorNode *dhead = load_doctors_list();
                        DoctorNode *dcur = dhead;
                        while (dcur) {
                            if (strcmp(dcur->data.doctor_id, current_apt->data.doctor_id) == 0) {
                                if (dcur->data.busy_level > 0) dcur->data.busy_level--;
                                break;
                            }
                            dcur = dcur->next;
                        }
                        save_doctors_list(dhead);
                        free_doctor_list(dhead);
                    }
                    ui_ok("预约已取消。");
                    cancelled = 1;
                } else {
                    ui_warn("该预约当前状态无法取消。");
                    cancelled = 1;
                }
                break;
            }
            current_apt = current_apt->next;
        }

        if (!cancelled) {
            // 在现场队列中查找
            current_onsite = onsite_queue.front;
            while (current_onsite) {
                if (strcmp(current_onsite->data.onsite_id, cancel_id) == 0 &&
                    strcmp(current_onsite->data.patient_id, patient_id_copy) == 0) {
                    if (strcmp(current_onsite->data.status, "排队中") == 0) {
                        strcpy(current_onsite->data.status, "已退号");
                        save_onsite_registration_queue(&onsite_queue);
                        {
                            DoctorNode *dhead = load_doctors_list();
                            DoctorNode *dcur = dhead;
                            while (dcur) {
                                if (strcmp(dcur->data.doctor_id, current_onsite->data.doctor_id) == 0) {
                                    if (dcur->data.busy_level > 0) dcur->data.busy_level--;
                                    break;
                                }
                                dcur = dcur->next;
                            }
                            save_doctors_list(dhead);
                            free_doctor_list(dhead);
                        }
                        ui_ok("现场挂号已退号。");
                        cancelled = 1;
                    } else {
                        ui_warn("该现场挂号当前状态无法退号。");
                        cancelled = 1;
                    }
                    break;
                }
                current_onsite = current_onsite->next;
            }
        }

        if (!cancelled) {
            ui_err("未找到该单号，或该单号不属于当前患者。");
        }
    }

    free_appointment_list(apt_head);
    free_onsite_registration_queue(&onsite_queue);
    return SUCCESS;
}

int patient_query_diagnosis_menu(const User *current_user) {
    PatientNode *patient_head = NULL;
    PatientNode *current_patient_node = NULL;
    MedicalRecordNode *record_head = NULL;
    MedicalRecordNode *current_record = NULL;
    char patient_id_copy[MAX_ID];
    int patient_found = 0;
    int found = 0;

    ui_header("诊断结果查询");

    if (ensure_patient_profile(current_user->username) != SUCCESS) {
        ui_err("患者信息初始化失败!");
        return ERROR_FILE_IO;
    }

    patient_head = load_patients_list();
    current_patient_node = patient_head;
    while (current_patient_node) {
        if (strcmp(current_patient_node->data.username, current_user->username) == 0) {
            patient_found = 1;
            strcpy(patient_id_copy, current_patient_node->data.patient_id);
            break;
        }
        current_patient_node = current_patient_node->next;
    }

    if (!patient_found) {
        free_patient_list(patient_head);
        ui_err("患者信息不存在!");
        return ERROR_NOT_FOUND;
    }
    free_patient_list(patient_head);

    record_head = load_medical_records_list();

    // paginated: 诊断记录
    {
        int match = 0;
        MedicalRecordNode *mr = record_head;
        while (mr) {
            if (strcmp(mr->data.patient_id, patient_id_copy) == 0) match++;
            mr = mr->next;
        }
        if (match > 0) {
            const char **ri = malloc(match * sizeof(const char *));
            char (*rb)[80] = malloc(match * sizeof(*rb));
            int idx = 0;
            mr = record_head;
            while (mr && idx < match) {
                if (strcmp(mr->data.patient_id, patient_id_copy) == 0) {
                    snprintf(rb[idx], 80, "%-16s %-14s %-20s %-12s",
                             mr->data.record_id, mr->data.doctor_id,
                             mr->data.diagnosis_date, mr->data.status);
                    ri[idx] = rb[idx]; idx++; found++;
                }
                mr = mr->next;
            }
            ui_paginate(ri, match, 15, "我的诊断记录");
            free((void*)ri); free(rb);
        }
    }

    if (found == 0) {
        ui_warn("暂无诊断记录。");
    } else {
        // Build record list for selection
        int match = 0;
        MedicalRecordNode *mr;
        mr = record_head;
        while (mr) {
            if (strcmp(mr->data.patient_id, patient_id_copy) == 0) match++;
            mr = mr->next;
        }
        if (match > 0) {
            const char **ri = malloc(match * sizeof(const char *));
            char (*rb)[80] = malloc(match * sizeof(*rb));
            int idx = 0;
            mr = record_head;
            while (mr && idx < match) {
                if (strcmp(mr->data.patient_id, patient_id_copy) == 0) {
                    char dn[MAX_NAME];
                    print_doctor_name_by_id(mr->data.doctor_id, dn, sizeof(dn));
                    snprintf(rb[idx], 80, "%-16s %-12s %-20s %-12s",
                             mr->data.record_id, dn,
                             mr->data.diagnosis_date, mr->data.status);
                    ri[idx] = rb[idx]; idx++;
                }
                mr = mr->next;
            }
            int sel = ui_search_list("选择记录查看详情", ri, match);
            free((void*)ri); free(rb);
            if (sel >= 0) {
                mr = record_head;
                for (int j = 0; j < sel; j++) mr = mr->next;
                PrescriptionNode *prescription_head = load_prescriptions_list();
                PrescriptionNode *current_prescription = prescription_head;
                int prescription_found = 0;

                ui_sub_header("诊断详情");
                ui_info("诊断结果", mr->data.diagnosis);
                ui_info("诊断日期", mr->data.diagnosis_date);
                ui_info("治疗状态", mr->data.status);
                ui_sub_header("处方摘要");

                while (current_prescription) {
                    if (strcmp(current_prescription->data.record_id, mr->data.record_id) == 0) {
                        Drug *drug = find_drug_by_id(current_prescription->data.drug_id);
                        printf("  ");
                        printf(S_LABEL);
                        printf("药品: ");
                        printf(C_RESET);
                        ui_print_col(drug ? drug->name : current_prescription->data.drug_id, 15);
                        printf(S_LABEL);
                        printf("  数量: ");
                        printf(C_RESET);
                        ui_print_col_int(current_prescription->data.quantity, 6);
                        printf(S_LABEL);
                        printf("  金额: ");
                        printf(C_RESET);
                        printf("%.2f\n", current_prescription->data.total_price);
                        if (drug) {
                            free(drug);
                        }
                        prescription_found = 1;
                    }
                    current_prescription = current_prescription->next;
                }

                if (!prescription_found) {
                    printf("  " C_DIM "暂无处方记录。" C_RESET "\n");
                }

                free_prescription_list(prescription_head);
            }
        }
    }

    free_medical_record_list(record_head);
    return SUCCESS;
}

int patient_view_ward_menu(const User *current_user) {
    WardNode *ward_head = NULL;
    WardNode *current_ward = NULL;
    (void)current_user;

    ui_header("住院信息查看");

    ward_head = load_wards_list();
    if (!ward_head) {
        ui_warn("暂无病房信息。");
        return SUCCESS;
    }

    {
        int wc = count_ward_list(ward_head);
        if (wc > 0) {
            const char **wi = malloc(wc * sizeof(const char *));
            char (*wb)[70] = malloc(wc * sizeof(*wb));
            int idx = 0;
            WardNode *wu = ward_head;
            while (wu) {
                snprintf(wb[idx], 70, "%-10s %-15s %3d张 %3d张 %3d",
                         wu->data.ward_id, wu->data.type,
                         wu->data.total_beds, wu->data.remain_beds, wu->data.warning_line);
                wi[idx] = wb[idx]; idx++; wu = wu->next;
            }
            ui_paginate(wi, wc, 15, "病房信息");
            free((void*)wi); free(wb);
        }
    }

    free_ward_list(ward_head);
    return SUCCESS;
}

int patient_view_treatment_progress_menu(const User *current_user) {
    PatientNode *patient_head = NULL;
    PatientNode *current_patient_node = NULL;
    int patient_found = 0;
    char name_copy[MAX_NAME];
    char stage_copy[20];
    char type_copy[20];
    int emergency = 0;

    ui_header("治疗进度查看");

    if (ensure_patient_profile(current_user->username) != SUCCESS) {
        ui_err("患者信息初始化失败!");
        return ERROR_FILE_IO;
    }

    patient_head = load_patients_list();
    current_patient_node = patient_head;
    while (current_patient_node) {
        if (strcmp(current_patient_node->data.username, current_user->username) == 0) {
            patient_found = 1;
            strcpy(name_copy, current_patient_node->data.name);
            strcpy(stage_copy, current_patient_node->data.treatment_stage);
            strcpy(type_copy, current_patient_node->data.patient_type);
            emergency = current_patient_node->data.is_emergency ? 1 : 0;
            break;
        }
        current_patient_node = current_patient_node->next;
    }
    free_patient_list(patient_head);

    if (patient_found) {
        ui_divider();
        ui_info("患者", name_copy);
        ui_info("当前治疗阶段", stage_copy);
        ui_info("紧急状态", emergency ? "是" : "否");
        ui_info("患者类型", type_copy);
    } else {
        ui_err("患者信息不存在!");
    }

    return SUCCESS;
}

int patient_edit_profile_menu(const User *current_user) {
    PatientNode *patient_head = NULL;
    PatientNode *current_patient_node = NULL;
    Patient *p = NULL;
    int patient_found = 0;
    int choice;
    int result;

    ui_header("修改个人信息");

    if (ensure_patient_profile(current_user->username) != SUCCESS) {
        ui_err("患者信息初始化失败!");
        return ERROR_FILE_IO;
    }

    patient_head = load_patients_list();
    current_patient_node = patient_head;
    while (current_patient_node) {
        if (strcmp(current_patient_node->data.username, current_user->username) == 0) {
            patient_found = 1;
            break;
        }
        current_patient_node = current_patient_node->next;
    }

    if (!patient_found) {
        free_patient_list(patient_head);
        ui_err("患者信息不存在!");
        return ERROR_NOT_FOUND;
    }

    p = &current_patient_node->data;
    printf("\n");
    ui_sub_header("当前信息");
    ui_info("姓名", p->name);
    ui_info("性别", p->gender);
    {
        char age_str[16];
        snprintf(age_str, sizeof(age_str), "%d", p->age);
        ui_info("年龄", age_str);
    }
    ui_info("电话", p->phone);
    ui_info("地址", p->address);
    ui_info("患者类型", p->patient_type);

    ui_divider();
    ui_menu_item(1, "姓名");
    ui_menu_item(2, "性别");
    ui_menu_item(3, "年龄");
    ui_menu_item(4, "电话");
    ui_menu_item(5, "地址");
    ui_menu_item(6, "患者类型 (普通/医保/军人)");
    ui_menu_exit(0, "返回");

    choice = get_menu_choice(0, 6);
    switch (choice) {
        case 1:
            printf(S_LABEL "  请输入新姓名: " C_RESET);
            if (fgets(p->name, MAX_NAME, stdin)) {
                p->name[strcspn(p->name, "\n")] = 0;
            }
            break;
        case 2:
            printf(S_LABEL "  请输入新性别(男/女): " C_RESET);
            if (fgets(p->gender, sizeof(p->gender), stdin)) {
                p->gender[strcspn(p->gender, "\n")] = 0;
            }
            break;
        case 3:
            printf(S_LABEL "  请输入新年龄(0-150): " C_RESET);
            {
                int new_age;
                if (scanf("%d", &new_age) == 1) {
                    if (is_valid_age(new_age)) {
                        p->age = new_age;
                    } else {
                        ui_err("年龄必须在 0-150 之间。");
                        clear_input_buffer();
                        free_patient_list(patient_head);
                        return ERROR_INVALID_INPUT;
                    }
                }
                clear_input_buffer();
            }
            break;
        case 4:
            printf(S_LABEL "  请输入新电话(7-15位数字): " C_RESET);
            if (fgets(p->phone, sizeof(p->phone), stdin)) {
                p->phone[strcspn(p->phone, "\n")] = 0;
                if (p->phone[0] != '\0' && !is_valid_phone(p->phone)) {
                    ui_err("电话号码无效! 需要7-15位纯数字。");
                    free_patient_list(patient_head);
                    return ERROR_INVALID_INPUT;
                }
            }
            break;
        case 5:
            printf(S_LABEL "  请输入新地址: " C_RESET);
            if (fgets(p->address, sizeof(p->address), stdin)) {
                p->address[strcspn(p->address, "\n")] = 0;
            }
            break;
        case 6:
            printf(S_LABEL "  请输入患者类型(普通/医保/军人): " C_RESET);
            if (fgets(p->patient_type, sizeof(p->patient_type), stdin)) {
                p->patient_type[strcspn(p->patient_type, "\n")] = 0;
            }
            if (strcmp(p->patient_type, "普通") != 0 &&
                strcmp(p->patient_type, "医保") != 0 &&
                strcmp(p->patient_type, "军人") != 0) {
                ui_err("无效的患者类型，已重置为\"普通\"");
                strcpy(p->patient_type, "普通");
            }
            break;
        case 0:
            free_patient_list(patient_head);
            return SUCCESS;
    }

    result = save_patients_list(patient_head);
    free_patient_list(patient_head);

    if (result == SUCCESS) {
        ui_ok("个人信息更新成功!");
    }

    return result;
}