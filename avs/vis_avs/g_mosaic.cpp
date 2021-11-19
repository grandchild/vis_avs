#include "c_mosaic.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_mosaic(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    C_THISCLASS* g_ConfigThis = (C_THISCLASS*)g_current_render;
    switch (uMsg) {
        case WM_INITDIALOG:
            SendDlgItemMessage(hwndDlg, IDC_QUALITY, TBM_SETTICFREQ, 10, 0);
            SendDlgItemMessage(
                hwndDlg, IDC_QUALITY, TBM_SETRANGE, TRUE, MAKELONG(1, 100));
            SendDlgItemMessage(
                hwndDlg, IDC_QUALITY, TBM_SETPOS, TRUE, g_ConfigThis->quality);
            SendDlgItemMessage(hwndDlg, IDC_QUALITY2, TBM_SETTICFREQ, 10, 0);
            SendDlgItemMessage(
                hwndDlg, IDC_QUALITY2, TBM_SETRANGE, TRUE, MAKELONG(1, 100));
            SendDlgItemMessage(
                hwndDlg, IDC_QUALITY2, TBM_SETPOS, TRUE, g_ConfigThis->quality2);
            SendDlgItemMessage(hwndDlg, IDC_BEATDUR, TBM_SETTICFREQ, 10, 0);
            SendDlgItemMessage(
                hwndDlg, IDC_BEATDUR, TBM_SETRANGE, TRUE, MAKELONG(1, 100));
            SendDlgItemMessage(
                hwndDlg, IDC_BEATDUR, TBM_SETPOS, TRUE, g_ConfigThis->durFrames);
            if (g_ConfigThis->enabled) CheckDlgButton(hwndDlg, IDC_CHECK1, BST_CHECKED);
            if (g_ConfigThis->onbeat) CheckDlgButton(hwndDlg, IDC_ONBEAT, BST_CHECKED);
            if (g_ConfigThis->blend) CheckDlgButton(hwndDlg, IDC_ADDITIVE, BST_CHECKED);
            if (g_ConfigThis->blendavg) CheckDlgButton(hwndDlg, IDC_5050, BST_CHECKED);
            if (!g_ConfigThis->blend && !g_ConfigThis->blendavg)
                CheckDlgButton(hwndDlg, IDC_REPLACE, BST_CHECKED);
            return 1;
        case WM_NOTIFY: {
            if (LOWORD(wParam) == IDC_QUALITY2)
                g_ConfigThis->quality2 =
                    SendDlgItemMessage(hwndDlg, IDC_QUALITY2, TBM_GETPOS, 0, 0);
            if (LOWORD(wParam) == IDC_BEATDUR)
                g_ConfigThis->durFrames =
                    SendDlgItemMessage(hwndDlg, IDC_BEATDUR, TBM_GETPOS, 0, 0);
            if (LOWORD(wParam) == IDC_QUALITY)
                g_ConfigThis->quality =
                    SendDlgItemMessage(hwndDlg, IDC_QUALITY, TBM_GETPOS, 0, 0);
        }
            return 0;
        case WM_COMMAND:
            if ((LOWORD(wParam) == IDC_CHECK1) || (LOWORD(wParam) == IDC_ONBEAT)
                || (LOWORD(wParam) == IDC_ADDITIVE) || (LOWORD(wParam) == IDC_REPLACE)
                || (LOWORD(wParam) == IDC_5050)) {
                g_ConfigThis->enabled = IsDlgButtonChecked(hwndDlg, IDC_CHECK1) ? 1 : 0;
                g_ConfigThis->onbeat = IsDlgButtonChecked(hwndDlg, IDC_ONBEAT) ? 1 : 0;
                g_ConfigThis->blend = IsDlgButtonChecked(hwndDlg, IDC_ADDITIVE) ? 1 : 0;
                g_ConfigThis->blendavg = IsDlgButtonChecked(hwndDlg, IDC_5050) ? 1 : 0;
            }
    }
    return 0;
}
