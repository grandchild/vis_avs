#include "c_transition.h"

#include "g__defs.h"
#include "g__lib.h"

#include "render.h"  // g_single_instance
#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_transition(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    auto g_this = &g_single_instance->transition;
    const Parameter& p_effect = Transition_Info::parameters[0];
    const Parameter& p_time_ms = Transition_Info::parameters[1];
    AVS_Parameter_Handle p_on_load = Transition_Info::parameters[2].handle;
    AVS_Parameter_Handle p_on_next_prev = Transition_Info::parameters[3].handle;
    AVS_Parameter_Handle p_on_random = Transition_Info::parameters[4].handle;
    AVS_Parameter_Handle p_keep_rendering_old_preset =
        Transition_Info::parameters[5].handle;
    AVS_Parameter_Handle p_preinit_on_load = Transition_Info::parameters[6].handle;
    AVS_Parameter_Handle p_preinit_on_next_prev = Transition_Info::parameters[7].handle;
    AVS_Parameter_Handle p_preinit_on_random = Transition_Info::parameters[8].handle;
    AVS_Parameter_Handle p_preinit_low_priority = Transition_Info::parameters[9].handle;
    AVS_Parameter_Handle p_preinit_only_in_fullscreen =
        Transition_Info::parameters[10].handle;

    switch (uMsg) {
        case WM_INITDIALOG: {
            auto effect = g_this->get_int(p_effect.handle);
            init_select(p_effect, effect, hwndDlg, IDC_TRANSITION);
            auto time_ms = g_this->get_int(p_time_ms.handle);
            init_ranged_slider(p_time_ms, time_ms, hwndDlg, IDC_SPEED);
            CheckDlgButton(
                hwndDlg, IDC_CHECK9, g_this->get_bool(p_keep_rendering_old_preset));
            CheckDlgButton(hwndDlg, IDC_CHECK2, g_this->get_bool(p_on_load));
            CheckDlgButton(hwndDlg, IDC_CHECK1, g_this->get_bool(p_on_next_prev));
            CheckDlgButton(hwndDlg, IDC_CHECK8, g_this->get_bool(p_on_random));
            CheckDlgButton(hwndDlg, IDC_CHECK10, g_this->get_bool(p_preinit_on_load));
            CheckDlgButton(
                hwndDlg, IDC_CHECK11, g_this->get_bool(p_preinit_on_next_prev));
            CheckDlgButton(hwndDlg, IDC_CHECK3, g_this->get_bool(p_preinit_on_random));
            CheckDlgButton(
                hwndDlg, IDC_CHECK4, g_this->get_bool(p_preinit_low_priority));
            CheckDlgButton(
                hwndDlg, IDC_CHECK5, g_this->get_bool(p_preinit_only_in_fullscreen));
            return 1;
        }
        case WM_COMMAND: {
            auto control = LOWORD(wParam);
            if (control == IDC_TRANSITION) {
                if (HIWORD(wParam) == CBN_SELCHANGE) {
                    int r =
                        SendDlgItemMessage(hwndDlg, IDC_TRANSITION, CB_GETCURSEL, 0, 0);
                    if (r != CB_ERR) {
                        g_this->set_int(p_effect.handle, r);
                    }
                }
            } else if (control == IDC_CHECK9 || control == IDC_CHECK2
                       || control == IDC_CHECK1 || control == IDC_CHECK8
                       || control == IDC_CHECK10 || control == IDC_CHECK11
                       || control == IDC_CHECK3 || control == IDC_CHECK4
                       || control == IDC_CHECK5) {
                g_this->set_bool(p_keep_rendering_old_preset,
                                 IsDlgButtonChecked(hwndDlg, IDC_CHECK9));
                g_this->set_bool(p_on_load, IsDlgButtonChecked(hwndDlg, IDC_CHECK2));
                g_this->set_bool(p_on_next_prev,
                                 IsDlgButtonChecked(hwndDlg, IDC_CHECK1));
                g_this->set_bool(p_on_random, IsDlgButtonChecked(hwndDlg, IDC_CHECK8));
                g_this->set_bool(p_preinit_on_load,
                                 IsDlgButtonChecked(hwndDlg, IDC_CHECK10));
                g_this->set_bool(p_preinit_on_next_prev,
                                 IsDlgButtonChecked(hwndDlg, IDC_CHECK11));
                g_this->set_bool(p_preinit_on_random,
                                 IsDlgButtonChecked(hwndDlg, IDC_CHECK3));
                g_this->set_bool(p_preinit_low_priority,
                                 IsDlgButtonChecked(hwndDlg, IDC_CHECK4));
                g_this->set_bool(p_preinit_only_in_fullscreen,
                                 IsDlgButtonChecked(hwndDlg, IDC_CHECK5));
            }
            break;
        }
        case WM_NOTIFY:
            if (LOWORD(wParam) == IDC_SPEED) {
                g_this->set_int(
                    p_time_ms.handle,
                    SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_GETPOS, 0, 0));
            }
            break;
    }
    return 0;
}
