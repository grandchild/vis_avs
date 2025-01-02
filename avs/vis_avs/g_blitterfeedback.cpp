#include "e_blitterfeedback.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_blitterfeedback(HWND hwndDlg,
                                  UINT uMsg,
                                  WPARAM wParam,
                                  LPARAM lParam) {
    auto g_this = (E_BlitterFeedback*)g_current_render;
    const Parameter& p_zoom = BlitterFeedback_Info::parameters[0];
    AVS_Parameter_Handle p_on_beat = BlitterFeedback_Info::parameters[1].handle;
    const Parameter& p_on_beat_zoom = BlitterFeedback_Info::parameters[2];
    AVS_Parameter_Handle p_blend_mode = BlitterFeedback_Info::parameters[3].handle;
    AVS_Parameter_Handle p_bilinear = BlitterFeedback_Info::parameters[4].handle;

    switch (uMsg) {
        case WM_INITDIALOG: {
            auto zoom = g_this->get_int(p_zoom.handle);
            init_ranged_slider(p_zoom, zoom, hwndDlg, IDC_SLIDER1);
            auto on_beat_zoom = g_this->get_int(p_on_beat_zoom.handle);
            init_ranged_slider(p_on_beat_zoom, on_beat_zoom, hwndDlg, IDC_SLIDER2);

            CheckDlgButton(hwndDlg,
                           IDC_BLEND,
                           g_this->get_int(p_blend_mode) == BLITTER_BLEND_5050);
            CheckDlgButton(hwndDlg, IDC_CHECK1, g_this->get_bool(p_on_beat));
            CheckDlgButton(hwndDlg, IDC_CHECK2, g_this->get_bool(p_bilinear));
            return 1;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_BLEND:
                    g_this->set_int(p_blend_mode,
                                    IsDlgButtonChecked(hwndDlg, IDC_BLEND)
                                        ? BLITTER_BLEND_5050
                                        : BLITTER_BLEND_REPLACE);
                    break;
                case IDC_CHECK1:
                    g_this->set_bool(p_on_beat,
                                     IsDlgButtonChecked(hwndDlg, IDC_CHECK1));
                    break;
                case IDC_CHECK2:
                    g_this->set_bool(p_bilinear,
                                     IsDlgButtonChecked(hwndDlg, IDC_CHECK2));
                    break;
            }
            return 0;
        case WM_HSCROLL: {
            HWND swnd = (HWND)lParam;
            auto t = (int)SendMessage(swnd, TBM_GETPOS, 0, 0);
            if (swnd == GetDlgItem(hwndDlg, IDC_SLIDER1)) {
                g_this->set_int(p_zoom.handle, t);
            }
            if (swnd == GetDlgItem(hwndDlg, IDC_SLIDER2)) {
                g_this->set_int(p_on_beat_zoom.handle, t);
            }
        }
    }
    return 0;
}
