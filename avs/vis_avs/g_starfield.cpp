#include "e_starfield.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

#define SPEED_FACTOR         100.0
#define ON_BEAT_SPEED_FACTOR 100.0

#define RGB_TO_BGR(color) \
    (((color) & 0xff0000) >> 16 | ((color) & 0xff00) | ((color) & 0xff) << 16)
#define BGR_TO_RGB(color) RGB_TO_BGR(color)  // is its own inverse

int win32_dlgproc_starfield(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto g_this = (E_Starfield*)g_current_render;
    AVS_Parameter_Handle p_color = Starfield_Info::parameters[0].handle;
    const Parameter& p_blend_mode = Starfield_Info::parameters[1];
    const Parameter& p_speed = Starfield_Info::parameters[2];
    const Parameter& p_stars = Starfield_Info::parameters[3];
    const AVS_Parameter_Handle p_on_beat = Starfield_Info::parameters[4].handle;
    const Parameter& p_on_beat_speed = Starfield_Info::parameters[5];
    const Parameter& p_on_beat_duration = Starfield_Info::parameters[6];

    switch (uMsg) {
        case WM_INITDIALOG: {
            CheckDlgButton(hwndDlg, IDC_CHECK1, g_this->enabled);
            auto blend_mode = g_this->get_int(p_blend_mode.handle);
            static constexpr size_t num_blend_modes = 3;
            uint32_t controls_blend_mode[num_blend_modes] = {
                IDC_REPLACE, IDC_5050, IDC_ADDITIVE};
            init_select_radio(p_blend_mode,
                              blend_mode,
                              hwndDlg,
                              controls_blend_mode,
                              num_blend_modes);
            auto speed = g_this->get_float(p_speed.handle);
            init_ranged_slider_float(p_speed, speed, SPEED_FACTOR, hwndDlg, IDC_SPEED);
            auto stars = g_this->get_int(p_stars.handle);
            init_ranged_slider(p_stars, stars, hwndDlg, IDC_NUMSTARS);
            CheckDlgButton(hwndDlg, IDC_ONBEAT2, g_this->get_bool(p_on_beat));
            auto on_beat_speed = g_this->get_float(p_on_beat_speed.handle);
            init_ranged_slider_float(p_on_beat_speed,
                                     on_beat_speed,
                                     ON_BEAT_SPEED_FACTOR,
                                     hwndDlg,
                                     IDC_SPDCHG);

            auto on_beat_duration = g_this->get_int(p_on_beat_duration.handle);
            init_ranged_slider(
                p_on_beat_duration, on_beat_duration, hwndDlg, IDC_SPDDUR);

            return 1;
        }
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* draw = (DRAWITEMSTRUCT*)lParam;
            if (draw->CtlID == IDC_DEFCOL) {
                uint32_t color = RGB_TO_BGR((uint32_t)(g_this->get_color(p_color)));
                LOGBRUSH lb = {BS_SOLID, color, 0};
                HPEN pen = (HPEN)CreatePen(PS_SOLID, 0, color);
                HBRUSH brush = CreateBrushIndirect(&lb);
                HPEN pen_obj = (HPEN)SelectObject(draw->hDC, pen);
                HBRUSH brush_obj = (HBRUSH)SelectObject(draw->hDC, brush);
                Rectangle(draw->hDC,
                          draw->rcItem.left,
                          draw->rcItem.top,
                          draw->rcItem.right,
                          draw->rcItem.bottom);
                SelectObject(draw->hDC, pen_obj);
                SelectObject(draw->hDC, brush_obj);
                DeleteObject(brush);
                DeleteObject(pen);
            }
            return 0;
        }
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDC_CHECK1:
                    g_this->set_enabled(IsDlgButtonChecked(hwndDlg, IDC_CHECK1));
                    break;
                case IDC_ONBEAT2:
                    g_this->set_bool(p_on_beat,
                                     IsDlgButtonChecked(hwndDlg, IDC_ONBEAT2));
                    break;
                case IDC_REPLACE:
                case IDC_ADDITIVE:
                case IDC_5050:
                    g_this->set_int(
                        p_blend_mode.handle,
                        IsDlgButtonChecked(hwndDlg, IDC_ADDITIVE)
                            ? 2
                            : (IsDlgButtonChecked(hwndDlg, IDC_5050) ? 1 : 0));
                    break;
                case IDC_DEFCOL: {
                    uint32_t color = g_this->get_color(p_color);
                    static COLORREF custom_colors[16];
                    CHOOSECOLOR choose;
                    choose.lStructSize = sizeof(choose);
                    choose.hwndOwner = hwndDlg;
                    choose.hInstance = 0;
                    choose.rgbResult = RGB_TO_BGR(color);
                    choose.lpCustColors = custom_colors;
                    choose.Flags = CC_RGBINIT | CC_FULLOPEN;
                    if (ChooseColor(&choose)) {
                        g_this->set_color(p_color, BGR_TO_RGB(choose.rgbResult));
                    }
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_DEFCOL), NULL, TRUE);
                    break;
                }
            }
            return 0;
        }
        case WM_HSCROLL: {
            HWND control = (HWND)lParam;
            int value = (int)SendMessage(control, TBM_GETPOS, 0, 0);
            if (control == GetDlgItem(hwndDlg, IDC_NUMSTARS)) {
                g_this->set_int(p_stars.handle, value);
            } else if (control == GetDlgItem(hwndDlg, IDC_SPDDUR)) {
                g_this->set_int(p_on_beat_duration.handle, value);
            } else if (control == GetDlgItem(hwndDlg, IDC_SPEED)) {
                g_this->set_float(p_speed.handle, (double)value / SPEED_FACTOR);
            } else if (control == GetDlgItem(hwndDlg, IDC_SPDCHG)) {
                g_this->set_float(p_on_beat_speed.handle,
                                  (double)value / ON_BEAT_SPEED_FACTOR);
            }
            return 0;
        }
    }
    return 0;
}
