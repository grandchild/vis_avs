#include "e_video.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

static void EnableWindows(HWND hwndDlg, E_Video* g_this) {
    bool enable = g_this->config.blend_mode == VIDEO_BLEND_FIFTY_FIFTY_ONBEAT_ADD;
    EnableWindow(GetDlgItem(hwndDlg, IDC_PERSIST), enable);
    EnableWindow(GetDlgItem(hwndDlg, IDC_PERSIST_TITLE), enable);
}

int win32_dlgproc_video(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    E_Video* g_this = (E_Video*)g_current_render;
    const Parameter& p_filename = g_this->info.parameters[0];
    const Parameter& p_blend_mode = g_this->info.parameters[1];
    const Parameter& p_on_beat_persist = g_this->info.parameters[2];
    const Parameter& p_playback_speed = g_this->info.parameters[3];
    const Parameter& p_resampling = g_this->info.parameters[4];

    switch (uMsg) {
        case WM_INITDIALOG: {
            auto persist_range =
                MAKELONG(p_on_beat_persist.int_min, p_on_beat_persist.int_max);
            SendDlgItemMessage(hwndDlg, IDC_PERSIST, TBM_SETRANGE, TRUE, persist_range);
            SendDlgItemMessage(hwndDlg,
                               IDC_PERSIST,
                               TBM_SETPOS,
                               TRUE,
                               g_this->get_int(p_on_beat_persist.handle));
            auto speed_range =
                MAKELONG(p_playback_speed.int_min, p_playback_speed.int_max);
            SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_SETTICFREQ, 50, 0);
            SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_SETRANGE, TRUE, speed_range);
            SendDlgItemMessage(hwndDlg,
                               IDC_SPEED,
                               TBM_SETPOS,
                               TRUE,
                               g_this->get_int(p_playback_speed.handle));
            if (g_this->enabled) {
                CheckDlgButton(hwndDlg, IDC_CHECK1, BST_CHECKED);
            }

            auto blend_mode = g_this->get_int(p_blend_mode.handle);
            static constexpr size_t num_controls = 4;
            uint32_t controls[num_controls] = {
                IDC_REPLACE, IDC_ADDITIVE, IDC_5050, IDC_ADAPT};
            init_select_radio(
                p_blend_mode, blend_mode, hwndDlg, controls, num_controls);
            EnableWindows(hwndDlg, g_this);

            auto filename = g_this->get_string(p_filename.handle);
            init_resource(p_filename, filename, hwndDlg, OBJ_COMBO, MAX_PATH);
            auto resampling = g_this->get_int(p_resampling.handle);
            init_select(p_resampling, resampling, hwndDlg, IDC_VIDEO_RESAMPLE);

            return 1;
        }
        case WM_NOTIFY:
            if (LOWORD(wParam) == IDC_PERSIST) {
                g_this->config.on_beat_persist =
                    SendDlgItemMessage(hwndDlg, IDC_PERSIST, TBM_GETPOS, 0, 0);
            }
            if (LOWORD(wParam) == IDC_SPEED) {
                g_this->config.playback_speed =
                    SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_GETPOS, 0, 0);
            }
            break;
        case WM_COMMAND:
            if ((LOWORD(wParam) == IDC_CHECK1) || (LOWORD(wParam) == IDC_ADDITIVE)
                || (LOWORD(wParam) == IDC_REPLACE) || (LOWORD(wParam) == IDC_ADAPT)
                || (LOWORD(wParam) == IDC_5050)) {
                g_this->set_enabled(IsDlgButtonChecked(hwndDlg, IDC_CHECK1));
                if (IsDlgButtonChecked(hwndDlg, IDC_ADDITIVE)) {
                    g_this->config.blend_mode = VIDEO_BLEND_ADD;
                } else if (IsDlgButtonChecked(hwndDlg, IDC_5050)) {
                    g_this->config.blend_mode = VIDEO_BLEND_FIFTY_FIFTY;
                } else if (IsDlgButtonChecked(hwndDlg, IDC_ADAPT)) {
                    g_this->config.blend_mode = VIDEO_BLEND_FIFTY_FIFTY_ONBEAT_ADD;
                } else {
                    g_this->config.blend_mode = VIDEO_BLEND_REPLACE;
                }
                EnableWindows(hwndDlg, g_this);
            }
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                auto widget = LOWORD(wParam);
                int sel = SendDlgItemMessage(hwndDlg, widget, CB_GETCURSEL, 0, 0);
                if (sel < 0) {
                    return 0;
                }
                switch (widget) {
                    case OBJ_COMBO: {
                        size_t filename_length =
                            SendDlgItemMessage(hwndDlg, widget, CB_GETLBTEXTLEN, sel, 0)
                            + 1;
                        char* buf = (char*)calloc(filename_length, sizeof(char));
                        SendDlgItemMessage(
                            hwndDlg, widget, CB_GETLBTEXT, sel, (LPARAM)buf);
                        g_this->set_string(p_filename.handle, buf);
                        free(buf);
                        break;
                    }
                    case IDC_VIDEO_RESAMPLE:
                        g_this->set_int(p_resampling.handle, sel);
                        break;
                }
            }
            return 0;
    }
    return 0;
}
