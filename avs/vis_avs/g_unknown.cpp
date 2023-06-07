#include "e_unknown.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_unknown(HWND hwndDlg, UINT uMsg, WPARAM, LPARAM) {
    auto g_this = (E_Unknown*)g_current_render;
    AVS_Parameter_Handle p_id = Unknown_Info::parameters[0].handle;
    AVS_Parameter_Handle p_config = Unknown_Info::parameters[1].handle;

    switch (uMsg) {
        case WM_INITDIALOG: {
            char s[512] = "";
            wsprintf(s, "Effect ID: %s\r\n", g_this->get_string(p_id));
            const char* data = g_this->get_string(p_config);
            size_t data_len = strnlen(data, MAX_CODE_LEN + 1);
            if (data_len == MAX_CODE_LEN + 1) {
                wsprintf(s + strlen(s), "Config size: >%d\r\n", MAX_CODE_LEN);

            } else {
                wsprintf(s + strlen(s), "Config size: %d\r\n", data_len);
            }
            SetDlgItemText(hwndDlg, IDC_EDIT1, s);
            return 1;
        }
    }
    return 0;
}
