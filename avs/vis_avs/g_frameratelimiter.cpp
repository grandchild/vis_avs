#include "c_frameratelimiter.h"

#include "c__defs.h"
#include "g__defs.h"
#include "g__lib.h"
#include "resource.h"

#include <commctrl.h>
#include <windows.h>

int win32_dlgproc_frameratelimiter(HWND hwndDlg,
                                   UINT uMsg,
                                   WPARAM wParam,
                                   LPARAM lParam) {
    C_FramerateLimiter* config_this = (C_FramerateLimiter*)g_current_render;
    char fps_slider_label[32];
    switch (uMsg) {
        case WM_INITDIALOG: {
            CheckDlgButton(hwndDlg, IDC_FPSLIMITER_ENABLED, config_this->enabled);
            SendDlgItemMessage(
                hwndDlg, IDC_FPSLIMITER_FPS_SLIDER, TBM_SETRANGE, 1, MAKELONG(1, 60));
            SendDlgItemMessage(hwndDlg,
                               IDC_FPSLIMITER_FPS_SLIDER,
                               TBM_SETPOS,
                               1,
                               config_this->framerate_limit);
            wsprintf(fps_slider_label, "%d FPS max.", config_this->framerate_limit);
            SetDlgItemText(hwndDlg, IDC_FPSLIMITER_FPS_LABEL, fps_slider_label);
            return 1;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) == IDC_FPSLIMITER_ENABLED) {
                config_this->enabled =
                    IsDlgButtonChecked(hwndDlg, IDC_FPSLIMITER_ENABLED);
            }
            return 1;
        }
        case WM_HSCROLL: {
            if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_FPSLIMITER_FPS_SLIDER)) {
                config_this->framerate_limit =
                    SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
                config_this->update_time_diff();
                wsprintf(fps_slider_label, "%d FPS max.", config_this->framerate_limit);
                SetDlgItemText(hwndDlg, IDC_FPSLIMITER_FPS_LABEL, fps_slider_label);
                return 1;
            }
        }
    }
    return 0;
}
