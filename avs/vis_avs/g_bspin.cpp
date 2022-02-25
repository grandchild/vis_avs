#include "c_bspin.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_bassspin(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    C_THISCLASS* g_this = (C_THISCLASS*)g_current_render;

    switch (uMsg) {
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* di = (DRAWITEMSTRUCT*)lParam;
            switch (di->CtlID) {
                case IDC_LC: GR_DrawColoredButton(di, g_this->colors[0]); break;
                case IDC_RC: GR_DrawColoredButton(di, g_this->colors[1]); break;
            }
        }
            return 0;
        case WM_INITDIALOG:
            if (g_this->enabled & 1) CheckDlgButton(hwndDlg, IDC_LEFT, BST_CHECKED);
            if (g_this->enabled & 2) CheckDlgButton(hwndDlg, IDC_RIGHT, BST_CHECKED);
            if (g_this->mode == 0) CheckDlgButton(hwndDlg, IDC_LINES, BST_CHECKED);
            if (g_this->mode == 1) CheckDlgButton(hwndDlg, IDC_TRI, BST_CHECKED);

            return 1;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_LINES:
                    g_this->mode = IsDlgButtonChecked(hwndDlg, IDC_LINES) ? 0 : 1;
                    return 0;
                case IDC_TRI:
                    g_this->mode = IsDlgButtonChecked(hwndDlg, IDC_TRI) ? 1 : 0;
                    return 0;
                case IDC_LEFT:
                    g_this->enabled &= ~1;
                    g_this->enabled |= IsDlgButtonChecked(hwndDlg, IDC_LEFT) ? 1 : 0;
                    return 0;
                case IDC_RIGHT:
                    g_this->enabled &= ~2;
                    g_this->enabled |= IsDlgButtonChecked(hwndDlg, IDC_RIGHT) ? 2 : 0;
                    return 0;
                case IDC_LC:
                    GR_SelectColor(hwndDlg, &g_this->colors[0]);
                    InvalidateRect(GetDlgItem(hwndDlg, LOWORD(wParam)), NULL, FALSE);
                    return 0;
                case IDC_RC:
                    GR_SelectColor(hwndDlg, &g_this->colors[1]);
                    InvalidateRect(GetDlgItem(hwndDlg, LOWORD(wParam)), NULL, FALSE);
                    return 0;
            }
    }
    return 0;
}
