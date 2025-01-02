#include "e_root.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include "../platform.h"

#include <windows.h>

int win32_dlgproc_root(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    auto* g_this = (E_Root*)g_current_render;
    AVS_Parameter_Handle p_clear = Root_Info::parameters[0].handle;

    switch (uMsg) {
        case WM_INITDIALOG: {
            if (g_this->get_bool(p_clear)) {
                CheckDlgButton(hwndDlg, IDC_CHECK1, BST_CHECKED);
            }
            return 1;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) == IDC_CHECK1) {
                g_this->set_bool(p_clear, IsDlgButtonChecked(hwndDlg, IDC_CHECK1));
            }
            return 0;
        }
    }
    return 0;
}
