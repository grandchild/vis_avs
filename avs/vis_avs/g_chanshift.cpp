#include "c_chanshift.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_chanshift(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    C_THISCLASS* g_ConfigThis = (C_THISCLASS*)g_current_render;
    int ids[] = {IDC_RBG, IDC_BRG, IDC_BGR, IDC_GBR, IDC_GRB, IDC_RGB};
    switch (uMsg) {
        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED) {
                for (unsigned int i = 0; i < sizeof(ids) / sizeof(ids[0]); i++)
                    if (IsDlgButtonChecked(hwndDlg, ids[i]))
                        g_ConfigThis->config.mode = ids[i];
                g_ConfigThis->config.onbeat =
                    IsDlgButtonChecked(hwndDlg, IDC_ONBEAT) ? 1 : 0;
            }
            return 1;

        case WM_INITDIALOG:
            CheckDlgButton(hwndDlg, g_ConfigThis->config.mode, 1);
            if (g_ConfigThis->config.onbeat) CheckDlgButton(hwndDlg, IDC_ONBEAT, 1);

            return 1;

        case WM_DESTROY:
            KillTimer(hwndDlg, 1);
            return 1;
    }
    return 0;
}
