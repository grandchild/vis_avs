#include "e_interferences.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_interferences(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    E_Interferences* g_this = (E_Interferences*)g_current_render;
    const Parameter& p_num_layers = g_this->info.parameters[0];
    AVS_Parameter_Handle p_separate_rgb = g_this->info.parameters[1].handle;
    const Parameter& p_distance = g_this->info.parameters[2];
    const Parameter& p_alpha = g_this->info.parameters[3];
    const Parameter& p_rotation = g_this->info.parameters[4];
    const Parameter& p_init_rotation = g_this->info.parameters[5];
    AVS_Parameter_Handle p_on_beat = g_this->info.parameters[6].handle;
    const Parameter& p_on_beat_distance = g_this->info.parameters[7];
    const Parameter& p_on_beat_alpha = g_this->info.parameters[8];
    const Parameter& p_on_beat_rotation = g_this->info.parameters[9];
    const Parameter& p_on_beat_speed = g_this->info.parameters[10];
    AVS_Parameter_Handle p_blend_mode = g_this->info.parameters[11].handle;

    auto element = LOWORD(wParam);
    switch (uMsg) {
        case WM_INITDIALOG: {
            auto num_layers = g_this->get_int(p_num_layers.handle);
            init_ranged_slider(p_num_layers, num_layers, hwndDlg, IDC_NPOINTS);
            auto alpha = g_this->get_int(p_alpha.handle);
            init_ranged_slider(p_alpha, alpha, hwndDlg, IDC_ALPHA);
            auto distance = g_this->get_int(p_distance.handle);
            init_ranged_slider(p_distance, distance, hwndDlg, IDC_DISTANCE);
            auto rotation = -g_this->get_int(p_rotation.handle) - p_rotation.int_min;
            init_ranged_slider(p_rotation, rotation, hwndDlg, IDC_ROTATE);
            auto on_beat_alpha = g_this->get_int(p_on_beat_alpha.handle);
            init_ranged_slider(p_on_beat_alpha, on_beat_alpha, hwndDlg, IDC_ALPHA2);
            auto on_beat_distance = g_this->get_int(p_on_beat_distance.handle);
            init_ranged_slider(
                p_on_beat_distance, on_beat_distance, hwndDlg, IDC_DISTANCE2);
            auto on_beat_rotation = -g_this->get_int(p_on_beat_rotation.handle)
                                    - p_on_beat_rotation.int_min;
            init_ranged_slider(
                p_on_beat_rotation, on_beat_rotation, hwndDlg, IDC_ROTATE2);
            auto init_rotation =
                p_init_rotation.int_max - g_this->get_int(p_init_rotation.handle);
            init_ranged_slider(p_init_rotation, init_rotation, hwndDlg, IDC_INITROT);

            auto speed = g_this->get_float(p_on_beat_speed.handle);
            auto speed_range = MAKELONG((int)(p_on_beat_speed.float_min * 100.0),
                                        (int)(p_on_beat_speed.float_max * 100.0));
            SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_SETRANGE, TRUE, speed_range);
            SendDlgItemMessage(
                hwndDlg, IDC_SPEED, TBM_SETPOS, TRUE, (int)(speed * 100.0));

            SendDlgItemMessage(hwndDlg, IDC_NPOINTS, TBM_SETTICFREQ, 1, 0);
            SendDlgItemMessage(hwndDlg, IDC_ALPHA, TBM_SETTICFREQ, 16, 0);
            SendDlgItemMessage(hwndDlg, IDC_DISTANCE, TBM_SETTICFREQ, 8, 0);
            SendDlgItemMessage(hwndDlg, IDC_ROTATE, TBM_SETTICFREQ, 2, 0);
            SendDlgItemMessage(hwndDlg, IDC_ALPHA2, TBM_SETTICFREQ, 16, 0);
            SendDlgItemMessage(hwndDlg, IDC_DISTANCE2, TBM_SETTICFREQ, 8, 0);
            SendDlgItemMessage(hwndDlg, IDC_ROTATE2, TBM_SETTICFREQ, 2, 0);
            SendDlgItemMessage(hwndDlg, IDC_INITROT, TBM_SETTICFREQ, 16, 0);
            SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_SETTICFREQ, 1000, 0);

            if (g_this->enabled) {
                CheckDlgButton(hwndDlg, IDC_CHECK1, BST_CHECKED);
            }
            switch (g_this->get_int(p_blend_mode)) {
                default:
                case BLEND_SIMPLE_REPLACE:
                    CheckDlgButton(hwndDlg, IDC_REPLACE, BST_CHECKED);
                    break;
                case BLEND_SIMPLE_ADDITIVE:
                    CheckDlgButton(hwndDlg, IDC_ADDITIVE, BST_CHECKED);
                    break;
                case BLEND_SIMPLE_5050:
                    CheckDlgButton(hwndDlg, IDC_5050, BST_CHECKED);
                    break;
            }
            EnableWindow(GetDlgItem(hwndDlg, IDC_RGB),
                         g_this->get_int(p_num_layers.handle) % 3 == 0);
            CheckDlgButton(hwndDlg, IDC_RGB, g_this->get_bool(p_separate_rgb));
            CheckDlgButton(hwndDlg, IDC_ONBEAT, g_this->get_bool(p_on_beat));
            return 1;
        }
        case WM_COMMAND:
            if (element == IDC_CHECK1 || element == IDC_RGB || element == IDC_ONBEAT) {
                g_this->set_enabled(IsDlgButtonChecked(hwndDlg, IDC_CHECK1));
                g_this->set_bool(p_separate_rgb, IsDlgButtonChecked(hwndDlg, IDC_RGB));
                g_this->set_bool(p_on_beat, IsDlgButtonChecked(hwndDlg, IDC_ONBEAT));
            }
            if (element == IDC_REPLACE || element == IDC_ADDITIVE
                || element == IDC_5050) {
                if (IsDlgButtonChecked(hwndDlg, IDC_REPLACE)) {
                    g_this->set_int(p_blend_mode, BLEND_SIMPLE_REPLACE);
                } else if (IsDlgButtonChecked(hwndDlg, IDC_ADDITIVE)) {
                    g_this->set_int(p_blend_mode, BLEND_SIMPLE_ADDITIVE);
                } else if (IsDlgButtonChecked(hwndDlg, IDC_5050)) {
                    g_this->set_int(p_blend_mode, BLEND_SIMPLE_5050);
                }
            }
            return 0;
        case WM_NOTIFY:
            if (element == IDC_NPOINTS) {
                g_this->set_int(
                    p_num_layers.handle,
                    SendDlgItemMessage(hwndDlg, IDC_NPOINTS, TBM_GETPOS, 0, 0));
                EnableWindow(GetDlgItem(hwndDlg, IDC_RGB),
                             g_this->get_int(p_num_layers.handle) % 3 == 0);
            }
            if (element == IDC_ALPHA) {
                g_this->set_int(
                    p_alpha.handle,
                    SendDlgItemMessage(hwndDlg, IDC_ALPHA, TBM_GETPOS, 0, 0));
            }
            if (element == IDC_DISTANCE) {
                g_this->set_int(
                    p_distance.handle,
                    SendDlgItemMessage(hwndDlg, IDC_DISTANCE, TBM_GETPOS, 0, 0));
            }
            if (element == IDC_ROTATE) {
                g_this->set_int(
                    p_rotation.handle,
                    -(SendDlgItemMessage(hwndDlg, IDC_ROTATE, TBM_GETPOS, 0, 0)
                      + p_rotation.int_min));
            }
            if (element == IDC_INITROT) {
                g_this->set_int(
                    p_init_rotation.handle,
                    p_init_rotation.int_max
                        - SendDlgItemMessage(hwndDlg, IDC_INITROT, TBM_GETPOS, 0, 0));
            }
            if (element == IDC_SPEED) {
                g_this->set_float(
                    p_on_beat_speed.handle,
                    (double)SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_GETPOS, 0, 0)
                        / 100.0);
            }
            if (element == IDC_ALPHA2) {
                g_this->set_int(
                    p_on_beat_alpha.handle,
                    SendDlgItemMessage(hwndDlg, IDC_ALPHA2, TBM_GETPOS, 0, 0));
            }
            if (element == IDC_DISTANCE2) {
                g_this->set_int(
                    p_on_beat_distance.handle,
                    SendDlgItemMessage(hwndDlg, IDC_DISTANCE2, TBM_GETPOS, 0, 0));
            }
            if (element == IDC_ROTATE2) {
                g_this->set_int(
                    p_on_beat_rotation.handle,
                    -(SendDlgItemMessage(hwndDlg, IDC_ROTATE2, TBM_GETPOS, 0, 0)
                      + p_on_beat_rotation.int_min));
            }
            return 0;
    }
    return 0;
}
