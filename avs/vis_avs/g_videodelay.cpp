#include "e_videodelay.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_videodelay(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    E_VideoDelay* g_this = (E_VideoDelay*)g_current_render;
    const AVS_Parameter_Handle p_use_beats = g_this->info.parameters[0].handle;
    const AVS_Parameter_Handle p_delay = g_this->info.parameters[1].handle;

    char delay_value_str[16];
    int delay_value;
    switch (uMsg) {
        case WM_INITDIALOG:
            CheckDlgButton(hwndDlg, IDC_CHECK1, g_this->enabled);
            CheckDlgButton(hwndDlg, IDC_RADIO1, g_this->get_bool(p_use_beats));
            CheckDlgButton(hwndDlg, IDC_RADIO2, !g_this->get_bool(p_use_beats));
            itoa(g_this->get_int(p_delay), delay_value_str, 10);
            SetWindowText(GetDlgItem(hwndDlg, IDC_EDIT1), delay_value_str);
            return 1;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_CHECK1) {
                g_this->set_enabled(IsDlgButtonChecked(hwndDlg, IDC_CHECK1));
                return 0;
            }
            if (LOWORD(wParam) == IDC_RADIO1) {
                if (IsDlgButtonChecked(hwndDlg, IDC_RADIO1) == 1) {
                    g_this->set_bool(p_use_beats, true);
                    CheckDlgButton(hwndDlg, IDC_RADIO2, BST_UNCHECKED);
                    if (g_this->get_int(p_delay) > 16) {
                        g_this->set_int(p_delay, 16);
                        SetWindowText(GetDlgItem(hwndDlg, IDC_EDIT1), "16");
                    }
                } else {
                    g_this->set_bool(p_use_beats, false);
                }
                return 0;
            }
            if (LOWORD(wParam) == IDC_RADIO2) {
                if (IsDlgButtonChecked(hwndDlg, IDC_RADIO2) == 1) {
                    g_this->set_bool(p_use_beats, false);
                    CheckDlgButton(hwndDlg, IDC_RADIO1, BST_UNCHECKED);
                } else {
                    g_this->set_bool(p_use_beats, true);
                }
                return 0;
            }
            if (LOWORD(wParam) == IDC_EDIT1) {
                if (HIWORD(wParam) == EN_CHANGE) {
                    int success = false;
                    delay_value = GetDlgItemInt(hwndDlg, IDC_EDIT1, &success, false);
                    if (success) {
                        g_this->set_int(p_delay, delay_value);
                    }
                } else if (HIWORD(wParam) == EN_KILLFOCUS) {
                    itoa(g_this->get_int(p_delay), delay_value_str, 10);
                    SetWindowText(GetDlgItem(hwndDlg, IDC_EDIT1), delay_value_str);
                }
                return 0;
            }
    }
    return 0;
}
