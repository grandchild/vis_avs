#include "e_multifilter.h"

#include "g__defs.h"
#include "g__lib.h"

#include "c__defs.h"
#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_multifilter(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto g_this = (E_MultiFilter*)g_current_render;
    const Parameter& p_effect = MultiFilter_Info::parameters[0];
    AVS_Parameter_Handle p_toggle_on_beat = MultiFilter_Info::parameters[1].handle;

    switch (uMsg) {
        case WM_INITDIALOG: {
            CheckDlgButton(hwndDlg, IDC_MULTIFILTER_ENABLED, g_this->enabled);
            auto effect = g_this->get_int(p_effect.handle);
            init_select(p_effect, effect, hwndDlg, IDC_MULTIFILTER_EFFECT);
            CheckDlgButton(hwndDlg,
                           IDC_MULTIFILTER_TOGGLEONBEAT,
                           g_this->get_bool(p_toggle_on_beat));
            return 1;
        }
        case WM_COMMAND: {
            int command = HIWORD(wParam);
            if (command == CBN_SELCHANGE && LOWORD(wParam) == IDC_MULTIFILTER_EFFECT) {
                g_this->set_int(p_effect.handle,
                                SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0));
            } else if (command == BN_CLICKED) {
                switch (LOWORD(wParam)) {
                    case IDC_MULTIFILTER_ENABLED:
                        g_this->set_enabled(
                            IsDlgButtonChecked(hwndDlg, IDC_MULTIFILTER_ENABLED));
                        break;
                    case IDC_MULTIFILTER_TOGGLEONBEAT:
                        g_this->set_bool(
                            p_toggle_on_beat,
                            IsDlgButtonChecked(hwndDlg, IDC_MULTIFILTER_TOGGLEONBEAT));
                        break;
                }
            }
            return 1;
        }
    }
    return 0;
}
