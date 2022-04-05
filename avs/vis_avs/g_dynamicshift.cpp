#include "e_dynamicshift.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_dynamicshift(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    E_DynamicShift* g_this = (E_DynamicShift*)g_current_render;
    const AVS_Parameter_Handle p_init = g_this->info.parameters[0].handle;
    const AVS_Parameter_Handle p_frame = g_this->info.parameters[1].handle;
    const AVS_Parameter_Handle p_beat = g_this->info.parameters[2].handle;
    const AVS_Parameter_Handle p_blend_mode = g_this->info.parameters[3].handle;
    const AVS_Parameter_Handle p_bilinear = g_this->info.parameters[4].handle;

    static int isstart;
    switch (uMsg) {
        case WM_INITDIALOG:
            isstart = 1;
            isstart = 1;
            SetDlgItemText(hwndDlg, IDC_EDIT1, (char*)g_this->get_string(p_init));
            SetDlgItemText(hwndDlg, IDC_EDIT2, (char*)g_this->get_string(p_frame));
            SetDlgItemText(hwndDlg, IDC_EDIT3, (char*)g_this->get_string(p_beat));
            isstart = 0;
            CheckDlgButton(hwndDlg, IDC_CHECK1, g_this->get_bool(p_blend_mode));
            CheckDlgButton(hwndDlg, IDC_CHECK2, g_this->get_bool(p_bilinear));
            return 1;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_BUTTON1) {
                compilerfunctionlist(
                    hwndDlg, g_this->info.get_name(), g_this->info.get_help());
            } else if (LOWORD(wParam) == IDC_CHECK1) {
                g_this->set_int(p_blend_mode,
                                IsDlgButtonChecked(hwndDlg, IDC_CHECK1)
                                    ? BLEND_SIMPLE_5050
                                    : BLEND_SIMPLE_REPLACE);
            } else if (LOWORD(wParam) == IDC_CHECK2) {
                g_this->set_bool(p_bilinear, IsDlgButtonChecked(hwndDlg, IDC_CHECK2));
            } else if (!isstart && HIWORD(wParam) == EN_CHANGE) {
                int length = 0;
                char* buf = NULL;
                switch (LOWORD(wParam)) {
                    case IDC_EDIT1:
                        length = GetWindowTextLength((HWND)lParam) + 1;
                        buf = new char[length];
                        GetDlgItemText(hwndDlg, IDC_EDIT1, buf, length);
                        g_this->set_string(p_init, buf);
                        delete[] buf;
                        break;
                    case IDC_EDIT2:
                        length = GetWindowTextLength((HWND)lParam) + 1;
                        buf = new char[length];
                        GetDlgItemText(hwndDlg, IDC_EDIT2, buf, length);
                        g_this->set_string(p_frame, buf);
                        delete[] buf;
                        break;
                    case IDC_EDIT3:
                        length = GetWindowTextLength((HWND)lParam) + 1;
                        buf = new char[length];
                        GetDlgItemText(hwndDlg, IDC_EDIT3, buf, length);
                        g_this->set_string(p_beat, buf);
                        delete[] buf;
                        break;
                }
            }
            return 0;
    }
    return 0;
}
