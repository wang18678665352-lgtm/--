#ifndef ADMIN_H
#define ADMIN_H

#include "common.h"

// Admin module main menu
void admin_main_menu(const User *current_user);

// Department management
int admin_department_menu(const User *current_user);

// Doctor management
int admin_doctor_menu(const User *current_user);

// Patient management
int admin_patient_menu(const User *current_user);

// Drug management
int admin_drug_menu(const User *current_user);

// Ward management
int admin_ward_menu(const User *current_user);

// Schedule management
int admin_schedule_menu(const User *current_user);

// Analysis / Report management
int admin_analysis_menu(const User *current_user);

// Log management
int admin_log_menu(const User *current_user);

// Data management (backup/restore)
int admin_data_menu(const User *current_user);

#endif // ADMIN_H
