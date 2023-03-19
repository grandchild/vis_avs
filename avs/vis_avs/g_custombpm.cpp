#include "e_custombpm.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>
#include <cstdint>

int win32_dlgproc_custombpm(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    auto g_this = (E_CustomBPM*)g_current_render;
    AVS_Parameter_Handle p_mode = CustomBPM_Info::parameters[0].handle;
    const Parameter& p_fixed_bpm = CustomBPM_Info::parameters[1];
    const Parameter& p_skip = CustomBPM_Info::parameters[2];
    const Parameter& p_skip_first_beats = CustomBPM_Info::parameters[3];
    AVS_Parameter_Handle p_beat_count_in = CustomBPM_Info::parameters[4].handle;
    AVS_Parameter_Handle p_beat_count_out = CustomBPM_Info::parameters[5].handle;

    char txt[40];
    switch (uMsg) {
        case WM_INITDIALOG: {
            auto fixed_bpm = g_this->get_int(p_fixed_bpm.handle);
            init_ranged_slider(p_fixed_bpm, fixed_bpm, hwndDlg, IDC_ARBVAL);
            auto skip = g_this->get_int(p_skip.handle);
            init_ranged_slider(p_skip, skip, hwndDlg, IDC_SKIPVAL);
            auto skip_first_beats = g_this->get_int(p_skip_first_beats.handle);
            init_ranged_slider(
                p_skip_first_beats, skip_first_beats, hwndDlg, IDC_SKIPFIRST);

            CheckDlgButton(hwndDlg, IDC_CHECK1, g_this->enabled);
            auto mode = g_this->get_int(p_mode);
            CheckDlgButton(hwndDlg, IDC_ARBITRARY, mode == CUSTOM_BPM_FIXED);
            CheckDlgButton(hwndDlg, IDC_SKIP, mode == CUSTOM_BPM_SKIP);
            CheckDlgButton(hwndDlg, IDC_INVERT, mode == CUSTOM_BPM_INVERT);

            SendDlgItemMessage(hwndDlg, IDC_IN, TBM_SETTICFREQ, 1, 0);
            SendDlgItemMessage(hwndDlg, IDC_IN, TBM_SETRANGE, true, MAKELONG(0, 8));
            SendDlgItemMessage(hwndDlg, IDC_OUT, TBM_SETTICFREQ, 1, 0);
            SendDlgItemMessage(hwndDlg, IDC_OUT, TBM_SETRANGE, true, MAKELONG(0, 8));

            SetTimer(hwndDlg, 0, 50, nullptr);
            return 1;
        }
        case WM_TIMER: {
            auto beat_count_in = g_this->get_int(p_beat_count_in);
            auto beat_count_out = g_this->get_int(p_beat_count_out);
            // map 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22
            // to  0  1  2  3  4  5  6  7  8  7  6  5  4  3  2  1  0  1  2  3  4  5  6
            // etc.
            auto in16 = int32_t(beat_count_in % 16);
            int32_t in_mapped = in16 - ((in16 / 8) ? (in16 - 8) * 2 : 0);
            auto out16 = int32_t(beat_count_out % 16);
            int32_t out_mapped = out16 - ((out16 / 8) ? (out16 - 8) * 2 : 0);
            SendDlgItemMessage(hwndDlg, IDC_IN, TBM_SETPOS, true, in_mapped);
            SendDlgItemMessage(hwndDlg, IDC_OUT, TBM_SETPOS, true, out_mapped);
            return 0;
        }
        case WM_NOTIFY: {
            if (LOWORD(wParam) == IDC_ARBVAL) {
                g_this->set_int(
                    p_fixed_bpm.handle,
                    SendDlgItemMessage(hwndDlg, IDC_ARBVAL, TBM_GETPOS, 0, 0));
                auto fixed_bpm = g_this->get_int(p_fixed_bpm.handle);
                wsprintf(txt, "%d bpm", (int32_t)fixed_bpm);
                SetDlgItemText(hwndDlg, IDC_ARBTXT, txt);
            }
            if (LOWORD(wParam) == IDC_SKIPVAL) {
                g_this->set_int(
                    p_skip.handle,
                    SendDlgItemMessage(hwndDlg, IDC_SKIPVAL, TBM_GETPOS, 0, 0));
                auto skip = g_this->get_int(p_skip.handle);
                wsprintf(txt, "%d beat%s", (int32_t)skip, (skip != 1) ? "s" : "");
                SetDlgItemText(hwndDlg, IDC_SKIPTXT, txt);
            }
            if (LOWORD(wParam) == IDC_SKIPFIRST) {
                g_this->set_int(
                    p_skip_first_beats.handle,
                    SendDlgItemMessage(hwndDlg, IDC_SKIPFIRST, TBM_GETPOS, 0, 0));
                auto skip_first_beats = g_this->get_int(p_skip_first_beats.handle);
                wsprintf(txt,
                         "%d beat%s",
                         (int32_t)skip_first_beats,
                         (skip_first_beats != 1) ? "s" : "");
                SetDlgItemText(hwndDlg, IDC_SKIPFIRSTTXT, txt);
            }
            return 0;
        }
        case WM_COMMAND:
            if ((LOWORD(wParam) == IDC_CHECK1) || (LOWORD(wParam) == IDC_ARBITRARY)
                || (LOWORD(wParam) == IDC_SKIP) || (LOWORD(wParam) == IDC_INVERT)) {
                g_this->set_enabled(IsDlgButtonChecked(hwndDlg, IDC_CHECK1));
                g_this->set_int(
                    p_mode,
                    IsDlgButtonChecked(hwndDlg, IDC_ARBITRARY)
                        ? CUSTOM_BPM_FIXED
                        : (IsDlgButtonChecked(hwndDlg, IDC_SKIP) ? CUSTOM_BPM_SKIP
                                                                 : CUSTOM_BPM_INVERT));
            }
            return 0;
        case WM_DESTROY: KillTimer(hwndDlg, 0); return 0;
    }
    return 0;
}
