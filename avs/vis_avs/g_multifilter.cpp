#include "c_multifilter.h"

#include "c__defs.h"
#include "g__defs.h"
#include "g__lib.h"
#include "resource.h"

#include <commctrl.h>
#include <windows.h>

int win32_dlgproc_multifilter(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    C_MultiFilter* config_this = (C_MultiFilter*)g_current_render;
    unsigned int i;

    switch (uMsg) {
        case WM_INITDIALOG: {
            if (config_this->config.enabled) {
                CheckDlgButton(hwndDlg, IDC_MULTIFILTER_ENABLED, BST_CHECKED);
            }
            for (i = 0; i < MULTIFILTER_NUM_EFFECTS; ++i) {
                SendDlgItemMessage(hwndDlg,
                                   IDC_MULTIFILTER_EFFECT,
                                   CB_ADDSTRING,
                                   0,
                                   (LPARAM)config_this->effects[i]);
            }
            SendDlgItemMessage(hwndDlg,
                               IDC_MULTIFILTER_EFFECT,
                               CB_SETCURSEL,
                               config_this->config.effect,
                               0);
            if (config_this->config.on_beat) {
                CheckDlgButton(hwndDlg, IDC_MULTIFILTER_ONBEAT, BST_CHECKED);
            }
            return 1;
        }
        case WM_COMMAND: {
            int command = HIWORD(wParam);
            if (command == CBN_SELCHANGE && LOWORD(wParam) == IDC_MULTIFILTER_EFFECT) {
                config_this->config.effect =
                    (MultiFilterEffect)SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
            } else if (command == BN_CLICKED) {
                switch (LOWORD(wParam)) {
                    case IDC_MULTIFILTER_ENABLED:
                        config_this->config.enabled =
                            IsDlgButtonChecked(hwndDlg, IDC_MULTIFILTER_ENABLED);
                        break;
                    case IDC_MULTIFILTER_ONBEAT:
                        config_this->config.on_beat =
                            IsDlgButtonChecked(hwndDlg, IDC_MULTIFILTER_ONBEAT);
                        break;
                }
            }
            return 1;
        }
    }
    return 0;
}
