#ifndef DATA_STORAGE_H
#define DATA_STORAGE_H

#include "common.h"

#define DATA_DIR "data"
#define USERS_FILE DATA_DIR "/users.txt"
#define PATIENTS_FILE DATA_DIR "/patients.txt"
#define DOCTORS_FILE DATA_DIR "/doctors.txt"
#define DEPARTMENTS_FILE DATA_DIR "/departments.txt"
#define DRUGS_FILE DATA_DIR "/drugs.txt"
#define WARDS_FILE DATA_DIR "/wards.txt"
#define APPOINTMENTS_FILE DATA_DIR "/appointments.txt"
#define ONSITE_REGISTRATIONS_FILE DATA_DIR "/onsite_registrations.txt"
#define WARD_CALLS_FILE DATA_DIR "/ward_calls.txt"
#define MEDICAL_RECORDS_FILE DATA_DIR "/medical_records.txt"
#define PRESCRIPTIONS_FILE DATA_DIR "/prescriptions.txt"
#define TEMPLATES_FILE    DATA_DIR "/templates.txt"

// 数据结构定义
typedef struct {
    char patient_id[MAX_ID];
    char username[MAX_USERNAME];
    char name[MAX_NAME];
    char gender[10];
    int age;
    char phone[20];
    char address[200];
    char patient_type[20];
    char treatment_stage[20];
    bool is_emergency;
} Patient;

typedef struct {
    char doctor_id[MAX_ID];
    char username[MAX_USERNAME];
    char name[MAX_NAME];
    char department_id[MAX_ID];
    char title[50];
    int busy_level;
} Doctor;

typedef struct {
    char department_id[MAX_ID];
    char name[MAX_NAME];
    char leader[MAX_NAME];
    char phone[20];
} Department;

typedef struct {
    char drug_id[MAX_ID];
    char name[MAX_NAME];
    float price;
    int stock_num;
    int warning_line;
    bool is_special;
    float reimbursement_ratio;
} Drug;

typedef struct {
    char ward_id[MAX_ID];
    char type[50];
    int total_beds;
    int remain_beds;
    int warning_line;
} Ward;

typedef struct {
    char appointment_id[MAX_ID];
    char patient_id[MAX_ID];
    char doctor_id[MAX_ID];
    char department_id[MAX_ID];
    char appointment_date[20];
    char appointment_time[20];
    char status[20];
    char create_time[30];
} Appointment;

typedef struct {
    char onsite_id[MAX_ID];
    char patient_id[MAX_ID];
    char doctor_id[MAX_ID];
    char department_id[MAX_ID];
    int queue_number;
    char status[20];
    char create_time[30];
} OnsiteRegistration;

typedef struct {
    char call_id[MAX_ID];
    char ward_id[MAX_ID];
    char department_id[MAX_ID];
    char patient_id[MAX_ID];
    char message[200];
    char status[20];
    char create_time[30];
} WardCall;

typedef struct {
    char record_id[MAX_ID];
    char patient_id[MAX_ID];
    char doctor_id[MAX_ID];
    char appointment_id[MAX_ID];
    char diagnosis[500];
    char diagnosis_date[20];
    char status[20];
} MedicalRecord;

typedef struct {
    char prescription_id[MAX_ID];
    char record_id[MAX_ID];
    char patient_id[MAX_ID];
    char doctor_id[MAX_ID];
    char drug_id[MAX_ID];
    int quantity;
    float total_price;
    char prescription_date[20];
} Prescription;

typedef struct {
    char template_id[MAX_ID];
    char category[20];
    char shortcut[60];
    char text[500];
} MedicalTemplate;

// 链表节点结构定义
typedef struct UserNode {
    User data;
    struct UserNode *next;
} UserNode;

typedef struct PatientNode {
    Patient data;
    struct PatientNode *next;
} PatientNode;

typedef struct DoctorNode {
    Doctor data;
    struct DoctorNode *next;
} DoctorNode;

typedef struct DepartmentNode {
    Department data;
    struct DepartmentNode *next;
} DepartmentNode;

typedef struct DrugNode {
    Drug data;
    struct DrugNode *next;
} DrugNode;

typedef struct WardNode {
    Ward data;
    struct WardNode *next;
} WardNode;

typedef struct AppointmentNode {
    Appointment data;
    struct AppointmentNode *next;
} AppointmentNode;

typedef struct OnsiteRegistrationNode {
    OnsiteRegistration data;
    struct OnsiteRegistrationNode *next;
} OnsiteRegistrationNode;

typedef struct {
    OnsiteRegistrationNode *front;
    OnsiteRegistrationNode *rear;
    int size;
} OnsiteRegistrationQueue;

typedef struct WardCallNode {
    WardCall data;
    struct WardCallNode *next;
} WardCallNode;

typedef struct MedicalRecordNode {
    MedicalRecord data;
    struct MedicalRecordNode *next;
} MedicalRecordNode;

typedef struct PrescriptionNode {
    Prescription data;
    struct PrescriptionNode *next;
} PrescriptionNode;

typedef struct TemplateNode {
    MedicalTemplate data;
    struct TemplateNode *next;
} TemplateNode;

int init_data_storage(void);

// 链表操作函数
UserNode* create_user_node(const User *user);
void free_user_list(UserNode *head);
int count_user_list(UserNode *head);

PatientNode* create_patient_node(const Patient *patient);
void free_patient_list(PatientNode *head);
int count_patient_list(PatientNode *head);

DoctorNode* create_doctor_node(const Doctor *doctor);
void free_doctor_list(DoctorNode *head);
int count_doctor_list(DoctorNode *head);

DepartmentNode* create_department_node(const Department *department);
void free_department_list(DepartmentNode *head);
int count_department_list(DepartmentNode *head);

DrugNode* create_drug_node(const Drug *drug);
void free_drug_list(DrugNode *head);
int count_drug_list(DrugNode *head);

WardNode* create_ward_node(const Ward *ward);
void free_ward_list(WardNode *head);
int count_ward_list(WardNode *head);

AppointmentNode* create_appointment_node(const Appointment *appointment);
void free_appointment_list(AppointmentNode *head);
int count_appointment_list(AppointmentNode *head);

OnsiteRegistrationNode* create_onsite_registration_node(const OnsiteRegistration *registration);
void free_onsite_registration_list(OnsiteRegistrationNode *head);
int count_onsite_registration_list(OnsiteRegistrationNode *head);

void init_onsite_registration_queue(OnsiteRegistrationQueue *queue);
int enqueue_onsite_registration(OnsiteRegistrationQueue *queue, const OnsiteRegistration *registration);
int dequeue_onsite_registration(OnsiteRegistrationQueue *queue, OnsiteRegistration *registration);
void free_onsite_registration_queue(OnsiteRegistrationQueue *queue);

WardCallNode* create_ward_call_node(const WardCall *call);
void free_ward_call_list(WardCallNode *head);
int count_ward_call_list(WardCallNode *head);

MedicalRecordNode* create_medical_record_node(const MedicalRecord *record);
void free_medical_record_list(MedicalRecordNode *head);
int count_medical_record_list(MedicalRecordNode *head);

PrescriptionNode* create_prescription_node(const Prescription *prescription);
void free_prescription_list(PrescriptionNode *head);
int count_prescription_list(PrescriptionNode *head);

// 数据加载和保存函数
UserNode* load_users_list(void);
int save_users_list(UserNode *head);

PatientNode* load_patients_list(void);
int save_patients_list(PatientNode *head);
Patient* find_patient_by_username(const char *username);
Patient* find_patient_by_id(const char *patient_id);
int ensure_patient_profile(const char *username);

DoctorNode* load_doctors_list(void);
int save_doctors_list(DoctorNode *head);
Doctor* find_doctor_by_username(const char *username);
Doctor* find_doctor_by_id(const char *doctor_id);
int ensure_doctor_profile(const char *username);
int create_doctor_profile_with_details(const char *username, const char *name, const char *title, const char *department_id);
void generate_doctor_id(const char *department_id, char *out_buf, int buf_size);
void migrate_doctor_ids(void);
void update_doctor_id_across_files(const char *old_id, const char *new_id);

DepartmentNode* load_departments_list(void);
int save_departments_list(DepartmentNode *head);

DrugNode* load_drugs_list(void);
int save_drugs_list(DrugNode *head);
Drug* find_drug_by_id(const char *drug_id);

WardNode* load_wards_list(void);
int save_wards_list(WardNode *head);

AppointmentNode* load_appointments_list(void);
int save_appointments_list(AppointmentNode *head);
Appointment* find_appointments_by_patient(const char *patient_id);
Appointment* find_appointments_by_doctor(const char *doctor_id);

OnsiteRegistrationQueue load_onsite_registration_queue(void);
int save_onsite_registration_queue(const OnsiteRegistrationQueue *queue);
int get_next_onsite_queue_number(const char *doctor_id, const char *department_id);

WardCallNode* load_ward_calls_list(void);
int save_ward_calls_list(WardCallNode *head);

MedicalRecordNode* load_medical_records_list(void);
int save_medical_records_list(MedicalRecordNode *head);
MedicalRecord* find_records_by_patient(const char *patient_id);

PrescriptionNode* load_prescriptions_list(void);
int save_prescriptions_list(PrescriptionNode *head);

// 模板操作
TemplateNode* load_templates_list(void);
int save_templates_list(TemplateNode *head);
TemplateNode* create_template_node(const MedicalTemplate *tmpl);
void free_template_list(TemplateNode *head);
int ensure_default_templates(void);

#endif // DATA_STORAGE_H
