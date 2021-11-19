#include "c_ddm.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_dynamicdistancemodifier(HWND hwndDlg,
                                          UINT uMsg,
                                          WPARAM wParam,
                                          LPARAM) {
    C_THISCLASS* g_this = (C_THISCLASS*)g_current_render;

    static int isstart;
    switch (uMsg) {
        case WM_INITDIALOG:
            isstart = 1;
            SetDlgItemText(hwndDlg, IDC_EDIT1, (char*)g_this->effect_exp[0].c_str());
            SetDlgItemText(hwndDlg, IDC_EDIT2, (char*)g_this->effect_exp[1].c_str());
            SetDlgItemText(hwndDlg, IDC_EDIT3, (char*)g_this->effect_exp[2].c_str());
            SetDlgItemText(hwndDlg, IDC_EDIT4, (char*)g_this->effect_exp[3].c_str());
            isstart = 0;
            if (g_this->blend) CheckDlgButton(hwndDlg, IDC_CHECK1, BST_CHECKED);
            if (g_this->subpixel) CheckDlgButton(hwndDlg, IDC_CHECK2, BST_CHECKED);
            return 1;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_BUTTON1) {
                char* text =
                    "Dynamic Distance Modifier\0"
                    "The dynamic distance modifier allows you to dynamically (once per "
                    "frame)\r\n"
                    "change the source pixels for each ring of pixels out from the "
                    "center.\r\n"
                    "In the 'pixel' code section, 'd' represents the distance in "
                    "pixels\r\n"
                    "the current ring is from the center, and code can modify it to\r\n"
                    "change the distance from the center where the source pixels "
                    "for\r\n"
                    "that ring would be read. This is a terrible explanation, and "
                    "if\r\n"
                    "you want to make a better one send it to me. \r\n"
                    "\r\n"
                    "Examples:\r\n"
                    "Zoom in: 'd=d*0.9'\r\n"
                    "Zoom out: 'd=d*1.1'\r\n"
                    "Back and forth: pixel='d=d*(1.0+0.1*cos(t));', "
                    "frame='t=t+0.1'\r\n";
                compilerfunctionlist(hwndDlg, text);
            }

            if (LOWORD(wParam) == IDC_CHECK1) {
                g_this->blend = IsDlgButtonChecked(hwndDlg, IDC_CHECK1) ? 1 : 0;
            }
            if (LOWORD(wParam) == IDC_CHECK2) {
                g_this->subpixel = IsDlgButtonChecked(hwndDlg, IDC_CHECK2) ? 1 : 0;
            }
            if (!isstart
                && (LOWORD(wParam) == IDC_EDIT1 || LOWORD(wParam) == IDC_EDIT2
                    || LOWORD(wParam) == IDC_EDIT3 || LOWORD(wParam) == IDC_EDIT4)
                && HIWORD(wParam) == EN_CHANGE) {
                EnterCriticalSection(&g_this->rcs);
                g_this->effect_exp[0] = string_from_dlgitem(hwndDlg, IDC_EDIT1);
                g_this->effect_exp[1] = string_from_dlgitem(hwndDlg, IDC_EDIT2);
                g_this->effect_exp[2] = string_from_dlgitem(hwndDlg, IDC_EDIT3);
                g_this->effect_exp[3] = string_from_dlgitem(hwndDlg, IDC_EDIT4);
                g_this->need_recompile = 1;
                if (LOWORD(wParam) == IDC_EDIT4) g_this->inited = 0;
                LeaveCriticalSection(&g_this->rcs);
            }
            return 0;
    }
    return 0;
}
