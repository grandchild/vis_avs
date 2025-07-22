#include "e_text.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_text(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    E_Text* g_ConfigThis = (E_Text*)g_current_render;

    auto g_this = (E_Text*)g_current_render;
    AVS_Parameter_Handle p_color = Text_Info::parameters[0].handle;
    const Parameter& p_blend_mode = Text_Info::parameters[1];
    AVS_Parameter_Handle p_on_beat = Text_Info::parameters[2].handle;
    AVS_Parameter_Handle p_insert_blanks = Text_Info::parameters[3].handle;
    AVS_Parameter_Handle p_random_position = Text_Info::parameters[4].handle;
    const Parameter& p_vertical_align = Text_Info::parameters[5];
    const Parameter& p_horizontal_align = Text_Info::parameters[6];
    const Parameter& p_on_beat_speed = Text_Info::parameters[7];
    const Parameter& p_speed = Text_Info::parameters[8];
    const Parameter& p_weight = Text_Info::parameters[9];
    AVS_Parameter_Handle p_italic = Text_Info::parameters[10].handle;
    AVS_Parameter_Handle p_underline = Text_Info::parameters[11].handle;
    AVS_Parameter_Handle p_strike_out = Text_Info::parameters[12].handle;
    const Parameter& p_char_set = Text_Info::parameters[13];
    AVS_Parameter_Handle p_font_name = Text_Info::parameters[14].handle;
    AVS_Parameter_Handle p_text = Text_Info::parameters[15].handle;
    const Parameter& p_border = Text_Info::parameters[16];
    AVS_Parameter_Handle p_border_color = Text_Info::parameters[17].handle;
    const Parameter& p_border_size = Text_Info::parameters[18];
    const Parameter& p_shift_x = Text_Info::parameters[19];
    const Parameter& p_shift_y = Text_Info::parameters[20];
    AVS_Parameter_Handle p_random_word = Text_Info::parameters[21].handle;

    switch (uMsg) {
        case WM_INITDIALOG: {
            SetDlgItemText(hwndDlg, IDC_EDIT, g_this->get_string(p_text));
            auto speed = g_this->get_int(p_speed.handle);
            auto on_beat_speed = g_this->get_int(p_on_beat_speed.handle);
            auto on_beat = g_this->get_bool(p_on_beat);
            init_ranged_slider(
                p_speed, on_beat ? on_beat_speed : speed, hwndDlg, IDC_SPEED, 10);
            auto border_size = g_this->get_int(p_border_size.handle);
            init_ranged_slider(
                p_border_size, border_size, hwndDlg, IDC_OUTLINESIZE, 15);
            auto shift_x = g_this->get_int(p_shift_x.handle);
            auto shift_y = g_this->get_int(p_shift_y.handle);
            init_ranged_slider(p_shift_x, shift_x, hwndDlg, IDC_HSHIFT, 50);
            init_ranged_slider(p_shift_y, shift_y, hwndDlg, IDC_VSHIFT, 50);

            CheckDlgButton(hwndDlg, IDC_CHECK1, g_this->enabled);
            CheckDlgButton(hwndDlg, IDC_ONBEAT, g_this->get_bool(p_on_beat));

            auto blend_mode = g_this->get_int(p_blend_mode.handle);
            static constexpr size_t num_blend_modes = 3;
            uint32_t controls_blend_mode[num_blend_modes] = {
                IDC_REPLACE, IDC_5050, IDC_ADDITIVE};
            init_select_radio(p_blend_mode,
                              blend_mode,
                              hwndDlg,
                              controls_blend_mode,
                              num_blend_modes);

            auto border_mode = g_this->get_int(p_border.handle);
            static constexpr size_t num_border_modes = 3;
            uint32_t controls_border_mode[num_border_modes] = {
                IDC_PLAIN, IDC_OUTLINE, IDC_SHADOW};
            init_select_radio(
                p_border, border_mode, hwndDlg, controls_border_mode, num_border_modes);
            CheckDlgButton(hwndDlg, IDC_RANDWORD, g_this->get_bool(p_random_word));

            auto v_align = g_this->get_int(p_vertical_align.handle);
            static constexpr size_t num_v_aligns = 3;
            uint32_t controls_v_align[num_v_aligns] = {
                IDC_VTOP, IDC_VBOTTOM, IDC_VCENTER};
            init_select_radio(
                p_vertical_align, v_align, hwndDlg, controls_v_align, num_v_aligns);

            auto h_align = g_this->get_int(p_horizontal_align.handle);
            static constexpr size_t num_h_aligns = 3;
            uint32_t controls_h_align[num_h_aligns] = {
                IDC_HLEFT, IDC_HRIGHT, IDC_HCENTER};
            init_select_radio(
                p_horizontal_align, h_align, hwndDlg, controls_h_align, num_h_aligns);
            CheckDlgButton(hwndDlg, IDC_BLANKS, g_this->get_bool(p_insert_blanks));
            if (g_this->get_bool(p_random_position)) {
                CheckDlgButton(hwndDlg, IDC_RANDOMPOS, BST_CHECKED);
                EnableWindow(GetDlgItem(hwndDlg, IDC_VTOP), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_VBOTTOM), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_VCENTER), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_HLEFT), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_HRIGHT), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_HCENTER), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_HSHIFT), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_VSHIFT), FALSE);
            }
            return 1;
        }
        case WM_NOTIFY: {
            if ((LOWORD(wParam) == IDC_VSHIFT) || (LOWORD(wParam) == IDC_HSHIFT)) {
                g_ConfigThis->config.shift_y =
                    SendDlgItemMessage(hwndDlg, IDC_VSHIFT, TBM_GETPOS, 0, 0) - 100;
                g_ConfigThis->config.shift_x =
                    SendDlgItemMessage(hwndDlg, IDC_HSHIFT, TBM_GETPOS, 0, 0) - 100;
                g_ConfigThis->forceshift = 1;
                g_ConfigThis->forceredraw = 1;
                g_ConfigThis->shiftinit = 1;
            }
            if (LOWORD(wParam) == IDC_OUTLINESIZE) {
                g_ConfigThis->config.border_size =
                    SendDlgItemMessage(hwndDlg, IDC_OUTLINESIZE, TBM_GETPOS, 0, 0);
                g_ConfigThis->forceredraw = 1;
            }
            if (LOWORD(wParam) == IDC_SPEED) {
                if (g_ConfigThis->config.on_beat) {
                    g_ConfigThis->config.on_beat_speed =
                        SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_GETPOS, 0, 0);
                    if (g_ConfigThis->nb > g_ConfigThis->config.on_beat_speed) {
                        g_ConfigThis->nb = g_ConfigThis->config.on_beat_speed;
                    }
                } else {
                    g_ConfigThis->config.speed =
                        SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_GETPOS, 0, 0);
                }
            }
            return 0;
        }
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* di = (DRAWITEMSTRUCT*)lParam;
            if (di->CtlID == IDC_DEFCOL || di->CtlID == IDC_DEFOUTCOL) {
                GR_DrawColoredButton(di,
                                     di->CtlID == IDC_DEFCOL
                                         ? g_ConfigThis->config.color
                                         : g_ConfigThis->config.border_color);
            }
            return 0;
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_HRESET) {
                g_ConfigThis->config.shift_x = 0;
                SendDlgItemMessage(hwndDlg, IDC_HSHIFT, TBM_SETPOS, TRUE, 100);
                g_ConfigThis->forceshift = 1;
                g_ConfigThis->forceredraw = 1;
                g_ConfigThis->shiftinit = 1;
            }
            if (LOWORD(wParam) == IDC_VRESET) {
                g_ConfigThis->config.shift_y = 0;
                SendDlgItemMessage(hwndDlg, IDC_VSHIFT, TBM_SETPOS, TRUE, 100);
                g_ConfigThis->forceshift = 1;
                g_ConfigThis->forceredraw = 1;
                g_ConfigThis->shiftinit = 1;
            }
            if (LOWORD(wParam) == IDC_EDIT) {
                if (HIWORD(wParam) == EN_CHANGE) {
                    int l;
                    l = SendDlgItemMessage(hwndDlg, IDC_EDIT, WM_GETTEXTLENGTH, 0, 0);
                    if (!l) {
                        g_ConfigThis->config.text = "";
                    } else {
                        char buf[256];
                        GetDlgItemText(hwndDlg, IDC_EDIT, buf, l + 1);
                        g_ConfigThis->config.text = buf;
                    }
                    g_ConfigThis->forceredraw = 1;
                }
            }
            if (LOWORD(wParam) == IDC_CHOOSEFONT) {
                g_ConfigThis->cf.hwndOwner = hwndDlg;
                ChooseFont(&(g_ConfigThis->cf));
                g_ConfigThis->updating = true;
                g_ConfigThis->myFont = CreateFontIndirect(&(g_ConfigThis->lf));
                g_ConfigThis->forceredraw = 1;
                if (g_ConfigThis->hOldFont) {
                    SelectObject(g_ConfigThis->hBitmapDC, g_ConfigThis->hOldFont);
                }
                if (g_ConfigThis->myFont) {
                    g_ConfigThis->hOldFont = (HFONT)SelectObject(
                        g_ConfigThis->hBitmapDC, g_ConfigThis->myFont);
                } else {
                    g_ConfigThis->hOldFont = NULL;
                }
                g_ConfigThis->updating = false;
            }
            if ((LOWORD(wParam) == IDC_CHECK1) || (LOWORD(wParam) == IDC_ONBEAT)
                || (LOWORD(wParam) == IDC_ADDITIVE) || (LOWORD(wParam) == IDC_REPLACE)
                || (LOWORD(wParam) == IDC_BLANKS) || (LOWORD(wParam) == IDC_RANDOMPOS)
                || (LOWORD(wParam) == IDC_OUTLINE) || (LOWORD(wParam) == IDC_SHADOW)
                || (LOWORD(wParam) == IDC_PLAIN) || (LOWORD(wParam) == IDC_HLEFT)
                || (LOWORD(wParam) == IDC_HRIGHT) || (LOWORD(wParam) == IDC_HCENTER)
                || (LOWORD(wParam) == IDC_VTOP) || (LOWORD(wParam) == IDC_VBOTTOM)
                || (LOWORD(wParam) == IDC_VCENTER) || (LOWORD(wParam) == IDC_RANDWORD)
                || (LOWORD(wParam) == IDC_5050)) {
                g_ConfigThis->forceredraw = 1;
                g_ConfigThis->config.random_word =
                    IsDlgButtonChecked(hwndDlg, IDC_RANDWORD) ? 1 : 0;
                g_ConfigThis->enabled = IsDlgButtonChecked(hwndDlg, IDC_CHECK1) ? 1 : 0;
                g_ConfigThis->config.border = IsDlgButtonChecked(hwndDlg, IDC_SHADOW)
                                                  ? TEXT_BORDER_SHADOW
                                                  : TEXT_BORDER_NONE;
                g_ConfigThis->config.border = IsDlgButtonChecked(hwndDlg, IDC_OUTLINE)
                                                  ? TEXT_BORDER_OUTLINE
                                                  : TEXT_BORDER_NONE;
                g_ConfigThis->config.on_beat =
                    IsDlgButtonChecked(hwndDlg, IDC_ONBEAT) ? 1 : 0;
                g_ConfigThis->config.blend_mode =
                    IsDlgButtonChecked(hwndDlg, IDC_ADDITIVE) ? BLEND_SIMPLE_ADDITIVE
                                                              : BLEND_SIMPLE_REPLACE;
                g_ConfigThis->config.blend_mode = IsDlgButtonChecked(hwndDlg, IDC_5050)
                                                      ? BLEND_SIMPLE_5050
                                                      : BLEND_SIMPLE_REPLACE;
                if (g_ConfigThis->config.on_beat) {
                    SendDlgItemMessage(hwndDlg,
                                       IDC_SPEED,
                                       TBM_SETPOS,
                                       TRUE,
                                       (int)(g_ConfigThis->config.on_beat_speed));
                } else {
                    SendDlgItemMessage(hwndDlg,
                                       IDC_SPEED,
                                       TBM_SETPOS,
                                       TRUE,
                                       (int)(g_ConfigThis->config.speed));
                }
                g_ConfigThis->config.insert_blanks =
                    IsDlgButtonChecked(hwndDlg, IDC_BLANKS) ? 1 : 0;
                g_ConfigThis->config.random_position =
                    IsDlgButtonChecked(hwndDlg, IDC_RANDOMPOS) ? 1 : 0;
                g_ConfigThis->config.vertical_align =
                    IsDlgButtonChecked(hwndDlg, IDC_VTOP)
                        ? VPOS_TOP
                        : g_ConfigThis->config.vertical_align;
                g_ConfigThis->config.vertical_align =
                    IsDlgButtonChecked(hwndDlg, IDC_VCENTER)
                        ? VPOS_CENTER
                        : g_ConfigThis->config.vertical_align;
                g_ConfigThis->config.vertical_align =
                    IsDlgButtonChecked(hwndDlg, IDC_VBOTTOM)
                        ? VPOS_BOTTOM
                        : g_ConfigThis->config.vertical_align;
                g_ConfigThis->config.horizontal_align =
                    IsDlgButtonChecked(hwndDlg, IDC_HLEFT)
                        ? HPOS_LEFT
                        : g_ConfigThis->config.horizontal_align;
                g_ConfigThis->config.horizontal_align =
                    IsDlgButtonChecked(hwndDlg, IDC_HRIGHT)
                        ? HPOS_RIGHT
                        : g_ConfigThis->config.horizontal_align;
                g_ConfigThis->config.horizontal_align =
                    IsDlgButtonChecked(hwndDlg, IDC_HCENTER)
                        ? HPOS_CENTER
                        : g_ConfigThis->config.horizontal_align;
                EnableWindow(GetDlgItem(hwndDlg, IDC_VTOP),
                             !g_ConfigThis->config.random_position);
                EnableWindow(GetDlgItem(hwndDlg, IDC_VBOTTOM),
                             !g_ConfigThis->config.random_position);
                EnableWindow(GetDlgItem(hwndDlg, IDC_VCENTER),
                             !g_ConfigThis->config.random_position);
                EnableWindow(GetDlgItem(hwndDlg, IDC_HLEFT),
                             !g_ConfigThis->config.random_position);
                EnableWindow(GetDlgItem(hwndDlg, IDC_HRIGHT),
                             !g_ConfigThis->config.random_position);
                EnableWindow(GetDlgItem(hwndDlg, IDC_HCENTER),
                             !g_ConfigThis->config.random_position);
                EnableWindow(GetDlgItem(hwndDlg, IDC_HSHIFT),
                             !g_ConfigThis->config.random_position);
                EnableWindow(GetDlgItem(hwndDlg, IDC_VSHIFT),
                             !g_ConfigThis->config.random_position);
                if ((LOWORD(wParam) == IDC_ONBEAT) && g_ConfigThis->config.on_beat) {
                    g_ConfigThis->nb = g_ConfigThis->config.on_beat_speed;
                }
                if ((LOWORD(wParam) == IDC_RANDOMPOS)
                    && g_ConfigThis->config.random_position) {
                    g_ConfigThis->forcealign = 1;
                }
            }
            if (LOWORD(wParam) == IDC_DEFCOL || LOWORD(wParam) == IDC_DEFOUTCOL) {
                int a = LOWORD(wParam) == IDC_DEFCOL
                            ? g_ConfigThis->config.color
                            : g_ConfigThis->config.border_color;
                static COLORREF custcolors[16];
                CHOOSECOLOR cs;
                cs.lStructSize = sizeof(cs);
                cs.hwndOwner = hwndDlg;
                cs.hInstance = 0;
                cs.rgbResult =
                    ((a >> 16) & 0xff) | (a & 0xff00) | ((a << 16) & 0xff0000);
                cs.lpCustColors = custcolors;
                cs.Flags = CC_RGBINIT | CC_FULLOPEN;
                if (ChooseColor(&cs)) {
                    a = ((cs.rgbResult >> 16) & 0xff) | (cs.rgbResult & 0xff00)
                        | ((cs.rgbResult << 16) & 0xff0000);
                    if (LOWORD(wParam) == IDC_DEFCOL) {
                        g_ConfigThis->config.color = a;
                    } else {
                        g_ConfigThis->config.border_color = a;
                    }
                }
                InvalidateRect(GetDlgItem(hwndDlg, LOWORD(wParam)), NULL, TRUE);
                g_ConfigThis->updating = true;
                SetTextColor(g_ConfigThis->hBitmapDC,
                             ((g_ConfigThis->config.color & 0xFF0000) >> 16)
                                 | (g_ConfigThis->config.color & 0xFF00)
                                 | (g_ConfigThis->config.color & 0xFF) << 16);
                g_ConfigThis->forceredraw = 1;
                g_ConfigThis->updating = false;
            }
    }
    return 0;
}
