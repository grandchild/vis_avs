#include "e_triangle.h"

#include "g__defs.h"
#include "g__lib.h"

#include "c__defs.h"
#include "resource.h"

#include <windows.h>

int win32_dlgproc_triangle(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    E_Triangle* g_this = (E_Triangle*)g_current_render;
    AVS_Parameter_Handle h_init = g_this->info.parameters[0].handle;
    AVS_Parameter_Handle h_frame = g_this->info.parameters[1].handle;
    AVS_Parameter_Handle h_beat = g_this->info.parameters[2].handle;
    AVS_Parameter_Handle h_point = g_this->info.parameters[3].handle;

    switch (uMsg) {
        case WM_COMMAND: {
            int wNotifyCode = HIWORD(wParam);
            HWND h = (HWND)lParam;

            if (wNotifyCode == EN_CHANGE) {
                int l = GetWindowTextLength(h) + 1;
                char* buf = new char[l];
                GetWindowText(h, buf, l);

                switch (LOWORD(wParam)) {
                    case IDC_TRIANGLE_INIT: g_this->set_string(h_init, buf); break;
                    case IDC_TRIANGLE_FRAME: g_this->set_string(h_frame, buf); break;
                    case IDC_TRIANGLE_BEAT: g_this->set_string(h_beat, buf); break;
                    case IDC_TRIANGLE_POINT: g_this->set_string(h_point, buf); break;
                    default: break;
                }
                delete[] buf;
            }
            return 1;
        }

        case WM_INITDIALOG: {
            SetDlgItemText(hwndDlg, IDC_TRIANGLE_INIT, g_this->get_string(h_init));
            SetDlgItemText(hwndDlg, IDC_TRIANGLE_FRAME, g_this->get_string(h_frame));
            SetDlgItemText(hwndDlg, IDC_TRIANGLE_BEAT, g_this->get_string(h_beat));
            SetDlgItemText(hwndDlg, IDC_TRIANGLE_POINT, g_this->get_string(h_point));
            return 1;
        }

        case WM_DESTROY: return 1;
    }
    return 0;
}
