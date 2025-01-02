#include "e_convolution.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

// Desired behavior:
//   - UI-to-config on (valid) change
//   - Config-to-UI only on de-focus
//     Values may change internally when clamping to range or resetting scale=0 to 1.
//   - Empty field -> 0
void edit_num_field(HWND hwndDlg,
                    int control_id,
                    UINT event,
                    E_Convolution* g_this,
                    AVS_Parameter_Handle param,
                    std::vector<int64_t> param_path = {}) {
    if (event == EN_CHANGE) {
        BOOL int_parse_success = false;
        auto value =
            (int32_t)GetDlgItemInt(hwndDlg, control_id, &int_parse_success, true);
        if (int_parse_success) {
            g_this->set_int(param, value, param_path);
        } else if (GetWindowTextLength(GetDlgItem(hwndDlg, control_id)) == 0) {
            g_this->set_int(param, 0, param_path);
        }
    } else if (event == EN_KILLFOCUS) {
        SetDlgItemInt(hwndDlg, control_id, g_this->get_int(param, param_path), true);
    }
}

int win32_dlgproc_convolution(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    auto g_this = (E_Convolution*)g_current_render;
    AVS_Parameter_Handle p_wrap = Convolution_Info::parameters[0].handle;
    AVS_Parameter_Handle p_absolute = Convolution_Info::parameters[1].handle;
    AVS_Parameter_Handle p_two_pass = Convolution_Info::parameters[2].handle;
    AVS_Parameter_Handle p_bias = Convolution_Info::parameters[3].handle;
    AVS_Parameter_Handle p_scale = Convolution_Info::parameters[4].handle;
    AVS_Parameter_Handle p_kernel_value = Convolution_Info::kernel_params[0].handle;
    AVS_Parameter_Handle p_save_file = Convolution_Info::parameters[6].handle;
    AVS_Parameter_Handle p_autoscale = Convolution_Info::parameters[7].handle;
    AVS_Parameter_Handle p_clear = Convolution_Info::parameters[8].handle;
    AVS_Parameter_Handle p_save = Convolution_Info::parameters[9].handle;
    AVS_Parameter_Handle p_load = Convolution_Info::parameters[10].handle;

    static bool ignore_command_messages = false;
    switch (uMsg) {
        case WM_INITDIALOG: {
            ignore_command_messages = true;
            CheckDlgButton(hwndDlg, IDC_CONVOLUTION_ENABLED, g_this->enabled);
            CheckDlgButton(hwndDlg, IDC_CONVOLUTION_WRAP, g_this->get_bool(p_wrap));
            CheckDlgButton(
                hwndDlg, IDC_CONVOLUTION_ABSOLUTE, g_this->get_bool(p_absolute));
            CheckDlgButton(
                hwndDlg, IDC_CONVOLUTION_TWOPASS, g_this->get_bool(p_two_pass));

            for (int i = 0; i < CONVO_KERNEL_SIZE; i++) {
                SetDlgItemInt(hwndDlg,
                              IDC_CONVOLUTION_EDIT1 + i,
                              g_this->get_int(p_kernel_value, {i}),
                              true);
            }

            SetDlgItemInt(hwndDlg, IDC_CONVOLUTION_BIAS, g_this->get_int(p_bias), true);
            SetDlgItemInt(
                hwndDlg, IDC_CONVOLUTION_SCALE, g_this->get_int(p_scale), true);
            ignore_command_messages = false;
            return 1;
        }
        case WM_COMMAND: {
            if (ignore_command_messages) {
                return 0;
            }
            WORD control_id = LOWORD(wParam);
            WORD message = HIWORD(wParam);
            int32_t kernel_index = control_id - IDC_CONVOLUTION_EDIT1;
            if (control_id == IDC_CONVOLUTION_ENABLED && message == BN_CLICKED) {
                g_this->set_enabled(
                    IsDlgButtonChecked(hwndDlg, IDC_CONVOLUTION_ENABLED));
            } else if (control_id == IDC_CONVOLUTION_WRAP && message == BN_CLICKED) {
                g_this->set_bool(p_wrap,
                                 IsDlgButtonChecked(hwndDlg, IDC_CONVOLUTION_WRAP));
                ignore_command_messages = true;
                CheckDlgButton(
                    hwndDlg, IDC_CONVOLUTION_ABSOLUTE, g_this->get_bool(p_absolute));
                ignore_command_messages = false;
            } else if (control_id == IDC_CONVOLUTION_ABSOLUTE
                       && message == BN_CLICKED) {
                g_this->set_bool(p_absolute,
                                 IsDlgButtonChecked(hwndDlg, IDC_CONVOLUTION_ABSOLUTE));
                ignore_command_messages = true;
                CheckDlgButton(hwndDlg, IDC_CONVOLUTION_WRAP, g_this->get_bool(p_wrap));
                ignore_command_messages = false;
            } else if (control_id == IDC_CONVOLUTION_TWOPASS && message == BN_CLICKED) {
                g_this->set_bool(p_two_pass,
                                 IsDlgButtonChecked(hwndDlg, IDC_CONVOLUTION_TWOPASS));
            } else if (control_id == IDC_CONVOLUTION_BIAS) {
                ignore_command_messages = true;
                edit_num_field(hwndDlg, IDC_CONVOLUTION_BIAS, message, g_this, p_bias);
                ignore_command_messages = false;
            } else if (control_id == IDC_CONVOLUTION_SCALE) {
                ignore_command_messages = true;
                edit_num_field(
                    hwndDlg, IDC_CONVOLUTION_SCALE, message, g_this, p_scale);
                ignore_command_messages = false;
            } else if (kernel_index >= 0 && kernel_index < CONVO_KERNEL_SIZE) {
                ignore_command_messages = true;
                edit_num_field(hwndDlg,
                               IDC_CONVOLUTION_EDIT1 + kernel_index,
                               message,
                               g_this,
                               p_kernel_value,
                               {kernel_index});
                ignore_command_messages = false;
            } else if (control_id == IDC_CONVOLUTION_AUTOSCALE
                       && message == BN_CLICKED) {
                g_this->run_action(p_autoscale);
                ignore_command_messages = true;
                SetDlgItemInt(
                    hwndDlg, IDC_CONVOLUTION_SCALE, g_this->get_int(p_scale), true);
                ignore_command_messages = false;
            } else if (control_id == IDC_CONVOLUTION_CLEAR && message == BN_CLICKED) {
                if (MessageBox(hwndDlg,
                               "Really clear all data?",
                               "convolution",
                               MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_APPLMODAL
                                   | MB_SETFOREGROUND)
                    == IDYES) {
                    g_this->run_action(p_clear);
                    SendMessage(hwndDlg, WM_INITDIALOG, 0, 0);
                }
            } else if ((control_id == IDC_CONVOLUTION_SAVE
                        || control_id == IDC_CONVOLUTION_LOAD)
                       && message == BN_CLICKED) {
                char filename_buf[1024];
                strncpy(filename_buf,
                        g_this->get_string(p_save_file),
                        sizeof(filename_buf));
                OPENFILENAME open_file = {};
                open_file.lStructSize = sizeof(OPENFILENAME);
                open_file.hwndOwner = hwndDlg;
                open_file.lpstrFilter =
                    "Convolution Filter File (*.cff)\0*.cff\0All Files (*.*)\0*.*\0";
                open_file.nFilterIndex = 1;
                open_file.nMaxFile = sizeof(filename_buf);
                open_file.lpstrFile = filename_buf;
                open_file.lpstrDefExt = "cff";
                // 0x02000000 is OFN_DONTADDTORECENT
                // for some reason the compiler is not recognising it.
                open_file.Flags = 0x02000000 | OFN_HIDEREADONLY;

                if (control_id == IDC_CONVOLUTION_SAVE) {
                    if (GetSaveFileName(&(open_file))) {
                        g_this->set_string(p_save_file, open_file.lpstrFile);
                        g_this->run_action(p_save);
                    }
                } else {
                    open_file.Flags |= OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                    if (GetOpenFileName(&open_file)) {
                        g_this->set_string(p_save_file, open_file.lpstrFile);
                        g_this->run_action(p_load);
                        SendMessage(hwndDlg, WM_INITDIALOG, 0, 0);
                    }
                }
            }
            return 0;
        }
    }
    return 0;
}
