/*
 * doctor.h — 医生工作模块 / Doctor workflow module
 *
 * 医生角色的核心业务功能: 挂号提醒、接诊问诊、开药处方、病房呼叫、
 * 急诊标记、治疗进度更新、诊断模板管理。
 *
 * 医生只能查看和处理与自己相关的患者 (通过挂号/现场挂号/病历关联)。
 * 所有操作都会生成日志记录 (append_log)。
 *
 * Core doctor workflow: appointment reminders, patient consultation,
 * prescription, ward calls, emergency flagging, treatment progress updates,
 * and diagnosis template management.
 *
 * Doctors can only view/process patients linked to them via appointments,
 * onsite registrations, or medical records. All actions produce audit logs.
 */

#ifndef DOCTOR_H
#define DOCTOR_H

#include "common.h"
#include "data_storage.h"

/* 医生模块主菜单 (8 项功能入口) / Doctor main menu (8 function entries) */
void doctor_main_menu(const User *current_user);

/* 挂号提醒: 显示今日排班 + 当前在待患者列表
   Appointment reminder: show today's schedule + current waiting patients */
int doctor_appointment_reminder_menu(const User *current_user);

/* 接诊问诊: 从待诊队列选择患者，使用模板快速录入诊断/治疗/检查建议
   自动生成/更新病历，推进治疗阶段，标记挂号状态为"已完成"
   Consultation: select from pending queue, use templates for diagnosis/
   treatment/exam input, auto-create/update medical record, advance stage */
int doctor_consultation_menu(const User *current_user);

/* 开药处方: 基于病历开具处方，支持药品搜索/库存检查/重复处方检测(7天内)
   扣减药品库存，计算医保/军人报销比例，生成处方记录
   Prescribe: create prescriptions based on medical records, with drug search,
   stock check, 7-day duplicate detection, stock deduction, reimbursement calc */
int doctor_prescribe_menu(const User *current_user);

/* 病房呼叫: 查看和处理护士站发来的患者病房呼叫请求
   Ward call: view and respond to ward call requests from nurse station */
int doctor_ward_call_menu(const User *current_user);

/* 急诊标记: 为患者设置/取消急诊状态，急诊患者自动创建病房呼叫
   Emergency flag: toggle emergency status for a patient, auto-creates ward call */
int doctor_emergency_flag_menu(const User *current_user);

/* 更新治疗进度: 推进患者的治疗阶段 (初诊→检查→治疗→复查→康复/出院)
   Update treatment progress: advance patient through stages
   (initial→exam→treatment→review→recovery/discharged) */
int doctor_update_progress_menu(const User *current_user);

/* 模板管理: 增删改查诊断/治疗/检查等快捷模板，支持 #编号 快速输入
   Template management: CRUD for diagnosis/treatment/exam quick templates */
int doctor_template_menu(const User *current_user);

#endif // DOCTOR_H
