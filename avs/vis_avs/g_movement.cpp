#include "e_movement.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_movement(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    E_Movement* g_this = (E_Movement*)g_current_render;
    AVS_Parameter_Handle p_source_map = g_this->info.parameters[0].handle;
    AVS_Parameter_Handle p_on_beat_source_map = g_this->info.parameters[1].handle;
    AVS_Parameter_Handle p_wrap = g_this->info.parameters[2].handle;
    AVS_Parameter_Handle p_blend_mode = g_this->info.parameters[3].handle;
    AVS_Parameter_Handle p_bilinear = g_this->info.parameters[4].handle;
    AVS_Parameter_Handle p_coordinates = g_this->info.parameters[5].handle;
    AVS_Parameter_Handle p_code = g_this->info.parameters[6].handle;
    const Parameter& p_effect = g_this->info.parameters[7];
    AVS_Parameter_Handle p_use_custom_code = g_this->info.parameters[8].handle;
    AVS_Parameter_Handle p_load_effect = g_this->info.parameters[9].handle;

    // Note that, specifically in Movement, `options_length`, while it is the length of
    // builtin effects, is less than the list of options in the UI, which additionally
    // includes "None" at the beginning and "(user defined)" at the end.
    // So beware of various off-by-one caveats below.
    int64_t options_length;
    const char* const* options;

    static bool is_start;
    switch (uMsg) {
        case WM_INITDIALOG: {
            is_start = true;
            SetDlgItemText(hwndDlg, IDC_EDIT1, g_this->get_string(p_code));
            is_start = false;

            CheckDlgButton(hwndDlg,
                           IDC_CHECK1,
                           g_this->get_int(p_blend_mode) == BLEND_SIMPLE_5050);
            CheckDlgButton(hwndDlg, IDC_CHECK4, g_this->get_bool(p_bilinear));
            CheckDlgButton(hwndDlg, IDC_WRAP, g_this->get_bool(p_wrap));
            CheckDlgButton(hwndDlg,
                           IDC_CHECK3,
                           g_this->get_int(p_coordinates) == COORDS_CARTESIAN);
            auto source_map_state = g_this->get_bool(p_on_beat_source_map)
                                        ? BST_INDETERMINATE
                                        : g_this->get_bool(p_source_map);
            CheckDlgButton(hwndDlg, IDC_CHECK2, source_map_state);

            SendDlgItemMessage(hwndDlg, IDC_LIST1, LB_ADDSTRING, 0, (LPARAM) "None");
            options = p_effect.get_options(&options_length);
            for (uint32_t x = 0; x < options_length; x++) {
                SendDlgItemMessage(
                    hwndDlg, IDC_LIST1, LB_ADDSTRING, 0, (LPARAM)options[x]);
            }
            auto effect = g_this->get_int(p_effect.handle);
            auto use_custom_code = g_this->get_bool(p_use_custom_code);
            SendDlgItemMessage(
                hwndDlg, IDC_LIST1, LB_ADDSTRING, 0, (LPARAM) "(user defined)");
            SendDlgItemMessage(hwndDlg,
                               IDC_LIST1,
                               LB_SETCURSEL,
                               (use_custom_code ? options_length : effect) + 1,
                               0);
            CheckDlgButton(hwndDlg,
                           IDC_CHECK3,
                           g_this->get_int(p_coordinates) == COORDS_CARTESIAN);
            return 1;
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_EDIT1 && HIWORD(wParam) == EN_CHANGE
                && !is_start) {
                int length = GetWindowTextLength((HWND)lParam) + 1;
                char* buf = new char[length];
                GetDlgItemText(hwndDlg, IDC_EDIT1, buf, length);
                g_this->set_string(p_code, buf);
                delete[] buf;
                p_effect.get_options(&options_length);
                if (SendDlgItemMessage(hwndDlg, IDC_LIST1, LB_GETCURSEL, 0, 0)
                    <= options_length) {
                    g_this->set_int(p_coordinates,
                                    IsDlgButtonChecked(hwndDlg, IDC_CHECK3)
                                        ? COORDS_CARTESIAN
                                        : COORDS_POLAR);
                    SendDlgItemMessage(
                        hwndDlg, IDC_LIST1, LB_SETCURSEL, options_length + 1, 0);
                }
            }
            if (LOWORD(wParam) == IDC_LIST1 && HIWORD(wParam) == LBN_SELCHANGE) {
                int selection =
                    SendDlgItemMessage(hwndDlg, IDC_LIST1, LB_GETCURSEL, 0, 0);
                p_effect.get_options(&options_length);
                if (selection == 0) {
                    g_this->set_enabled(false);
                } else if (selection <= options_length) {
                    g_this->set_enabled(true);
                    g_this->set_int(p_effect.handle, selection - 1);
                    g_this->set_bool(p_use_custom_code, false);
                    g_this->run_action(p_load_effect);
                    SetDlgItemText(
                        hwndDlg, IDC_EDIT1, (char*)g_this->get_string(p_code));
                    CheckDlgButton(hwndDlg,
                                   IDC_CHECK3,
                                   g_this->get_int(p_coordinates) == COORDS_CARTESIAN);
                    CheckDlgButton(hwndDlg, IDC_WRAP, g_this->get_bool(p_wrap));
                } else {
                    g_this->set_enabled(true);
                    g_this->set_int(p_effect.handle, -1);
                    g_this->set_bool(p_use_custom_code, true);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT1), true);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK3), true);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_LABEL1), true);

                    SetDlgItemText(
                        hwndDlg, IDC_EDIT1, (char*)g_this->get_string(p_code));
                    CheckDlgButton(hwndDlg,
                                   IDC_CHECK3,
                                   g_this->get_int(p_coordinates) == COORDS_CARTESIAN);

                    HWND edit = GetDlgItem(hwndDlg, IDC_EDIT1);
                    int l = GetWindowTextLength(edit) + 1;
                    char* buf = new char[l];
                    GetWindowText(edit, buf, l);
                    g_this->set_string(p_code, buf);
                    delete[] buf;
                }
            }
            if (LOWORD(wParam) == IDC_CHECK1) {
                g_this->set_int(p_blend_mode,
                                IsDlgButtonChecked(hwndDlg, IDC_CHECK1)
                                    ? BLEND_SIMPLE_5050
                                    : BLEND_SIMPLE_REPLACE);
            }
            if (LOWORD(wParam) == IDC_CHECK4) {
                g_this->set_bool(p_bilinear, IsDlgButtonChecked(hwndDlg, IDC_CHECK4));
            }
            if (LOWORD(wParam) == IDC_WRAP) {
                g_this->set_bool(p_wrap, IsDlgButtonChecked(hwndDlg, IDC_WRAP));
            }
            if (LOWORD(wParam) == IDC_CHECK2) {
                switch (IsDlgButtonChecked(hwndDlg, IDC_CHECK2)) {
                    case BST_INDETERMINATE:
                        g_this->set_bool(p_on_beat_source_map, true);
                        break;
                    case BST_CHECKED:
                        g_this->set_bool(p_on_beat_source_map, false);
                        g_this->set_bool(p_source_map, true);
                        break;
                    default:
                        g_this->set_bool(p_on_beat_source_map, false);
                        g_this->set_bool(p_source_map, false);
                        break;
                }
            }
            if (LOWORD(wParam) == IDC_CHECK3) {
                g_this->set_int(p_coordinates,
                                IsDlgButtonChecked(hwndDlg, IDC_CHECK3)
                                    ? COORDS_CARTESIAN
                                    : COORDS_POLAR);
                p_effect.get_options(&options_length);
                if (SendDlgItemMessage(hwndDlg, IDC_LIST1, LB_GETCURSEL, 0, 0)
                    <= options_length) {
                    g_this->set_int(p_effect.handle, -1);
                    g_this->set_bool(p_use_custom_code, true);
                    SendDlgItemMessage(
                        hwndDlg, IDC_LIST1, LB_SETCURSEL, options_length + 1, 0);
                }
                HWND edit = GetDlgItem(hwndDlg, IDC_EDIT1);
                int l = GetWindowTextLength(edit) + 1;
                char* buf = new char[l];
                GetWindowText(edit, buf, l);
                g_this->set_string(p_code, buf);
                delete[] buf;
            }
            if (LOWORD(wParam) == IDC_BUTTON2) {
                compilerfunctionlist(
                    hwndDlg, g_this->info.get_name(), g_this->info.get_help());
            }
            return 0;
    }
    return 0;
}
