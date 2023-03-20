#include "e_colorclip.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_colorclip(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto g_this = (E_ColorClip*)g_current_render;
    const Parameter& p_mode = ColorClip_Info::parameters[0];
    AVS_Parameter_Handle p_color_in = ColorClip_Info::parameters[1].handle;
    AVS_Parameter_Handle p_color_out = ColorClip_Info::parameters[2].handle;
    const Parameter& p_distance = ColorClip_Info::parameters[3];
    AVS_Parameter_Handle p_copy_in_to_out = ColorClip_Info::parameters[4].handle;

    switch (uMsg) {
        case WM_DRAWITEM: {
            auto di = (DRAWITEMSTRUCT*)lParam;
            switch (di->CtlID) {
                case IDC_LC:
                    GR_DrawColoredButton(di, g_this->get_color(p_color_in));
                    break;
                case IDC_LC2:
                    GR_DrawColoredButton(di, g_this->get_color(p_color_out));
                    break;
            }
            return 0;
        }
        case WM_INITDIALOG: {
            CheckDlgButton(hwndDlg, IDC_OFF, !g_this->enabled);
            if (g_this->enabled) {
                auto mode = g_this->get_int(p_mode.handle);
                CheckDlgButton(hwndDlg, IDC_BELOW, mode == COLORCLIP_BELOW);
                CheckDlgButton(hwndDlg, IDC_ABOVE, mode == COLORCLIP_ABOVE);
                CheckDlgButton(hwndDlg, IDC_NEAR, mode == COLORCLIP_NEAR);
            }
            auto distance = g_this->get_int(p_distance.handle);
            init_ranged_slider(p_distance, distance, hwndDlg, IDC_DISTANCE, 4);
            return 1;
        }
        case WM_NOTIFY:
            if (LOWORD(wParam) == IDC_DISTANCE) {
                g_this->set_int(
                    p_distance.handle,
                    SendDlgItemMessage(hwndDlg, IDC_DISTANCE, TBM_GETPOS, 0, 0));
            }
            return 0;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_OFF:
                case IDC_BELOW:
                case IDC_ABOVE:
                case IDC_NEAR:
                    if (IsDlgButtonChecked(hwndDlg, IDC_OFF)) {
                        g_this->set_enabled(false);
                    } else if (IsDlgButtonChecked(hwndDlg, IDC_BELOW)) {
                        g_this->set_enabled(true);
                        g_this->set_int(p_mode.handle, COLORCLIP_BELOW);
                    } else if (IsDlgButtonChecked(hwndDlg, IDC_ABOVE)) {
                        g_this->set_enabled(true);
                        g_this->set_int(p_mode.handle, COLORCLIP_ABOVE);
                    } else {
                        g_this->set_enabled(true);
                        g_this->set_int(p_mode.handle, COLORCLIP_NEAR);
                    }
                    return 0;
                case IDC_LC: {
                    int32_t color = (int32_t)g_this->get_color(p_color_in);
                    GR_SelectColor(hwndDlg, &color);
                    g_this->set_color(p_color_in, color);
                    InvalidateRect(GetDlgItem(hwndDlg, LOWORD(wParam)), nullptr, false);
                    return 0;
                }
                case IDC_LC2: {
                    int32_t color = (int32_t)g_this->get_color(p_color_out);
                    GR_SelectColor(hwndDlg, &color);
                    g_this->set_color(p_color_out, color);
                    InvalidateRect(GetDlgItem(hwndDlg, LOWORD(wParam)), nullptr, false);
                    return 0;
                }
                case IDC_BUTTON1:
                    g_this->run_action(p_copy_in_to_out);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_LC2), nullptr, false);
                    return 0;
            }
            return 0;
    }
    return 0;
}

Effect_Info* create_ColorClip_Info() { return new ColorClip_Info(); }
Effect* create_ColorClip() { return new E_ColorClip(); }
void set_ColorClip_desc(char* desc) { E_ColorClip::set_desc(desc); }
