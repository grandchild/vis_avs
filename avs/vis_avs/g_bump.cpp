#include "c_bump.h"

#include "g__defs.h"
#include "g__lib.h"

#include "c__defs.h"
#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_bump(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    C_THISCLASS* g_ConfigThis = (C_THISCLASS*)g_current_render;
    switch (uMsg) {
        case WM_INITDIALOG:
            SetDlgItemText(hwndDlg, IDC_CODE1, (char*)g_ConfigThis->code1.c_str());
            SetDlgItemText(hwndDlg, IDC_CODE2, (char*)g_ConfigThis->code2.c_str());
            SetDlgItemText(hwndDlg, IDC_CODE3, (char*)g_ConfigThis->code3.c_str());
            SendDlgItemMessage(hwndDlg, IDC_DEPTH, TBM_SETTICFREQ, 10, 0);
            SendDlgItemMessage(
                hwndDlg, IDC_DEPTH, TBM_SETRANGE, TRUE, MAKELONG(1, 100));
            SendDlgItemMessage(
                hwndDlg, IDC_DEPTH, TBM_SETPOS, TRUE, g_ConfigThis->depth);
            SendDlgItemMessage(hwndDlg, IDC_DEPTH2, TBM_SETTICFREQ, 10, 0);
            SendDlgItemMessage(
                hwndDlg, IDC_DEPTH2, TBM_SETRANGE, TRUE, MAKELONG(1, 100));
            SendDlgItemMessage(
                hwndDlg, IDC_DEPTH2, TBM_SETPOS, TRUE, g_ConfigThis->depth2);
            SendDlgItemMessage(hwndDlg, IDC_BEATDUR, TBM_SETTICFREQ, 10, 0);
            SendDlgItemMessage(
                hwndDlg, IDC_BEATDUR, TBM_SETRANGE, TRUE, MAKELONG(1, 100));
            SendDlgItemMessage(
                hwndDlg, IDC_BEATDUR, TBM_SETPOS, TRUE, g_ConfigThis->durFrames);
            if (g_ConfigThis->enabled) CheckDlgButton(hwndDlg, IDC_CHECK1, BST_CHECKED);
            if (g_ConfigThis->invert)
                CheckDlgButton(hwndDlg, IDC_INVERTDEPTH, BST_CHECKED);
            if (g_ConfigThis->onbeat) CheckDlgButton(hwndDlg, IDC_ONBEAT, BST_CHECKED);
            if (g_ConfigThis->blend) CheckDlgButton(hwndDlg, IDC_ADDITIVE, BST_CHECKED);
            if (g_ConfigThis->blendavg) CheckDlgButton(hwndDlg, IDC_5050, BST_CHECKED);
            if (g_ConfigThis->showlight) CheckDlgButton(hwndDlg, IDC_DOT, BST_CHECKED);
            if (!g_ConfigThis->blend && !g_ConfigThis->blendavg)
                CheckDlgButton(hwndDlg, IDC_REPLACE, BST_CHECKED);
            SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_ADDSTRING, 0, (int)"Current");
            {
                int i = 0;
                char txt[64];
                for (i = 0; i < NBUF; i++) {
                    wsprintf(txt, "Buffer %d", i + 1);
                    SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_ADDSTRING, 0, (int)txt);
                }
            }
            SendDlgItemMessage(
                hwndDlg, IDC_COMBO1, CB_SETCURSEL, (WPARAM)g_ConfigThis->buffern, 0);
            return 1;
        case WM_NOTIFY: {
            if (LOWORD(wParam) == IDC_DEPTH)
                g_ConfigThis->depth =
                    SendDlgItemMessage(hwndDlg, IDC_DEPTH, TBM_GETPOS, 0, 0);
            if (LOWORD(wParam) == IDC_DEPTH2)
                g_ConfigThis->depth2 =
                    SendDlgItemMessage(hwndDlg, IDC_DEPTH2, TBM_GETPOS, 0, 0);
            if (LOWORD(wParam) == IDC_BEATDUR)
                g_ConfigThis->durFrames =
                    SendDlgItemMessage(hwndDlg, IDC_BEATDUR, TBM_GETPOS, 0, 0);
        }
            return 0;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_HELPBTN) {
                char* text =
                    "Bump Light Position\0"
                    "How to use the custom light position evaluator:\r\n"
                    " * Init code will be executed each time the window size is "
                    "changed\r\n"
                    "   or when the effect loads\r\n"
                    " * Frame code is executed before rendering a new frame\r\n"
                    " * Beat code is executed when a beat is detected\r\n"
                    "\r\n"
                    "Predefined variables:\r\n"
                    " x : Light x position, ranges from 0 (left) to 1 (right) (0.5 = "
                    "center)\r\n"
                    " y : Light y position, ranges from 0 (top) to 1 (bottom) (0.5 = "
                    "center)\r\n"
                    " isBeat : 1 if no beat, -1 if beat (weird, but old)\r\n"
                    " isLBeat: same as isBeat but persists according to "
                    "'shorter/longer' settings\r\n"
                    "          (usable only with OnBeat checked)\r\n"
                    " bi:	Bump intensity, ranges	from 0 (flat) to 1 (max "
                    "specified bump, default)\r\n"
                    " You may also use temporary variables accross code segments\r\n"
                    "\r\n"
                    "Some examples:\r\n"
                    "   Circular move\r\n"
                    "      Init : t=0\r\n"
                    "      Frame: x=0.5+cos(t)*0.3; y=0.5+sin(t)*0.3; t=t+0.1;\r\n"
                    "   Nice motion:\r\n"
                    "      Init : t=0;u=0\r\n"
                    "      Frame: x=0.5+cos(t)*0.3; y=0.5+cos(u)*0.3; t=t+0.1; "
                    "u=u+0.012;\r\n";
                compilerfunctionlist(hwndDlg, text);
                return 0;
            }
            if ((LOWORD(wParam) == IDC_CHECK1) || (LOWORD(wParam) == IDC_ONBEAT)
                || (LOWORD(wParam) == IDC_ADDITIVE) || (LOWORD(wParam) == IDC_REPLACE)
                || (LOWORD(wParam) == IDC_DOT) || (LOWORD(wParam) == IDC_INVERTDEPTH)
                || (LOWORD(wParam) == IDC_5050)) {
                g_ConfigThis->enabled = IsDlgButtonChecked(hwndDlg, IDC_CHECK1) ? 1 : 0;
                g_ConfigThis->onbeat = IsDlgButtonChecked(hwndDlg, IDC_ONBEAT) ? 1 : 0;
                g_ConfigThis->blend = IsDlgButtonChecked(hwndDlg, IDC_ADDITIVE) ? 1 : 0;
                g_ConfigThis->blendavg = IsDlgButtonChecked(hwndDlg, IDC_5050) ? 1 : 0;
                g_ConfigThis->showlight = IsDlgButtonChecked(hwndDlg, IDC_DOT) ? 1 : 0;
                g_ConfigThis->invert =
                    IsDlgButtonChecked(hwndDlg, IDC_INVERTDEPTH) ? 1 : 0;
            }
            if (LOWORD(wParam) == IDC_CODE1 && HIWORD(wParam) == EN_CHANGE) {
                EnterCriticalSection(&g_ConfigThis->rcs);
                g_ConfigThis->code1 = string_from_dlgitem(hwndDlg, IDC_CODE1);
                g_ConfigThis->need_recompile = 1;
                LeaveCriticalSection(&g_ConfigThis->rcs);
            }
            if (LOWORD(wParam) == IDC_CODE2 && HIWORD(wParam) == EN_CHANGE) {
                EnterCriticalSection(&g_ConfigThis->rcs);
                g_ConfigThis->code2 = string_from_dlgitem(hwndDlg, IDC_CODE2);
                g_ConfigThis->need_recompile = 1;
                LeaveCriticalSection(&g_ConfigThis->rcs);
            }
            if (LOWORD(wParam) == IDC_CODE3 && HIWORD(wParam) == EN_CHANGE) {
                EnterCriticalSection(&g_ConfigThis->rcs);
                g_ConfigThis->code3 = string_from_dlgitem(hwndDlg, IDC_CODE3);
                g_ConfigThis->need_recompile = 1;
                g_ConfigThis->initted = 0;
                LeaveCriticalSection(&g_ConfigThis->rcs);
            }
            if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_COMBO1)  // handle
                                                                                  // clicks
                                                                                  // to
                                                                                  // combo
                                                                                  // box
                g_ConfigThis->buffern =
                    SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0);
            return 0;
    }
    return 0;
}
