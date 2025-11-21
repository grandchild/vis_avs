#include "e_text.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

static int config_family_to_font_family(FontFamily family) {
    switch (family) {
        default:
        case FONT_FAMILY_DONTCARE: return 0;
        case FONT_FAMILY_SERIF: return 1;
        case FONT_FAMILY_SANSSERIF: return 2;
        case FONT_FAMILY_MONOSPACE: return 3;
        case FONT_FAMILY_CURSIVE: return 4;
        case FONT_FAMILY_FANTASY: return 5;
    }
}
static FontFamily font_pitchfamily_to_config_family(int pitch_and_family) {
    switch (pitch_and_family >> 4) {
        default:
        case 0: return FONT_FAMILY_DONTCARE;
        case 1: return FONT_FAMILY_SERIF;
        case 2: return FONT_FAMILY_SANSSERIF;
        case 3: return FONT_FAMILY_MONOSPACE;
        case 4: return FONT_FAMILY_CURSIVE;
        case 5: return FONT_FAMILY_FANTASY;
    }
}

int win32_dlgproc_text(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    E_Text* g_ConfigThis = (E_Text*)g_current_render;
    auto g_this = (E_Text*)g_current_render;
    const Parameter* p_text = &Text_Info::parameters[0];
    const Parameter* p_color = &Text_Info::parameters[1];
    const Parameter* p_blend_mode = &Text_Info::parameters[2];
    const Parameter* p_duration = &Text_Info::parameters[3];
    const Parameter* p_on_beat = &Text_Info::parameters[4];
    const Parameter* p_on_beat_duration = &Text_Info::parameters[5];
    const Parameter* p_insert_blanks = &Text_Info::parameters[6];
    const Parameter* p_random_position = &Text_Info::parameters[7];
    const Parameter* p_vertical_align = &Text_Info::parameters[8];
    const Parameter* p_horizontal_align = &Text_Info::parameters[9];
    const Parameter* p_weight = &Text_Info::parameters[10];
    const Parameter* p_height = &Text_Info::parameters[11];
    const Parameter* p_width = &Text_Info::parameters[12];
    const Parameter* p_italic = &Text_Info::parameters[13];
    const Parameter* p_underline = &Text_Info::parameters[14];
    const Parameter* p_strike_out = &Text_Info::parameters[15];
    const Parameter* p_char_set = &Text_Info::parameters[16];
    const Parameter* p_family = &Text_Info::parameters[17];
    const Parameter* p_font_name = &Text_Info::parameters[18];
    const Parameter* p_border = &Text_Info::parameters[19];
    const Parameter* p_border_color = &Text_Info::parameters[20];
    const Parameter* p_border_size = &Text_Info::parameters[21];
    const Parameter* p_shift_x = &Text_Info::parameters[22];
    const Parameter* p_shift_y = &Text_Info::parameters[23];
    const Parameter* p_random_word = &Text_Info::parameters[24];

    switch (uMsg) {
        case WM_INITDIALOG: {
            SetDlgItemText(hwndDlg, IDC_EDIT, g_this->get_string(p_text));
            auto speed = g_this->get_int(p_duration);
            auto on_beat_duration = g_this->get_int(p_on_beat_duration);
            auto on_beat = g_this->get_bool(p_on_beat);
            init_ranged_slider(
                p_duration, on_beat ? on_beat_duration : speed, hwndDlg, IDC_SPEED, 10);
            auto border_size = g_this->get_int(p_border_size);
            init_ranged_slider(
                p_border_size, border_size, hwndDlg, IDC_OUTLINESIZE, 15);
            auto shift_x = g_this->get_int(p_shift_x);
            auto shift_y = g_this->get_int(p_shift_y);
            init_ranged_slider(p_shift_x, shift_x, hwndDlg, IDC_HSHIFT, 50);
            init_ranged_slider(p_shift_y, shift_y, hwndDlg, IDC_VSHIFT, 50);

            CheckDlgButton(hwndDlg, IDC_CHECK1, g_this->enabled);
            CheckDlgButton(hwndDlg, IDC_ONBEAT, g_this->get_bool(p_on_beat));

            auto blend_mode = g_this->get_int(p_blend_mode);
            static constexpr size_t num_blend_modes = 3;
            uint32_t controls_blend_mode[num_blend_modes] = {
                IDC_REPLACE, IDC_5050, IDC_ADDITIVE};
            init_select_radio(p_blend_mode,
                              blend_mode,
                              hwndDlg,
                              controls_blend_mode,
                              num_blend_modes);

            auto border_mode = g_this->get_int(p_border);
            static constexpr size_t num_border_modes = 3;
            uint32_t controls_border_mode[num_border_modes] = {
                IDC_PLAIN, IDC_OUTLINE, IDC_SHADOW};
            init_select_radio(
                p_border, border_mode, hwndDlg, controls_border_mode, num_border_modes);
            CheckDlgButton(hwndDlg, IDC_RANDWORD, g_this->get_bool(p_random_word));

            auto v_align = g_this->get_int(p_vertical_align);
            static constexpr size_t num_v_aligns = 3;
            uint32_t controls_v_align[num_v_aligns] = {
                IDC_VTOP, IDC_VBOTTOM, IDC_VCENTER};
            init_select_radio(
                p_vertical_align, v_align, hwndDlg, controls_v_align, num_v_aligns);

            auto h_align = g_this->get_int(p_horizontal_align);
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
                g_this->set_int(
                    p_shift_y,
                    SendDlgItemMessage(hwndDlg, IDC_VSHIFT, TBM_GETPOS, 0, 0));
                g_this->set_int(
                    p_shift_x,
                    SendDlgItemMessage(hwndDlg, IDC_HSHIFT, TBM_GETPOS, 0, 0));
            }
            if (LOWORD(wParam) == IDC_OUTLINESIZE) {
                g_this->set_int(
                    p_border_size,
                    SendDlgItemMessage(hwndDlg, IDC_OUTLINESIZE, TBM_GETPOS, 0, 0));
            }
            if (LOWORD(wParam) == IDC_SPEED) {
                if (g_this->get_bool(p_on_beat)) {
                    g_this->set_int(
                        p_on_beat_duration,
                        SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_GETPOS, 0, 0));
                } else {
                    g_this->set_int(
                        p_duration,
                        SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_GETPOS, 0, 0));
                }
            }
            return 0;
        }
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* di = (DRAWITEMSTRUCT*)lParam;
            if (di->CtlID == IDC_DEFCOL || di->CtlID == IDC_DEFOUTCOL) {
                GR_DrawColoredButton(di,
                                     di->CtlID == IDC_DEFCOL
                                         ? g_this->get_color(p_color)
                                         : g_this->get_color(p_border_color));
            }
            return 0;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_HRESET:
                    g_this->set_int(p_shift_x, 0);
                    SendDlgItemMessage(hwndDlg, IDC_HSHIFT, TBM_SETPOS, TRUE, 0);
                    break;
                case IDC_VRESET:
                    g_this->set_int(p_shift_y, 0);
                    SendDlgItemMessage(hwndDlg, IDC_VSHIFT, TBM_SETPOS, TRUE, 0);
                    break;
                case IDC_EDIT:
                    if (HIWORD(wParam) == EN_CHANGE) {
                        int l;
                        l = SendDlgItemMessage(
                            hwndDlg, IDC_EDIT, WM_GETTEXTLENGTH, 0, 0);
                        if (!l) {
                            g_this->set_string(p_text, "");
                        } else {
                            char buf[1024];
                            GetDlgItemText(hwndDlg, IDC_EDIT, buf, l + 1);
                            g_this->set_string(p_text, buf);
                        }
                    }
                    break;
                case IDC_CHOOSEFONT: {
                    LOGFONT lf;
                    memset(&lf, 0, sizeof(LOGFONT));
                    lf.lfHeight = -g_this->get_int(p_height);
                    lf.lfWeight = g_this->get_int(p_weight) * 100;
                    lf.lfWidth = g_this->get_int(p_width);
                    lf.lfItalic = g_this->get_bool(p_italic);
                    lf.lfUnderline = g_this->get_bool(p_underline);
                    lf.lfStrikeOut = g_this->get_bool(p_strike_out);
                    lf.lfCharSet = g_this->get_int(p_char_set);
                    lf.lfPitchAndFamily = config_family_to_font_family(
                                              (FontFamily)g_this->get_int(p_family))
                                          << 4;
                    strncpy(lf.lfFaceName, g_this->get_string(p_font_name), 31);
                    lf.lfFaceName[31] = '\0';

                    CHOOSEFONT cf;
                    memset(&cf, 0, sizeof(CHOOSEFONT));
                    cf.lStructSize = sizeof(CHOOSEFONT);
                    cf.hwndOwner = hwndDlg;
                    cf.lpLogFont = &lf;
                    cf.Flags = CF_EFFECTS | CF_SCREENFONTS | CF_FORCEFONTEXIST
                               | CF_INITTOLOGFONTSTRUCT;
                    auto a = g_this->get_color(p_color);
                    cf.rgbColors =
                        ((a >> 16) & 0xff) | (a & 0xff00) | ((a << 16) & 0xff0000);
                    if (!ChooseFont(&cf)) {
                        break;
                    }
                    if (!CreateFontIndirect(&lf)) {
                        // g_ConfigThis->updating = true;
                        break;
                        g_this->set_string(p_font_name, lf.lfFaceName);
                        g_this->set_int(p_weight, lf.lfWeight / 100);
                        g_this->set_int(p_height, abs(lf.lfHeight));
                        g_this->set_int(p_width, lf.lfWidth);
                        g_this->set_bool(p_italic, lf.lfItalic);
                        g_this->set_bool(p_underline, lf.lfUnderline);
                        g_this->set_bool(p_strike_out, lf.lfStrikeOut);
                        g_this->set_int(p_char_set, lf.lfCharSet);
                        g_this->set_int(
                            p_family,
                            font_pitchfamily_to_config_family(lf.lfPitchAndFamily));
                        // g_ConfigThis->updating = false;
                    }
                    break;
                }
                case IDC_CHECK1:
                    g_this->set_enabled(IsDlgButtonChecked(hwndDlg, IDC_CHECK1));
                    break;
                case IDC_ONBEAT: {
                    bool is_on_beat = IsDlgButtonChecked(hwndDlg, IDC_ONBEAT);
                    g_this->set_bool(p_on_beat, is_on_beat);
                    SendDlgItemMessage(
                        hwndDlg,
                        IDC_SPEED,
                        TBM_SETPOS,
                        TRUE,
                        g_this->get_int(is_on_beat ? p_on_beat_duration : p_duration));
                    break;
                }
                case IDC_ADDITIVE:
                    g_this->set_int(p_blend_mode,
                                    IsDlgButtonChecked(hwndDlg, IDC_ADDITIVE)
                                        ? BLEND_SIMPLE_ADDITIVE
                                        : BLEND_SIMPLE_REPLACE);
                    break;
                case IDC_REPLACE:
                    g_this->set_int(p_blend_mode,
                                    IsDlgButtonChecked(hwndDlg, IDC_REPLACE)
                                        ? BLEND_SIMPLE_REPLACE
                                        : BLEND_SIMPLE_5050);
                    break;
                case IDC_BLANKS:
                    g_this->set_bool(p_insert_blanks,
                                     IsDlgButtonChecked(hwndDlg, IDC_BLANKS));
                    break;
                case IDC_RANDOMPOS: {
                    bool is_random = IsDlgButtonChecked(hwndDlg, IDC_RANDOMPOS);
                    g_this->set_bool(p_random_position, is_random);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_VTOP), !is_random);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_VBOTTOM), !is_random);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_VCENTER), !is_random);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_HLEFT), !is_random);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_HRIGHT), !is_random);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_HCENTER), !is_random);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_HSHIFT), !is_random);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_VSHIFT), !is_random);
                    // if (is_random) {
                    //     g_ConfigThis->forcealign = 1;
                    // }
                    break;
                }
                case IDC_PLAIN: [[fallthrough]];
                case IDC_OUTLINE:
                    g_this->set_int(p_border,
                                    IsDlgButtonChecked(hwndDlg, IDC_OUTLINE)
                                        ? TEXT_BORDER_OUTLINE
                                        : TEXT_BORDER_NONE);
                    break;
                case IDC_SHADOW:
                    g_this->set_int(p_border,
                                    IsDlgButtonChecked(hwndDlg, IDC_SHADOW)
                                        ? TEXT_BORDER_SHADOW
                                        : TEXT_BORDER_NONE);
                    break;
                case IDC_HLEFT:
                    if (IsDlgButtonChecked(hwndDlg, IDC_HLEFT)) {
                        g_this->set_int(p_horizontal_align, HPOS_LEFT);
                    }
                    break;
                case IDC_HCENTER:
                    if (IsDlgButtonChecked(hwndDlg, IDC_HCENTER)) {
                        g_this->set_int(p_horizontal_align, HPOS_CENTER);
                    }
                    break;
                case IDC_HRIGHT:
                    if (IsDlgButtonChecked(hwndDlg, IDC_HRIGHT)) {
                        g_this->set_int(p_horizontal_align, HPOS_RIGHT);
                    }
                    break;
                case IDC_VTOP:
                    if (IsDlgButtonChecked(hwndDlg, IDC_VTOP)) {
                        g_this->set_int(p_vertical_align, VPOS_TOP);
                    }
                    break;
                case IDC_VCENTER:
                    if (IsDlgButtonChecked(hwndDlg, IDC_VCENTER)) {
                        g_this->set_int(p_vertical_align, VPOS_CENTER);
                    }
                    break;
                case IDC_VBOTTOM:
                    if (IsDlgButtonChecked(hwndDlg, IDC_VBOTTOM)) {
                        g_this->set_int(p_vertical_align, VPOS_BOTTOM);
                    }
                    break;
                case IDC_RANDWORD:
                    g_this->set_bool(p_random_word,
                                     IsDlgButtonChecked(hwndDlg, IDC_RANDWORD));
                    break;
                case IDC_5050:
                    g_this->set_int(p_blend_mode,
                                    IsDlgButtonChecked(hwndDlg, IDC_5050)
                                        ? BLEND_SIMPLE_5050
                                        : BLEND_SIMPLE_REPLACE);
                    break;
            }
            if (LOWORD(wParam) == IDC_DEFCOL || LOWORD(wParam) == IDC_DEFOUTCOL) {
                int a = g_this->get_color(
                    LOWORD(wParam) == IDC_DEFCOL ? p_color : p_border_color);
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
                    g_this->set_color(
                        LOWORD(wParam) == IDC_DEFCOL ? p_color : p_border_color, a);
                }
                InvalidateRect(GetDlgItem(hwndDlg, LOWORD(wParam)), NULL, TRUE);
            }
    }
    return 0;
}
