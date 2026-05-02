#ifndef GUI_ADMIN_H
#define GUI_ADMIN_H

#include <windows.h>
#include "../common.h"

/* 创建管理员端页面，返回子窗口句柄 */
HWND CreateAdminPage(HWND hParent, int viewId, RECT *rc);

#endif /* GUI_ADMIN_H */
