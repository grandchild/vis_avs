#include "e_globalvariables.h"

#include "g__defs.h"
#include "g__lib.h"

#include "c__defs.h"
#include "resource.h"

#include <windows.h>

void toggle_large_code_view(HWND hwndDlg, int button);
static int enlarged = 0;

int win32_dlgproc_globalvariables(HWND hwndDlg,
                                  UINT uMsg,
                                  WPARAM wParam,
                                  LPARAM lParam) {
    auto g_this = (E_GlobalVariables*)g_current_render;
    AVS_Parameter_Handle p_load = GlobalVariables_Info::parameters[0].handle;
    AVS_Parameter_Handle p_init = GlobalVariables_Info::parameters[1].handle;
    AVS_Parameter_Handle p_frame = GlobalVariables_Info::parameters[2].handle;
    AVS_Parameter_Handle p_beat = GlobalVariables_Info::parameters[3].handle;
    AVS_Parameter_Handle p_file = GlobalVariables_Info::parameters[4].handle;
    AVS_Parameter_Handle p_save_reg_ranges = GlobalVariables_Info::parameters[5].handle;
    AVS_Parameter_Handle p_save_buf_ranges = GlobalVariables_Info::parameters[6].handle;
    AVS_Parameter_Handle p_error = GlobalVariables_Info::parameters[7].handle;
    AVS_Parameter_Handle p_save = GlobalVariables_Info::parameters[8].handle;

    switch (uMsg) {
        case WM_INITDIALOG: {
            SetDlgItemText(
                hwndDlg, IDC_GLOBALVARS_CODE_INIT, g_this->get_string(p_init));
            SetDlgItemText(
                hwndDlg, IDC_GLOBALVARS_CODE_FRAME, g_this->get_string(p_frame));
            SetDlgItemText(
                hwndDlg, IDC_GLOBALVARS_CODE_BEAT, g_this->get_string(p_beat));
            SetDlgItemText(
                hwndDlg, IDC_GLOBALVARS_FILEPATH, g_this->get_string(p_file));
            SetDlgItemText(hwndDlg,
                           IDC_GLOBALVARS_REG_RANGE,
                           g_this->get_string(p_save_reg_ranges));
            SetDlgItemText(hwndDlg,
                           IDC_GLOBALVARS_BUF_RANGE,
                           g_this->get_string(p_save_buf_ranges));
            switch (g_this->get_int(p_load)) {
                case GLOBALVARS_LOAD_NONE:
                    CheckDlgButton(hwndDlg, IDC_GLOBALVARS_LOAD_NONE, BST_CHECKED);
                    break;
                case GLOBALVARS_LOAD_ONCE:
                    CheckDlgButton(hwndDlg, IDC_GLOBALVARS_LOAD_ONCE, BST_CHECKED);
                    break;
                case GLOBALVARS_LOAD_CODE:
                    CheckDlgButton(hwndDlg, IDC_GLOBALVARS_LOAD_CODE, BST_CHECKED);
                    break;
                case GLOBALVARS_LOAD_FRAME:
                    CheckDlgButton(hwndDlg, IDC_GLOBALVARS_LOAD_FRAME, BST_CHECKED);
                    break;
            }
            toggle_large_code_view(hwndDlg, 0);
            return 1;
        }

        case WM_COMMAND: {
            int wNotifyCode = HIWORD(wParam);
            HWND h = (HWND)lParam;

            if (wNotifyCode == EN_CHANGE) {
                int l = GetWindowTextLength(h) + 1;
                char* buf = new char[l];
                GetWindowText(h, buf, l);

                switch (LOWORD(wParam)) {
                    case IDC_GLOBALVARS_CODE_INIT:
                        g_this->set_string(p_init, buf);
                        break;
                    case IDC_GLOBALVARS_CODE_FRAME:
                        g_this->set_string(p_frame, buf);
                        break;
                    case IDC_GLOBALVARS_CODE_BEAT:
                        g_this->set_string(p_beat, buf);
                        break;
                    case IDC_GLOBALVARS_FILEPATH:
                        g_this->set_string(p_file, buf);
                        break;
                    case IDC_GLOBALVARS_REG_RANGE:
                        g_this->set_string(p_save_reg_ranges, buf);
                        SetDlgItemText(hwndDlg,
                                       IDC_GLOBALVARS_REG_RANGE_ERR,
                                       g_this->get_string(p_error));
                        break;
                    case IDC_GLOBALVARS_BUF_RANGE:
                        g_this->set_string(p_save_buf_ranges, buf);
                        SetDlgItemText(hwndDlg,
                                       IDC_GLOBALVARS_BUF_RANGE_ERR,
                                       g_this->get_string(p_error));
                        break;
                    default: break;
                }
                delete[] buf;
            } else if (wNotifyCode == BN_CLICKED) {
                switch (LOWORD(wParam)) {
                    case IDC_GLOBALVARS_SAVE_NOW: g_this->run_action(p_save); break;
                    case IDC_GLOBALVARS_ENLARGE_INIT:
                        SetDlgItemText(hwndDlg,
                                       IDC_GLOBALVARS_CODE_LARGE,
                                       g_this->get_string(p_init));
                        toggle_large_code_view(hwndDlg, IDC_GLOBALVARS_ENLARGE_INIT);
                        break;
                    case IDC_GLOBALVARS_ENLARGE_FRAME:
                        SetDlgItemText(hwndDlg,
                                       IDC_GLOBALVARS_CODE_LARGE,
                                       g_this->get_string(p_frame));
                        toggle_large_code_view(hwndDlg, IDC_GLOBALVARS_ENLARGE_FRAME);
                        break;
                    case IDC_GLOBALVARS_ENLARGE_BEAT:
                        SetDlgItemText(hwndDlg,
                                       IDC_GLOBALVARS_CODE_LARGE,
                                       g_this->get_string(p_beat));
                        toggle_large_code_view(hwndDlg, IDC_GLOBALVARS_ENLARGE_BEAT);
                        break;
                    case IDC_GLOBALVARS_LOAD_NONE:
                        if (IsDlgButtonChecked(hwndDlg, IDC_GLOBALVARS_LOAD_NONE)) {
                            g_this->set_int(p_load, GLOBALVARS_LOAD_NONE);
                        }
                        break;
                    case IDC_GLOBALVARS_LOAD_ONCE:
                        if (IsDlgButtonChecked(hwndDlg, IDC_GLOBALVARS_LOAD_ONCE)) {
                            g_this->set_int(p_load, GLOBALVARS_LOAD_ONCE);
                        }
                        break;
                    case IDC_GLOBALVARS_LOAD_CODE:
                        if (IsDlgButtonChecked(hwndDlg, IDC_GLOBALVARS_LOAD_CODE)) {
                            g_this->set_int(p_load, GLOBALVARS_LOAD_CODE);
                        }
                        break;
                    case IDC_GLOBALVARS_LOAD_FRAME:
                        if (IsDlgButtonChecked(hwndDlg, IDC_GLOBALVARS_LOAD_FRAME)) {
                            g_this->set_int(p_load, GLOBALVARS_LOAD_FRAME);
                        }
                        break;
                    case IDC_GLOBALVARS_HELP:
                        compilerfunctionlist(
                            hwndDlg, g_this->info.get_name(), g_this->info.get_help());
                        break;
                }
            }
            return 1;
        }

        case WM_DESTROY: return 1;
    }
    return 0;
}

/* if button param is 0, initialize to un-enlarged view */
void toggle_large_code_view(HWND hwndDlg, int button) {
    int show_large;
    int show_normal;
    if (button == 0) {  // initialize
        show_large = SW_HIDE;
        show_normal = SW_SHOW;
        enlarged = 0;
    } else if (enlarged == 0) {  // enlarge from normal view
        SetDlgItemText(hwndDlg, button, "-");
        enlarged = button;
        show_large = SW_SHOW;
        show_normal = SW_HIDE;
    } else if (enlarged == button) {  // un-enlarge from same button
        SetDlgItemText(hwndDlg, button, "+");
        enlarged = 0;
        show_large = SW_HIDE;
        show_normal = SW_SHOW;
    } else {  // switch enlarged view to another code field
        SetDlgItemText(hwndDlg, enlarged, "+");
        SetDlgItemText(hwndDlg, button, "-");
        enlarged = button;
        return;
    }
    ShowWindow(GetDlgItem(hwndDlg, IDC_GLOBALVARS_CODE_LARGE), show_large);
    ShowWindow(GetDlgItem(hwndDlg, IDC_GLOBALVARS_CODE_INIT), show_normal);
    ShowWindow(GetDlgItem(hwndDlg, IDC_GLOBALVARS_CODE_FRAME), show_normal);
    ShowWindow(GetDlgItem(hwndDlg, IDC_GLOBALVARS_CODE_BEAT), show_normal);
    ShowWindow(GetDlgItem(hwndDlg, IDC_GLOBALVARS_LOAD_ONCE), show_normal);
    ShowWindow(GetDlgItem(hwndDlg, IDC_GLOBALVARS_LOAD_FRAME), show_normal);
    ShowWindow(GetDlgItem(hwndDlg, IDC_GLOBALVARS_LOAD_CODE), show_normal);
    ShowWindow(GetDlgItem(hwndDlg, IDC_GLOBALVARS_LOAD_NONE), show_normal);
    ShowWindow(GetDlgItem(hwndDlg, IDC_GLOBALVARS_BOX_LOADSAVE), show_normal);
    ShowWindow(GetDlgItem(hwndDlg, IDC_GLOBALVARS_LABEL_LOADSAVE), show_normal);
    ShowWindow(GetDlgItem(hwndDlg, IDC_GLOBALVARS_FILEPATH), show_normal);
    ShowWindow(GetDlgItem(hwndDlg, IDC_GLOBALVARS_LABEL_REG_RANGE), show_normal);
    ShowWindow(GetDlgItem(hwndDlg, IDC_GLOBALVARS_REG_RANGE), show_normal);
    ShowWindow(GetDlgItem(hwndDlg, IDC_GLOBALVARS_LABEL_BUF_RANGE), show_normal);
    ShowWindow(GetDlgItem(hwndDlg, IDC_GLOBALVARS_BUF_RANGE), show_normal);
    ShowWindow(GetDlgItem(hwndDlg, IDC_GLOBALVARS_REG_RANGE_ERR), show_normal);
    ShowWindow(GetDlgItem(hwndDlg, IDC_GLOBALVARS_BUF_RANGE_ERR), show_normal);
    ShowWindow(GetDlgItem(hwndDlg, IDC_GLOBALVARS_SAVE_NOW), show_normal);
}
