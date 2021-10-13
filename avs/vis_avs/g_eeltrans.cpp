#include "c_eeltrans.h"

#include "c__defs.h"
#include "g__defs.h"
#include "g__lib.h"
#include "resource.h"

#include <windows.h>

#define SET_CHECKBOX(ID, PROP) \
    (CheckDlgButton(hwndDlg, (ID), g_EelTransThis->PROP ? BST_CHECKED : BST_UNCHECKED))
#define GET_CHECKBOX(PROP, ID) \
    (g_EelTransThis->PROP = IsDlgButtonChecked(hwndDlg, (ID)) == BST_CHECKED)

int win32_dlgproc_eeltrans(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    C_EelTrans* g_EelTransThis = (C_EelTrans*)g_current_render;
    switch (uMsg) {
        case WM_INITDIALOG:
            SetDlgItemText(hwndDlg, IDC_EELTRANS_LOGPATH, g_EelTransThis->logpath);
            SetDlgItemText(hwndDlg, IDC_EELTRANS_CODE, g_EelTransThis->code.c_str());
            SetDlgItemText(hwndDlg, IDC_EELTRANS_VERSION, EELTRANS_VERSION);

            SET_CHECKBOX(IDC_EELTRANS_LOG_ENABLED, log_enabled);
            SET_CHECKBOX(IDC_EELTRANS_ENABLED, translate_enabled);
            SET_CHECKBOX(IDC_EELTRANS_TRANSFIRST, translate_firstlevel);
            SET_CHECKBOX(IDC_EELTRANS_READCOMMENTCODES, read_comment_codes);
            if (g_EelTransThis->is_first_instance) {
                ShowWindow(GetDlgItem(hwndDlg, IDC_EELTRANS_NOTFIRST_MESSAGE), SW_HIDE);
            } else {
                ShowWindow(GetDlgItem(hwndDlg, IDC_EELTRANS_LOGPATH), SW_HIDE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_EELTRANS_ENABLED), false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_EELTRANS_LOG_ENABLED), false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_EELTRANS_TRANSFIRST), false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_EELTRANS_READCOMMENTCODES), false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_EELTRANS_VERSION_PREFIX), false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_EELTRANS_VERSION), false);
            }
            return 1;

        case WM_DESTROY:
            return 1;

        case WM_COMMAND:
            if (HIWORD(wParam) == EN_CHANGE) {
                int l = GetWindowTextLength((HWND)lParam);
                char* buf = new char[l + 1];
                GetWindowText((HWND)lParam, buf, l + 1);

                switch (LOWORD(wParam)) {
                    case IDC_EELTRANS_LOGPATH:
                        delete[] g_EelTransThis->logpath;
                        g_EelTransThis->logpath = buf;
                        break;
                    case IDC_EELTRANS_CODE:
                        g_EelTransThis->code = std::string(buf);
                        delete[] buf;
                        break;
                    default:
                        delete[] buf;
                        break;
                }
            }
            switch (LOWORD(wParam)) {
                case IDC_EELTRANS_LOG_ENABLED:
                    GET_CHECKBOX(log_enabled, IDC_EELTRANS_LOG_ENABLED);
                    break;
                case IDC_EELTRANS_ENABLED:
                    GET_CHECKBOX(translate_enabled, IDC_EELTRANS_ENABLED);
                    break;
                case IDC_EELTRANS_TRANSFIRST:
                    GET_CHECKBOX(translate_firstlevel, IDC_EELTRANS_TRANSFIRST);
                    break;
                case IDC_EELTRANS_READCOMMENTCODES:
                    GET_CHECKBOX(read_comment_codes, IDC_EELTRANS_READCOMMENTCODES);
                    break;
            }
            return 1;
    }
    return 0;
}
