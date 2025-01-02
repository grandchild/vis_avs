#include "e_picture.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

static void enable_controls(HWND hwndDlg, bool on_beat, bool stretch) {
    EnableWindow(GetDlgItem(hwndDlg, IDC_PERSIST), on_beat);
    EnableWindow(GetDlgItem(hwndDlg, IDC_PERSIST_TITLE), on_beat);
    EnableWindow(GetDlgItem(hwndDlg, IDC_PERSIST_TEXT1), on_beat);
    EnableWindow(GetDlgItem(hwndDlg, IDC_PERSIST_TEXT2), on_beat);
    EnableWindow(GetDlgItem(hwndDlg, IDC_X_RATIO), !stretch);
    EnableWindow(GetDlgItem(hwndDlg, IDC_Y_RATIO), !stretch);
}

int win32_dlgproc_picture(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    auto g_this = (E_Picture*)g_current_render;
    const Parameter& p_image = Picture_Info::parameters[0];
    AVS_Parameter_Handle p_blend_mode = Picture_Info::parameters[1].handle;
    AVS_Parameter_Handle p_on_beat_additive = Picture_Info::parameters[2].handle;
    const Parameter& p_on_beat_duration = Picture_Info::parameters[3];
    AVS_Parameter_Handle p_fit = Picture_Info::parameters[4].handle;
    AVS_Parameter_Handle p_error_msg = Picture_Info::parameters[5].handle;

    switch (uMsg) {
        case WM_INITDIALOG: {
            CheckDlgButton(hwndDlg, IDC_ENABLED, g_this->enabled);

            auto image = g_this->get_string(p_image.handle);
            init_resource(p_image, image, hwndDlg, OBJ_COMBO, MAX_PATH);
            SetDlgItemText(hwndDlg, IDC_PICTURE_ERROR, g_this->get_string(p_error_msg));

            auto blend_mode = g_this->get_int(p_blend_mode);
            auto on_beat = g_this->get_bool(p_on_beat_additive);
            CheckDlgButton(
                hwndDlg, IDC_REPLACE, blend_mode == BLEND_SIMPLE_REPLACE && !on_beat);
            CheckDlgButton(
                hwndDlg, IDC_ADDITIVE, blend_mode == BLEND_SIMPLE_ADDITIVE && !on_beat);
            CheckDlgButton(
                hwndDlg, IDC_5050, blend_mode == BLEND_SIMPLE_5050 && !on_beat);
            CheckDlgButton(hwndDlg, IDC_ADAPT, on_beat);
            auto on_beat_duration = g_this->get_int(p_on_beat_duration.handle);
            init_ranged_slider(
                p_on_beat_duration, on_beat_duration, hwndDlg, IDC_PERSIST);

            auto fit = g_this->get_int(p_fit);
            CheckDlgButton(hwndDlg, IDC_RATIO, fit != PICTURE_FIT_STRETCH);
            CheckDlgButton(hwndDlg,
                           IDC_X_RATIO,
                           fit == PICTURE_FIT_WIDTH || fit == PICTURE_FIT_STRETCH);
            CheckDlgButton(hwndDlg, IDC_Y_RATIO, fit == PICTURE_FIT_HEIGHT);
            enable_controls(hwndDlg, on_beat, fit == PICTURE_FIT_STRETCH);
            return 1;
        }
        case WM_NOTIFY:
            if (LOWORD(wParam) == IDC_PERSIST) {
                g_this->set_int(
                    p_on_beat_duration.handle,
                    SendDlgItemMessage(hwndDlg, IDC_PERSIST, TBM_GETPOS, 0, 0));
            }
            return 0;
        case WM_COMMAND:
            if ((LOWORD(wParam) == IDC_ENABLED) || (LOWORD(wParam) == IDC_ADDITIVE)
                || (LOWORD(wParam) == IDC_REPLACE) || (LOWORD(wParam) == IDC_ADAPT)
                || (LOWORD(wParam) == IDC_5050)) {
                g_this->set_enabled(IsDlgButtonChecked(hwndDlg, IDC_ENABLED));
                bool blend_additive = IsDlgButtonChecked(hwndDlg, IDC_ADDITIVE);
                bool blend_5050 = IsDlgButtonChecked(hwndDlg, IDC_5050);
                bool on_beat = IsDlgButtonChecked(hwndDlg, IDC_ADAPT);
                g_this->set_int(p_blend_mode,
                                blend_additive
                                    ? BLEND_SIMPLE_ADDITIVE
                                    : (blend_5050 || on_beat ? BLEND_SIMPLE_5050
                                                             : BLEND_SIMPLE_REPLACE));
                g_this->set_bool(p_on_beat_additive, on_beat);
                enable_controls(
                    hwndDlg, on_beat, g_this->get_int(p_fit) == PICTURE_FIT_STRETCH);
            }
            if (LOWORD(wParam) == IDC_RATIO || LOWORD(wParam) == IDC_X_RATIO
                || LOWORD(wParam) == IDC_Y_RATIO) {
                bool fit_stretch = !IsDlgButtonChecked(hwndDlg, IDC_RATIO);
                bool fit_height = IsDlgButtonChecked(hwndDlg, IDC_Y_RATIO);
                g_this->set_int(p_fit,
                                fit_stretch ? PICTURE_FIT_STRETCH
                                            : (fit_height ? PICTURE_FIT_HEIGHT
                                                          : PICTURE_FIT_WIDTH));

                enable_controls(
                    hwndDlg, g_this->get_bool(p_on_beat_additive), fit_stretch);
            }
            if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == OBJ_COMBO) {
                int sel = SendDlgItemMessage(hwndDlg, OBJ_COMBO, CB_GETCURSEL, 0, 0);
                if (sel < 0) {
                    break;
                }
                size_t filename_length =
                    SendDlgItemMessage(hwndDlg, OBJ_COMBO, CB_GETLBTEXTLEN, sel, 0) + 1;
                char* buf = (char*)calloc(filename_length, sizeof(char));
                SendDlgItemMessage(hwndDlg, OBJ_COMBO, CB_GETLBTEXT, sel, (LPARAM)buf);
                g_this->set_string(p_image.handle, buf);
                SetDlgItemText(
                    hwndDlg, IDC_PICTURE_ERROR, g_this->get_string(p_error_msg));
                free(buf);
            }
            return 0;
        case WM_HSCROLL: return 0;
    }
    return 0;
}
