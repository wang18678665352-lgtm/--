#include "data_storage.h"
#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <io.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#define TEXT_LINE_SIZE 4096

static int read_data_line(FILE *fp, char *buffer, size_t size) {
    while (fgets(buffer, (int)size, fp)) {
        size_t len = strlen(buffer);
        while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
            buffer[--len] = '\0';
        }
        if (buffer[0] == '\0' || buffer[0] == '#') {
            continue;
        }
        return 1;
    }
    return 0;
}

static char *next_token(char **cursor) {
    static char empty_buf[1] = {0};
    char *start = *cursor;
    char *sep;

    if (!start) {
        return empty_buf;
    }

    sep = strchr(start, '\t');
    if (sep) {
        *sep = '\0';
        *cursor = sep + 1;
    } else {
        *cursor = NULL;
    }

    return start;
}

// ==================== 字段转义/反转义（防止数据中含 Tab/换行符破坏文件格式） ====================

static void escape_field(const char *input, char *output, size_t output_size) {
    size_t j = 0;
    for (size_t i = 0; input[i] != '\0' && j + 4 < output_size; i++) {
        unsigned char c = (unsigned char)input[i];
        if (c == '\\') {
            output[j++] = '\\'; output[j++] = '\\';
        } else if (c == '\t') {
            output[j++] = '\\'; output[j++] = 't';
        } else if (c == '\n') {
            output[j++] = '\\'; output[j++] = 'n';
        } else if (c == '\r') {
            output[j++] = '\\'; output[j++] = 'r';
        } else {
            output[j++] = c;
        }
    }
    output[j] = '\0';
}

static void unescape_field_inplace(char *str) {
    size_t j = 0;
    for (size_t i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\\' && str[i+1] != '\0') {
            if (str[i+1] == '\\') { str[j++] = '\\'; i++; }
            else if (str[i+1] == 't') { str[j++] = '\t'; i++; }
            else if (str[i+1] == 'n') { str[j++] = '\n'; i++; }
            else if (str[i+1] == 'r') { str[j++] = '\r'; i++; }
            else { str[j++] = str[i]; }
        } else {
            str[j++] = str[i];
        }
    }
    str[j] = '\0';
}

// 保存时安全地写一个字符串字段（自动转义）
static void fprintf_escaped(FILE *fp, const char *str) {
    char buf[4096];
    escape_field(str, buf, sizeof(buf));
    fprintf(fp, "%s", buf);
}

static int parse_int_token(char **cursor) {
    return atoi(next_token(cursor));
}

static float parse_float_token(char **cursor) {
    return (float)atof(next_token(cursor));
}

static bool parse_bool_token(char **cursor) {
    const char *token = next_token(cursor);
    return strcmp(token, "1") == 0 || strcmp(token, "true") == 0 || strcmp(token, "TRUE") == 0;
}

int init_data_storage(void) {
#ifdef _WIN32
    DWORD dwAttrib = GetFileAttributesA(DATA_DIR);
    if (dwAttrib == INVALID_FILE_ATTRIBUTES) {
        if (!CreateDirectoryA(DATA_DIR, NULL)) {
            printf("创建数据目录失败!\n");
            return ERROR_FILE_IO;
        }
    }
#else
    struct stat st = {0};
    if (stat(DATA_DIR, &st) == -1) {
        if (mkdir(DATA_DIR, 0755) != 0) {
            printf("创建数据目录失败: %s\n", strerror(errno));
            return ERROR_FILE_IO;
        }
    }
#endif
    return SUCCESS;
}

// ==================== 链表操作函数 ====================

UserNode* create_user_node(const User *user) {
    UserNode *node = (UserNode *)malloc(sizeof(UserNode));
    if (node) {
        node->data = *user;
        node->next = NULL;
    }
    return node;
}

void free_user_list(UserNode *head) {
    UserNode *current = head;
    while (current) {
        UserNode *next = current->next;
        free(current);
        current = next;
    }
}

int count_user_list(UserNode *head) {
    int count = 0;
    UserNode *current = head;
    while (current) {
        count++;
        current = current->next;
    }
    return count;
}

PatientNode* create_patient_node(const Patient *patient) {
    PatientNode *node = (PatientNode *)malloc(sizeof(PatientNode));
    if (node) {
        node->data = *patient;
        node->next = NULL;
    }
    return node;
}

void free_patient_list(PatientNode *head) {
    PatientNode *current = head;
    while (current) {
        PatientNode *next = current->next;
        free(current);
        current = next;
    }
}

int count_patient_list(PatientNode *head) {
    int count = 0;
    PatientNode *current = head;
    while (current) {
        count++;
        current = current->next;
    }
    return count;
}

DoctorNode* create_doctor_node(const Doctor *doctor) {
    DoctorNode *node = (DoctorNode *)malloc(sizeof(DoctorNode));
    if (node) {
        node->data = *doctor;
        node->next = NULL;
    }
    return node;
}

void free_doctor_list(DoctorNode *head) {
    DoctorNode *current = head;
    while (current) {
        DoctorNode *next = current->next;
        free(current);
        current = next;
    }
}

int count_doctor_list(DoctorNode *head) {
    int count = 0;
    DoctorNode *current = head;
    while (current) {
        count++;
        current = current->next;
    }
    return count;
}

DepartmentNode* create_department_node(const Department *department) {
    DepartmentNode *node = (DepartmentNode *)malloc(sizeof(DepartmentNode));
    if (node) {
        node->data = *department;
        node->next = NULL;
    }
    return node;
}

void free_department_list(DepartmentNode *head) {
    DepartmentNode *current = head;
    while (current) {
        DepartmentNode *next = current->next;
        free(current);
        current = next;
    }
}

int count_department_list(DepartmentNode *head) {
    int count = 0;
    DepartmentNode *current = head;
    while (current) {
        count++;
        current = current->next;
    }
    return count;
}

DrugNode* create_drug_node(const Drug *drug) {
    DrugNode *node = (DrugNode *)malloc(sizeof(DrugNode));
    if (node) {
        node->data = *drug;
        node->next = NULL;
    }
    return node;
}

void free_drug_list(DrugNode *head) {
    DrugNode *current = head;
    while (current) {
        DrugNode *next = current->next;
        free(current);
        current = next;
    }
}

int count_drug_list(DrugNode *head) {
    int count = 0;
    DrugNode *current = head;
    while (current) {
        count++;
        current = current->next;
    }
    return count;
}

WardNode* create_ward_node(const Ward *ward) {
    WardNode *node = (WardNode *)malloc(sizeof(WardNode));
    if (node) {
        node->data = *ward;
        node->next = NULL;
    }
    return node;
}

void free_ward_list(WardNode *head) {
    WardNode *current = head;
    while (current) {
        WardNode *next = current->next;
        free(current);
        current = next;
    }
}

int count_ward_list(WardNode *head) {
    int count = 0;
    WardNode *current = head;
    while (current) {
        count++;
        current = current->next;
    }
    return count;
}

AppointmentNode* create_appointment_node(const Appointment *appointment) {
    AppointmentNode *node = (AppointmentNode *)malloc(sizeof(AppointmentNode));
    if (node) {
        node->data = *appointment;
        node->next = NULL;
    }
    return node;
}

void free_appointment_list(AppointmentNode *head) {
    AppointmentNode *current = head;
    while (current) {
        AppointmentNode *next = current->next;
        free(current);
        current = next;
    }
}

int count_appointment_list(AppointmentNode *head) {
    int count = 0;
    AppointmentNode *current = head;
    while (current) {
        count++;
        current = current->next;
    }
    return count;
}

OnsiteRegistrationNode* create_onsite_registration_node(const OnsiteRegistration *registration) {
    OnsiteRegistrationNode *node = (OnsiteRegistrationNode *)malloc(sizeof(OnsiteRegistrationNode));
    if (node) {
        node->data = *registration;
        node->next = NULL;
    }
    return node;
}

void free_onsite_registration_list(OnsiteRegistrationNode *head) {
    OnsiteRegistrationNode *current = head;
    while (current) {
        OnsiteRegistrationNode *next = current->next;
        free(current);
        current = next;
    }
}

int count_onsite_registration_list(OnsiteRegistrationNode *head) {
    int count = 0;
    OnsiteRegistrationNode *current = head;
    while (current) {
        count++;
        current = current->next;
    }
    return count;
}

void init_onsite_registration_queue(OnsiteRegistrationQueue *queue) {
    if (!queue) {
        return;
    }
    queue->front = NULL;
    queue->rear = NULL;
    queue->size = 0;
}

int enqueue_onsite_registration(OnsiteRegistrationQueue *queue, const OnsiteRegistration *registration, bool front) {
    OnsiteRegistrationNode *node;

    if (!queue || !registration) {
        return ERROR_INVALID_INPUT;
    }

    node = create_onsite_registration_node(registration);
    if (!node) {
        return ERROR_FILE_IO;
    }

    if (front) {
        // Insert at front (for emergency patients)
        node->next = queue->front;
        queue->front = node;
        if (!queue->rear) queue->rear = node;
    } else {
        // Normal: insert at rear
        if (!queue->rear) {
            queue->front = node;
            queue->rear = node;
        } else {
            queue->rear->next = node;
            queue->rear = node;
        }
    }

    queue->size++;
    return SUCCESS;
}

int dequeue_onsite_registration(OnsiteRegistrationQueue *queue, OnsiteRegistration *registration) {
    OnsiteRegistrationNode *node;

    if (!queue || !queue->front) {
        return ERROR_NOT_FOUND;
    }

    node = queue->front;
    if (registration) {
        *registration = node->data;
    }

    queue->front = node->next;
    if (!queue->front) {
        queue->rear = NULL;
    }

    queue->size--;
    free(node);
    return SUCCESS;
}

void free_onsite_registration_queue(OnsiteRegistrationQueue *queue) {
    if (!queue) {
        return;
    }
    free_onsite_registration_list(queue->front);
    init_onsite_registration_queue(queue);
}

WardCallNode* create_ward_call_node(const WardCall *call) {
    WardCallNode *node = (WardCallNode *)malloc(sizeof(WardCallNode));
    if (node) {
        node->data = *call;
        node->next = NULL;
    }
    return node;
}

void free_ward_call_list(WardCallNode *head) {
    WardCallNode *current = head;
    while (current) {
        WardCallNode *next = current->next;
        free(current);
        current = next;
    }
}

int count_ward_call_list(WardCallNode *head) {
    int count = 0;
    WardCallNode *current = head;
    while (current) {
        count++;
        current = current->next;
    }
    return count;
}

MedicalRecordNode* create_medical_record_node(const MedicalRecord *record) {
    MedicalRecordNode *node = (MedicalRecordNode *)malloc(sizeof(MedicalRecordNode));
    if (node) {
        node->data = *record;
        node->next = NULL;
    }
    return node;
}

void free_medical_record_list(MedicalRecordNode *head) {
    MedicalRecordNode *current = head;
    while (current) {
        MedicalRecordNode *next = current->next;
        free(current);
        current = next;
    }
}

int count_medical_record_list(MedicalRecordNode *head) {
    int count = 0;
    MedicalRecordNode *current = head;
    while (current) {
        count++;
        current = current->next;
    }
    return count;
}

PrescriptionNode* create_prescription_node(const Prescription *prescription) {
    PrescriptionNode *node = (PrescriptionNode *)malloc(sizeof(PrescriptionNode));
    if (node) {
        node->data = *prescription;
        node->next = NULL;
    }
    return node;
}

void free_prescription_list(PrescriptionNode *head) {
    PrescriptionNode *current = head;
    while (current) {
        PrescriptionNode *next = current->next;
        free(current);
        current = next;
    }
}

int count_prescription_list(PrescriptionNode *head) {
    int count = 0;
    PrescriptionNode *current = head;
    while (current) {
        count++;
        current = current->next;
    }
    return count;
}

// ==================== 链表版本的数据加载和保存函数 ====================

UserNode* load_users_list(void) {
    FILE *fp = fopen(USERS_FILE, "r");
    if (!fp) {
        return NULL;
    }

    UserNode *head = NULL;
    UserNode *tail = NULL;
    char line[TEXT_LINE_SIZE];

    while (read_data_line(fp, line, sizeof(line))) {
        char *cursor = line;
        User user;
        memset(&user, 0, sizeof(user));
        strncpy(user.username, next_token(&cursor), sizeof(user.username) - 1);
        user.username[sizeof(user.username) - 1] = '\0';
        strncpy(user.password, next_token(&cursor), sizeof(user.password) - 1);
        user.password[sizeof(user.password) - 1] = '\0';
        strncpy(user.role, next_token(&cursor), sizeof(user.role) - 1);
        user.role[sizeof(user.role) - 1] = '\0';

        UserNode *node = create_user_node(&user);
        if (!node) {
            free_user_list(head);
            fclose(fp);
            return NULL;
        }
        
        if (!head) {
            head = node;
            tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
    }
    
    fclose(fp);
    return head;
}

int save_users_list(UserNode *head) {
    FILE *fp = fopen(USERS_FILE, "w");
    if (!fp) {
        return ERROR_FILE_IO;
    }

    fprintf(fp, "# username\tpassword\trole\n");
    UserNode *current = head;
    while (current) {
        fprintf_escaped(fp, current->data.username); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.password); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.role); fprintf(fp, "\n");
        current = current->next;
    }

    fclose(fp);
    return SUCCESS;
}

PatientNode* load_patients_list(void) {
    FILE *fp = fopen(PATIENTS_FILE, "r");
    if (!fp) {
        return NULL;
    }

    PatientNode *head = NULL;
    PatientNode *tail = NULL;
    char line[TEXT_LINE_SIZE];

    while (read_data_line(fp, line, sizeof(line))) {
        char *cursor = line;
        Patient patient;
        memset(&patient, 0, sizeof(patient));
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(patient.patient_id, tk, sizeof(patient.patient_id) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(patient.username, tk, sizeof(patient.username) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(patient.name, tk, sizeof(patient.name) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(patient.gender, tk, sizeof(patient.gender) - 1); }
        patient.age = parse_int_token(&cursor);
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(patient.phone, tk, sizeof(patient.phone) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(patient.address, tk, sizeof(patient.address) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(patient.patient_type, tk, sizeof(patient.patient_type) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(patient.treatment_stage, tk, sizeof(patient.treatment_stage) - 1); }
        patient.is_emergency = parse_bool_token(&cursor);

        PatientNode *node = create_patient_node(&patient);
        if (!node) {
            free_patient_list(head);
            fclose(fp);
            return NULL;
        }
        
        if (!head) {
            head = node;
            tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
    }
    
    fclose(fp);
    return head;
}

int save_patients_list(PatientNode *head) {
    FILE *fp = fopen(PATIENTS_FILE, "w");
    if (!fp) {
        return ERROR_FILE_IO;
    }

    fprintf(fp, "# patient_id\tusername\tname\tgender\tage\tphone\taddress\tpatient_type\ttreatment_stage\tis_emergency\n");
    PatientNode *current = head;
    while (current) {
        fprintf_escaped(fp, current->data.patient_id); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.username); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.name); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.gender); fprintf(fp, "\t");
        fprintf(fp, "%d\t", current->data.age);
        fprintf_escaped(fp, current->data.phone); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.address); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.patient_type); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.treatment_stage); fprintf(fp, "\t");
        fprintf(fp, "%d\n", current->data.is_emergency ? 1 : 0);
        current = current->next;
    }

    fclose(fp);
    return SUCCESS;
}

DoctorNode* load_doctors_list(void) {
    FILE *fp = fopen(DOCTORS_FILE, "r");
    if (!fp) {
        return NULL;
    }

    DoctorNode *head = NULL;
    DoctorNode *tail = NULL;
    char line[TEXT_LINE_SIZE];

    while (read_data_line(fp, line, sizeof(line))) {
        char *cursor = line;
        Doctor doctor;
        memset(&doctor, 0, sizeof(doctor));
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(doctor.doctor_id, tk, sizeof(doctor.doctor_id) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(doctor.username, tk, sizeof(doctor.username) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(doctor.name, tk, sizeof(doctor.name) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(doctor.department_id, tk, sizeof(doctor.department_id) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(doctor.title, tk, sizeof(doctor.title) - 1); }
        doctor.busy_level = parse_int_token(&cursor);

        DoctorNode *node = create_doctor_node(&doctor);
        if (!node) {
            free_doctor_list(head);
            fclose(fp);
            return NULL;
        }
        
        if (!head) {
            head = node;
            tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
    }
    
    fclose(fp);
    return head;
}

int save_doctors_list(DoctorNode *head) {
    FILE *fp = fopen(DOCTORS_FILE, "w");
    if (!fp) {
        return ERROR_FILE_IO;
    }

    fprintf(fp, "# doctor_id\tusername\tname\tdepartment_id\ttitle\tbusy_level\n");
    DoctorNode *current = head;
    while (current) {
        fprintf_escaped(fp, current->data.doctor_id); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.username); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.name); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.department_id); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.title); fprintf(fp, "\t");
        fprintf(fp, "%d\n", current->data.busy_level);
        current = current->next;
    }

    fclose(fp);
    return SUCCESS;
}

DepartmentNode* load_departments_list(void) {
    FILE *fp = fopen(DEPARTMENTS_FILE, "r");
    if (!fp) {
        return NULL;
    }

    DepartmentNode *head = NULL;
    DepartmentNode *tail = NULL;
    char line[TEXT_LINE_SIZE];

    while (read_data_line(fp, line, sizeof(line))) {
        char *cursor = line;
        Department department;
        memset(&department, 0, sizeof(department));
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(department.department_id, tk, sizeof(department.department_id) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(department.name, tk, sizeof(department.name) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(department.leader, tk, sizeof(department.leader) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(department.phone, tk, sizeof(department.phone) - 1); }

        DepartmentNode *node = create_department_node(&department);
        if (!node) {
            free_department_list(head);
            fclose(fp);
            return NULL;
        }
        
        if (!head) {
            head = node;
            tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
    }
    
    fclose(fp);
    return head;
}

int save_departments_list(DepartmentNode *head) {
    FILE *fp = fopen(DEPARTMENTS_FILE, "w");
    if (!fp) {
        return ERROR_FILE_IO;
    }

    fprintf(fp, "# department_id\tname\tleader\tphone\n");
    DepartmentNode *current = head;
    while (current) {
        fprintf_escaped(fp, current->data.department_id); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.name); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.leader); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.phone); fprintf(fp, "\n");
        current = current->next;
    }

    fclose(fp);
    return SUCCESS;
}

DrugNode* load_drugs_list(void) {
    FILE *fp = fopen(DRUGS_FILE, "r");
    if (!fp) {
        return NULL;
    }

    DrugNode *head = NULL;
    DrugNode *tail = NULL;
    char line[TEXT_LINE_SIZE];

    while (read_data_line(fp, line, sizeof(line))) {
        char *cursor = line;
        Drug drug;
        memset(&drug, 0, sizeof(drug));
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(drug.drug_id, tk, sizeof(drug.drug_id) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(drug.name, tk, sizeof(drug.name) - 1); }
        drug.price = parse_float_token(&cursor);
        drug.stock_num = parse_int_token(&cursor);
        drug.warning_line = parse_int_token(&cursor);
        drug.is_special = parse_bool_token(&cursor);
        drug.reimbursement_ratio = parse_float_token(&cursor);
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(drug.category, tk, sizeof(drug.category) - 1); }
        if (drug.category[0] == '\0') strcpy(drug.category, "西药");

        DrugNode *node = create_drug_node(&drug);
        if (!node) {
            free_drug_list(head);
            fclose(fp);
            return NULL;
        }
        
        if (!head) {
            head = node;
            tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
    }
    
    fclose(fp);
    return head;
}

int save_drugs_list(DrugNode *head) {
    FILE *fp = fopen(DRUGS_FILE, "w");
    if (!fp) {
        return ERROR_FILE_IO;
    }

    fprintf(fp, "# drug_id\tname\tprice\tstock_num\twarning_line\tis_special\treimbursement_ratio\tcategory\n");
    DrugNode *current = head;
    while (current) {
        fprintf_escaped(fp, current->data.drug_id); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.name); fprintf(fp, "\t");
        fprintf(fp, "%.2f\t", current->data.price);
        fprintf(fp, "%d\t", current->data.stock_num);
        fprintf(fp, "%d\t", current->data.warning_line);
        fprintf(fp, "%d\t", current->data.is_special ? 1 : 0);
        fprintf(fp, "%.2f\t", current->data.reimbursement_ratio);
        fprintf_escaped(fp, current->data.category); fprintf(fp, "\n");
        current = current->next;
    }

    fclose(fp);
    return SUCCESS;
}

WardNode* load_wards_list(void) {
    FILE *fp = fopen(WARDS_FILE, "r");
    if (!fp) {
        return NULL;
    }

    WardNode *head = NULL;
    WardNode *tail = NULL;
    char line[TEXT_LINE_SIZE];

    while (read_data_line(fp, line, sizeof(line))) {
        char *cursor = line;
        Ward ward;
        memset(&ward, 0, sizeof(ward));
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(ward.ward_id, tk, sizeof(ward.ward_id) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(ward.type, tk, sizeof(ward.type) - 1); }
        ward.total_beds = parse_int_token(&cursor);
        ward.remain_beds = parse_int_token(&cursor);
        ward.warning_line = parse_int_token(&cursor);

        WardNode *node = create_ward_node(&ward);
        if (!node) {
            free_ward_list(head);
            fclose(fp);
            return NULL;
        }
        
        if (!head) {
            head = node;
            tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
    }
    
    fclose(fp);
    return head;
}

int save_wards_list(WardNode *head) {
    FILE *fp = fopen(WARDS_FILE, "w");
    if (!fp) {
        return ERROR_FILE_IO;
    }

    fprintf(fp, "# ward_id\ttype\ttotal_beds\tremain_beds\twarning_line\n");
    WardNode *current = head;
    while (current) {
        fprintf_escaped(fp, current->data.ward_id); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.type); fprintf(fp, "\t");
        fprintf(fp, "%d\t%d\t%d\n",
                current->data.total_beds,
                current->data.remain_beds,
                current->data.warning_line);
        current = current->next;
    }

    fclose(fp);
    return SUCCESS;
}

AppointmentNode* load_appointments_list(void) {
    FILE *fp = fopen(APPOINTMENTS_FILE, "r");
    if (!fp) {
        return NULL;
    }

    AppointmentNode *head = NULL;
    AppointmentNode *tail = NULL;
    char line[TEXT_LINE_SIZE];

    while (read_data_line(fp, line, sizeof(line))) {
        char *cursor = line;
        Appointment appointment;
        memset(&appointment, 0, sizeof(appointment));
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(appointment.appointment_id, tk, sizeof(appointment.appointment_id) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(appointment.patient_id, tk, sizeof(appointment.patient_id) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(appointment.doctor_id, tk, sizeof(appointment.doctor_id) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(appointment.department_id, tk, sizeof(appointment.department_id) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(appointment.appointment_date, tk, sizeof(appointment.appointment_date) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(appointment.appointment_time, tk, sizeof(appointment.appointment_time) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(appointment.status, tk, sizeof(appointment.status) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(appointment.create_time, tk, sizeof(appointment.create_time) - 1); }

        AppointmentNode *node = create_appointment_node(&appointment);
        if (!node) {
            free_appointment_list(head);
            fclose(fp);
            return NULL;
        }

        if (!head) {
            head = node;
            tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
    }

    fclose(fp);
    return head;
}

int save_appointments_list(AppointmentNode *head) {
    FILE *fp = fopen(APPOINTMENTS_FILE, "w");
    if (!fp) {
        return ERROR_FILE_IO;
    }

    fprintf(fp, "# appointment_id\tpatient_id\tdoctor_id\tdepartment_id\tappointment_date\tappointment_time\tstatus\tcreate_time\n");
    AppointmentNode *current = head;
    while (current) {
        fprintf_escaped(fp, current->data.appointment_id); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.patient_id); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.doctor_id); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.department_id); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.appointment_date); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.appointment_time); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.status); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.create_time); fprintf(fp, "\n");
        current = current->next;
    }

    fclose(fp);
    return SUCCESS;
}

OnsiteRegistrationQueue load_onsite_registration_queue(void) {
    FILE *fp = fopen(ONSITE_REGISTRATIONS_FILE, "r");
    OnsiteRegistrationQueue queue;
    char line[TEXT_LINE_SIZE];

    init_onsite_registration_queue(&queue);
    if (!fp) {
        return queue;
    }

    while (read_data_line(fp, line, sizeof(line))) {
        char *cursor = line;
        OnsiteRegistration registration;
        memset(&registration, 0, sizeof(registration));
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(registration.onsite_id, tk, sizeof(registration.onsite_id) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(registration.patient_id, tk, sizeof(registration.patient_id) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(registration.doctor_id, tk, sizeof(registration.doctor_id) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(registration.department_id, tk, sizeof(registration.department_id) - 1); }
        registration.queue_number = parse_int_token(&cursor);
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(registration.status, tk, sizeof(registration.status) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(registration.create_time, tk, sizeof(registration.create_time) - 1); }

        if (enqueue_onsite_registration(&queue, &registration, false) != SUCCESS) {
            free_onsite_registration_queue(&queue);
            break;
        }
    }

    fclose(fp);
    return queue;
}

int save_onsite_registration_queue(const OnsiteRegistrationQueue *queue) {
    FILE *fp = fopen(ONSITE_REGISTRATIONS_FILE, "w");
    OnsiteRegistrationNode *current;

    if (!fp) {
        return ERROR_FILE_IO;
    }

    fprintf(fp, "# onsite_id\tpatient_id\tdoctor_id\tdepartment_id\tqueue_number\tstatus\tcreate_time\n");
    current = queue ? queue->front : NULL;
    while (current) {
        fprintf_escaped(fp, current->data.onsite_id); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.patient_id); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.doctor_id); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.department_id); fprintf(fp, "\t");
        fprintf(fp, "%d\t", current->data.queue_number);
        fprintf_escaped(fp, current->data.status); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.create_time); fprintf(fp, "\n");
        current = current->next;
    }

    fclose(fp);
    return SUCCESS;
}

int get_next_onsite_queue_number(const char *doctor_id, const char *department_id) {
    OnsiteRegistrationQueue queue = load_onsite_registration_queue();
    OnsiteRegistrationNode *current = queue.front;
    int max_queue_number = 0;

    while (current) {
        if (strcmp(current->data.doctor_id, doctor_id) == 0 &&
            strcmp(current->data.department_id, department_id) == 0 &&
            current->data.queue_number > max_queue_number) {
            max_queue_number = current->data.queue_number;
        }
        current = current->next;
    }

    free_onsite_registration_queue(&queue);
    return max_queue_number + 1;
}

WardCallNode* load_ward_calls_list(void) {
    FILE *fp = fopen(WARD_CALLS_FILE, "r");
    WardCallNode *head = NULL;
    WardCallNode *tail = NULL;
    char line[TEXT_LINE_SIZE];

    if (!fp) {
        return NULL;
    }

    while (read_data_line(fp, line, sizeof(line))) {
        char *cursor = line;
        WardCall call;
        memset(&call, 0, sizeof(call));
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(call.call_id, tk, sizeof(call.call_id) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(call.ward_id, tk, sizeof(call.ward_id) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(call.department_id, tk, sizeof(call.department_id) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(call.patient_id, tk, sizeof(call.patient_id) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(call.message, tk, sizeof(call.message) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(call.status, tk, sizeof(call.status) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(call.create_time, tk, sizeof(call.create_time) - 1); }

        WardCallNode *node = create_ward_call_node(&call);
        if (!node) {
            free_ward_call_list(head);
            fclose(fp);
            return NULL;
        }

        if (!head) {
            head = node;
            tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
    }

    fclose(fp);
    return head;
}

int save_ward_calls_list(WardCallNode *head) {
    FILE *fp = fopen(WARD_CALLS_FILE, "w");
    WardCallNode *current = head;

    if (!fp) {
        return ERROR_FILE_IO;
    }

    fprintf(fp, "# call_id\tward_id\tdepartment_id\tpatient_id\tmessage\tstatus\tcreate_time\n");
    while (current) {
        fprintf_escaped(fp, current->data.call_id); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.ward_id); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.department_id); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.patient_id); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.message); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.status); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.create_time); fprintf(fp, "\n");
        current = current->next;
    }

    fclose(fp);
    return SUCCESS;
}

// ==================== 排班操作函数 ====================

ScheduleNode* create_schedule_node(const Schedule *schedule) {
    ScheduleNode *node = (ScheduleNode *)malloc(sizeof(ScheduleNode));
    if (node) {
        node->data = *schedule;
        node->next = NULL;
    }
    return node;
}

void free_schedule_list(ScheduleNode *head) {
    ScheduleNode *current = head;
    while (current) {
        ScheduleNode *next = current->next;
        free(current);
        current = next;
    }
}

int count_schedule_list(ScheduleNode *head) {
    int count = 0;
    ScheduleNode *current = head;
    while (current) {
        count++;
        current = current->next;
    }
    return count;
}

ScheduleNode* load_schedules_list(void) {
    FILE *fp = fopen(SCHEDULES_FILE, "r");
    if (!fp) {
        return NULL;
    }

    ScheduleNode *head = NULL;
    ScheduleNode *tail = NULL;
    char line[TEXT_LINE_SIZE];

    while (read_data_line(fp, line, sizeof(line))) {
        char *cursor = line;
        Schedule schedule;
        memset(&schedule, 0, sizeof(schedule));
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(schedule.schedule_id, tk, sizeof(schedule.schedule_id) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(schedule.doctor_id, tk, sizeof(schedule.doctor_id) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(schedule.work_date, tk, sizeof(schedule.work_date) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(schedule.time_slot, tk, sizeof(schedule.time_slot) - 1); }
        schedule.max_appt = parse_int_token(&cursor);
        schedule.max_onsite = parse_int_token(&cursor);
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(schedule.status, tk, sizeof(schedule.status) - 1); }

        ScheduleNode *node = create_schedule_node(&schedule);
        if (!node) {
            free_schedule_list(head);
            fclose(fp);
            return NULL;
        }

        if (!head) {
            head = node;
            tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
    }

    fclose(fp);
    return head;
}

int save_schedules_list(ScheduleNode *head) {
    FILE *fp = fopen(SCHEDULES_FILE, "w");
    if (!fp) {
        return ERROR_FILE_IO;
    }

    fprintf(fp, "# schedule_id\tdoctor_id\twork_date\ttime_slot\tmax_appt\tmax_onsite\tstatus\n");
    ScheduleNode *current = head;
    while (current) {
        fprintf_escaped(fp, current->data.schedule_id); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.doctor_id); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.work_date); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.time_slot); fprintf(fp, "\t");
        fprintf(fp, "%d\t%d\t", current->data.max_appt, current->data.max_onsite);
        fprintf_escaped(fp, current->data.status); fprintf(fp, "\n");
        current = current->next;
    }

    fclose(fp);
    return SUCCESS;
}

int has_doctor_schedule(const char *doctor_id, const char *date) {
    ScheduleNode *head = load_schedules_list();
    if (!head) return 0;

    ScheduleNode *current = head;
    while (current) {
        if (strcmp(current->data.doctor_id, doctor_id) == 0 &&
            strcmp(current->data.work_date, date) == 0 &&
            strcmp(current->data.status, "正常") == 0) {
            free_schedule_list(head);
            return 1;
        }
        current = current->next;
    }

    free_schedule_list(head);
    return 0;
}

MedicalRecordNode* load_medical_records_list(void) {
    FILE *fp = fopen(MEDICAL_RECORDS_FILE, "r");
    if (!fp) {
        return NULL;
    }

    MedicalRecordNode *head = NULL;
    MedicalRecordNode *tail = NULL;
    char line[TEXT_LINE_SIZE];

    while (read_data_line(fp, line, sizeof(line))) {
        char *cursor = line;
        MedicalRecord record;
        memset(&record, 0, sizeof(record));
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(record.record_id, tk, sizeof(record.record_id) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(record.patient_id, tk, sizeof(record.patient_id) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(record.doctor_id, tk, sizeof(record.doctor_id) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(record.appointment_id, tk, sizeof(record.appointment_id) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(record.diagnosis, tk, sizeof(record.diagnosis) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(record.diagnosis_date, tk, sizeof(record.diagnosis_date) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(record.status, tk, sizeof(record.status) - 1); }

        MedicalRecordNode *node = create_medical_record_node(&record);
        if (!node) {
            free_medical_record_list(head);
            fclose(fp);
            return NULL;
        }
        
        if (!head) {
            head = node;
            tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
    }
    
    fclose(fp);
    return head;
}

int save_medical_records_list(MedicalRecordNode *head) {
    FILE *fp = fopen(MEDICAL_RECORDS_FILE, "w");
    if (!fp) {
        return ERROR_FILE_IO;
    }

    fprintf(fp, "# record_id\tpatient_id\tdoctor_id\tappointment_id\tdiagnosis\tdiagnosis_date\tstatus\n");
    MedicalRecordNode *current = head;
    while (current) {
        fprintf_escaped(fp, current->data.record_id); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.patient_id); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.doctor_id); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.appointment_id); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.diagnosis); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.diagnosis_date); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.status); fprintf(fp, "\n");
        current = current->next;
    }

    fclose(fp);
    return SUCCESS;
}

PrescriptionNode* load_prescriptions_list(void) {
    FILE *fp = fopen(PRESCRIPTIONS_FILE, "r");
    if (!fp) {
        return NULL;
    }

    PrescriptionNode *head = NULL;
    PrescriptionNode *tail = NULL;
    char line[TEXT_LINE_SIZE];

    while (read_data_line(fp, line, sizeof(line))) {
        char *cursor = line;
        Prescription prescription;
        memset(&prescription, 0, sizeof(prescription));
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(prescription.prescription_id, tk, sizeof(prescription.prescription_id) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(prescription.record_id, tk, sizeof(prescription.record_id) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(prescription.patient_id, tk, sizeof(prescription.patient_id) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(prescription.doctor_id, tk, sizeof(prescription.doctor_id) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(prescription.drug_id, tk, sizeof(prescription.drug_id) - 1); }
        prescription.quantity = parse_int_token(&cursor);
        prescription.total_price = parse_float_token(&cursor);
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(prescription.prescription_date, tk, sizeof(prescription.prescription_date) - 1); }

        PrescriptionNode *node = create_prescription_node(&prescription);
        if (!node) {
            free_prescription_list(head);
            fclose(fp);
            return NULL;
        }
        
        if (!head) {
            head = node;
            tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
    }
    
    fclose(fp);
    return head;
}

int save_prescriptions_list(PrescriptionNode *head) {
    FILE *fp = fopen(PRESCRIPTIONS_FILE, "w");
    if (!fp) {
        return ERROR_FILE_IO;
    }

    fprintf(fp, "# prescription_id\trecord_id\tpatient_id\tdoctor_id\tdrug_id\tquantity\ttotal_price\tprescription_date\n");
    PrescriptionNode *current = head;
    while (current) {
        fprintf_escaped(fp, current->data.prescription_id); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.record_id); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.patient_id); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.doctor_id); fprintf(fp, "\t");
        fprintf_escaped(fp, current->data.drug_id); fprintf(fp, "\t");
        fprintf(fp, "%d\t%.2f\t", current->data.quantity, current->data.total_price);
        fprintf_escaped(fp, current->data.prescription_date); fprintf(fp, "\n");
        current = current->next;
    }

    fclose(fp);
    return SUCCESS;
}

// ==================== Helper Functions ====================

Patient* find_patient_by_username(const char *username) {
    PatientNode *head = load_patients_list();
    if (!head) {
        return NULL;
    }
    
    PatientNode *current = head;
    while (current) {
        if (strcmp(current->data.username, username) == 0) {
            Patient *patient = (Patient *)malloc(sizeof(Patient));
            if (patient) {
                *patient = current->data;
            }
            free_patient_list(head);
            return patient;
        }
        current = current->next;
    }
    
    free_patient_list(head);
    return NULL;
}

Patient* find_patient_by_id(const char *patient_id) {
    PatientNode *head = load_patients_list();
    if (!head) {
        return NULL;
    }
    
    PatientNode *current = head;
    while (current) {
        if (strcmp(current->data.patient_id, patient_id) == 0) {
            Patient *patient = (Patient *)malloc(sizeof(Patient));
            if (patient) {
                *patient = current->data;
            }
            free_patient_list(head);
            return patient;
        }
        current = current->next;
    }
    
    free_patient_list(head);
    return NULL;
}

int ensure_patient_profile(const char *username) {
    PatientNode *head = load_patients_list();
    if (!head) {
        // 创建新患者
        Patient new_patient = {0};
        generate_id(new_patient.patient_id, MAX_ID, "P");
        strcpy(new_patient.username, username);
        strcpy(new_patient.name, username);
        strcpy(new_patient.gender, "未知");
        strcpy(new_patient.patient_type, "普通");
        strcpy(new_patient.treatment_stage, "初诊");
        new_patient.is_emergency = false;
        
        PatientNode *node = create_patient_node(&new_patient);
        if (!node) {
            return ERROR_FILE_IO;
        }
        
        int result = save_patients_list(node);
        free_patient_list(node);
        return result;
    }
    
    // 检查是否已存在
    PatientNode *current = head;
    while (current) {
        if (strcmp(current->data.username, username) == 0) {
            free_patient_list(head);
            return SUCCESS;
        }
        current = current->next;
    }
    
    // 添加新患者
    Patient new_patient = {0};
    generate_id(new_patient.patient_id, MAX_ID, "P");
    strcpy(new_patient.username, username);
    strcpy(new_patient.name, username);
    strcpy(new_patient.gender, "未知");
    strcpy(new_patient.patient_type, "普通");
    strcpy(new_patient.treatment_stage, "初诊");
    new_patient.is_emergency = false;
    
    PatientNode *node = create_patient_node(&new_patient);
    if (!node) {
        free_patient_list(head);
        return ERROR_FILE_IO;
    }
    
    // 找到链表尾部
    PatientNode *tail = head;
    while (tail->next) {
        tail = tail->next;
    }
    tail->next = node;
    
    int result = save_patients_list(head);
    free_patient_list(head);
    return result;
}

Doctor* find_doctor_by_username(const char *username) {
    DoctorNode *head = load_doctors_list();
    if (!head) {
        return NULL;
    }
    
    DoctorNode *current = head;
    while (current) {
        if (strcmp(current->data.username, username) == 0) {
            Doctor *doctor = (Doctor *)malloc(sizeof(Doctor));
            if (doctor) {
                *doctor = current->data;
            }
            free_doctor_list(head);
            return doctor;
        }
        current = current->next;
    }
    
    free_doctor_list(head);
    return NULL;
}

Doctor* find_doctor_by_id(const char *doctor_id) {
    DoctorNode *head = load_doctors_list();
    if (!head) {
        return NULL;
    }
    
    DoctorNode *current = head;
    while (current) {
        if (strcmp(current->data.doctor_id, doctor_id) == 0) {
            Doctor *doctor = (Doctor *)malloc(sizeof(Doctor));
            if (doctor) {
                *doctor = current->data;
            }
            free_doctor_list(head);
            return doctor;
        }
        current = current->next;
    }
    
    free_doctor_list(head);
    return NULL;
}

void generate_doctor_id(const char *department_id, char *out_buf, int buf_size) {
    char dept_letter = 'X';
    int max_seq = 0;

    if (department_id && department_id[0] != '\0') {
        DepartmentNode *dept_head = load_departments_list();
        DepartmentNode *dc = dept_head;
        int letter_idx = 0;
        while (dc) {
            if (strcmp(dc->data.department_id, department_id) == 0) {
                dept_letter = (char)('A' + letter_idx);
                break;
            }
            letter_idx++;
            if (letter_idx > 25) letter_idx = 25;
            dc = dc->next;
        }
        free_department_list(dept_head);
    }

    {
        DoctorNode *dh = load_doctors_list();
        DoctorNode *dcur = dh;
        while (dcur) {
            if (dcur->data.doctor_id[0] == dept_letter) {
                int seq = (int)strtol(dcur->data.doctor_id + 1, NULL, 10);
                if (seq > max_seq) max_seq = seq;
            }
            dcur = dcur->next;
        }
        free_doctor_list(dh);
    }

    snprintf(out_buf, buf_size, "%c%04d", dept_letter, max_seq + 1);
}

void migrate_doctor_ids(void) {
    DoctorNode *dh = load_doctors_list();
    DoctorNode *doc = dh;
    int changed = 0;
    char old_to_new[500][2][MAX_ID];
    int map_count = 0;

    if (!dh) return;

    while (doc) {
        int id_len = (int)strlen(doc->data.doctor_id);
        if (id_len > 5) {
            char new_id[MAX_ID];
            DoctorNode *dh2 = load_doctors_list();
            DoctorNode *tmp = dh2;
            int dept_idx = 0;
            if (doc->data.department_id[0] != '\0') {
                DepartmentNode *dept_head = load_departments_list();
                DepartmentNode *dc = dept_head;
                while (dc) {
                    if (strcmp(dc->data.department_id, doc->data.department_id) == 0) break;
                    dept_idx++;
                    if (dept_idx > 25) dept_idx = 25;
                    dc = dc->next;
                }
                free_department_list(dept_head);
            }

            char letter = doc->data.department_id[0] ? (char)('A' + dept_idx) : 'X';
            int max_s = 0;
            while (tmp) {
                if (strcmp(tmp->data.doctor_id, doc->data.doctor_id) == 0) {
                    tmp = tmp->next;
                    continue;
                }
                if (tmp->data.doctor_id[0] == letter) {
                    int s = (int)strtol(tmp->data.doctor_id + 1, NULL, 10);
                    if (s > max_s) max_s = s;
                }
                tmp = tmp->next;
            }
            free_doctor_list(dh2);

            snprintf(new_id, sizeof(new_id), "%c%04d", letter, max_s + 1);

            if (map_count < 200) {
                strcpy(old_to_new[map_count][0], doc->data.doctor_id);
                strcpy(old_to_new[map_count][1], new_id);
                map_count++;
            }

            strcpy(doc->data.doctor_id, new_id);
            changed = 1;
        }
        doc = doc->next;
    }

    if (changed) {
        save_doctors_list(dh);

        {
            AppointmentNode *ah = load_appointments_list();
            AppointmentNode *ac = ah;
            while (ac) {
                int i;
                for (i = 0; i < map_count; i++) {
                    if (strcmp(ac->data.doctor_id, old_to_new[i][0]) == 0) {
                        strcpy(ac->data.doctor_id, old_to_new[i][1]);
                        break;
                    }
                }
                ac = ac->next;
            }
            save_appointments_list(ah);
            free_appointment_list(ah);
        }

        {
            OnsiteRegistrationQueue oq = load_onsite_registration_queue();
            OnsiteRegistrationNode *oc = oq.front;
            while (oc) {
                int i;
                for (i = 0; i < map_count; i++) {
                    if (strcmp(oc->data.doctor_id, old_to_new[i][0]) == 0) {
                        strcpy(oc->data.doctor_id, old_to_new[i][1]);
                        break;
                    }
                }
                oc = oc->next;
            }
            save_onsite_registration_queue(&oq);
            free_onsite_registration_queue(&oq);
        }

        {
            MedicalRecordNode *mh = load_medical_records_list();
            MedicalRecordNode *mc = mh;
            while (mc) {
                int i;
                for (i = 0; i < map_count; i++) {
                    if (strcmp(mc->data.doctor_id, old_to_new[i][0]) == 0) {
                        strcpy(mc->data.doctor_id, old_to_new[i][1]);
                        break;
                    }
                }
                mc = mc->next;
            }
            save_medical_records_list(mh);
            free_medical_record_list(mh);
        }

        {
            PrescriptionNode *ph = load_prescriptions_list();
            PrescriptionNode *pc = ph;
            while (pc) {
                int i;
                for (i = 0; i < map_count; i++) {
                    if (strcmp(pc->data.doctor_id, old_to_new[i][0]) == 0) {
                        strcpy(pc->data.doctor_id, old_to_new[i][1]);
                        break;
                    }
                }
                pc = pc->next;
            }
            save_prescriptions_list(ph);
            free_prescription_list(ph);
        }
    }

    free_doctor_list(dh);
}

void update_doctor_id_across_files(const char *old_id, const char *new_id) {
    {
        AppointmentNode *ah = load_appointments_list();
        AppointmentNode *ac = ah;
        while (ac) {
            if (strcmp(ac->data.doctor_id, old_id) == 0)
                strcpy(ac->data.doctor_id, new_id);
            ac = ac->next;
        }
        save_appointments_list(ah);
        free_appointment_list(ah);
    }
    {
        OnsiteRegistrationQueue oq = load_onsite_registration_queue();
        OnsiteRegistrationNode *oc = oq.front;
        while (oc) {
            if (strcmp(oc->data.doctor_id, old_id) == 0)
                strcpy(oc->data.doctor_id, new_id);
            oc = oc->next;
        }
        save_onsite_registration_queue(&oq);
        free_onsite_registration_queue(&oq);
    }
    {
        MedicalRecordNode *mh = load_medical_records_list();
        MedicalRecordNode *mc = mh;
        while (mc) {
            if (strcmp(mc->data.doctor_id, old_id) == 0)
                strcpy(mc->data.doctor_id, new_id);
            mc = mc->next;
        }
        save_medical_records_list(mh);
        free_medical_record_list(mh);
    }
    {
        PrescriptionNode *ph = load_prescriptions_list();
        PrescriptionNode *pc = ph;
        while (pc) {
            if (strcmp(pc->data.doctor_id, old_id) == 0)
                strcpy(pc->data.doctor_id, new_id);
            pc = pc->next;
        }
        save_prescriptions_list(ph);
        free_prescription_list(ph);
    }
}

int ensure_doctor_profile(const char *username) {
    DoctorNode *head = load_doctors_list();
    if (!head) {
        Doctor new_doctor = {0};
        generate_doctor_id("", new_doctor.doctor_id, MAX_ID);
        strcpy(new_doctor.username, username);
        strcpy(new_doctor.name, username);
        strcpy(new_doctor.title, "医生");
        
        DoctorNode *node = create_doctor_node(&new_doctor);
        if (!node) {
            return ERROR_FILE_IO;
        }
        
        int result = save_doctors_list(node);
        free_doctor_list(node);
        return result;
    }
    
    // 检查是否已存在
    DoctorNode *current = head;
    while (current) {
        if (strcmp(current->data.username, username) == 0) {
            free_doctor_list(head);
            return SUCCESS;
        }
        current = current->next;
    }
    
    // 添加新医生
    Doctor new_doctor = {0};
    generate_doctor_id("", new_doctor.doctor_id, MAX_ID);
    strcpy(new_doctor.username, username);
    strcpy(new_doctor.name, username);
    strcpy(new_doctor.title, "医生");
    
    DoctorNode *node = create_doctor_node(&new_doctor);
    if (!node) {
        free_doctor_list(head);
        return ERROR_FILE_IO;
    }
    
    // 找到链表尾部
    DoctorNode *tail = head;
    while (tail->next) {
        tail = tail->next;
    }
    tail->next = node;
    
    int result = save_doctors_list(head);
    free_doctor_list(head);
    return result;
}

int create_doctor_profile_with_details(const char *username, const char *name, const char *title, const char *department_id) {
    DoctorNode *head = load_doctors_list();
    DoctorNode *current = head;

    while (current) {
        if (strcmp(current->data.username, username) == 0) {
            free_doctor_list(head);
            return SUCCESS;
        }
        current = current->next;
    }

    Doctor new_doctor = {0};
    generate_doctor_id(department_id, new_doctor.doctor_id, MAX_ID);
    strcpy(new_doctor.username, username);
    strcpy(new_doctor.name, name);
    strcpy(new_doctor.title, title);
    if (department_id && department_id[0] != '\0') {
        strcpy(new_doctor.department_id, department_id);
    }

    DoctorNode *node = create_doctor_node(&new_doctor);
    if (!node) {
        if (head) free_doctor_list(head);
        return ERROR_FILE_IO;
    }

    if (!head) {
        int result = save_doctors_list(node);
        free_doctor_list(node);
        return result;
    }

    DoctorNode *tail = head;
    while (tail->next) {
        tail = tail->next;
    }
    tail->next = node;

    int result = save_doctors_list(head);
    free_doctor_list(head);
    return result;
}

Drug* find_drug_by_id(const char *drug_id) {
    DrugNode *head = load_drugs_list();
    if (!head) {
        return NULL;
    }
    
    DrugNode *current = head;
    while (current) {
        if (strcmp(current->data.drug_id, drug_id) == 0) {
            Drug *drug = (Drug *)malloc(sizeof(Drug));
            if (drug) {
                *drug = current->data;
            }
            free_drug_list(head);
            return drug;
        }
        current = current->next;
    }
    
    free_drug_list(head);
    return NULL;
}

Appointment* find_appointments_by_patient(const char *patient_id) {
    AppointmentNode *head = load_appointments_list();
    if (!head) {
        return NULL;
    }
    
    AppointmentNode *current = head;
    while (current) {
        if (strcmp(current->data.patient_id, patient_id) == 0) {
            Appointment *appointment = (Appointment *)malloc(sizeof(Appointment));
            if (appointment) {
                *appointment = current->data;
            }
            free_appointment_list(head);
            return appointment;
        }
        current = current->next;
    }
    
    free_appointment_list(head);
    return NULL;
}

Appointment* find_appointments_by_doctor(const char *doctor_id) {
    AppointmentNode *head = load_appointments_list();
    if (!head) {
        return NULL;
    }
    
    AppointmentNode *current = head;
    while (current) {
        if (strcmp(current->data.doctor_id, doctor_id) == 0) {
            Appointment *appointment = (Appointment *)malloc(sizeof(Appointment));
            if (appointment) {
                *appointment = current->data;
            }
            free_appointment_list(head);
            return appointment;
        }
        current = current->next;
    }
    
    free_appointment_list(head);
    return NULL;
}

MedicalRecord* find_records_by_patient(const char *patient_id) {
    MedicalRecordNode *head = load_medical_records_list();
    if (!head) {
        return NULL;
    }
    
    MedicalRecordNode *current = head;
    while (current) {
        if (strcmp(current->data.patient_id, patient_id) == 0) {
            MedicalRecord *record = (MedicalRecord *)malloc(sizeof(MedicalRecord));
            if (record) {
                *record = current->data;
            }
            free_medical_record_list(head);
            return record;
        }
        current = current->next;
    }
    
    free_medical_record_list(head);
    return NULL;
}

TemplateNode* create_template_node(const MedicalTemplate *tmpl) {
    TemplateNode *node = (TemplateNode *)malloc(sizeof(TemplateNode));
    if (node) {
        node->data = *tmpl;
        node->next = NULL;
    }
    return node;
}

void free_template_list(TemplateNode *head) {
    TemplateNode *current = head;
    while (current) {
        TemplateNode *next = current->next;
        free(current);
        current = next;
    }
}

TemplateNode* load_templates_list(void) {
    FILE *fp = fopen(TEMPLATES_FILE, "r");
    if (!fp) return NULL;
    TemplateNode *head = NULL, *tail = NULL;
    char line[TEXT_LINE_SIZE];
    while (read_data_line(fp, line, sizeof(line))) {
        char *cursor = line;
        MedicalTemplate tmpl;
        memset(&tmpl, 0, sizeof(tmpl));
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(tmpl.template_id, tk, sizeof(tmpl.template_id) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(tmpl.category, tk, sizeof(tmpl.category) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(tmpl.shortcut, tk, sizeof(tmpl.shortcut) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(tmpl.text, tk, sizeof(tmpl.text) - 1); }
        TemplateNode *node = create_template_node(&tmpl);
        if (!node) { free_template_list(head); fclose(fp); return NULL; }
        if (!head) { head = node; tail = node; }
        else { tail->next = node; tail = node; }
    }
    fclose(fp);
    return head;
}

int save_templates_list(TemplateNode *head) {
    FILE *fp = fopen(TEMPLATES_FILE, "w");
    if (!fp) return ERROR_FILE_IO;
    fprintf(fp, "# template_id\tcategory\tshortcut\ttext\n");
    TemplateNode *cur = head;
    while (cur) {
        fprintf_escaped(fp, cur->data.template_id); fprintf(fp, "\t");
        fprintf_escaped(fp, cur->data.category); fprintf(fp, "\t");
        fprintf_escaped(fp, cur->data.shortcut); fprintf(fp, "\t");
        fprintf_escaped(fp, cur->data.text); fprintf(fp, "\n");
        cur = cur->next;
    }
    fclose(fp);
    return SUCCESS;
}

int ensure_default_templates(void) {
    TemplateNode *existing = load_templates_list();
    if (existing) { free_template_list(existing); return SUCCESS; }

    MedicalTemplate defaults[] = {
        {"T001", "诊断", "上呼吸道感染", "急性上呼吸道感染，伴发热、咳嗽、咽痛"},
        {"T002", "治疗", "上呼吸道感染", "口服抗生素3天，多饮水，注意休息，复查体温"},
        {"T003", "检查", "上感套餐", "血常规、CRP"},
        {"T004", "诊断", "急性胃肠炎", "急性胃肠炎，伴腹痛、腹泻、恶心呕吐"},
        {"T005", "治疗", "急性胃肠炎", "口服补液盐，蒙脱石散，清淡饮食3天"},
        {"T006", "检查", "胃肠套餐", "血常规、粪便常规"},
        {"T007", "诊断", "高血压", "原发性高血压，测血压升高"},
        {"T008", "治疗", "高血压", "降压药物治疗，低盐低脂饮食，定期监测血压"},
        {"T009", "检查", "高血压套餐", "血压动态监测、血脂四项、心电图"},
        {"T010", "诊断", "糖尿病", "2型糖尿病，血糖控制不佳"},
        {"T011", "治疗", "糖尿病", "口服降糖药/胰岛素，控制饮食，适量运动"},
        {"T012", "检查", "糖尿病套餐", "空腹血糖、糖化血红蛋白、尿微量白蛋白"},
        {"T013", "诊断", "肺炎", "肺部感染，伴咳嗽咳痰、发热"},
        {"T014", "治疗", "肺炎", "抗生素治疗7天，祛痰止咳，必要时住院"},
        {"T015", "检查", "肺部套餐", "胸片、血常规、CRP、痰培养"},
        {"T016", "诊断", "皮炎", "皮肤炎症，局部红肿瘙痒"},
        {"T017", "治疗", "皮炎", "外用激素药膏，口服抗组胺药，避免刺激"},
        {"T018", "检查", "皮肤套餐", "皮损检查、过敏原筛查"},
    };
    int count = sizeof(defaults) / sizeof(defaults[0]);
    TemplateNode *head = NULL, *tail = NULL;
    int i;
    for (i = 0; i < count; i++) {
        TemplateNode *node = create_template_node(&defaults[i]);
        if (!node) { free_template_list(head); return ERROR_FILE_IO; }
        if (!head) { head = node; tail = node; }
        else { tail->next = node; tail = node; }
    }
    int result = save_templates_list(head);
    free_template_list(head);
    return result;
}

// ======================= 操作日志 =======================

LogEntryNode* create_log_entry_node(const LogEntry *entry) {
    LogEntryNode *node = malloc(sizeof(LogEntryNode));
    if (!node) return NULL;
    node->data = *entry;
    node->next = NULL;
    return node;
}

void free_log_entry_list(LogEntryNode *head) {
    while (head) {
        LogEntryNode *next = head->next;
        free(head);
        head = next;
    }
}

int count_log_entry_list(LogEntryNode *head) {
    int count = 0;
    while (head) { count++; head = head->next; }
    return count;
}

LogEntryNode* load_logs_list(void) {
    FILE *fp = fopen(LOGS_FILE, "r");
    if (!fp) return NULL;

    LogEntryNode *head = NULL, *tail = NULL;
    char line[TEXT_LINE_SIZE];
    while (read_data_line(fp, line, sizeof(line))) {
        char *cursor = line;
        LogEntry entry;
        memset(&entry, 0, sizeof(entry));
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(entry.log_id, tk, MAX_ID - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(entry.operator_name, tk, MAX_USERNAME - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(entry.action, tk, sizeof(entry.action) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(entry.target, tk, sizeof(entry.target) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(entry.target_id, tk, MAX_ID - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(entry.detail, tk, sizeof(entry.detail) - 1); }
        { char *tk = next_token(&cursor); unescape_field_inplace(tk); strncpy(entry.create_time, tk, sizeof(entry.create_time) - 1); }

        LogEntryNode *node = create_log_entry_node(&entry);
        if (!node) { free_log_entry_list(head); fclose(fp); return NULL; }
        if (!tail) { head = node; tail = node; }
        else { tail->next = node; tail = node; }
    }

    fclose(fp);
    return head;
}

int append_log(const char *operator_name, const char *action, const char *target,
               const char *target_id, const char *detail) {
    FILE *fp = fopen(LOGS_FILE, "a");
    if (!fp) return ERROR_FILE_IO;

    // If first time, write header
    fseek(fp, 0, SEEK_END);
    if (ftell(fp) == 0) {
        fprintf(fp, "# log_id\toperator\taction\ttarget\ttarget_id\tdetail\tcreate_time\n");
    }

    char log_id[MAX_ID];
    generate_id(log_id, MAX_ID, "L");
    char create_time[30];
    get_current_time(create_time, sizeof(create_time));

    fprintf_escaped(fp, log_id); fprintf(fp, "\t");
    fprintf_escaped(fp, operator_name); fprintf(fp, "\t");
    fprintf_escaped(fp, action); fprintf(fp, "\t");
    fprintf_escaped(fp, target); fprintf(fp, "\t");
    fprintf_escaped(fp, target_id); fprintf(fp, "\t");
    fprintf_escaped(fp, detail); fprintf(fp, "\t");
    fprintf_escaped(fp, create_time); fprintf(fp, "\n");
    fclose(fp);
    return SUCCESS;
}

// ======================= 数据备份与恢复 =======================

static const char *backup_data_files[] = {
    "users.txt", "patients.txt", "doctors.txt", "departments.txt",
    "drugs.txt", "wards.txt", "appointments.txt", "onsite_registrations.txt",
    "ward_calls.txt", "medical_records.txt", "prescriptions.txt",
    "templates.txt", "schedules.txt", "logs.txt"
};
static const int backup_file_count = sizeof(backup_data_files) / sizeof(backup_data_files[0]);
#define MAX_BACKUPS 20

#ifdef _WIN32
#include <io.h>
static void remove_oldest_backup(void) {
    struct _finddata_t fd;
    intptr_t handle = _findfirst(DATA_DIR "/backup/*", &fd);
    if (handle == -1L) return;

    char oldest[64] = "";
    int count = 0;
    do {
        if (fd.attrib & _A_SUBDIR) {
            if (strcmp(fd.name, ".") == 0 || strcmp(fd.name, "..") == 0) continue;
            count++;
            if (oldest[0] == '\0' || strcmp(fd.name, oldest) < 0)
                strncpy(oldest, fd.name, sizeof(oldest) - 1);
        }
    } while (_findnext(handle, &fd) == 0);
    _findclose(handle);

    if (count > MAX_BACKUPS && oldest[0]) {
        char path[256];
        snprintf(path, sizeof(path), DATA_DIR "/backup/%s", oldest);
        // Remove all files in the backup directory first
        char search[256];
        snprintf(search, sizeof(search), "%s/*", path);
        struct _finddata_t ffd;
        intptr_t fh = _findfirst(search, &ffd);
        if (fh != -1L) {
            do {
                if (strcmp(ffd.name, ".") != 0 && strcmp(ffd.name, "..") != 0) {
                    char fp[512];
                    snprintf(fp, sizeof(fp), "%s/%s", path, ffd.name);
                    remove(fp);
                }
            } while (_findnext(fh, &ffd) == 0);
            _findclose(fh);
        }
        _rmdir(path);
        printf("  ⚠ 已清理最旧备份: %s (超过 %d 个限制)\n", oldest, MAX_BACKUPS);
    }
}
#else
static void remove_oldest_backup(void) {
    // Unix version - simplified
    (void)0;
}
#endif

static int copy_file(const char *src, const char *dst) {
    FILE *fs = fopen(src, "rb");
    if (!fs) return ERROR_FILE_IO;
    FILE *fd = fopen(dst, "wb");
    if (!fd) { fclose(fs); return ERROR_FILE_IO; }
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fs)) > 0) {
        if (fwrite(buf, 1, n, fd) != n) { fclose(fs); fclose(fd); return ERROR_FILE_IO; }
    }
    fclose(fs);
    fclose(fd);
    return SUCCESS;
}

int backup_data(void) {
    // Create backup directory
#ifdef _WIN32
    _mkdir(DATA_DIR "/backup");
#else
    mkdir(DATA_DIR "/backup", 0755);
#endif

    char dir_name[64];
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(dir_name, sizeof(dir_name), DATA_DIR "/backup/%Y%m%d_%H%M%S", tm);

#ifdef _WIN32
    if (_mkdir(dir_name) != 0) return ERROR_FILE_IO;
#else
    if (mkdir(dir_name, 0755) != 0) return ERROR_FILE_IO;
#endif

    int success_count = 0;
    for (int i = 0; i < backup_file_count; i++) {
        char src[256], dst[256];
        snprintf(src, sizeof(src), DATA_DIR "/%s", backup_data_files[i]);
        snprintf(dst, sizeof(dst), "%s/%s", dir_name, backup_data_files[i]);
        if (copy_file(src, dst) == SUCCESS) success_count++;
    }

    printf("备份完成: %s (%d/%d 文件)\n", dir_name, success_count, backup_file_count);

    if (success_count > 0) {
        remove_oldest_backup();
    }

    return SUCCESS;
}

int list_backups(const char ***out_names, int *out_count) {
    *out_names = NULL;
    *out_count = 0;

#ifdef _WIN32
    struct _finddata_t fd;
    intptr_t handle = _findfirst(DATA_DIR "/backup/*", &fd);
    if (handle == -1L) return SUCCESS;

    int capacity = 0, count = 0;
    char **names = NULL;
    do {
        if (fd.attrib & _A_SUBDIR) {
            if (strcmp(fd.name, ".") == 0 || strcmp(fd.name, "..") == 0) continue;
            if (count >= capacity) {
                capacity = capacity ? capacity * 2 : 16;
                char **tmp = realloc(names, capacity * sizeof(char *));
                if (!tmp) { free_backups_list((const char **)names, count); _findclose(handle); return ERROR_FILE_IO; }
                names = tmp;
            }
            names[count] = malloc(strlen(fd.name) + 1);
            if (!names[count]) { free_backups_list((const char **)names, count); _findclose(handle); return ERROR_FILE_IO; }
            strcpy(names[count], fd.name);
            count++;
        }
    } while (_findnext(handle, &fd) == 0);
    _findclose(handle);

    *out_names = (const char **)names;
    *out_count = count;
#endif
    return SUCCESS;
}

void free_backups_list(const char **names, int count) {
    if (!names) return;
    for (int i = 0; i < count; i++) {
        free((void *)names[i]);
    }
    free(names);
}

int restore_data(const char *backup_dir) {
    char full_path[256];
    int success_count = 0;

    for (int i = 0; i < backup_file_count; i++) {
        char src[256], dst[256];
        snprintf(src, sizeof(src), DATA_DIR "/backup/%s/%s", backup_dir, backup_data_files[i]);
        snprintf(dst, sizeof(dst), DATA_DIR "/%s", backup_data_files[i]);
        if (copy_file(src, dst) == SUCCESS) success_count++;
    }

    printf("还原完成: %s (%d/%d 文件)\n", backup_dir, success_count, backup_file_count);
    return SUCCESS;
}

