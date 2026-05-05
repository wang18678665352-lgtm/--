/*
 * analysis.h — 数据分析与统计报表模块 / Data analysis & statistics report module
 *
 * 为管理员提供多维度的医院运营数据分析，包括当日运营概览、就诊趋势、
 * 医生负荷、患者画像、财务统计等视图，并支持数据导出为 CSV 格式。
 *
 * Provides admin with multi-dimensional hospital operations analysis:
 * daily overview, visit trends, doctor workload, patient demographics,
 * financial statistics, and CSV data export.
 */

#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "common.h"

/* 管理员分析菜单入口: 展示 5 种分析视图 + CSV 导出选项
   Admin analysis menu: 5 analysis views + CSV export option */
int admin_analysis_menu(const User *current_user);

#endif
