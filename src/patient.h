/*
 * patient.h — 患者功能模块 / Patient function module
 *
 * 患者角色的核心业务功能: 预约挂号 (选择科室→医生→日期→时段)、
 * 现场挂号 (当天就诊)、挂号状态查询与退号、诊断记录查询、病房信息查看、
 * 治疗进度查看、个人资料编辑。
 *
 * 关键约束: 同一患者不能同时有多个活跃挂号 (防冲突);
 * 预约限未来 7 天; 每半天时段限 6 人; 现场每天限 8 人。
 *
 * Core patient workflow: appointment booking (department→doctor→date→time slot),
 * onsite registration (same-day), appointment status query with cancellation,
 * diagnosis record query, ward info view, treatment progress view, profile editing.
 *
 * Key constraints: one active registration per patient (conflict prevention);
 * appointments limited to 7 future days; 6 per half-day slot; 8 onsite per day.
 */

#ifndef PATIENT_H
#define PATIENT_H

#include "common.h"
#include "data_storage.h"

/* 患者模块主菜单 (8 项功能入口) / Patient main menu (8 function entries) */
void patient_main_menu(const User *current_user);

/* 预约挂号: 选择科室→医生→日期→时段，检查排班容量和患者冲突
   Appointment registration: dept→doctor→date→time slot with schedule capacity &
   conflict checks, creates appointment record */
int patient_register_menu(const User *current_user);

/* 挂号状态查询: 查看预约/现场挂号列表，支持取消预约 (+退号扣减医生繁忙度)
   Appointment query: list appointments & onsite registrations, allow cancellation
   (cancellation decrements doctor busy_level) */
int patient_appointment_menu(const User *current_user);

/* 诊断查询: 分页查看本人的病历记录和处方详情
   Diagnosis query: paginated view of own medical records & prescription details */
int patient_query_diagnosis_menu(const User *current_user);

/* 病房信息查看: 查看各病房类型/床位/剩余床位等
   Ward info: view ward types, total/remaining beds */
int patient_view_ward_menu(const User *current_user);

/* 治疗进度查看: 查看当前治疗阶段和历史阶段变化
   Treatment progress: view current treatment stage and stage history */
int patient_view_treatment_progress_menu(const User *current_user);

/* 显示所有可用科室列表 / Display all available departments */
void show_available_departments(void);

/* 显示指定科室下的医生列表 / Display doctors in a given department */
void show_doctors_by_department(const char *department_id);

/* 编辑个人资料: 修改姓名/性别/年龄/电话/地址/患者类型等
   Edit profile: modify name/gender/age/phone/address/patient type */
int patient_edit_profile_menu(const User *current_user);

#endif // PATIENT_H
