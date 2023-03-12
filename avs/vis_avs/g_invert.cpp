#include "e_invert.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_invert(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    E_Invert* g_this = (E_Invert*)g_current_render;
    switch (uMsg) {
        case WM_INITDIALOG:
            if (g_this->enabled) {
                CheckDlgButton(hwndDlg, IDC_CHECK1, BST_CHECKED);
            }
            return 1;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_CHECK1) {
                g_this->enabled = IsDlgButtonChecked(hwndDlg, IDC_CHECK1) ? 1 : 0;
            }
    }
    return 0;
}
