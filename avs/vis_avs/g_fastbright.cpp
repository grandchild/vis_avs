#include "c_fastbright.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_fastbrightness(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    C_THISCLASS* g_this = (C_THISCLASS*)g_current_render;

    switch (uMsg) {
        case WM_INITDIALOG:
            if (g_this->dir == 0)
                CheckDlgButton(hwndDlg, IDC_RADIO1, BST_CHECKED);
            else if (g_this->dir == 1)
                CheckDlgButton(hwndDlg, IDC_RADIO2, BST_CHECKED);
            else
                CheckDlgButton(hwndDlg, IDC_RADIO3, BST_CHECKED);
            return 1;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_RADIO1)
                if (IsDlgButtonChecked(hwndDlg, IDC_RADIO1)) g_this->dir = 0;
            if (LOWORD(wParam) == IDC_RADIO2)
                if (IsDlgButtonChecked(hwndDlg, IDC_RADIO2)) g_this->dir = 1;
            if (LOWORD(wParam) == IDC_RADIO3)
                if (IsDlgButtonChecked(hwndDlg, IDC_RADIO3)) g_this->dir = 2;
            return 0;
    }
    return 0;
}
