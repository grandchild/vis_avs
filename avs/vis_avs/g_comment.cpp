#include "e_comment.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_comment(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    E_Comment* g_this = (E_Comment*)g_current_render;
    AVS_Parameter_Handle h_comment = g_this->info.parameters[0].handle;

    switch (uMsg) {
        case WM_INITDIALOG:
            SetDlgItemText(hwndDlg, IDC_EDIT1, g_this->get_string(h_comment));
            return 1;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_EDIT1 && HIWORD(wParam) == EN_CHANGE) {
                HWND h = (HWND)lParam;
                int l = GetWindowTextLength(h) + 1;
                char* buf = new char[l];
                GetWindowText(h, buf, l);
                g_this->set_string(h_comment, buf);
            }
            return 0;
    }
    return 0;
}
