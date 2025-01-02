#include "e_texer.h"

#include "g__defs.h"
#include "g__lib.h"

#include "c__defs.h"
#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_texer(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    E_Texer* g_this = (E_Texer*)g_current_render;
    const Parameter& p_image = g_this->info.parameters[0];
    const AVS_Parameter_Handle p_add_to_input = g_this->info.parameters[1].handle;
    const AVS_Parameter_Handle p_colorize = g_this->info.parameters[2].handle;
    const AVS_Parameter_Handle p_num_particles = g_this->info.parameters[3].handle;

    char num_particles_label[5];
    switch (uMsg) {
        case WM_INITDIALOG: {
            SendDlgItemMessage(
                hwndDlg, IDC_TEXER_NUM_PARTICLES, TBM_SETRANGE, 1, MAKELONG(1, 1024));
            SendDlgItemMessage(hwndDlg,
                               IDC_TEXER_NUM_PARTICLES,
                               TBM_SETPOS,
                               1,
                               g_this->get_int(p_num_particles));
            wsprintf(num_particles_label, "%d", g_this->get_int(p_num_particles));
            SetDlgItemText(hwndDlg, IDC_TEXER_NUM_PARTICLES_LABEL, num_particles_label);
            CheckDlgButton(hwndDlg,
                           g_this->get_bool(p_add_to_input) ? IDC_TEXER_INPUT_REPLACE
                                                            : IDC_TEXER_INPUT_IGNORE,
                           BST_CHECKED);
            CheckDlgButton(hwndDlg,
                           g_this->get_bool(p_colorize) ? IDC_TEXER_OUTPUT_MASKED
                                                        : IDC_TEXER_OUTPUT_NORMAL,
                           BST_CHECKED);

            auto image = g_this->get_string(p_image.handle);
            init_resource(p_image, image, hwndDlg, IDC_TEXER_IMAGE, MAX_PATH);
            return 1;
        }
        case WM_COMMAND: {
            int command = HIWORD(wParam);
            int control = LOWORD(wParam);
            if (command == CBN_SELCHANGE && control == IDC_TEXER_IMAGE) {
                int sel = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
                if (sel < 0) {
                    break;
                }
                size_t filename_length =
                    SendDlgItemMessage(hwndDlg, control, CB_GETLBTEXTLEN, sel, 0) + 1;
                char* buf = (char*)calloc(filename_length, sizeof(char));
                SendDlgItemMessage(hwndDlg, control, CB_GETLBTEXT, sel, (LPARAM)buf);
                g_this->set_string(p_image.handle, buf);
                free(buf);
                break;
            } else if (command == BN_CLICKED) {
                if (control == IDC_TEXER_INPUT_IGNORE
                    || control == IDC_TEXER_INPUT_REPLACE) {
                    g_this->set_bool(
                        p_add_to_input,
                        IsDlgButtonChecked(hwndDlg, IDC_TEXER_INPUT_REPLACE));
                } else if (control == IDC_TEXER_OUTPUT_NORMAL
                           || control == IDC_TEXER_OUTPUT_MASKED) {
                    g_this->set_bool(
                        p_colorize,
                        IsDlgButtonChecked(hwndDlg, IDC_TEXER_OUTPUT_MASKED));
                }
                break;
            }
            return 1;
        }
        case WM_HSCROLL: {
            if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_TEXER_NUM_PARTICLES)) {
                g_this->set_int(p_num_particles,
                                SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
                wsprintf(num_particles_label, "%d", g_this->get_int(p_num_particles));
                SetDlgItemText(
                    hwndDlg, IDC_TEXER_NUM_PARTICLES_LABEL, num_particles_label);
                return 1;
            }
        }
    }

    return 0;
}
