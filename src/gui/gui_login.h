#ifndef GUI_LOGIN_H
#define GUI_LOGIN_H

#include <windows.h>
#include "../common.h"

/* 显示登录对话框
 * 返回 SUCCESS 且 user 被填充，或返回错误码
 */
int ShowLoginDialog(HINSTANCE hInst, HWND hParent, User *user);
int ShowRegisterDialog(HINSTANCE hInst, HWND hParent);

INT_PTR CALLBACK LoginDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK RegisterDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

#endif /* GUI_LOGIN_H */
