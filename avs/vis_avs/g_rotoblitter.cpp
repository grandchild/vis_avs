#include "e_rotoblitter.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_rotoblitter(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto g_this = (E_RotoBlitter*)g_current_render;
    const Parameter& p_zoom = RotoBlitter_Info::parameters[0];
    const Parameter& p_rotate = RotoBlitter_Info::parameters[1];
    AVS_Parameter_Handle p_blend_mode = RotoBlitter_Info::parameters[2].handle;
    AVS_Parameter_Handle p_bilinear = RotoBlitter_Info::parameters[3].handle;
    AVS_Parameter_Handle p_on_beat_reverse = RotoBlitter_Info::parameters[4].handle;
    const Parameter& p_on_beat_reverse_speed = RotoBlitter_Info::parameters[5];
    AVS_Parameter_Handle p_on_beat_zoom_enable = RotoBlitter_Info::parameters[6].handle;
    const Parameter& p_on_beat_zoom = RotoBlitter_Info::parameters[7];

    switch (uMsg) {
        case WM_INITDIALOG: {
            auto zoom = g_this->get_int(p_zoom.handle);
            init_ranged_slider(p_zoom, zoom, hwndDlg, IDC_SLIDER1);
            auto on_beat_zoom = g_this->get_int(p_on_beat_zoom.handle);
            init_ranged_slider(p_on_beat_zoom, on_beat_zoom, hwndDlg, IDC_SLIDER6);
            auto rotate = g_this->get_int(p_rotate.handle);
            init_ranged_slider(p_rotate, rotate, hwndDlg, IDC_SLIDER2);
            auto on_beat_reverse_speed =
                g_this->get_int(p_on_beat_reverse_speed.handle);
            init_ranged_slider(
                p_on_beat_reverse_speed, on_beat_reverse_speed, hwndDlg, IDC_SLIDER5);
            CheckDlgButton(hwndDlg, IDC_CHECK2, g_this->get_bool(p_bilinear));
            CheckDlgButton(
                hwndDlg, IDC_BLEND, g_this->get_int(p_blend_mode) == BLEND_SIMPLE_5050);
            CheckDlgButton(hwndDlg, IDC_CHECK1, g_this->get_bool(p_on_beat_reverse));
            CheckDlgButton(
                hwndDlg, IDC_CHECK6, g_this->get_bool(p_on_beat_zoom_enable));
            return 1;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_BLEND:
                    g_this->set_int(p_blend_mode,
                                    IsDlgButtonChecked(hwndDlg, IDC_BLEND)
                                        ? BLEND_SIMPLE_5050
                                        : BLEND_SIMPLE_REPLACE);
                    break;
                case IDC_CHECK1:
                    g_this->set_bool(p_on_beat_reverse,
                                     IsDlgButtonChecked(hwndDlg, IDC_CHECK1));
                    break;
                case IDC_CHECK2:
                    g_this->set_bool(p_bilinear,
                                     IsDlgButtonChecked(hwndDlg, IDC_CHECK2));
                    break;
                case IDC_CHECK6:
                    g_this->set_bool(p_on_beat_zoom_enable,
                                     IsDlgButtonChecked(hwndDlg, IDC_CHECK6));
                    break;
            }
            return 0;
        case WM_HSCROLL: {
            HWND swnd = (HWND)lParam;
            int t = (int)SendMessage(swnd, TBM_GETPOS, 0, 0);
            if (swnd == GetDlgItem(hwndDlg, IDC_SLIDER1)) {
                g_this->set_int(p_zoom.handle, t);
            }
            if (swnd == GetDlgItem(hwndDlg, IDC_SLIDER6)) {
                g_this->set_int(p_on_beat_zoom.handle, t);
            }
            if (swnd == GetDlgItem(hwndDlg, IDC_SLIDER2)) {
                g_this->set_int(p_rotate.handle, t);
            }
            if (swnd == GetDlgItem(hwndDlg, IDC_SLIDER5)) {
                g_this->set_int(p_on_beat_reverse_speed.handle, t);
            }
            return 0;
        }
    }
    return 0;
}
