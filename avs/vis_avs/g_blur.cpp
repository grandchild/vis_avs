#include "e_blur.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_blur(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    E_Blur* g_this = (E_Blur*)g_current_render;
    const Parameter& p_level = g_this->info.parameters[0];
    const Parameter& p_round = g_this->info.parameters[1];

    switch (uMsg) {
        case WM_INITDIALOG:
            if (!g_this->enabled) {
                CheckDlgButton(hwndDlg, IDC_RADIO1, BST_CHECKED);
            } else {
                switch (g_this->get_int(p_level.handle)) {
                    case 0: CheckDlgButton(hwndDlg, IDC_RADIO3, BST_CHECKED); break;
                    default:
                    case 1: CheckDlgButton(hwndDlg, IDC_RADIO2, BST_CHECKED); break;
                    case 2: CheckDlgButton(hwndDlg, IDC_RADIO4, BST_CHECKED); break;
                }
            }
            switch (g_this->get_int(p_round.handle)) {
                default:
                case 0: CheckDlgButton(hwndDlg, IDC_ROUNDDOWN, BST_CHECKED); break;
                case 1: CheckDlgButton(hwndDlg, IDC_ROUNDUP, BST_CHECKED); break;
            }
            return 1;

        case WM_COMMAND: {
            int control = LOWORD(wParam);
            if (!IsDlgButtonChecked(hwndDlg, control)) {
                return 0;
            }
            g_this->enabled = true;
            switch (control) {
                case IDC_RADIO1: g_this->enabled = false; break;
                case IDC_RADIO2: g_this->set_int(p_level.handle, 1); break;
                case IDC_RADIO3: g_this->set_int(p_level.handle, 0); break;
                case IDC_RADIO4: g_this->set_int(p_level.handle, 2); break;
                case IDC_ROUNDUP: g_this->set_int(p_round.handle, 1); break;
                case IDC_ROUNDDOWN: g_this->set_int(p_round.handle, 0); break;
                default: break;
            }
            return 0;
        }
    }
    return 0;
}
