#include "e_normalise.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_normalise(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    E_Normalise* g_this = (E_Normalise*)g_current_render;
    switch (uMsg) {
        case WM_INITDIALOG:
            CheckDlgButton(hwndDlg, IDC_NORMALISE_ENABLED, g_this->enabled);
            return 1;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_NORMALISE_ENABLED) {
                g_this->enabled =
                    IsDlgButtonChecked(hwndDlg, IDC_NORMALISE_ENABLED) == 1;
                return 0;
            }
    }
    return 0;
}
