#include "c_shift.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_dynamicshift(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    C_THISCLASS* g_this = (C_THISCLASS*)g_current_render;

    static int isstart;
    switch (uMsg) {
        case WM_INITDIALOG:
            isstart = 1;
            SetDlgItemText(hwndDlg, IDC_EDIT1, (char*)g_this->effect_exp[0].c_str());
            SetDlgItemText(hwndDlg, IDC_EDIT2, (char*)g_this->effect_exp[1].c_str());
            SetDlgItemText(hwndDlg, IDC_EDIT3, (char*)g_this->effect_exp[2].c_str());
            isstart = 0;
            if (g_this->blend) CheckDlgButton(hwndDlg, IDC_CHECK1, BST_CHECKED);
            if (g_this->subpixel) CheckDlgButton(hwndDlg, IDC_CHECK2, BST_CHECKED);
            return 1;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_HELPBTN) {
                char* text =
                    "better Dynamic shift help goes here (send me some :)\r\n"
                    "Variables:\r\n"
                    "x,y = amount to shift (in pixels - set these)\r\n"
                    "w,h = width, height (in pixels)\r\n"
                    "b = isBeat\r\n"
                    "alpha = alpha value (0.0-1.0) for blend\r\n";
                compilerfunctionlist(hwndDlg, "Dynamic Shift", text);
            }
            if (LOWORD(wParam) == IDC_CHECK1) {
                g_this->blend = IsDlgButtonChecked(hwndDlg, IDC_CHECK1) ? 1 : 0;
            }
            if (LOWORD(wParam) == IDC_CHECK2) {
                g_this->subpixel = IsDlgButtonChecked(hwndDlg, IDC_CHECK2) ? 1 : 0;
            }
            if (!isstart
                && (LOWORD(wParam) == IDC_EDIT1 || LOWORD(wParam) == IDC_EDIT2
                    || LOWORD(wParam) == IDC_EDIT3)
                && HIWORD(wParam) == EN_CHANGE) {
                lock_lock(g_this->code_lock);
                g_this->effect_exp[0] = string_from_dlgitem(hwndDlg, IDC_EDIT1);
                g_this->effect_exp[1] = string_from_dlgitem(hwndDlg, IDC_EDIT2);
                g_this->effect_exp[2] = string_from_dlgitem(hwndDlg, IDC_EDIT3);
                g_this->need_recompile = 1;
                if (LOWORD(wParam) == IDC_EDIT1) g_this->inited = 0;
                lock_unlock(g_this->code_lock);
            }
            return 0;
    }
    return 0;
}
