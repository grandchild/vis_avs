#include "c_globalvars.h"

#include "g__defs.h"
#include "g__lib.h"

#include "c__defs.h"
#include "resource.h"

#include <windows.h>

void toggle_large_code_view(HWND hwndDlg, int button);
static int enlarged = 0;

int win32_dlgproc_globalvars(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    C_GlobalVars* config_this = (C_GlobalVars*)g_current_render;

    switch (uMsg) {
        case WM_INITDIALOG: {
            SetDlgItemText(
                hwndDlg, IDC_GLOBALVARS_CODE_INIT, config_this->code.init.string);
            SetDlgItemText(
                hwndDlg, IDC_GLOBALVARS_CODE_FRAME, config_this->code.frame.string);
            SetDlgItemText(
                hwndDlg, IDC_GLOBALVARS_CODE_BEAT, config_this->code.beat.string);
            SetDlgItemText(
                hwndDlg, IDC_GLOBALVARS_FILEPATH, config_this->filepath.c_str());
            SetDlgItemText(
                hwndDlg, IDC_GLOBALVARS_REG_RANGE, config_this->reg_ranges_str.c_str());
            SetDlgItemText(
                hwndDlg, IDC_GLOBALVARS_BUF_RANGE, config_this->buf_ranges_str.c_str());
            switch (config_this->load_option) {
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
                bool range_valid;

                switch (LOWORD(wParam)) {
                    case IDC_GLOBALVARS_CODE_INIT:
                        config_this->code.init.set(buf, l);
                        break;
                    case IDC_GLOBALVARS_CODE_FRAME:
                        config_this->code.frame.set(buf, l);
                        break;
                    case IDC_GLOBALVARS_CODE_BEAT:
                        config_this->code.beat.set(buf, l);
                        break;
                    case IDC_GLOBALVARS_FILEPATH: config_this->filepath = buf; break;
                    case IDC_GLOBALVARS_REG_RANGE:
                        config_this->reg_ranges_str = buf;
                        range_valid =
                            config_this->check_set_range(config_this->reg_ranges_str,
                                                         config_this->reg_ranges,
                                                         config_this->max_regs_index);
                        if (range_valid) {
                            SetDlgItemText(hwndDlg, IDC_GLOBALVARS_REG_RANGE_ERR, "");
                        } else {
                            SetDlgItemText(
                                hwndDlg, IDC_GLOBALVARS_REG_RANGE_ERR, "Err");
                        }
                        break;
                    case IDC_GLOBALVARS_BUF_RANGE:
                        config_this->buf_ranges_str = buf;
                        range_valid =
                            config_this->check_set_range(config_this->buf_ranges_str,
                                                         config_this->buf_ranges,
                                                         config_this->max_gmb_index);
                        if (range_valid) {
                            SetDlgItemText(hwndDlg, IDC_GLOBALVARS_BUF_RANGE_ERR, "");
                        } else {
                            SetDlgItemText(
                                hwndDlg, IDC_GLOBALVARS_BUF_RANGE_ERR, "Err");
                        }
                        break;
                    default: break;
                }
                delete[] buf;
            } else if (wNotifyCode == BN_CLICKED) {
                switch (LOWORD(wParam)) {
                    case IDC_GLOBALVARS_SAVE_NOW:
                        config_this->save_ranges_to_file();
                        break;
                    case IDC_GLOBALVARS_ENLARGE_INIT:
                        SetDlgItemText(hwndDlg,
                                       IDC_GLOBALVARS_CODE_LARGE,
                                       config_this->code.init.string);
                        toggle_large_code_view(hwndDlg, IDC_GLOBALVARS_ENLARGE_INIT);
                        break;
                    case IDC_GLOBALVARS_ENLARGE_FRAME:
                        SetDlgItemText(hwndDlg,
                                       IDC_GLOBALVARS_CODE_LARGE,
                                       config_this->code.frame.string);
                        toggle_large_code_view(hwndDlg, IDC_GLOBALVARS_ENLARGE_FRAME);
                        break;
                    case IDC_GLOBALVARS_ENLARGE_BEAT:
                        SetDlgItemText(hwndDlg,
                                       IDC_GLOBALVARS_CODE_LARGE,
                                       config_this->code.beat.string);
                        toggle_large_code_view(hwndDlg, IDC_GLOBALVARS_ENLARGE_BEAT);
                        break;
                    case IDC_GLOBALVARS_LOAD_NONE:
                        if (IsDlgButtonChecked(hwndDlg, IDC_GLOBALVARS_LOAD_NONE)) {
                            config_this->load_option = GLOBALVARS_LOAD_NONE;
                        }
                        break;
                    case IDC_GLOBALVARS_LOAD_ONCE:
                        if (IsDlgButtonChecked(hwndDlg, IDC_GLOBALVARS_LOAD_ONCE)) {
                            config_this->load_option = GLOBALVARS_LOAD_ONCE;
                        }
                        break;
                    case IDC_GLOBALVARS_LOAD_CODE:
                        if (IsDlgButtonChecked(hwndDlg, IDC_GLOBALVARS_LOAD_CODE)) {
                            config_this->load_option = GLOBALVARS_LOAD_CODE;
                        }
                        break;
                    case IDC_GLOBALVARS_LOAD_FRAME:
                        if (IsDlgButtonChecked(hwndDlg, IDC_GLOBALVARS_LOAD_FRAME)) {
                            config_this->load_option = GLOBALVARS_LOAD_FRAME;
                        }
                        break;
                    case IDC_GLOBALVARS_HELP:
                        compilerfunctionlist(
                            hwndDlg, "Global Variable Manager", config_this->help_text);
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
