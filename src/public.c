#include "public.h"
#include <ctype.h>

static int parse_date_value(const char *date_text, struct tm *date_value) {
    int year = 0;
    int month = 0;
    int day = 0;

    if (!date_text || !date_value) {
        return 0;
    }

    memset(date_value, 0, sizeof(*date_value));
    if (sscanf(date_text, "%d-%d-%d", &year, &month, &day) != 3) {
        return 0;
    }
    if (year < 1900 || month < 1 || month > 12 || day < 1 || day > 31) {
        return 0;
    }

    date_value->tm_year = year - 1900;
    date_value->tm_mon = month - 1;
    date_value->tm_mday = day;
    date_value->tm_hour = 0;
    date_value->tm_min = 0;
    date_value->tm_sec = 0;
    return 1;
}

static float get_patient_type_multiplier(const char *patient_type) {
    if (!patient_type) {
        return 0.0f;
    }
    if (strcmp(patient_type, "医保") == 0) {
        return 1.0f;
    }
    if (strcmp(patient_type, "军人") == 0) {
        return 1.15f;
    }
    return 0.0f;
}

// Input validation
bool validate_input(const char *input, int type) {
    if (!input || input[0] == '\0') {
        return false;
    }

    switch (type) {
        case 1:
            return is_valid_number(input);
        case 2:
            return is_valid_date(input);
        default:
            return strlen(input) > 0;
    }
}

bool is_valid_number(const char *str) {
    int dot_count = 0;

    if (!str || *str == '\0') {
        return false;
    }

    while (*str) {
        if (*str == '.') {
            dot_count++;
            if (dot_count > 1) {
                return false;
            }
        } else if (!isdigit((unsigned char)*str)) {
            return false;
        }
        str++;
    }
    return true;
}

bool is_valid_date(const char *date) {
    struct tm parsed_date;
    return parse_date_value(date, &parsed_date) != 0;
}

bool is_unique_id(const char *id, const char *id_type) {
    if (!id || !id_type) {
        return false;
    }

    if (strcmp(id_type, "patient") == 0) {
        PatientNode *head = load_patients_list();
        PatientNode *current = head;
        while (current) {
            if (strcmp(current->data.patient_id, id) == 0) {
                free_patient_list(head);
                return false;
            }
            current = current->next;
        }
        free_patient_list(head);
        return true;
    }

    if (strcmp(id_type, "doctor") == 0) {
        DoctorNode *head = load_doctors_list();
        DoctorNode *current = head;
        while (current) {
            if (strcmp(current->data.doctor_id, id) == 0) {
                free_doctor_list(head);
                return false;
            }
            current = current->next;
        }
        free_doctor_list(head);
        return true;
    }

    if (strcmp(id_type, "drug") == 0) {
        DrugNode *head = load_drugs_list();
        DrugNode *current = head;
        while (current) {
            if (strcmp(current->data.drug_id, id) == 0) {
                free_drug_list(head);
                return false;
            }
            current = current->next;
        }
        free_drug_list(head);
        return true;
    }

    return true;
}

// Warning functions
void check_drug_warning(void) {
    DrugNode *head = load_drugs_list();
    DrugNode *current = head;
    int found = 0;

    printf("\n========== 药品库存预警 ==========\n");
    while (current) {
        if (current->data.stock_num <= current->data.warning_line) {
            printf("药品 %s(%s) 库存不足: 当前 %d, 预警阈值 %d\n",
                   current->data.name,
                   current->data.drug_id,
                   current->data.stock_num,
                   current->data.warning_line);
            found++;
        }
        current = current->next;
    }

    if (!found) {
        printf("当前无药品库存预警。\n");
    }

    free_drug_list(head);
}

void check_ward_warning(void) {
    WardNode *head = load_wards_list();
    WardNode *current = head;
    int found = 0;

    printf("\n========== 病房床位预警 ==========\n");
    while (current) {
        if (current->data.remain_beds <= current->data.warning_line) {
            printf("病房 %s(%s) 床位紧张: 剩余 %d, 预警阈值 %d\n",
                   current->data.type,
                   current->data.ward_id,
                   current->data.remain_beds,
                   current->data.warning_line);
            found++;
        }
        current = current->next;
    }

    if (!found) {
        printf("当前无病房床位预警。\n");
    }

    free_ward_list(head);
}

// Reimbursement calculation
float calculate_reimbursement(float total_amount, const char *patient_type) {
    float ratio;

    if (total_amount <= 0.0f) {
        return 0.0f;
    }

    if (strcmp(patient_type, "医保") == 0) {
        ratio = 0.6f;
    } else if (strcmp(patient_type, "军人") == 0) {
        ratio = 0.8f;
    } else {
        ratio = 0.0f;
    }

    return total_amount * ratio;
}

float calculate_drug_reimbursement(const Drug *drug, int quantity, const char *patient_type) {
    float total_amount;
    float final_ratio;

    if (!drug || quantity <= 0) {
        return 0.0f;
    }

    total_amount = drug->price * quantity;
    final_ratio = drug->reimbursement_ratio * get_patient_type_multiplier(patient_type);
    if (final_ratio > 0.95f) {
        final_ratio = 0.95f;
    }
    if (final_ratio < 0.0f) {
        final_ratio = 0.0f;
    }
    return total_amount * final_ratio;
}

// Treatment progress control
const char* get_next_stage(const char *current_stage) {
    if (!current_stage || strcmp(current_stage, "初诊") == 0) {
        return "检查中";
    }
    if (strcmp(current_stage, "检查中") == 0) {
        return "治疗中";
    }
    if (strcmp(current_stage, "治疗中") == 0) {
        return "康复观察";
    }
    if (strcmp(current_stage, "康复观察") == 0) {
        return "已出院";
    }
    return current_stage;
}

// Doctor recommendation algorithm
Doctor* find_recommended_doctor(const char *department_id) {
    DoctorNode *head = load_doctors_list();
    DoctorNode *current = head;
    DoctorNode *best = NULL;
    Doctor *result = NULL;

    while (current) {
        if (strcmp(current->data.department_id, department_id) == 0) {
            if (!best || current->data.busy_level < best->data.busy_level) {
                best = current;
            }
        }
        current = current->next;
    }

    if (best) {
        result = (Doctor *)malloc(sizeof(Doctor));
        if (result) {
            *result = best->data;
        }
    }

    free_doctor_list(head);
    return result;
}

int recommend_doctor(const char *department_id) {
    Doctor *doctor = find_recommended_doctor(department_id);
    int result = -1;

    if (doctor) {
        if (doctor->doctor_id[0] != '\0') {
            result = atoi(doctor->doctor_id + 1);
        }
        free(doctor);
    }

    return result;
}

bool is_duplicate_prescription_risk(const char *patient_id, const char *drug_id) {
    PrescriptionNode *head = load_prescriptions_list();
    PrescriptionNode *current = head;
    time_t now = time(NULL);

    while (current) {
        if (strcmp(current->data.patient_id, patient_id) == 0 &&
            strcmp(current->data.drug_id, drug_id) == 0) {
            struct tm prescription_date;
            if (parse_date_value(current->data.prescription_date, &prescription_date)) {
                time_t prescribed_at = mktime(&prescription_date);
                double day_diff = difftime(now, prescribed_at) / (60.0 * 60.0 * 24.0);
                if (day_diff <= 7.0) {
                    free_prescription_list(head);
                    return true;
                }
            } else {
                free_prescription_list(head);
                return true;
            }
        }
        current = current->next;
    }

    free_prescription_list(head);
    return false;
}

// Record management
int archive_medical_record(int record_id) {
    MedicalRecordNode *head = load_medical_records_list();
    MedicalRecordNode *current = head;
    char record_key[MAX_ID];

    snprintf(record_key, sizeof(record_key), "MR%d", record_id);
    while (current) {
        if (strcmp(current->data.record_id, record_key) == 0 ||
            atoi(current->data.record_id + 2) == record_id) {
            strcpy(current->data.status, "已归档");
            save_medical_records_list(head);
            free_medical_record_list(head);
            return SUCCESS;
        }
        current = current->next;
    }

    free_medical_record_list(head);
    return ERROR_NOT_FOUND;
}
