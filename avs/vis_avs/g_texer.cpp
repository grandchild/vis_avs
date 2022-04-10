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

    int64_t options_length;
    const char* const* options;
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

            options = p_image.get_options(&options_length);
            for (int i = 0; i < options_length; i++) {
                SendDlgItemMessage(
                    hwndDlg, IDC_TEXER_IMAGE, CB_ADDSTRING, 0, (LPARAM)options[i]);
            }
            SendDlgItemMessage(hwndDlg,
                               IDC_TEXER_IMAGE,
                               CB_SETCURSEL,
                               g_this->get_int(p_image.handle),
                               0);
            return 1;
        }
        case WM_COMMAND: {
            int command = HIWORD(wParam);
            int control = LOWORD(wParam);
            if (command == CBN_SELCHANGE && control == IDC_TEXER_IMAGE) {
                int p = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
                if (p >= 0) {
                    g_this->set_int(p_image.handle, p);
                }
                return 1;
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
                return 1;
            }
            break;
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
