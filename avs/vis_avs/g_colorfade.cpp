#include "e_colorfade.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_colorfade(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto g_this = (E_Colorfade*)g_current_render;
    const Parameter& p_fader_max = Colorfade_Info::parameters[0];
    const Parameter& p_fader_2nd = Colorfade_Info::parameters[1];
    const Parameter& p_fader_3rd_gray = Colorfade_Info::parameters[2];
    AVS_Parameter_Handle p_on_beat = Colorfade_Info::parameters[3].handle;
    AVS_Parameter_Handle p_on_beat_random = Colorfade_Info::parameters[4].handle;
    const Parameter& p_on_beat_max = Colorfade_Info::parameters[5];
    const Parameter& p_on_beat_2nd = Colorfade_Info::parameters[6];
    const Parameter& p_on_beat_3rd_gray = Colorfade_Info::parameters[7];

    switch (uMsg) {
        case WM_INITDIALOG: {
            SendDlgItemMessage(
                hwndDlg, IDC_SLIDER1, TBM_SETRANGEMIN, 0, p_fader_2nd.int_min);
            SendDlgItemMessage(
                hwndDlg, IDC_SLIDER1, TBM_SETRANGEMAX, 0, p_fader_2nd.int_max);
            auto fader_2nd = g_this->get_int(p_fader_2nd.handle);
            SendDlgItemMessage(hwndDlg, IDC_SLIDER1, TBM_SETPOS, 1, fader_2nd);
            SendDlgItemMessage(
                hwndDlg, IDC_SLIDER2, TBM_SETRANGEMIN, 0, p_fader_max.int_min);
            SendDlgItemMessage(
                hwndDlg, IDC_SLIDER2, TBM_SETRANGEMAX, 0, p_fader_max.int_max);
            auto fader_max = g_this->get_int(p_fader_max.handle);
            SendDlgItemMessage(hwndDlg, IDC_SLIDER2, TBM_SETPOS, 1, fader_max);
            SendDlgItemMessage(
                hwndDlg, IDC_SLIDER3, TBM_SETRANGEMIN, 0, p_fader_3rd_gray.int_min);
            SendDlgItemMessage(
                hwndDlg, IDC_SLIDER3, TBM_SETRANGEMAX, 0, p_fader_3rd_gray.int_max);
            auto fader_3rd_gray = g_this->get_int(p_fader_3rd_gray.handle);
            SendDlgItemMessage(hwndDlg, IDC_SLIDER3, TBM_SETPOS, 1, fader_3rd_gray);

            SendDlgItemMessage(
                hwndDlg, IDC_SLIDER4, TBM_SETRANGEMIN, 0, p_on_beat_2nd.int_min);
            SendDlgItemMessage(
                hwndDlg, IDC_SLIDER4, TBM_SETRANGEMAX, 0, p_on_beat_2nd.int_max);
            auto on_beat_2nd = g_this->get_int(p_on_beat_2nd.handle);
            SendDlgItemMessage(hwndDlg, IDC_SLIDER4, TBM_SETPOS, 1, on_beat_2nd);
            SendDlgItemMessage(
                hwndDlg, IDC_SLIDER5, TBM_SETRANGEMIN, 0, p_on_beat_max.int_min);
            SendDlgItemMessage(
                hwndDlg, IDC_SLIDER5, TBM_SETRANGEMAX, 0, p_on_beat_max.int_max);
            auto on_beat_max = g_this->get_int(p_on_beat_max.handle);
            SendDlgItemMessage(hwndDlg, IDC_SLIDER5, TBM_SETPOS, 1, on_beat_max);
            SendDlgItemMessage(
                hwndDlg, IDC_SLIDER6, TBM_SETRANGEMIN, 0, p_on_beat_3rd_gray.int_min);
            SendDlgItemMessage(
                hwndDlg, IDC_SLIDER6, TBM_SETRANGEMAX, 0, p_on_beat_3rd_gray.int_max);
            auto on_beat_3rd_gray = g_this->get_int(p_on_beat_3rd_gray.handle);
            SendDlgItemMessage(hwndDlg, IDC_SLIDER6, TBM_SETPOS, 1, on_beat_3rd_gray);

            CheckDlgButton(hwndDlg, IDC_CHECK1, g_this->enabled);
            CheckDlgButton(hwndDlg, IDC_CHECK2, g_this->get_bool(p_on_beat_random));
            CheckDlgButton(hwndDlg, IDC_CHECK3, g_this->get_bool(p_on_beat));
            return 1;
        }
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                default:
                case IDC_CHECK1:
                    g_this->set_enabled(IsDlgButtonChecked(hwndDlg, IDC_CHECK1));
                    break;
                case IDC_CHECK2:
                    g_this->set_bool(p_on_beat_random,
                                     IsDlgButtonChecked(hwndDlg, IDC_CHECK2));
                    break;
                case IDC_CHECK3:
                    g_this->set_bool(p_on_beat,
                                     IsDlgButtonChecked(hwndDlg, IDC_CHECK3));
                    break;
            }
            return 0;
        }
        case WM_HSCROLL: {
            HWND swnd = (HWND)lParam;
            int t = (int)SendMessage(swnd, TBM_GETPOS, 0, 0);
            if (swnd == GetDlgItem(hwndDlg, IDC_SLIDER1)) {
                g_this->set_int(p_fader_2nd.handle, t);
            }
            if (swnd == GetDlgItem(hwndDlg, IDC_SLIDER2)) {
                g_this->set_int(p_fader_max.handle, t);
            }
            if (swnd == GetDlgItem(hwndDlg, IDC_SLIDER3)) {
                g_this->set_int(p_fader_3rd_gray.handle, t);
            }
            if (swnd == GetDlgItem(hwndDlg, IDC_SLIDER4)) {
                g_this->set_int(p_on_beat_2nd.handle, t);
            }
            if (swnd == GetDlgItem(hwndDlg, IDC_SLIDER5)) {
                g_this->set_int(p_on_beat_max.handle, t);
            }
            if (swnd == GetDlgItem(hwndDlg, IDC_SLIDER6)) {
                g_this->set_int(p_on_beat_3rd_gray.handle, t);
            }
            return 0;
        }
        default: break;
    }
    return 0;
}
