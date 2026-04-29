#include "data_storage.h"
#include <sys/stat.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
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
    char *start = *cursor;
    char *sep;

    if (!start) {
        return "";
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

int enqueue_onsite_registration(OnsiteRegistrationQueue *queue, const OnsiteRegistration *registration) {
    OnsiteRegistrationNode *node;

    if (!queue || !registration) {
        return ERROR_INVALID_INPUT;
    }

    node = create_onsite_registration_node(registration);
    if (!node) {
        return ERROR_FILE_IO;
    }

    if (!queue->rear) {
        queue->front = node;
        queue->rear = node;
    } else {
        queue->rear->next = node;
        queue->rear = node;
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
        fprintf(fp, "%s\t%s\t%s\n",
                current->data.username,
                current->data.password,
                current->data.role);
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
        strncpy(patient.patient_id, next_token(&cursor), sizeof(patient.patient_id) - 1);
        strncpy(patient.username, next_token(&cursor), sizeof(patient.username) - 1);
        strncpy(patient.name, next_token(&cursor), sizeof(patient.name) - 1);
        strncpy(patient.gender, next_token(&cursor), sizeof(patient.gender) - 1);
        patient.age = parse_int_token(&cursor);
        strncpy(patient.phone, next_token(&cursor), sizeof(patient.phone) - 1);
        strncpy(patient.address, next_token(&cursor), sizeof(patient.address) - 1);
        strncpy(patient.patient_type, next_token(&cursor), sizeof(patient.patient_type) - 1);
        strncpy(patient.treatment_stage, next_token(&cursor), sizeof(patient.treatment_stage) - 1);
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
        fprintf(fp, "%s\t%s\t%s\t%s\t%d\t%s\t%s\t%s\t%s\t%d\n",
                current->data.patient_id,
                current->data.username,
                current->data.name,
                current->data.gender,
                current->data.age,
                current->data.phone,
                current->data.address,
                current->data.patient_type,
                current->data.treatment_stage,
                current->data.is_emergency ? 1 : 0);
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
        strncpy(doctor.doctor_id, next_token(&cursor), sizeof(doctor.doctor_id) - 1);
        strncpy(doctor.username, next_token(&cursor), sizeof(doctor.username) - 1);
        strncpy(doctor.name, next_token(&cursor), sizeof(doctor.name) - 1);
        strncpy(doctor.department_id, next_token(&cursor), sizeof(doctor.department_id) - 1);
        strncpy(doctor.title, next_token(&cursor), sizeof(doctor.title) - 1);
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
        fprintf(fp, "%s\t%s\t%s\t%s\t%s\t%d\n",
                current->data.doctor_id,
                current->data.username,
                current->data.name,
                current->data.department_id,
                current->data.title,
                current->data.busy_level);
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
        strncpy(department.department_id, next_token(&cursor), sizeof(department.department_id) - 1);
        strncpy(department.name, next_token(&cursor), sizeof(department.name) - 1);
        strncpy(department.leader, next_token(&cursor), sizeof(department.leader) - 1);
        strncpy(department.phone, next_token(&cursor), sizeof(department.phone) - 1);

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
        fprintf(fp, "%s\t%s\t%s\t%s\n",
                current->data.department_id,
                current->data.name,
                current->data.leader,
                current->data.phone);
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
        strncpy(drug.drug_id, next_token(&cursor), sizeof(drug.drug_id) - 1);
        strncpy(drug.name, next_token(&cursor), sizeof(drug.name) - 1);
        drug.price = parse_float_token(&cursor);
        drug.stock_num = parse_int_token(&cursor);
        drug.warning_line = parse_int_token(&cursor);
        drug.is_special = parse_bool_token(&cursor);
        drug.reimbursement_ratio = parse_float_token(&cursor);

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

    fprintf(fp, "# drug_id\tname\tprice\tstock_num\twarning_line\tis_special\treimbursement_ratio\n");
    DrugNode *current = head;
    while (current) {
        fprintf(fp, "%s\t%s\t%.2f\t%d\t%d\t%d\t%.2f\n",
                current->data.drug_id,
                current->data.name,
                current->data.price,
                current->data.stock_num,
                current->data.warning_line,
                current->data.is_special ? 1 : 0,
                current->data.reimbursement_ratio);
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
        strncpy(ward.ward_id, next_token(&cursor), sizeof(ward.ward_id) - 1);
        strncpy(ward.type, next_token(&cursor), sizeof(ward.type) - 1);
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
        fprintf(fp, "%s\t%s\t%d\t%d\t%d\n",
                current->data.ward_id,
                current->data.type,
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
        strncpy(appointment.appointment_id, next_token(&cursor), sizeof(appointment.appointment_id) - 1);
        strncpy(appointment.patient_id, next_token(&cursor), sizeof(appointment.patient_id) - 1);
        strncpy(appointment.doctor_id, next_token(&cursor), sizeof(appointment.doctor_id) - 1);
        strncpy(appointment.department_id, next_token(&cursor), sizeof(appointment.department_id) - 1);
        strncpy(appointment.appointment_date, next_token(&cursor), sizeof(appointment.appointment_date) - 1);
        strncpy(appointment.appointment_time, next_token(&cursor), sizeof(appointment.appointment_time) - 1);
        strncpy(appointment.status, next_token(&cursor), sizeof(appointment.status) - 1);
        strncpy(appointment.create_time, next_token(&cursor), sizeof(appointment.create_time) - 1);

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
        fprintf(fp, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
                current->data.appointment_id,
                current->data.patient_id,
                current->data.doctor_id,
                current->data.department_id,
                current->data.appointment_date,
                current->data.appointment_time,
                current->data.status,
                current->data.create_time);
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
        strncpy(registration.onsite_id, next_token(&cursor), sizeof(registration.onsite_id) - 1);
        strncpy(registration.patient_id, next_token(&cursor), sizeof(registration.patient_id) - 1);
        strncpy(registration.doctor_id, next_token(&cursor), sizeof(registration.doctor_id) - 1);
        strncpy(registration.department_id, next_token(&cursor), sizeof(registration.department_id) - 1);
        registration.queue_number = parse_int_token(&cursor);
        strncpy(registration.status, next_token(&cursor), sizeof(registration.status) - 1);
        strncpy(registration.create_time, next_token(&cursor), sizeof(registration.create_time) - 1);

        if (enqueue_onsite_registration(&queue, &registration) != SUCCESS) {
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
        fprintf(fp, "%s\t%s\t%s\t%s\t%d\t%s\t%s\n",
                current->data.onsite_id,
                current->data.patient_id,
                current->data.doctor_id,
                current->data.department_id,
                current->data.queue_number,
                current->data.status,
                current->data.create_time);
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
        strncpy(call.call_id, next_token(&cursor), sizeof(call.call_id) - 1);
        strncpy(call.ward_id, next_token(&cursor), sizeof(call.ward_id) - 1);
        strncpy(call.department_id, next_token(&cursor), sizeof(call.department_id) - 1);
        strncpy(call.patient_id, next_token(&cursor), sizeof(call.patient_id) - 1);
        strncpy(call.message, next_token(&cursor), sizeof(call.message) - 1);
        strncpy(call.status, next_token(&cursor), sizeof(call.status) - 1);
        strncpy(call.create_time, next_token(&cursor), sizeof(call.create_time) - 1);

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
        fprintf(fp, "%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
                current->data.call_id,
                current->data.ward_id,
                current->data.department_id,
                current->data.patient_id,
                current->data.message,
                current->data.status,
                current->data.create_time);
        current = current->next;
    }

    fclose(fp);
    return SUCCESS;
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
        strncpy(record.record_id, next_token(&cursor), sizeof(record.record_id) - 1);
        strncpy(record.patient_id, next_token(&cursor), sizeof(record.patient_id) - 1);
        strncpy(record.doctor_id, next_token(&cursor), sizeof(record.doctor_id) - 1);
        strncpy(record.appointment_id, next_token(&cursor), sizeof(record.appointment_id) - 1);
        strncpy(record.diagnosis, next_token(&cursor), sizeof(record.diagnosis) - 1);
        strncpy(record.diagnosis_date, next_token(&cursor), sizeof(record.diagnosis_date) - 1);
        strncpy(record.status, next_token(&cursor), sizeof(record.status) - 1);

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
        fprintf(fp, "%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
                current->data.record_id,
                current->data.patient_id,
                current->data.doctor_id,
                current->data.appointment_id,
                current->data.diagnosis,
                current->data.diagnosis_date,
                current->data.status);
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
        strncpy(prescription.prescription_id, next_token(&cursor), sizeof(prescription.prescription_id) - 1);
        strncpy(prescription.record_id, next_token(&cursor), sizeof(prescription.record_id) - 1);
        strncpy(prescription.patient_id, next_token(&cursor), sizeof(prescription.patient_id) - 1);
        strncpy(prescription.doctor_id, next_token(&cursor), sizeof(prescription.doctor_id) - 1);
        strncpy(prescription.drug_id, next_token(&cursor), sizeof(prescription.drug_id) - 1);
        prescription.quantity = parse_int_token(&cursor);
        prescription.total_price = parse_float_token(&cursor);
        strncpy(prescription.prescription_date, next_token(&cursor), sizeof(prescription.prescription_date) - 1);

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
        fprintf(fp, "%s\t%s\t%s\t%s\t%s\t%d\t%.2f\t%s\n",
                current->data.prescription_id,
                current->data.record_id,
                current->data.patient_id,
                current->data.doctor_id,
                current->data.drug_id,
                current->data.quantity,
                current->data.total_price,
                current->data.prescription_date);
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
        strncpy(tmpl.template_id, next_token(&cursor), sizeof(tmpl.template_id) - 1);
        strncpy(tmpl.category, next_token(&cursor), sizeof(tmpl.category) - 1);
        strncpy(tmpl.shortcut, next_token(&cursor), sizeof(tmpl.shortcut) - 1);
        strncpy(tmpl.text, next_token(&cursor), sizeof(tmpl.text) - 1);
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
        fprintf(fp, "%s\t%s\t%s\t%s\n",
            cur->data.template_id, cur->data.category,
            cur->data.shortcut, cur->data.text);
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
