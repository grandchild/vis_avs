#include "e_avi.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

static void EnableWindows(HWND hwndDlg, E_AVI* g_this) {
    bool enable = g_this->config.blend_mode == AVI_BLEND_FIFTY_FIFTY_ONBEAT_ADD;
    EnableWindow(GetDlgItem(hwndDlg, IDC_PERSIST), enable);
    EnableWindow(GetDlgItem(hwndDlg, IDC_PERSIST_TITLE), enable);
}

int win32_dlgproc_avi(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    E_AVI* g_this = (E_AVI*)g_current_render;
    const Parameter& p_filename = g_this->info.parameters[0];
    const Parameter& p_on_beat_persist = g_this->info.parameters[2];
    const Parameter& p_playback_speed = g_this->info.parameters[3];
    const Parameter& p_resampling = g_this->info.parameters[4];

    int64_t options_length;
    const char* const* options;

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
            switch (g_this->config.blend_mode) {
                default:
                case AVI_BLEND_REPLACE:
                    CheckDlgButton(hwndDlg, IDC_REPLACE, BST_CHECKED);
                    break;
                case AVI_BLEND_ADD:
                    CheckDlgButton(hwndDlg, IDC_ADDITIVE, BST_CHECKED);
                    break;
                case AVI_BLEND_FIFTY_FIFTY:
                    CheckDlgButton(hwndDlg, IDC_5050, BST_CHECKED);
                    break;
                case AVI_BLEND_FIFTY_FIFTY_ONBEAT_ADD:
                    CheckDlgButton(hwndDlg, IDC_ADAPT, BST_CHECKED);
                    break;
            }
            EnableWindows(hwndDlg, g_this);
            options = p_filename.get_options(&options_length);
            for (unsigned int i = 0; i < options_length; i++) {
                SendDlgItemMessage(
                    hwndDlg, OBJ_COMBO, CB_ADDSTRING, 0, (LPARAM)options[i]);
            }
            int64_t filename_sel = g_this->get_int(p_filename.handle);
            if (filename_sel >= 0) {
                SendDlgItemMessage(hwndDlg, OBJ_COMBO, CB_SETCURSEL, filename_sel, 0);
            }
            options = p_resampling.get_options(&options_length);
            for (unsigned int i = 0; i < options_length; i++) {
                SendDlgItemMessage(
                    hwndDlg, IDC_AVI_RESAMPLE, CB_ADDSTRING, 0, (LPARAM)options[i]);
            }
            int64_t resampling_sel = g_this->get_int(p_resampling.handle);
            SendDlgItemMessage(
                hwndDlg, IDC_AVI_RESAMPLE, CB_SETCURSEL, resampling_sel, 0);

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
                g_this->enabled = IsDlgButtonChecked(hwndDlg, IDC_CHECK1) ? 1 : 0;
                if (IsDlgButtonChecked(hwndDlg, IDC_ADDITIVE)) {
                    g_this->config.blend_mode = AVI_BLEND_ADD;
                } else if (IsDlgButtonChecked(hwndDlg, IDC_5050)) {
                    g_this->config.blend_mode = AVI_BLEND_FIFTY_FIFTY;
                } else if (IsDlgButtonChecked(hwndDlg, IDC_ADAPT)) {
                    g_this->config.blend_mode = AVI_BLEND_FIFTY_FIFTY_ONBEAT_ADD;
                } else {
                    g_this->config.blend_mode = AVI_BLEND_REPLACE;
                }
                EnableWindows(hwndDlg, g_this);
            }
            // handle clicks to combo box
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                auto widget = LOWORD(wParam);
                int64_t sel = SendDlgItemMessage(hwndDlg, widget, CB_GETCURSEL, 0, 0);
                switch (widget) {
                    case OBJ_COMBO: {
                        int64_t old_sel = g_this->get_int(p_filename.handle);
                        g_this->set_int(p_filename.handle, sel);
                        if (old_sel != sel) {
                            g_this->load_file();
                        }
                        break;
                    }
                    case IDC_AVI_RESAMPLE:
                        g_this->set_int(p_resampling.handle, sel);
                        break;
                }
            }
            return 0;
    }
    return 0;
}
