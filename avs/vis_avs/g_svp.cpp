#include "e_svp.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_svp(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    E_SVP* g_this = (E_SVP*)g_current_render;
    const Parameter& p_library = g_this->info.parameters[0];

    int64_t options_length;
    const char* const* options;

    switch (uMsg) {
        case WM_INITDIALOG: {
            options = p_library.get_options(&options_length);
            for (unsigned int i = 0; i < options_length; i++) {
                SendDlgItemMessage(
                    hwndDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)options[i]);
            }
            int64_t filename_sel = g_this->get_int(p_library.handle);
            if (filename_sel >= 0) {
                SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_SETCURSEL, filename_sel, 0);
            }
            return 1;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_COMBO1: {
                    int a = SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0);
                    if (a != CB_ERR) {
                        g_this->set_int(p_library.handle, a);
                    } else {
                        g_this->set_int(p_library.handle, -1);
                    }
                    return 0;
                }
            }
            return 0;
    }
    return 0;
}
