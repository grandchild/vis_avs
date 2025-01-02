#include "e_fadeout.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_fade(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    E_Fadeout* g_this = (E_Fadeout*)g_current_render;
    const Parameter& p_fadelen = g_this->info.parameters[0];
    const AVS_Parameter_Handle p_color = g_this->info.parameters[1].handle;

    switch (uMsg) {
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* di = (DRAWITEMSTRUCT*)lParam;
            switch (di->CtlID) {
                case IDC_LC:
                    GR_DrawColoredButton(di, g_this->get_color(p_color));
                    break;
            }
            return 0;
        }
        case WM_INITDIALOG: {
            auto fadelen = g_this->get_int(p_fadelen.handle);
            init_ranged_slider(p_fadelen, fadelen, hwndDlg, IDC_SLIDER1);
            return 1;
        }
        case WM_HSCROLL: {
            HWND swnd = (HWND)lParam;
            int t = (int)SendMessage(swnd, TBM_GETPOS, 0, 0);
            if (swnd == GetDlgItem(hwndDlg, IDC_SLIDER1)) {
                g_this->set_int(p_fadelen.handle, t);
            }
            return 0;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_LC:
                    int out_color;
                    GR_SelectColor(hwndDlg, &out_color);
                    g_this->set_color(p_color, out_color);
                    InvalidateRect(GetDlgItem(hwndDlg, LOWORD(wParam)), NULL, FALSE);
                    return 0;
            }
            return 0;
    }
    return 0;
}
