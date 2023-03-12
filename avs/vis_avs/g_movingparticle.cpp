#include "e_movingparticle.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_movingparticle(HWND hwndDlg,
                                 UINT uMsg,
                                 WPARAM wParam,
                                 LPARAM lParam) {
    auto g_this = (E_MovingParticle*)g_current_render;
    AVS_Parameter_Handle p_color = MovingParticle_Info::parameters[0].handle;
    const Parameter& p_distance = MovingParticle_Info::parameters[1];
    const Parameter& p_size = MovingParticle_Info::parameters[2];
    AVS_Parameter_Handle p_on_beat_size_change =
        MovingParticle_Info::parameters[3].handle;
    const Parameter& p_on_beat_size = MovingParticle_Info::parameters[4];
    AVS_Parameter_Handle p_blend_mode = MovingParticle_Info::parameters[5].handle;

    int32_t color;
    switch (uMsg) {
        case WM_INITDIALOG: {
            auto distance = g_this->get_int(p_distance.handle);
            init_ranged_slider(p_distance, distance, hwndDlg, IDC_SLIDER1);
            auto particle_size = g_this->get_int(p_size.handle);
            init_ranged_slider(p_size, particle_size, hwndDlg, IDC_SLIDER3);
            auto on_beat_particle_size = g_this->get_int(p_on_beat_size.handle);
            init_ranged_slider(
                p_on_beat_size, on_beat_particle_size, hwndDlg, IDC_SLIDER4);

            CheckDlgButton(hwndDlg, IDC_LEFT, g_this->enabled);
            CheckDlgButton(
                hwndDlg, IDC_CHECK1, g_this->get_bool(p_on_beat_size_change));

            int32_t blend_control = IDC_RADIO2;  // additive
            switch (g_this->get_int(p_blend_mode)) {
                case PARTICLE_BLEND_REPLACE: blend_control = IDC_RADIO1; break;
                case PARTICLE_BLEND_ADDITIVE: blend_control = IDC_RADIO2; break;
                case PARTICLE_BLEND_5050: blend_control = IDC_RADIO3; break;
                case PARTICLE_BLEND_DEFAULT: blend_control = IDC_RADIO4; break;
            }
            CheckDlgButton(hwndDlg, blend_control, BST_CHECKED);

            return 1;
        }
        case WM_DRAWITEM: {
            auto di = (DRAWITEMSTRUCT*)lParam;
            switch (di->CtlID) {
                case IDC_LC:
                    GR_DrawColoredButton(di, g_this->get_color(p_color));
                    break;
            }
            return 0;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_RADIO1:
                case IDC_RADIO2:
                case IDC_RADIO3:
                case IDC_RADIO4:
                    if (IsDlgButtonChecked(hwndDlg, IDC_RADIO1)) {
                        g_this->set_int(p_blend_mode, PARTICLE_BLEND_REPLACE);
                    } else if (IsDlgButtonChecked(hwndDlg, IDC_RADIO2)) {
                        g_this->set_int(p_blend_mode, PARTICLE_BLEND_ADDITIVE);
                    } else if (IsDlgButtonChecked(hwndDlg, IDC_RADIO3)) {
                        g_this->set_int(p_blend_mode, PARTICLE_BLEND_5050);
                    } else if (IsDlgButtonChecked(hwndDlg, IDC_RADIO4)) {
                        g_this->set_int(p_blend_mode, PARTICLE_BLEND_DEFAULT);
                    }
                    break;
                case IDC_LEFT:
                    g_this->set_enabled(IsDlgButtonChecked(hwndDlg, IDC_LEFT));
                    break;
                case IDC_CHECK1:
                    g_this->set_bool(p_on_beat_size_change,
                                     IsDlgButtonChecked(hwndDlg, IDC_CHECK1));
                    break;
                case IDC_LC:
                    color = (int32_t)g_this->get_color(p_color);
                    GR_SelectColor(hwndDlg, &color);
                    g_this->set_color(p_color, color);
                    InvalidateRect(GetDlgItem(hwndDlg, LOWORD(wParam)), nullptr, FALSE);
                    break;
            }
            return 0;
        case WM_HSCROLL: {
            HWND swnd = (HWND)lParam;
            int t = (int)SendMessage(swnd, TBM_GETPOS, 0, 0);
            if (swnd == GetDlgItem(hwndDlg, IDC_SLIDER1)) {
                g_this->set_int(p_distance.handle, t);
            }
            if (swnd == GetDlgItem(hwndDlg, IDC_SLIDER3)) {
                g_this->set_int(p_size.handle, t);
            }
            if (swnd == GetDlgItem(hwndDlg, IDC_SLIDER4)) {
                g_this->set_int(p_on_beat_size.handle, t);
            }
            return 0;
        }
    }
    return 0;
}
