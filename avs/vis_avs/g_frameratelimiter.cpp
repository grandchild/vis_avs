#include "e_frameratelimiter.h"

#include "g__defs.h"
#include "g__lib.h"

#include "c__defs.h"
#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_frameratelimiter(HWND hwndDlg,
                                   UINT uMsg,
                                   WPARAM wParam,
                                   LPARAM lParam) {
    E_FramerateLimiter* g_this = (E_FramerateLimiter*)g_current_render;
    const Parameter& p_limit = g_this->info.parameters[0];

    char fps_slider_label[32];
    switch (uMsg) {
        case WM_INITDIALOG: {
            auto limit = g_this->get_int(p_limit.handle);
            init_ranged_slider(p_limit, limit, hwndDlg, IDC_FPSLIMITER_FPS_SLIDER);
            CheckDlgButton(hwndDlg, IDC_FPSLIMITER_ENABLED, g_this->enabled);
            wsprintf(fps_slider_label, "%d FPS max.", limit);
            SetDlgItemText(hwndDlg, IDC_FPSLIMITER_FPS_LABEL, fps_slider_label);
            return 1;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) == IDC_FPSLIMITER_ENABLED) {
                g_this->enabled = IsDlgButtonChecked(hwndDlg, IDC_FPSLIMITER_ENABLED);
            }
            return 1;
        }
        case WM_HSCROLL: {
            if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_FPSLIMITER_FPS_SLIDER)) {
                g_this->set_int(p_limit.handle,
                                SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
                auto limit = g_this->get_int(p_limit.handle);
                wsprintf(fps_slider_label, "%d FPS max.", limit);
                SetDlgItemText(hwndDlg, IDC_FPSLIMITER_FPS_LABEL, fps_slider_label);
                return 1;
            }
        }
    }
    return 0;
}
