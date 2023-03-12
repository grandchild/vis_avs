#include "c_unkn.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_unknown(HWND hwndDlg, UINT uMsg, WPARAM, LPARAM) {
    C_UnknClass* g_this = (C_UnknClass*)g_current_render;
    switch (uMsg) {
        case WM_INITDIALOG: {
            char s[512] = "";
            if (g_this->idString[0]) {
                wsprintf(s, "APE: %s\r\n", g_this->idString);
            } else {
                wsprintf(s, "Built-in ID: %d\r\n", g_this->id);
            }
            wsprintf(s + strlen(s), "Config size: %d\r\n", g_this->configdata_len);
            SetDlgItemText(hwndDlg, IDC_EDIT1, s);
        }
            return 1;
    }
    return 0;
}
