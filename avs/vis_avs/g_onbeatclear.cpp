#include "e_onbeatclear.h"

#include "g__defs.h"
#include "g__lib.h"

#include "avs_editor.h"
#include "effect_common.h"
#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_onbeatclear(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto g_this = (E_OnBeatClear*)g_current_render;
    AVS_Parameter_Handle p_color = OnBeatClear_Info::parameters[0].handle;
    AVS_Parameter_Handle p_blend_mode = OnBeatClear_Info::parameters[1].handle;
    const Parameter& p_every_n_beats = OnBeatClear_Info::parameters[2];

    switch (uMsg) {
        case WM_INITDIALOG: {
            auto blend_mode = g_this->get_int(p_blend_mode);
            CheckDlgButton(hwndDlg, IDC_BLEND, blend_mode == BLEND_SIMPLE_5050);
            auto every_n_beats = g_this->get_int(p_every_n_beats.handle);
            init_ranged_slider(p_every_n_beats, every_n_beats, hwndDlg, IDC_SLIDER1);
            return 1;
        }
        case WM_DRAWITEM: {
            auto di = (DRAWITEMSTRUCT*)lParam;
            switch (di->CtlID) {
                case IDC_BUTTON1:
                    GR_DrawColoredButton(di, g_this->get_color(p_color));
                    break;
            }
            return 0;
        }
        case WM_HSCROLL: {
            HWND swnd = (HWND)lParam;
            if (swnd == GetDlgItem(hwndDlg, IDC_SLIDER1)) {
                g_this->set_int(p_every_n_beats.handle,
                                SendMessage(swnd, TBM_GETPOS, 0, 0));
            }
        }
            return 0;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_BUTTON1) {
                auto color = (int32_t)g_this->get_color(p_color);
                GR_SelectColor(hwndDlg, &color);
                InvalidateRect(GetDlgItem(hwndDlg, LOWORD(wParam)), nullptr, false);
                g_this->set_color(p_color, color);
            }
            if (LOWORD(wParam) == IDC_BLEND) {
                g_this->set_int(p_blend_mode,
                                IsDlgButtonChecked(hwndDlg, IDC_BLEND)
                                    ? BLEND_SIMPLE_5050
                                    : BLEND_SIMPLE_REPLACE);
            }
    }
    return 0;
}
