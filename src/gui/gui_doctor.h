#ifndef GUI_DOCTOR_H
#define GUI_DOCTOR_H

#include <windows.h>
#include "../common.h"

/* 创建医生端页面，返回子窗口句柄 */
HWND CreateDoctorPage(HWND hParent, int viewId, RECT *rc);

#endif /* GUI_DOCTOR_H */
