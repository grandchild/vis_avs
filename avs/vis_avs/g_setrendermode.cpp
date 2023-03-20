#include "e_setrendermode.h"

#include "g__defs.h"
#include "g__lib.h"

#include "effect_common.h"
#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_setrendermode(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    auto g_this = (E_SetRenderMode*)g_current_render;
    const Parameter& p_blend = SetRenderMode_Info::parameters[0];
    const Parameter& p_adjustable_blend = SetRenderMode_Info::parameters[1];
    const Parameter& p_line_size = SetRenderMode_Info::parameters[2];

    switch (uMsg) {
        case WM_INITDIALOG: {
            CheckDlgButton(hwndDlg, IDC_CHECK1, g_this->enabled);

            auto adjustable_blend = g_this->get_int(p_adjustable_blend.handle);
            init_ranged_slider(
                p_adjustable_blend, adjustable_blend, hwndDlg, IDC_ALPHASLIDE);

            auto line_size = g_this->get_int(p_line_size.handle);
            SetDlgItemInt(hwndDlg, IDC_EDIT1, line_size, false);

            auto blend = g_this->get_int(p_blend.handle);
            init_select(p_blend, blend, hwndDlg, IDC_COMBO1);
            EnableWindow(GetDlgItem(hwndDlg, IDC_OUTSLIDE), blend == BLEND_ADJUSTABLE);
            return 1;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_EDIT1: {
                    BOOL success;
                    uint32_t line_size =
                        GetDlgItemInt(hwndDlg, IDC_EDIT1, &success, /*signed*/ false);
                    if (success) {
                        g_this->set_int(p_line_size.handle, line_size);
                    }
                    break;
                }
                case IDC_CHECK1:
                    g_this->set_enabled(IsDlgButtonChecked(hwndDlg, IDC_CHECK1));
                    break;
                case IDC_COMBO1:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        int blend =
                            SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0);
                        if (blend != CB_ERR) {
                            g_this->set_int(p_blend.handle, blend);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_OUTSLIDE),
                                         blend == BLEND_ADJUSTABLE);
                        }
                    }
                    break;
            }
            return 0;
        case WM_NOTIFY:
            if (LOWORD(wParam) == IDC_ALPHASLIDE) {
                g_this->set_int(
                    p_adjustable_blend.handle,
                    SendDlgItemMessage(hwndDlg, IDC_ALPHASLIDE, TBM_GETPOS, 0, 0));
            }
            break;
    }
    return 0;
}
