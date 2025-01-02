#include "e_scatter.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_scatter(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    E_Scatter* g_this = (E_Scatter*)g_current_render;

    switch (uMsg) {
        case WM_INITDIALOG:
            if (g_this->enabled) {
                CheckDlgButton(hwndDlg, IDC_CHECK1, BST_CHECKED);
            }
            return 1;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_CHECK1) {
                if (IsDlgButtonChecked(hwndDlg, IDC_CHECK1)) {
                    g_this->enabled = 1;
                } else {
                    g_this->enabled = 0;
                }
            }
            return 0;
    }
    return 0;
}
