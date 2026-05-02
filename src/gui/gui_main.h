#ifndef GUI_MAIN_H
#define GUI_MAIN_H

#include <windows.h>
#include <commctrl.h>
#include "../common.h"

/* 兼容旧版 MinGW 缺少的扩展样式 */
#ifndef LVS_EX_ALTERNATINGROWCOLORS
#define LVS_EX_ALTERNATINGROWCOLORS 0x00000040
#endif
#ifndef LVS_EX_DOUBLEBUFFER
#define LVS_EX_DOUBLEBUFFER 0x00010000
#endif

/* 窗口尺寸 */
#define WINDOW_WIDTH  1024
#define WINDOW_HEIGHT 768
#define WINDOW_MIN_WIDTH  800
#define WINDOW_MIN_HEIGHT 600
#define NAV_WIDTH     200
#define STATUS_HEIGHT 25

/* 导航项 ID（通过 WM_COMMAND wParam 低字节传递） */
#define NAV_BASE          1000
#define NAV_PATIENT_BASE  2000
#define NAV_DOCTOR_BASE   3000
#define NAV_ADMIN_BASE    4000

/* 患者导航 */
enum {
    NAV_PATIENT_REGISTER = NAV_PATIENT_BASE + 1,
    NAV_PATIENT_APPOINTMENT,
    NAV_PATIENT_DIAGNOSIS,
    NAV_PATIENT_PRESCRIPTION,
    NAV_PATIENT_WARD,
    NAV_PATIENT_PROGRESS,
    NAV_PATIENT_PROFILE,
};

/* 医生导航 */
enum {
    NAV_DOCTOR_REMINDER = NAV_DOCTOR_BASE + 1,
    NAV_DOCTOR_CONSULTATION,
    NAV_DOCTOR_WARD_CALL,
    NAV_DOCTOR_EMERGENCY,
    NAV_DOCTOR_PROGRESS,
    NAV_DOCTOR_TEMPLATE,
};

/* 管理员导航 */
enum {
    NAV_ADMIN_DEPT = NAV_ADMIN_BASE + 1,
    NAV_ADMIN_DOCTOR,
    NAV_ADMIN_PATIENT,
    NAV_ADMIN_DRUG,
    NAV_ADMIN_WARD,
    NAV_ADMIN_SCHEDULE,
    NAV_ADMIN_LOG,
    NAV_ADMIN_DATA,
    NAV_ADMIN_ANALYSIS,
};

/* 主窗口函数 */
int RunMainWindow(HINSTANCE hInstance);
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

/* 视图切换 */
void SwitchView(HWND hWnd, int viewId);
HWND GetContentView(HWND hWnd);
void SetStatusText(HWND hWnd, const char *text);

/* 自定义刷新消息：PostMessage(hWnd, WM_APP_REFRESH, viewId, 0) 触发 SwitchView */
#define WM_APP_REFRESH (WM_APP + 1)

/* 全局实例句柄（各模块需要用到） */
extern HINSTANCE g_hInst;
extern User g_currentUser;

#endif /* GUI_MAIN_H */
