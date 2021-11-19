#include "c_mirror.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int getMode(HWND hwndDlg) {
    int a;
    a = IsDlgButtonChecked(hwndDlg, IDC_HORIZONTAL1) ? HORIZONTAL1 : 0;
    a |= IsDlgButtonChecked(hwndDlg, IDC_HORIZONTAL2) ? HORIZONTAL2 : 0;
    a |= IsDlgButtonChecked(hwndDlg, IDC_VERTICAL1) ? VERTICAL1 : 0;
    a |= IsDlgButtonChecked(hwndDlg, IDC_VERTICAL2) ? VERTICAL2 : 0;
    return a;
}

int win32_dlgproc_mirror(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    C_THISCLASS* g_ConfigThis = (C_THISCLASS*)g_current_render;
    switch (uMsg) {
        case WM_INITDIALOG:
            SendDlgItemMessage(hwndDlg, IDC_SLOWER, TBM_SETTICFREQ, 1, 0);
            SendDlgItemMessage(
                hwndDlg, IDC_SLOWER, TBM_SETRANGE, TRUE, MAKELONG(1, 16));
            SendDlgItemMessage(
                hwndDlg, IDC_SLOWER, TBM_SETPOS, TRUE, g_ConfigThis->slower);
            if (g_ConfigThis->enabled) CheckDlgButton(hwndDlg, IDC_CHECK1, BST_CHECKED);
            if (g_ConfigThis->smooth) CheckDlgButton(hwndDlg, IDC_SMOOTH, BST_CHECKED);
            if (g_ConfigThis->mode & VERTICAL1)
                CheckDlgButton(hwndDlg, IDC_VERTICAL1, BST_CHECKED);
            if (g_ConfigThis->mode & VERTICAL2)
                CheckDlgButton(hwndDlg, IDC_VERTICAL2, BST_CHECKED);
            if (g_ConfigThis->mode & HORIZONTAL1)
                CheckDlgButton(hwndDlg, IDC_HORIZONTAL1, BST_CHECKED);
            if (g_ConfigThis->mode & HORIZONTAL2)
                CheckDlgButton(hwndDlg, IDC_HORIZONTAL2, BST_CHECKED);
            if (g_ConfigThis->onbeat)
                CheckDlgButton(hwndDlg, IDC_ONBEAT, BST_CHECKED);
            else
                CheckDlgButton(hwndDlg, IDC_STAT, BST_CHECKED);
            return 1;
        case WM_NOTIFY: {
            if (LOWORD(wParam) == IDC_SLOWER)
                g_ConfigThis->slower =
                    SendDlgItemMessage(hwndDlg, IDC_SLOWER, TBM_GETPOS, 0, 0);
            return 0;
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_CHECK1) {
                g_ConfigThis->enabled = IsDlgButtonChecked(hwndDlg, IDC_CHECK1) ? 1 : 0;
            }
            if (LOWORD(wParam) == IDC_HORIZONTAL1) {
                g_ConfigThis->mode = getMode(hwndDlg);
                if ((g_ConfigThis->mode & HORIZONTAL1)
                    && (g_ConfigThis->mode & HORIZONTAL2) && !(g_ConfigThis->onbeat)) {
                    CheckDlgButton(hwndDlg, IDC_HORIZONTAL2, BST_UNCHECKED);
                    g_ConfigThis->mode = getMode(hwndDlg);
                }
            }
            if (LOWORD(wParam) == IDC_HORIZONTAL2) {
                g_ConfigThis->mode = getMode(hwndDlg);
                if ((g_ConfigThis->mode & HORIZONTAL1)
                    && (g_ConfigThis->mode & HORIZONTAL2) && !(g_ConfigThis->onbeat)) {
                    CheckDlgButton(hwndDlg, IDC_HORIZONTAL1, BST_UNCHECKED);
                    g_ConfigThis->mode = getMode(hwndDlg);
                }
            }
            if (LOWORD(wParam) == IDC_VERTICAL1) {
                g_ConfigThis->mode = getMode(hwndDlg);
                if ((g_ConfigThis->mode & VERTICAL1) && (g_ConfigThis->mode & VERTICAL2)
                    && !(g_ConfigThis->onbeat)) {
                    CheckDlgButton(hwndDlg, IDC_VERTICAL2, BST_UNCHECKED);
                    g_ConfigThis->mode = getMode(hwndDlg);
                }
            }
            if (LOWORD(wParam) == IDC_VERTICAL2) {
                g_ConfigThis->mode = getMode(hwndDlg);
                if ((g_ConfigThis->mode & VERTICAL1) && (g_ConfigThis->mode & VERTICAL2)
                    && !(g_ConfigThis->onbeat)) {
                    CheckDlgButton(hwndDlg, IDC_VERTICAL1, BST_UNCHECKED);
                    g_ConfigThis->mode = getMode(hwndDlg);
                }
            }
            if (LOWORD(wParam) == IDC_STAT || LOWORD(wParam) == IDC_ONBEAT) {
                g_ConfigThis->onbeat = IsDlgButtonChecked(hwndDlg, IDC_ONBEAT) ? 1 : 0;
                if (!(g_ConfigThis->onbeat)) {
                    if ((g_ConfigThis->mode & HORIZONTAL1)
                        && (g_ConfigThis->mode & HORIZONTAL2))
                        CheckDlgButton(hwndDlg, IDC_HORIZONTAL2, BST_UNCHECKED);
                    if ((g_ConfigThis->mode & VERTICAL1)
                        && (g_ConfigThis->mode & VERTICAL2))
                        CheckDlgButton(hwndDlg, IDC_VERTICAL2, BST_UNCHECKED);
                    g_ConfigThis->mode = getMode(hwndDlg);
                }
            }
            if (LOWORD(wParam) == IDC_SMOOTH)
                g_ConfigThis->smooth = IsDlgButtonChecked(hwndDlg, IDC_SMOOTH) ? 1 : 0;
            return 0;
    }
    return 0;
}
