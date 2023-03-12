#include "e_colorreduction.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_colorreduction(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    E_ColorReduction* g_this = (E_ColorReduction*)g_current_render;
    const Parameter& p_levels = g_this->info.parameters[0];

    int64_t options_length;

    switch (uMsg) {
        case WM_HSCROLL: {
            {
                if (LOWORD(wParam) == TB_ENDTRACK) {
                    g_this->config.levels =
                        SendMessage(GetDlgItem(hwndDlg, IDC_LEVELS), TBM_GETPOS, 0, 0);
                }
                char buf[4];
                int a, b;
                a = 8 - g_this->config.levels;
                b = 0x100;
                while (a--) {
                    b >>= 1;
                }
                wsprintf(buf, "%d", b);
                SetDlgItemText(hwndDlg, IDC_LEVELTEXT, buf);
            }
            return 1;
        }
        case WM_INITDIALOG: {
            p_levels.get_options(&options_length);
            SendMessage(GetDlgItem(hwndDlg, IDC_LEVELS),
                        TBM_SETRANGE,
                        TRUE,
                        MAKELONG(1, options_length));
            int64_t levels = g_this->get_int(p_levels.handle);
            SendMessage(GetDlgItem(hwndDlg, IDC_LEVELS), TBM_SETPOS, TRUE, levels);
            SetFocus(GetDlgItem(hwndDlg, IDC_LEVELS));
            char buf[4];
            int a, b;
            a = 8 - levels;
            b = 0x100;
            while (a--) {
                b >>= 1;
            }
            wsprintf(buf, "%d", b);
            SetDlgItemText(hwndDlg, IDC_LEVELTEXT, buf);
            return 1;
        }

        case WM_DESTROY: KillTimer(hwndDlg, 1); return 1;
    }
    return 0;
}
