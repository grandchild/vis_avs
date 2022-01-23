#include "e_fadeout.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_fade(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    E_Fadeout* g_this = (E_Fadeout*)g_current_render;
    Parameter fadelen_info = g_this->info.parameters[0];
    AVS_Parameter_Handle fadelen_handle = fadelen_info.handle;
    AVS_Parameter_Handle color_handle = g_this->info.parameters[1].handle;

    switch (uMsg) {
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* di = (DRAWITEMSTRUCT*)lParam;
            switch (di->CtlID) {
                case IDC_LC:
                    GR_DrawColoredButton(di, g_this->get_color(color_handle, {}));
                    break;
            }
            return 0;
        }
        case WM_INITDIALOG:
            SendDlgItemMessage(
                hwndDlg, IDC_SLIDER1, TBM_SETRANGEMIN, 0, fadelen_info.int_min);
            SendDlgItemMessage(
                hwndDlg, IDC_SLIDER1, TBM_SETRANGEMAX, 0, fadelen_info.int_max);
            SendDlgItemMessage(hwndDlg,
                               IDC_SLIDER1,
                               TBM_SETPOS,
                               1,
                               g_this->get_int(fadelen_handle, {}));

            return 1;

        case WM_HSCROLL: {
            HWND swnd = (HWND)lParam;
            int t = (int)SendMessage(swnd, TBM_GETPOS, 0, 0);
            if (swnd == GetDlgItem(hwndDlg, IDC_SLIDER1)) {
                g_this->set_int(fadelen_handle, t, {});
            }
        }
            return 0;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_LC:
                    int out_color;
                    GR_SelectColor(hwndDlg, &out_color);
                    g_this->set_color(color_handle, out_color, {});
                    InvalidateRect(GetDlgItem(hwndDlg, LOWORD(wParam)), NULL, FALSE);
                    return 0;
            }
            return 0;
    }
    return 0;
}
