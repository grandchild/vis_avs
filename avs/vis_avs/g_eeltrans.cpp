#include "e_eeltrans.h"

#include "g__defs.h"
#include "g__lib.h"

#include "c__defs.h"
#include "resource.h"

#include <windows.h>

#define EELTRANS_VERSION "1.25.00"

#define SET_CONFIG(PROP, ID) \
    (g_this->config.PROP = IsDlgButtonChecked(hwndDlg, (ID)) == BST_CHECKED)

int win32_dlgproc_eeltrans(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    E_EelTrans* g_this = (E_EelTrans*)g_current_render;
    const AVS_Parameter_Handle p_log_enabled = g_this->info.parameters[0].handle;
    const AVS_Parameter_Handle p_log_path = g_this->info.parameters[1].handle;
    const AVS_Parameter_Handle p_translate_firstlevel =
        g_this->info.parameters[2].handle;
    const AVS_Parameter_Handle p_read_comment_codes = g_this->info.parameters[3].handle;
    const AVS_Parameter_Handle p_code = g_this->info.parameters[4].handle;

    switch (uMsg) {
        case WM_INITDIALOG:
            SetDlgItemText(
                hwndDlg, IDC_EELTRANS_LOGPATH, g_this->get_string(p_log_path));
            SetDlgItemText(hwndDlg, IDC_EELTRANS_CODE, g_this->get_string(p_code));
            SetDlgItemText(hwndDlg, IDC_EELTRANS_VERSION, EELTRANS_VERSION);

            CheckDlgButton(hwndDlg, IDC_EELTRANS_ENABLED, g_this->enabled);
            CheckDlgButton(
                hwndDlg, IDC_EELTRANS_LOG_ENABLED, g_this->get_bool(p_log_enabled));
            CheckDlgButton(hwndDlg,
                           IDC_EELTRANS_TRANSFIRST,
                           g_this->get_bool(p_translate_firstlevel));
            CheckDlgButton(hwndDlg,
                           IDC_EELTRANS_READCOMMENTCODES,
                           g_this->get_bool(p_read_comment_codes));
            if (g_this->is_first_instance) {
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

        case WM_DESTROY: return 1;

        case WM_COMMAND:
            if (HIWORD(wParam) == EN_CHANGE) {
                int length = GetWindowTextLength((HWND)lParam) + 1;
                char* buf = new char[length];
                GetWindowText((HWND)lParam, buf, length);

                switch (LOWORD(wParam)) {
                    case IDC_EELTRANS_LOGPATH:
                        g_this->set_string(p_log_path, buf);
                        break;
                    case IDC_EELTRANS_CODE: g_this->set_string(p_code, buf); break;
                    default: break;
                }
                delete[] buf;
            }
            switch (LOWORD(wParam)) {
                case IDC_EELTRANS_ENABLED:
                    g_this->set_enabled(
                        IsDlgButtonChecked(hwndDlg, IDC_EELTRANS_ENABLED));
                    break;
                case IDC_EELTRANS_LOG_ENABLED:
                    g_this->set_bool(
                        p_log_enabled,
                        IsDlgButtonChecked(hwndDlg, IDC_EELTRANS_LOG_ENABLED));
                    break;
                case IDC_EELTRANS_TRANSFIRST:
                    g_this->set_bool(
                        p_translate_firstlevel,
                        IsDlgButtonChecked(hwndDlg, IDC_EELTRANS_TRANSFIRST));
                    break;
                case IDC_EELTRANS_READCOMMENTCODES:
                    g_this->set_bool(
                        p_read_comment_codes,
                        IsDlgButtonChecked(hwndDlg, IDC_EELTRANS_READCOMMENTCODES));
                    break;
            }
            return 1;
    }
    return 0;
}
