#include "e_mirror.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_mirror(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto g_this = (E_Mirror*)g_current_render;
    AVS_Parameter_Handle p_top_to_bottom = Mirror_Info::parameters[0].handle;
    AVS_Parameter_Handle p_bottom_to_top = Mirror_Info::parameters[1].handle;
    AVS_Parameter_Handle p_left_to_right = Mirror_Info::parameters[2].handle;
    AVS_Parameter_Handle p_right_to_left = Mirror_Info::parameters[3].handle;
    AVS_Parameter_Handle p_on_beat_random = Mirror_Info::parameters[4].handle;
    const Parameter& p_transition_duration = Mirror_Info::parameters[5];

    static int64_t transition_duration_save;
    switch (uMsg) {
        case WM_INITDIALOG: {
            CheckDlgButton(hwndDlg, IDC_CHECK1, g_this->enabled);
            CheckDlgButton(hwndDlg, IDC_HORIZONTAL1, g_this->get_bool(p_top_to_bottom));
            CheckDlgButton(hwndDlg, IDC_HORIZONTAL2, g_this->get_bool(p_bottom_to_top));
            CheckDlgButton(hwndDlg, IDC_VERTICAL1, g_this->get_bool(p_left_to_right));
            CheckDlgButton(hwndDlg, IDC_VERTICAL2, g_this->get_bool(p_right_to_left));
            auto on_beat = g_this->get_bool(p_on_beat_random);
            CheckDlgButton(hwndDlg, IDC_STAT, !on_beat);
            CheckDlgButton(hwndDlg, IDC_ONBEAT, on_beat);
            auto transition_duration = g_this->get_int(p_transition_duration.handle);
            transition_duration_save = transition_duration;
            CheckDlgButton(hwndDlg, IDC_SMOOTH, transition_duration > 0);
            init_ranged_slider(
                p_transition_duration, transition_duration, hwndDlg, IDC_SLOWER);
            return 1;
        }
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDC_CHECK1:
                    g_this->set_enabled(IsDlgButtonChecked(hwndDlg, IDC_CHECK1));
                    break;
                case IDC_HORIZONTAL1:
                    g_this->set_bool(p_top_to_bottom,
                                     IsDlgButtonChecked(hwndDlg, IDC_HORIZONTAL1));
                    CheckDlgButton(
                        hwndDlg, IDC_HORIZONTAL2, g_this->get_bool(p_bottom_to_top));
                    break;
                case IDC_HORIZONTAL2:
                    g_this->set_bool(p_bottom_to_top,
                                     IsDlgButtonChecked(hwndDlg, IDC_HORIZONTAL2));
                    CheckDlgButton(
                        hwndDlg, IDC_HORIZONTAL1, g_this->get_bool(p_top_to_bottom));
                    break;
                case IDC_VERTICAL1:
                    g_this->set_bool(p_left_to_right,
                                     IsDlgButtonChecked(hwndDlg, IDC_VERTICAL1));
                    CheckDlgButton(
                        hwndDlg, IDC_VERTICAL2, g_this->get_bool(p_right_to_left));
                    break;
                case IDC_VERTICAL2:
                    g_this->set_bool(p_right_to_left,
                                     IsDlgButtonChecked(hwndDlg, IDC_VERTICAL2));
                    CheckDlgButton(
                        hwndDlg, IDC_VERTICAL1, g_this->get_bool(p_left_to_right));
                    break;
                case IDC_ONBEAT:
                case IDC_STAT:
                    g_this->set_bool(p_on_beat_random,
                                     IsDlgButtonChecked(hwndDlg, IDC_ONBEAT));
                    CheckDlgButton(
                        hwndDlg, IDC_HORIZONTAL1, g_this->get_bool(p_top_to_bottom));
                    CheckDlgButton(
                        hwndDlg, IDC_HORIZONTAL2, g_this->get_bool(p_bottom_to_top));
                    CheckDlgButton(
                        hwndDlg, IDC_VERTICAL1, g_this->get_bool(p_left_to_right));
                    CheckDlgButton(
                        hwndDlg, IDC_VERTICAL2, g_this->get_bool(p_right_to_left));
                    break;
                case IDC_SMOOTH:
                    bool smooth_on = IsDlgButtonChecked(hwndDlg, IDC_SMOOTH);
                    int32_t value =
                        smooth_on ? max((int32_t)transition_duration_save, 1) : 0;
                    g_this->set_int(p_transition_duration.handle, value);
                    SendDlgItemMessage(hwndDlg, IDC_SLOWER, TBM_SETPOS, 1, value);
                    break;
            }
            return 0;
        }
        case WM_HSCROLL: {
            HWND control = (HWND)lParam;
            if (control == GetDlgItem(hwndDlg, IDC_SLOWER)) {
                int value = (int)SendMessage(control, TBM_GETPOS, 0, 0);
                g_this->set_int(p_transition_duration.handle, value);
                transition_duration_save = value;
                CheckDlgButton(hwndDlg, IDC_SMOOTH, value > 0);
            }
            return 0;
        }
    }
    return 0;
}
