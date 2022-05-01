#include "e_bump.h"

#include "g__defs.h"
#include "g__lib.h"

#include "c__defs.h"
#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_bump(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    E_Bump* g_this = (E_Bump*)g_current_render;
    AVS_Parameter_Handle p_init = g_this->info.parameters[0].handle;
    AVS_Parameter_Handle p_frame = g_this->info.parameters[1].handle;
    AVS_Parameter_Handle p_beat = g_this->info.parameters[2].handle;
    const Parameter& p_depth = g_this->info.parameters[3];
    AVS_Parameter_Handle p_on_beat = g_this->info.parameters[4].handle;
    const Parameter& p_on_beat_duration = g_this->info.parameters[5];
    const Parameter& p_on_beat_depth = g_this->info.parameters[6];
    AVS_Parameter_Handle p_blend_mode = g_this->info.parameters[7].handle;
    AVS_Parameter_Handle p_show_light_pos = g_this->info.parameters[8].handle;
    AVS_Parameter_Handle p_invert_depth = g_this->info.parameters[9].handle;
    const Parameter& p_depth_buffer = g_this->info.parameters[10];

    auto element = LOWORD(wParam);
    switch (uMsg) {
        case WM_INITDIALOG: {
            SetDlgItemText(hwndDlg, IDC_CODE1, g_this->get_string(p_frame));
            SetDlgItemText(hwndDlg, IDC_CODE2, g_this->get_string(p_beat));
            SetDlgItemText(hwndDlg, IDC_CODE3, g_this->get_string(p_init));
            SendDlgItemMessage(hwndDlg, IDC_DEPTH, TBM_SETTICFREQ, 10, 0);
            auto depth_range = MAKELONG(p_depth.int_min, p_depth.int_max);
            SendDlgItemMessage(hwndDlg, IDC_DEPTH, TBM_SETRANGE, TRUE, depth_range);
            SendDlgItemMessage(
                hwndDlg, IDC_DEPTH, TBM_SETPOS, TRUE, g_this->get_int(p_depth.handle));
            SendDlgItemMessage(hwndDlg, IDC_DEPTH2, TBM_SETTICFREQ, 10, 0);
            auto on_beat_depth_range =
                MAKELONG(p_on_beat_depth.int_min, p_on_beat_depth.int_max);
            SendDlgItemMessage(
                hwndDlg, IDC_DEPTH2, TBM_SETRANGE, TRUE, on_beat_depth_range);
            SendDlgItemMessage(hwndDlg,
                               IDC_DEPTH2,
                               TBM_SETPOS,
                               TRUE,
                               g_this->get_int(p_on_beat_depth.handle));
            SendDlgItemMessage(hwndDlg, IDC_BEATDUR, TBM_SETTICFREQ, 10, 0);
            auto on_beat_duration_range =
                MAKELONG(p_on_beat_duration.int_min, p_on_beat_duration.int_max);
            SendDlgItemMessage(
                hwndDlg, IDC_BEATDUR, TBM_SETRANGE, TRUE, on_beat_duration_range);
            SendDlgItemMessage(hwndDlg,
                               IDC_BEATDUR,
                               TBM_SETPOS,
                               TRUE,
                               g_this->get_int(p_on_beat_duration.handle));
            CheckDlgButton(hwndDlg, IDC_CHECK1, g_this->enabled);
            CheckDlgButton(hwndDlg, IDC_INVERTDEPTH, g_this->get_bool(p_invert_depth));
            CheckDlgButton(hwndDlg, IDC_ONBEAT, g_this->get_bool(p_on_beat));
            CheckDlgButton(hwndDlg, IDC_DOT, g_this->get_bool(p_show_light_pos));
            auto blend_mode = g_this->get_int(p_blend_mode);
            CheckDlgButton(hwndDlg, IDC_REPLACE, blend_mode == BLEND_SIMPLE_REPLACE);
            CheckDlgButton(hwndDlg, IDC_ADDITIVE, blend_mode == BLEND_SIMPLE_ADDITIVE);
            CheckDlgButton(hwndDlg, IDC_5050, blend_mode == BLEND_SIMPLE_5050);
            SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_ADDSTRING, 0, (int)"Current");
            char buffer_label[16];
            for (int i = 1; i <= p_depth_buffer.int_max; i++) {
                wsprintf(buffer_label, "Buffer %d", i);
                SendDlgItemMessage(
                    hwndDlg, IDC_COMBO1, CB_ADDSTRING, 0, (int)buffer_label);
            }
            SendDlgItemMessage(hwndDlg,
                               IDC_COMBO1,
                               CB_SETCURSEL,
                               (WPARAM)g_this->get_int(p_depth_buffer.handle),
                               0);
            return 1;
        }
        case WM_NOTIFY: {
            if (element == IDC_DEPTH) {
                g_this->set_int(
                    p_depth.handle,
                    SendDlgItemMessage(hwndDlg, IDC_DEPTH, TBM_GETPOS, 0, 0));
            }
            if (element == IDC_DEPTH2) {
                g_this->set_int(
                    p_on_beat_depth.handle,
                    SendDlgItemMessage(hwndDlg, IDC_DEPTH2, TBM_GETPOS, 0, 0));
            }
            if (element == IDC_BEATDUR) {
                g_this->set_int(
                    p_on_beat_duration.handle,
                    SendDlgItemMessage(hwndDlg, IDC_BEATDUR, TBM_GETPOS, 0, 0));
            }
            return 0;
        }
        case WM_COMMAND: {
            if (element == IDC_HELPBTN) {
                compilerfunctionlist(
                    hwndDlg, g_this->info.get_name(), g_this->info.get_help());
                return 0;
            }
            if (element == IDC_CHECK1 || element == IDC_ONBEAT
                || element == IDC_ADDITIVE || element == IDC_REPLACE
                || element == IDC_DOT || element == IDC_INVERTDEPTH
                || element == IDC_5050) {
                g_this->set_enabled(IsDlgButtonChecked(hwndDlg, IDC_CHECK1));
                g_this->set_bool(p_on_beat, IsDlgButtonChecked(hwndDlg, IDC_ONBEAT));
                g_this->set_bool(p_show_light_pos,
                                 IsDlgButtonChecked(hwndDlg, IDC_DOT));
                g_this->set_bool(p_invert_depth,
                                 IsDlgButtonChecked(hwndDlg, IDC_INVERTDEPTH));
                if (IsDlgButtonChecked(hwndDlg, IDC_REPLACE)) {
                    g_this->set_int(p_blend_mode, BLEND_SIMPLE_REPLACE);
                } else if (IsDlgButtonChecked(hwndDlg, IDC_ADDITIVE)) {
                    g_this->set_int(p_blend_mode, BLEND_SIMPLE_ADDITIVE);
                } else if (IsDlgButtonChecked(hwndDlg, IDC_5050)) {
                    g_this->set_int(p_blend_mode, BLEND_SIMPLE_5050);
                }
            }
            int length;
            char* buf;
            switch (LOWORD(wParam)) {
                case IDC_CODE3:
                    length = GetWindowTextLength((HWND)lParam) + 1;
                    buf = new char[length];
                    GetDlgItemText(hwndDlg, IDC_CODE3, buf, length);
                    g_this->set_string(p_init, buf);
                    delete[] buf;
                    break;
                case IDC_CODE1:
                    length = GetWindowTextLength((HWND)lParam) + 1;
                    buf = new char[length];
                    GetDlgItemText(hwndDlg, IDC_CODE1, buf, length);
                    g_this->set_string(p_frame, buf);
                    delete[] buf;
                    break;
                case IDC_CODE2:
                    length = GetWindowTextLength((HWND)lParam) + 1;
                    buf = new char[length];
                    GetDlgItemText(hwndDlg, IDC_CODE2, buf, length);
                    g_this->set_string(p_beat, buf);
                    delete[] buf;
                    break;
            }
            if (HIWORD(wParam) == CBN_SELCHANGE && element == IDC_COMBO1)
                g_this->set_int(
                    p_depth_buffer.handle,
                    SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0));
            return 0;
        }
    }
    return 0;
}
