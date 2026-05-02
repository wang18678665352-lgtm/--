#ifndef GUI_PATIENT_H
#define GUI_PATIENT_H

#include <windows.h>
#include "../common.h"

/* 创建患者端页面，返回子窗口句柄 */
HWND CreatePatientPage(HWND hParent, int viewId, RECT *rc);

#endif /* GUI_PATIENT_H */
