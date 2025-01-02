#include "e_timescope.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_timescope(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto g_this = (E_Timescope*)g_current_render;
    AVS_Parameter_Handle p_color = Timescope_Info::parameters[0].handle;
    AVS_Parameter_Handle p_blend_mode = Timescope_Info::parameters[1].handle;
    AVS_Parameter_Handle p_audio_channel = Timescope_Info::parameters[2].handle;
    const Parameter& p_bands = Timescope_Info::parameters[3];

    switch (uMsg) {
        case WM_INITDIALOG: {
            CheckDlgButton(hwndDlg, IDC_CHECK1, g_this->enabled);

            auto blend_mode = g_this->get_int(p_blend_mode);
            CheckDlgButton(
                hwndDlg, IDC_ADDITIVE, blend_mode == TIMESCOPE_BLEND_ADDITIVE);
            CheckDlgButton(hwndDlg, IDC_5050, blend_mode == TIMESCOPE_BLEND_5050);
            CheckDlgButton(
                hwndDlg, IDC_DEFAULTBLEND, blend_mode == TIMESCOPE_BLEND_DEFAULT);
            CheckDlgButton(hwndDlg, IDC_REPLACE, blend_mode == TIMESCOPE_BLEND_REPLACE);

            auto bands = g_this->get_int(p_bands.handle);
            init_ranged_slider(p_bands, bands, hwndDlg, IDC_BANDS, 32);
            char txt[64];
            wsprintf(txt, "Draw %d bands", bands);
            SetDlgItemText(hwndDlg, IDC_BANDTXT, txt);

            auto audio_channel = g_this->get_int(p_audio_channel);
            CheckDlgButton(hwndDlg, IDC_LEFT, audio_channel == AUDIO_LEFT);
            CheckDlgButton(hwndDlg, IDC_RIGHT, audio_channel == AUDIO_RIGHT);
            CheckDlgButton(hwndDlg, IDC_CENTER, audio_channel == AUDIO_CENTER);
            return 1;
        }
        case WM_DRAWITEM: {
            auto di = (DRAWITEMSTRUCT*)lParam;
            switch (di->CtlID) {
                case IDC_DEFCOL:
                    GR_DrawColoredButton(di, g_this->get_color(p_color));
                    break;
            }
            return 0;
        }
        case WM_COMMAND:
            if ((LOWORD(wParam) == IDC_CHECK1) || (LOWORD(wParam) == IDC_ADDITIVE)
                || (LOWORD(wParam) == IDC_REPLACE) || (LOWORD(wParam) == IDC_5050)
                || (LOWORD(wParam) == IDC_DEFAULTBLEND)) {
                g_this->set_enabled(IsDlgButtonChecked(hwndDlg, IDC_CHECK1));
                if (IsDlgButtonChecked(hwndDlg, IDC_REPLACE)) {
                    g_this->set_int(p_blend_mode, TIMESCOPE_BLEND_REPLACE);
                } else if (IsDlgButtonChecked(hwndDlg, IDC_ADDITIVE)) {
                    g_this->set_int(p_blend_mode, TIMESCOPE_BLEND_ADDITIVE);
                } else if (IsDlgButtonChecked(hwndDlg, IDC_5050)) {
                    g_this->set_int(p_blend_mode, TIMESCOPE_BLEND_5050);
                } else if (IsDlgButtonChecked(hwndDlg, IDC_DEFAULTBLEND)) {
                    g_this->set_int(p_blend_mode, TIMESCOPE_BLEND_DEFAULT);
                }
            }
            if (LOWORD(wParam) == IDC_DEFCOL) {
                auto color = (uint32_t)g_this->get_color(p_color);
                GR_SelectColor(hwndDlg, (int32_t*)&color);
                g_this->set_color(p_color, color);
                InvalidateRect(GetDlgItem(hwndDlg, LOWORD(wParam)), nullptr, false);
                return 0;
            }
            if (LOWORD(wParam) == IDC_LEFT || LOWORD(wParam) == IDC_RIGHT
                || LOWORD(wParam) == IDC_CENTER) {
                if (IsDlgButtonChecked(hwndDlg, IDC_LEFT)) {
                    g_this->set_int(p_audio_channel, AUDIO_LEFT);
                } else if (IsDlgButtonChecked(hwndDlg, IDC_RIGHT)) {
                    g_this->set_int(p_audio_channel, AUDIO_RIGHT);
                } else {
                    g_this->set_int(p_audio_channel, AUDIO_CENTER);
                }
            }
            break;
        case WM_NOTIFY:
            if (LOWORD(wParam) == IDC_BANDS) {
                auto bands = SendDlgItemMessage(hwndDlg, IDC_BANDS, TBM_GETPOS, 0, 0);
                g_this->set_int(p_bands.handle, bands);
                char txt[64];
                wsprintf(txt, "Draw %d bands", bands);
                SetDlgItemText(hwndDlg, IDC_BANDTXT, txt);
            }
            return 0;
    }
    return 0;
}
