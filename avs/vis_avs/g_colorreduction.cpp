#include "c_colorreduction.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_colorreduction(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    C_THISCLASS* g_ConfigThis = (C_THISCLASS*)g_current_render;
    switch (uMsg) {
        case WM_HSCROLL: {
            if (LOWORD(wParam) == TB_ENDTRACK)
                g_ConfigThis->config.levels =
                    SendMessage(GetDlgItem(hwndDlg, IDC_LEVELS), TBM_GETPOS, 0, 0);
            {
                char buf[4];
                int a, b;
                a = 8 - g_ConfigThis->config.levels;
                b = 0x100;
                while (a--) b >>= 1;
                wsprintf(buf, "%d", b);
                SetDlgItemText(hwndDlg, IDC_LEVELTEXT, buf);
            }
        }
            return 1;

        case WM_INITDIALOG:
            SendMessage(
                GetDlgItem(hwndDlg, IDC_LEVELS), TBM_SETRANGE, TRUE, MAKELONG(1, 8));
            SendMessage(GetDlgItem(hwndDlg, IDC_LEVELS),
                        TBM_SETPOS,
                        TRUE,
                        g_ConfigThis->config.levels);
            SetFocus(GetDlgItem(hwndDlg, IDC_LEVELS));
            {
                char buf[4];
                int a, b;
                a = 8 - g_ConfigThis->config.levels;
                b = 0x100;
                while (a--) b >>= 1;
                wsprintf(buf, "%d", b);
                SetDlgItemText(hwndDlg, IDC_LEVELTEXT, buf);
            }
            return 1;

        case WM_DESTROY:
            KillTimer(hwndDlg, 1);
            return 1;
    }
    return 0;
}
