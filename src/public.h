#ifndef PUBLIC_H
#define PUBLIC_H

#include "common.h"
#include "data_storage.h"

// Input validation
bool validate_input(const char *input, int type);
bool is_valid_number(const char *str);
bool is_valid_date(const char *date);
bool is_unique_id(const char *id, const char *id_type);

// Warning functions
void check_drug_warning(void);
void check_ward_warning(void);

// Reimbursement calculation
float calculate_reimbursement(float total_amount, const char *patient_type);
float calculate_drug_reimbursement(const Drug *drug, int quantity, const char *patient_type);

// Treatment progress control
const char* get_next_stage(const char *current_stage);

// Doctor recommendation algorithm
int recommend_doctor(const char *department_id);
Doctor* find_recommended_doctor(const char *department_id);
bool is_duplicate_prescription_risk(const char *patient_id, const char *drug_id);

// Record management
int archive_medical_record(int record_id);

#endif // PUBLIC_H
