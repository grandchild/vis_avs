#include "e_fastbright.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_fastbrightness(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    E_FastBright* g_this = (E_FastBright*)g_current_render;

    switch (uMsg) {
        case WM_INITDIALOG:
            if (g_this->enabled) {
                if (g_this->config.multiply == FASTBRIGHT_MULTIPLY_DOUBLE) {
                    CheckDlgButton(hwndDlg, IDC_RADIO1, BST_CHECKED);
                } else if (g_this->config.multiply == FASTBRIGHT_MULTIPLY_HALF) {
                    CheckDlgButton(hwndDlg, IDC_RADIO2, BST_CHECKED);
                }
            } else {
                CheckDlgButton(hwndDlg, IDC_RADIO3, BST_CHECKED);
            }
            return 1;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_RADIO1)
                if (IsDlgButtonChecked(hwndDlg, IDC_RADIO1)) {
                    g_this->enabled = true;
                    g_this->config.multiply = FASTBRIGHT_MULTIPLY_DOUBLE;
                }
            if (LOWORD(wParam) == IDC_RADIO2)
                if (IsDlgButtonChecked(hwndDlg, IDC_RADIO2)) {
                    g_this->enabled = true;
                    g_this->config.multiply = FASTBRIGHT_MULTIPLY_HALF;
                }
            if (LOWORD(wParam) == IDC_RADIO3)
                if (IsDlgButtonChecked(hwndDlg, IDC_RADIO3)) {
                    g_this->enabled = false;
                }
            return 0;
    }
    return 0;
}
