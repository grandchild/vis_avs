#include "e_effectlist.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include "../platform.h"

#include <windows.h>
#include <commctrl.h>
#include <string>

void show_hide_blendmode_dependent_ui(E_EffectList* g_this,
                                      const Parameter& p_input_blend_mode,
                                      const Parameter& p_output_blend_mode,
                                      HWND hwndDlg) {
    int64_t length = 0;
    auto blend_mode_strings = p_input_blend_mode.get_options(&length);
    std::string input_blend_mode_str =
        blend_mode_strings[g_this->get_int(p_input_blend_mode.handle)];
    std::string output_blend_mode_str =
        blend_mode_strings[g_this->get_int(p_output_blend_mode.handle)];

    bool input_blend_is_adjustable = input_blend_mode_str == "Adjustable";
    bool output_blend_is_adjustable = output_blend_mode_str == "Adjustable";
    ShowWindow(GetDlgItem(hwndDlg, IDC_INSLIDE), input_blend_is_adjustable);
    ShowWindow(GetDlgItem(hwndDlg, IDC_OUTSLIDE), output_blend_is_adjustable);
    bool input_blend_is_buffer = input_blend_mode_str == "Buffer";
    bool output_blend_is_buffer = output_blend_mode_str == "Buffer";
    ShowWindow(GetDlgItem(hwndDlg, IDC_CBBUF1), input_blend_is_buffer);
    ShowWindow(GetDlgItem(hwndDlg, IDC_CBBUF2), output_blend_is_buffer);
    ShowWindow(GetDlgItem(hwndDlg, IDC_INVERT1), input_blend_is_buffer);
    ShowWindow(GetDlgItem(hwndDlg, IDC_INVERT2), output_blend_is_buffer);
}

int win32_dlgproc_effectlist(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto g_this = (E_EffectList*)g_current_render;
    AVS_Parameter_Handle p_enabled_on_beat = EffectList_Info::parameters[0].handle;
    AVS_Parameter_Handle p_enabled_on_beat_frames =
        EffectList_Info::parameters[1].handle;
    AVS_Parameter_Handle p_clear_every_frame = EffectList_Info::parameters[2].handle;
    const Parameter& p_input_blend_mode = EffectList_Info::parameters[3];
    const Parameter& p_output_blend_mode = EffectList_Info::parameters[4];
    const Parameter& p_input_blend_adjustable = EffectList_Info::parameters[5];
    const Parameter& p_output_blend_adjustable = EffectList_Info::parameters[6];
    const Parameter& p_input_blend_buffer = EffectList_Info::parameters[7];
    const Parameter& p_output_blend_buffer = EffectList_Info::parameters[8];
    AVS_Parameter_Handle p_input_blend_buffer_invert =
        EffectList_Info::parameters[9].handle;
    AVS_Parameter_Handle p_output_blend_buffer_invert =
        EffectList_Info::parameters[10].handle;
    AVS_Parameter_Handle p_use_code = EffectList_Info::parameters[11].handle;
    AVS_Parameter_Handle p_init = EffectList_Info::parameters[12].handle;
    AVS_Parameter_Handle p_frame = EffectList_Info::parameters[13].handle;

    static bool is_start = false;

    switch (uMsg) {
        case WM_INITDIALOG: {
            // if (((g_this->mode & 2) ^ 2)) {
            CheckDlgButton(hwndDlg, IDC_CHECK2, g_this->enabled);
            auto enabled_on_beat = g_this->get_bool(p_enabled_on_beat);
            CheckDlgButton(hwndDlg, IDC_CHECK3, enabled_on_beat);
            EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT1), enabled_on_beat);

            auto input_blend_mode = g_this->get_int(p_input_blend_mode.handle);
            init_select(p_input_blend_mode, input_blend_mode, hwndDlg, IDC_COMBO2);
            auto output_blend_mode = g_this->get_int(p_output_blend_mode.handle);
            init_select(p_output_blend_mode, output_blend_mode, hwndDlg, IDC_COMBO1);

            auto input_blend_adjustable =
                g_this->get_int(p_input_blend_adjustable.handle);
            init_ranged_slider(
                p_input_blend_adjustable, input_blend_adjustable, hwndDlg, IDC_INSLIDE);
            auto output_blend_adjustable =
                g_this->get_int(p_output_blend_adjustable.handle);
            init_ranged_slider(p_output_blend_adjustable,
                               output_blend_adjustable,
                               hwndDlg,
                               IDC_OUTSLIDE);

            show_hide_blendmode_dependent_ui(
                g_this, p_input_blend_mode, p_output_blend_mode, hwndDlg);

            char txt[16];
            for (int i = 0; i < p_input_blend_buffer.int_max; i++) {
                wsprintf(txt, "Buffer %d", i + 1);
                SendDlgItemMessage(hwndDlg, IDC_CBBUF1, CB_ADDSTRING, 0, (int)txt);
                SendDlgItemMessage(hwndDlg, IDC_CBBUF2, CB_ADDSTRING, 0, (int)txt);
            }
            auto input_blend_buffer = g_this->get_int(p_input_blend_buffer.handle);
            SendDlgItemMessage(
                hwndDlg, IDC_CBBUF1, CB_SETCURSEL, (WPARAM)input_blend_buffer, 0);
            auto output_blend_buffer = g_this->get_int(p_output_blend_buffer.handle);
            SendDlgItemMessage(
                hwndDlg, IDC_CBBUF1, CB_SETCURSEL, (WPARAM)output_blend_buffer, 0);

            CheckDlgButton(
                hwndDlg, IDC_INVERT1, g_this->get_bool(p_input_blend_buffer_invert));
            CheckDlgButton(
                hwndDlg, IDC_INVERT2, g_this->get_bool(p_output_blend_buffer_invert));
            CheckDlgButton(hwndDlg, IDC_CHECK1, g_this->get_bool(p_clear_every_frame));

            is_start = true;
            auto enabled_on_beat_frames = g_this->get_int(p_enabled_on_beat_frames);
            SetDlgItemInt(hwndDlg, IDC_EDIT1, enabled_on_beat_frames, /*signed*/ false);
            SetDlgItemText(hwndDlg, IDC_EDIT4, g_this->get_string(p_init));
            SetDlgItemText(hwndDlg, IDC_EDIT5, g_this->get_string(p_frame));
            is_start = false;

            CheckDlgButton(hwndDlg, IDC_CHECK4, g_this->get_bool(p_use_code));
            return 1;
        }
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDC_EDIT4:
                case IDC_EDIT5:
                    if (!is_start && HIWORD(wParam) == EN_CHANGE) {
                        int l = GetWindowTextLength((HWND)lParam) + 1;
                        char* buf = new char[l];
                        GetWindowText((HWND)lParam, buf, l);
                        switch (LOWORD(wParam)) {
                            case IDC_EDIT4: g_this->set_string(p_init, buf); break;
                            case IDC_EDIT5: g_this->set_string(p_frame, buf); break;
                            default: break;
                        }
                        delete[] buf;
                    }
                    break;
                case IDC_COMBO1:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        int sel =
                            SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0);
                        if (sel != CB_ERR) {
                            g_this->set_int(p_output_blend_mode.handle, sel);
                            show_hide_blendmode_dependent_ui(g_this,
                                                             p_input_blend_mode,
                                                             p_output_blend_mode,
                                                             hwndDlg);
                        }
                    }
                    break;
                case IDC_COMBO2:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        int sel =
                            SendDlgItemMessage(hwndDlg, IDC_COMBO2, CB_GETCURSEL, 0, 0);
                        if (sel != CB_ERR) {
                            g_this->set_int(p_input_blend_mode.handle, sel);
                            show_hide_blendmode_dependent_ui(g_this,
                                                             p_input_blend_mode,
                                                             p_output_blend_mode,
                                                             hwndDlg);
                        }
                    }
                    break;
                case IDC_CBBUF1:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        g_this->set_int(p_input_blend_buffer.handle,
                                        SendDlgItemMessage(
                                            hwndDlg, IDC_CBBUF1, CB_GETCURSEL, 0, 0));
                    }
                    break;
                case IDC_CBBUF2:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        g_this->set_int(p_output_blend_buffer.handle,
                                        SendDlgItemMessage(
                                            hwndDlg, IDC_CBBUF2, CB_GETCURSEL, 0, 0));
                    }
                    break;
                case IDC_CHECK1:
                    g_this->set_bool(p_clear_every_frame,
                                     IsDlgButtonChecked(hwndDlg, IDC_CHECK1));
                    break;
                case IDC_CHECK2:
                    g_this->set_enabled(IsDlgButtonChecked(hwndDlg, IDC_CHECK2));
                    break;
                case IDC_INVERT1:
                    g_this->set_bool(p_input_blend_buffer_invert,
                                     IsDlgButtonChecked(hwndDlg, IDC_INVERT1));
                    break;
                case IDC_INVERT2:
                    g_this->set_bool(p_output_blend_buffer_invert,
                                     IsDlgButtonChecked(hwndDlg, IDC_INVERT2));
                    break;
                case IDC_CHECK3: {
                    bool on_beat = IsDlgButtonChecked(hwndDlg, IDC_CHECK3);
                    g_this->set_bool(p_enabled_on_beat, on_beat);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT1), on_beat);
                    break;
                }
                case IDC_CHECK4:
                    g_this->set_bool(p_use_code,
                                     IsDlgButtonChecked(hwndDlg, IDC_CHECK4));
                    break;
                case IDC_EDIT1:
                    if (!is_start && HIWORD(wParam) == EN_CHANGE) {
                        int success = false;
                        auto on_beat_frames = GetDlgItemInt(
                            hwndDlg, IDC_EDIT1, &success, /*signed*/ false);
                        if (success) {
                            g_this->set_int(p_enabled_on_beat_frames, on_beat_frames);
                        }
                    }
                    break;
                case IDC_BUTTON2: {
                    compilerfunctionlist(hwndDlg,
                                         E_EffectList::info.get_name(),
                                         E_EffectList::info.get_help());
                } break;
            }
            break;
        }
        case WM_HSCROLL: {
            HWND control = (HWND)lParam;
            auto value = SendMessage(control, TBM_GETPOS, 0, 0);
            if (control == GetDlgItem(hwndDlg, IDC_INSLIDE)) {
                g_this->set_int(p_input_blend_adjustable.handle, value);
            } else if (control == GetDlgItem(hwndDlg, IDC_OUTSLIDE)) {
                g_this->set_int(p_output_blend_adjustable.handle, value);
            }
            return 1;
        }
    }
    return 0;
}
