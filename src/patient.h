#ifndef PATIENT_H
#define PATIENT_H

#include "common.h"
#include "data_storage.h"

// Patient module main menu
void patient_main_menu(const User *current_user);

// Registration (挂号)
int patient_register_menu(const User *current_user);

// Appointment status query
int patient_appointment_menu(const User *current_user);

// Diagnosis query
int patient_query_diagnosis_menu(const User *current_user);

// Ward info view
int patient_view_ward_menu(const User *current_user);

// Treatment progress view
int patient_view_treatment_progress_menu(const User *current_user);

// Helper: Show all available departments
void show_available_departments(void);

// Helper: Show doctors in a department
void show_doctors_by_department(const char *department_id);

// Edit profile
int patient_edit_profile_menu(const User *current_user);

#endif // PATIENT_H
