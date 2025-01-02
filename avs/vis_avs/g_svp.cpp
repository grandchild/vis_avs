#include "e_svp.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_svp(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto g_this = (E_SVP*)g_current_render;
    const Parameter& p_library = SVP_Info::parameters[0];

    switch (uMsg) {
        case WM_INITDIALOG: {
            auto library = g_this->get_string(p_library.handle);
            init_resource(p_library, library, hwndDlg, IDC_COMBO1, MAX_PATH);
            return 1;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_COMBO1: {
                    int sel = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
                    if (sel < 0) {
                        break;
                    }
                    size_t filename_length =
                        SendMessage((HWND)lParam, CB_GETLBTEXTLEN, sel, 0) + 1;
                    char* buf = (char*)calloc(filename_length, sizeof(char));
                    SendMessage((HWND)lParam, CB_GETLBTEXT, sel, (LPARAM)buf);
                    g_this->set_string(p_library.handle, buf);
                    free(buf);
                    return 0;
                }
            }
            return 0;
    }
    return 0;
}
