#include "e_waterbump.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_waterbump(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto g_this = (E_WaterBump*)g_current_render;
    const Parameter& p_fluidity = WaterBump_Info::parameters[0];
    const Parameter& p_depth = WaterBump_Info::parameters[1];
    AVS_Parameter_Handle p_random = WaterBump_Info::parameters[2].handle;
    const Parameter& p_drop_position_x = WaterBump_Info::parameters[3];
    const Parameter& p_drop_position_y = WaterBump_Info::parameters[4];
    const Parameter& p_drop_radius = WaterBump_Info::parameters[5];

    switch (uMsg) {
        case WM_INITDIALOG: {
            CheckDlgButton(hwndDlg, IDC_CHECK1, g_this->enabled);

            auto fluidity = g_this->get_int(p_fluidity.handle);
            init_ranged_slider(p_fluidity, fluidity, hwndDlg, IDC_DAMP);
            auto depth = g_this->get_int(p_depth.handle);
            init_ranged_slider(p_depth, depth, hwndDlg, IDC_DEPTH);
            auto drop_radius = g_this->get_int(p_drop_radius.handle);
            init_ranged_slider(p_drop_radius, drop_radius, hwndDlg, IDC_RADIUS);

            CheckDlgButton(hwndDlg, IDC_RANDOM_DROP, g_this->get_bool(p_random));

            auto drop_position_x = g_this->get_int(p_drop_position_x.handle);
            static constexpr size_t num_x_positions = 3;
            uint32_t controls_x_position[num_x_positions] = {
                IDC_DROP_LEFT, IDC_DROP_CENTER, IDC_DROP_RIGHT};
            init_select_radio(p_drop_position_x,
                              drop_position_x,
                              hwndDlg,
                              controls_x_position,
                              num_x_positions);
            auto drop_position_y = g_this->get_int(p_drop_position_y.handle);
            static constexpr size_t num_y_positions = 3;
            uint32_t controls_y_position[num_y_positions] = {
                IDC_DROP_TOP, IDC_DROP_MIDDLE, IDC_DROP_BOTTOM};
            init_select_radio(p_drop_position_y,
                              drop_position_y,
                              hwndDlg,
                              controls_y_position,
                              num_y_positions);
            return 1;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_CHECK1:
                    g_this->set_enabled(IsDlgButtonChecked(hwndDlg, IDC_CHECK1));
                    break;
                case IDC_RANDOM_DROP:
                    g_this->set_bool(p_random,
                                     IsDlgButtonChecked(hwndDlg, IDC_RANDOM_DROP));
                    break;
                case IDC_DROP_LEFT: g_this->set_int(p_drop_position_x.handle, 0); break;
                case IDC_DROP_CENTER:
                    g_this->set_int(p_drop_position_x.handle, 1);
                    break;
                case IDC_DROP_RIGHT:
                    g_this->set_int(p_drop_position_x.handle, 2);
                    break;
                case IDC_DROP_TOP: g_this->set_int(p_drop_position_y.handle, 0); break;
                case IDC_DROP_MIDDLE:
                    g_this->set_int(p_drop_position_y.handle, 1);
                    break;
                case IDC_DROP_BOTTOM:
                    g_this->set_int(p_drop_position_y.handle, 2);
                    break;
            }
            return 0;
        case WM_HSCROLL: {
            HWND control = (HWND)lParam;
            int value = (int)SendMessage(control, TBM_GETPOS, 0, 0);
            if (control == GetDlgItem(hwndDlg, IDC_DAMP)) {
                g_this->set_int(p_fluidity.handle, value);
            } else if (control == GetDlgItem(hwndDlg, IDC_DEPTH)) {
                g_this->set_int(p_depth.handle, value);
            } else if (control == GetDlgItem(hwndDlg, IDC_RADIUS)) {
                g_this->set_int(p_drop_radius.handle, value);
            }
        }
            return 0;
    }
    return 0;
}
