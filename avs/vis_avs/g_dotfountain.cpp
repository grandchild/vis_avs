#include "e_dotfountain.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_dotfountain(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto g_this = (E_DotFountain*)g_current_render;
    const Parameter& p_rotation_speed = DotFountain_Info::parameters[1];
    const Parameter& p_angle = DotFountain_Info::parameters[2];
    AVS_Parameter_Handle p_color = DotFountain_Info::color_params[0].handle;

    switch (uMsg) {
        case WM_INITDIALOG: {
            auto rotation_speed = g_this->get_int(p_rotation_speed.handle);
            init_ranged_slider(p_rotation_speed, rotation_speed, hwndDlg, IDC_SLIDER1);
            auto angle = g_this->get_int(p_angle.handle);
            init_ranged_slider(p_angle, angle, hwndDlg, IDC_ANGLE);
            return 1;
        }
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* di = (DRAWITEMSTRUCT*)lParam;
            int color_index = IDC_C5 - di->CtlID;
            if (color_index >= 0 && color_index < 5) {
                auto color_value = g_this->get_color(p_color, {color_index});
                GR_DrawColoredButton(di, (uint32_t)color_value);
            }
            return 0;
        }
        case WM_COMMAND: {
            auto control = LOWORD(wParam);
            if (control == IDC_BUTTON1) {
                g_this->set_int(p_rotation_speed.handle, 0);
                SendDlgItemMessage(hwndDlg, IDC_SLIDER1, TBM_SETPOS, 1, 0);
                return 0;
            }
            int color_index = IDC_C5 - control;
            if (color_index >= 0 && color_index < 5) {
                auto color_value = (int32_t)g_this->get_color(p_color, {color_index});
                GR_SelectColor(hwndDlg, &color_value);
                InvalidateRect(GetDlgItem(hwndDlg, control), NULL, false);
                g_this->set_color(p_color, color_value, {color_index});
                return 0;
            }
            return 0;
        }
        case WM_HSCROLL: {
            HWND control = (HWND)lParam;
            int value = (int)SendMessage(control, TBM_GETPOS, 0, 0);
            if (control == GetDlgItem(hwndDlg, IDC_SLIDER1)) {
                g_this->set_int(p_rotation_speed.handle, value);
            } else if (control == GetDlgItem(hwndDlg, IDC_ANGLE)) {
                g_this->set_int(p_angle.handle, value);
            }
            return 0;
        }
    }
    return 0;
}
