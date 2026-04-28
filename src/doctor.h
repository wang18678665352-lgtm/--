#ifndef DOCTOR_H
#define DOCTOR_H

#include "common.h"
#include "data_storage.h"

// Doctor module main menu
void doctor_main_menu(const User *current_user);

// Registration reminder
int doctor_appointment_reminder_menu(const User *current_user);

// Patient consultation
int doctor_consultation_menu(const User *current_user);

// Prescribe medication
int doctor_prescribe_menu(const User *current_user);

// Ward call reminder
int doctor_ward_call_menu(const User *current_user);

// Emergency patient flag
int doctor_emergency_flag_menu(const User *current_user);

// Update treatment progress
int doctor_update_progress_menu(const User *current_user);

// Template management
int doctor_template_menu(const User *current_user);

#endif // DOCTOR_H
