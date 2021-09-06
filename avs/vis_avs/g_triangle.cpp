#include <windows.h>

#include "c__defs.h"
#include "c_triangle.h"
#include "g__defs.h"
#include "g__lib.h"
#include "resource.h"

int win32_dlgproc_triangle(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    C_Triangle* config_this = (C_Triangle*)g_current_render;
    switch (uMsg) {
        case WM_COMMAND: {
            int wNotifyCode = HIWORD(wParam);
            HWND h = (HWND)lParam;

            if (wNotifyCode == EN_CHANGE) {
                int l = GetWindowTextLength(h) + 1;
                char* buf = new char[l];
                GetWindowText(h, buf, l);

                switch (LOWORD(wParam)) {
                    case IDC_TRIANGLE_INIT:
                        config_this->code.init.set(buf, l);
                        break;
                    case IDC_TRIANGLE_FRAME:
                        config_this->code.frame.set(buf, l);
                        break;
                    case IDC_TRIANGLE_BEAT:
                        config_this->code.beat.set(buf, l);
                        break;
                    case IDC_TRIANGLE_POINT:
                        config_this->code.point.set(buf, l);
                        break;
                    default:
                        break;
                }
                delete buf;
            }
            return 1;
        }

        case WM_INITDIALOG: {
            SetDlgItemText(hwndDlg, IDC_TRIANGLE_INIT, config_this->code.init.string);
            SetDlgItemText(hwndDlg, IDC_TRIANGLE_FRAME, config_this->code.frame.string);
            SetDlgItemText(hwndDlg, IDC_TRIANGLE_BEAT, config_this->code.beat.string);
            SetDlgItemText(hwndDlg, IDC_TRIANGLE_POINT, config_this->code.point.string);
            return 1;
        }

        case WM_DESTROY:
            return 1;
    }
    return 0;
}
