#include "c_contrast.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_colorclip(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    C_THISCLASS* g_this = (C_THISCLASS*)g_current_render;
    switch (uMsg) {
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* di = (DRAWITEMSTRUCT*)lParam;
            switch (di->CtlID) {
                case IDC_LC: GR_DrawColoredButton(di, g_this->color_clip); break;
                case IDC_LC2: GR_DrawColoredButton(di, g_this->color_clip_out); break;
            }
        }
            return 0;
        case WM_INITDIALOG:
            if (g_this->enabled == 0) {
                CheckDlgButton(hwndDlg, IDC_OFF, BST_CHECKED);
            } else if (g_this->enabled == 1) {
                CheckDlgButton(hwndDlg, IDC_BELOW, BST_CHECKED);
            } else if (g_this->enabled == 2) {
                CheckDlgButton(hwndDlg, IDC_ABOVE, BST_CHECKED);
            } else {
                CheckDlgButton(hwndDlg, IDC_NEAR, BST_CHECKED);
            }
            SendDlgItemMessage(
                hwndDlg, IDC_DISTANCE, TBM_SETRANGE, TRUE, MAKELONG(0, 64));
            SendDlgItemMessage(
                hwndDlg, IDC_DISTANCE, TBM_SETPOS, TRUE, g_this->color_dist);
            SendDlgItemMessage(hwndDlg, IDC_DISTANCE, TBM_SETTICFREQ, 4, 0);
            return 1;
        case WM_NOTIFY:
            if (LOWORD(wParam) == IDC_DISTANCE) {
                g_this->color_dist =
                    SendDlgItemMessage(hwndDlg, IDC_DISTANCE, TBM_GETPOS, 0, 0);
            }
            return 0;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_OFF:
                case IDC_BELOW:
                case IDC_ABOVE:
                case IDC_NEAR:
                    if (IsDlgButtonChecked(hwndDlg, IDC_OFF)) {
                        g_this->enabled = 0;
                    } else if (IsDlgButtonChecked(hwndDlg, IDC_BELOW)) {
                        g_this->enabled = 1;
                    } else if (IsDlgButtonChecked(hwndDlg, IDC_ABOVE)) {
                        g_this->enabled = 2;
                    } else {
                        g_this->enabled = 3;
                    }
                    return 0;
                case IDC_LC:
                    GR_SelectColor(hwndDlg, &g_this->color_clip);
                    InvalidateRect(GetDlgItem(hwndDlg, LOWORD(wParam)), NULL, FALSE);
                    return 0;
                case IDC_LC2:
                    GR_SelectColor(hwndDlg, &g_this->color_clip_out);
                    InvalidateRect(GetDlgItem(hwndDlg, LOWORD(wParam)), NULL, FALSE);
                    return 0;
                case IDC_BUTTON1:
                    g_this->color_clip_out = g_this->color_clip;
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_LC2), NULL, FALSE);
                    return 0;
            }
            return 0;
    }
    return 0;
}
