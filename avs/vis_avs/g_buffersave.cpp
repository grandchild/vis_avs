#include "e_buffersave.h"

#include "g__defs.h"
#include "g__lib.h"

#include "c__defs.h"
#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_buffersave(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    auto g_this = (E_BufferSave*)g_current_render;
    AVS_Parameter_Handle p_action = BufferSave_Info::parameters[0].handle;
    const Parameter& p_buffer = BufferSave_Info::parameters[1];
    AVS_Parameter_Handle p_blend_mode = BufferSave_Info::parameters[2].handle;
    const Parameter& p_adjustable_blend = BufferSave_Info::parameters[3];
    AVS_Parameter_Handle p_clear_buffer = BufferSave_Info::parameters[4].handle;
    AVS_Parameter_Handle p_nudge_parity = BufferSave_Info::parameters[5].handle;

    switch (uMsg) {
        case WM_INITDIALOG: {
            for (int x = p_buffer.int_min; x <= p_buffer.int_max; x++) {
                char s[32];
                wsprintf(s, "Buffer %d", x);
                SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)s);
            }
            SendDlgItemMessage(hwndDlg,
                               IDC_COMBO1,
                               CB_SETCURSEL,
                               g_this->get_int(p_buffer.handle) - p_buffer.int_min,
                               0);
            auto action = g_this->get_int(p_action);
            CheckDlgButton(hwndDlg, IDC_SAVEFB, action == BUFFER_ACTION_SAVE);
            CheckDlgButton(hwndDlg, IDC_RESTFB, action == BUFFER_ACTION_RESTORE);
            CheckDlgButton(hwndDlg, IDC_RADIO1, action == BUFFER_ACTION_SAVE_RESTORE);
            CheckDlgButton(hwndDlg, IDC_RADIO2, action == BUFFER_ACTION_RESTORE_SAVE);
            auto adjust_blend = g_this->get_int(p_adjustable_blend.handle);
            init_ranged_slider(
                p_adjustable_blend, adjust_blend, hwndDlg, IDC_BLENDSLIDE);
            auto blend = g_this->get_int(p_blend_mode);
            CheckDlgButton(hwndDlg, IDC_RSTACK_BLEND1, blend == BUFFER_BLEND_REPLACE);
            CheckDlgButton(hwndDlg, IDC_RSTACK_BLEND2, blend == BUFFER_BLEND_5050);
            CheckDlgButton(hwndDlg, IDC_RSTACK_BLEND3, blend == BUFFER_BLEND_ADDITIVE);
            CheckDlgButton(
                hwndDlg, IDC_RSTACK_BLEND4, blend == BUFFER_BLEND_EVERY_OTHER_PIXEL);
            CheckDlgButton(hwndDlg, IDC_RSTACK_BLEND5, blend == BUFFER_BLEND_SUB1);
            CheckDlgButton(
                hwndDlg, IDC_RSTACK_BLEND6, blend == BUFFER_BLEND_EVERY_OTHER_LINE);
            CheckDlgButton(hwndDlg, IDC_RSTACK_BLEND7, blend == BUFFER_BLEND_XOR);
            CheckDlgButton(hwndDlg, IDC_RSTACK_BLEND8, blend == BUFFER_BLEND_MAXIMUM);
            CheckDlgButton(hwndDlg, IDC_RSTACK_BLEND9, blend == BUFFER_BLEND_MINIMUM);
            CheckDlgButton(hwndDlg, IDC_RSTACK_BLEND10, blend == BUFFER_BLEND_SUB2);
            CheckDlgButton(hwndDlg, IDC_RSTACK_BLEND11, blend == BUFFER_BLEND_MULTIPLY);
            CheckDlgButton(
                hwndDlg, IDC_RSTACK_BLEND12, blend == BUFFER_BLEND_ADJUSTABLE);
            return 1;
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_BUTTON2) {
                g_this->run_action(p_nudge_parity);
            }
            if (LOWORD(wParam) == IDC_BUTTON1) {
                g_this->run_action(p_clear_buffer);
            }
            if (LOWORD(wParam) == IDC_SAVEFB || LOWORD(wParam) == IDC_RESTFB
                || LOWORD(wParam) == IDC_RADIO1 || LOWORD(wParam) == IDC_RADIO2) {
                if (IsDlgButtonChecked(hwndDlg, IDC_SAVEFB)) {
                    g_this->set_int(p_action, BUFFER_ACTION_SAVE);
                } else if (IsDlgButtonChecked(hwndDlg, IDC_RESTFB)) {
                    g_this->set_int(p_action, BUFFER_ACTION_RESTORE);
                } else if (IsDlgButtonChecked(hwndDlg, IDC_RADIO1)) {
                    g_this->set_int(p_action, BUFFER_ACTION_SAVE_RESTORE);
                } else if (IsDlgButtonChecked(hwndDlg, IDC_RADIO2)) {
                    g_this->set_int(p_action, BUFFER_ACTION_RESTORE_SAVE);
                }
            }
            if (IsDlgButtonChecked(hwndDlg, LOWORD(wParam))) {
                if (LOWORD(wParam) == IDC_RSTACK_BLEND1) {
                    g_this->set_int(p_blend_mode, BUFFER_BLEND_REPLACE);
                }
                if (LOWORD(wParam) == IDC_RSTACK_BLEND2) {
                    g_this->set_int(p_blend_mode, BUFFER_BLEND_5050);
                }
                if (LOWORD(wParam) == IDC_RSTACK_BLEND3) {
                    g_this->set_int(p_blend_mode, BUFFER_BLEND_ADDITIVE);
                }
                if (LOWORD(wParam) == IDC_RSTACK_BLEND4) {
                    g_this->set_int(p_blend_mode, BUFFER_BLEND_EVERY_OTHER_PIXEL);
                }
                if (LOWORD(wParam) == IDC_RSTACK_BLEND5) {
                    g_this->set_int(p_blend_mode, BUFFER_BLEND_SUB1);
                }
                if (LOWORD(wParam) == IDC_RSTACK_BLEND6) {
                    g_this->set_int(p_blend_mode, BUFFER_BLEND_EVERY_OTHER_LINE);
                }
                if (LOWORD(wParam) == IDC_RSTACK_BLEND7) {
                    g_this->set_int(p_blend_mode, BUFFER_BLEND_XOR);
                }
                if (LOWORD(wParam) == IDC_RSTACK_BLEND8) {
                    g_this->set_int(p_blend_mode, BUFFER_BLEND_MAXIMUM);
                }
                if (LOWORD(wParam) == IDC_RSTACK_BLEND9) {
                    g_this->set_int(p_blend_mode, BUFFER_BLEND_MINIMUM);
                }
                if (LOWORD(wParam) == IDC_RSTACK_BLEND10) {
                    g_this->set_int(p_blend_mode, BUFFER_BLEND_SUB2);
                }
                if (LOWORD(wParam) == IDC_RSTACK_BLEND11) {
                    g_this->set_int(p_blend_mode, BUFFER_BLEND_MULTIPLY);
                }
                if (LOWORD(wParam) == IDC_RSTACK_BLEND12) {
                    g_this->set_int(p_blend_mode, BUFFER_BLEND_ADJUSTABLE);
                }
            }
            if (LOWORD(wParam) == IDC_COMBO1 && HIWORD(wParam) == CBN_SELCHANGE) {
                int i = SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0);
                if (i != CB_ERR) {
                    g_this->set_int(p_buffer.handle, i + p_buffer.int_min);
                }
            }
            return 0;
        case WM_NOTIFY:
            if (LOWORD(wParam) == IDC_BLENDSLIDE) {
                g_this->set_int(
                    p_adjustable_blend.handle,
                    SendDlgItemMessage(hwndDlg, IDC_BLENDSLIDE, TBM_GETPOS, 0, 0));
            }
            return 0;
    }
    return 0;
}
