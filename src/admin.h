/*
 * admin.h — 管理员模块 / Administrator module
 *
 * 管理员拥有系统最高权限，负责所有数据实体的增删改查管理:
 * 科室、医生、患者、药品、病房、排班、统计分析、日志查看、数据备份恢复。
 *
 * 关键约束: 删除前检查外键关联 (如科室下有医生则不能删除);
 * 医生换科室时自动更新所有关联文件中的医生 ID;
 * 药品/病房库存低于警戒线时自动告警。
 *
 * Admin has full system access, managing CRUD for all entities:
 * departments, doctors, patients, drugs, wards, schedules, analysis,
 * log viewing, and data backup/restore.
 *
 * Key constraints: cascade checks before delete (e.g. dept with doctors);
 * auto-update doctor IDs across all files when changing department;
 * automatic warning when drug stock / beds fall below threshold.
 */

#ifndef ADMIN_H
#define ADMIN_H

#include "common.h"

/* 管理员主菜单 (10 项功能入口) / Admin main menu (10 function entries) */
void admin_main_menu(const User *current_user);

/* 科室管理: 增加/修改/删除科室，删除时检查是否有关联医生
   Department management: add/modify/delete, checks for associated doctors */
int admin_department_menu(const User *current_user);

/* 医生管理: 增加/修改/删除医生，支持科室变更+ID自动迁移，
   删除时检查是否有活跃挂号/病历/处方等关联记录
   Doctor management: add/modify/delete with dept-change ID migration,
   cascade check on active records before delete */
int admin_doctor_menu(const User *current_user);

/* 患者管理: 增加/修改/删除患者，删除时检查活跃挂号
   Patient management: add/modify/delete, checks active registrations */
int admin_patient_menu(const User *current_user);

/* 药品管理: 增加/修改/删除/入库补货，自动库存+警戒线检查
   Drug management: add/modify/delete/restock, auto stock & warning checks */
int admin_drug_menu(const User *current_user);

/* 病房管理: 增加/修改/删除/调整床位/发起病房呼叫，床位警戒检查
   Ward management: add/modify/delete/adjust beds/initiate ward calls,
   bed warning threshold checks */
int admin_ward_menu(const User *current_user);

/* 排班管理: 为医生创建/取消每日分时段排班，设置预约+现场最大人数
   Schedule management: create/cancel daily time-slot schedules for doctors,
   set max appointments & onsite patients per slot */
int admin_schedule_menu(const User *current_user);

/* 统计分析: 运营概览/趋势分析/医生负荷/患者画像/财务统计/CSV导出
   Analysis: operations overview/trends/doctor load/patient profile/financial/CSV export */
int admin_analysis_menu(const User *current_user);

/* 日志查看: 查看系统操作日志 (谁在什么时间做了什么操作)
   Log management: view system audit log (who did what and when) */
int admin_log_menu(const User *current_user);

/* 数据管理: 手动备份/恢复数据，查看历史备份列表
   Data management: manual backup/restore, view backup history */
int admin_data_menu(const User *current_user);

#endif // ADMIN_H
