#include "e_mosaic.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_mosaic(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    auto g_this = (E_Mosaic*)g_current_render;
    const Parameter& p_size = Mosaic_Info::parameters[0];
    const Parameter& p_on_beat_size = Mosaic_Info::parameters[1];
    AVS_Parameter_Handle p_on_beat_size_change = Mosaic_Info::parameters[2].handle;
    const Parameter& p_on_beat_duration = Mosaic_Info::parameters[3];
    AVS_Parameter_Handle p_blend_mode = Mosaic_Info::parameters[4].handle;

    switch (uMsg) {
        case WM_INITDIALOG: {
            auto size = g_this->get_int(p_size.handle);
            init_ranged_slider(p_size, size, hwndDlg, IDC_QUALITY, 10);
            auto on_beat_size = g_this->get_int(p_on_beat_size.handle);
            init_ranged_slider(p_on_beat_size, on_beat_size, hwndDlg, IDC_QUALITY2, 10);
            auto on_beat_duration = g_this->get_int(p_on_beat_duration.handle);
            init_ranged_slider(
                p_on_beat_duration, on_beat_duration, hwndDlg, IDC_BEATDUR, 10);

            CheckDlgButton(hwndDlg, IDC_CHECK1, g_this->enabled);
            CheckDlgButton(
                hwndDlg, IDC_ONBEAT, g_this->get_bool(p_on_beat_size_change));
            auto blend_mode = g_this->get_int(p_blend_mode);
            CheckDlgButton(hwndDlg, IDC_ADDITIVE, blend_mode == BLEND_SIMPLE_ADDITIVE);
            CheckDlgButton(hwndDlg, IDC_5050, blend_mode == BLEND_SIMPLE_5050);
            CheckDlgButton(hwndDlg, IDC_REPLACE, blend_mode == BLEND_SIMPLE_REPLACE);
            return 1;
        }
        case WM_NOTIFY: {
            if (LOWORD(wParam) == IDC_QUALITY) {
                g_this->set_int(
                    p_size.handle,
                    SendDlgItemMessage(hwndDlg, IDC_QUALITY, TBM_GETPOS, 0, 0));
            }
            if (LOWORD(wParam) == IDC_QUALITY2) {
                g_this->set_int(
                    p_on_beat_size.handle,
                    SendDlgItemMessage(hwndDlg, IDC_QUALITY2, TBM_GETPOS, 0, 0));
            }
            if (LOWORD(wParam) == IDC_BEATDUR) {
                g_this->set_int(
                    p_on_beat_duration.handle,
                    SendDlgItemMessage(hwndDlg, IDC_BEATDUR, TBM_GETPOS, 0, 0));
            }
            return 0;
        }
        case WM_COMMAND: {
            if ((LOWORD(wParam) == IDC_CHECK1) || (LOWORD(wParam) == IDC_ONBEAT)
                || (LOWORD(wParam) == IDC_ADDITIVE) || (LOWORD(wParam) == IDC_REPLACE)
                || (LOWORD(wParam) == IDC_5050)) {
                g_this->set_enabled(IsDlgButtonChecked(hwndDlg, IDC_CHECK1));
                g_this->set_bool(p_on_beat_size_change,
                                 IsDlgButtonChecked(hwndDlg, IDC_ONBEAT));

                bool blend_additive = IsDlgButtonChecked(hwndDlg, IDC_ADDITIVE);
                bool blend_5050 = IsDlgButtonChecked(hwndDlg, IDC_5050);
                g_this->set_int(p_blend_mode,
                                blend_additive ? BLEND_SIMPLE_ADDITIVE
                                               : (blend_5050 ? BLEND_SIMPLE_5050
                                                             : BLEND_SIMPLE_REPLACE));
            }
        }
    }
    return 0;
}
