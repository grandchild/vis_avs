#include "e_water.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_water(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    E_Water* g_this = (E_Water*)g_current_render;

    switch (uMsg) {
        case WM_INITDIALOG:
            CheckDlgButton(hwndDlg, IDC_CHECK1, g_this->enabled);
            return 1;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_CHECK1) {
                g_this->enabled = IsDlgButtonChecked(hwndDlg, IDC_CHECK1);
            }
    }
    return 0;
}
