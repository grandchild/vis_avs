#include "c_texer.h"

#include "g__defs.h"
#include "g__lib.h"

#include "c__defs.h"
#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_texer(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    C_Texer* config_this = (C_Texer*)g_current_render;
    char num_particles_label[5];
    switch (uMsg) {
        case WM_INITDIALOG: {
            SendDlgItemMessage(
                hwndDlg, IDC_TEXER_NUM_PARTICLES, TBM_SETRANGE, 1, MAKELONG(1, 1024));
            SendDlgItemMessage(hwndDlg,
                               IDC_TEXER_NUM_PARTICLES,
                               TBM_SETPOS,
                               1,
                               config_this->num_particles);
            wsprintf(num_particles_label, "%d", config_this->num_particles);
            SetDlgItemText(hwndDlg, IDC_TEXER_NUM_PARTICLES_LABEL, num_particles_label);
            CheckDlgButton(hwndDlg,
                           config_this->input_mode == TEXER_INPUT_REPLACE
                               ? IDC_TEXER_INPUT_REPLACE
                               : IDC_TEXER_INPUT_IGNORE,
                           BST_CHECKED);
            CheckDlgButton(hwndDlg,
                           config_this->output_mode == TEXER_OUTPUT_MASKED
                               ? IDC_TEXER_OUTPUT_MASKED
                               : IDC_TEXER_OUTPUT_NORMAL,
                           BST_CHECKED);
            for (auto file : config_this->file_list) {
                int p = SendDlgItemMessage(
                    hwndDlg, IDC_TEXER_IMAGE, CB_ADDSTRING, 0, (LPARAM)file);

                if (strncmp(file, config_this->image, MAX_PATH) == 0) {
                    SendDlgItemMessage(hwndDlg, IDC_TEXER_IMAGE, CB_SETCURSEL, p, 0);
                }
            }
            return 1;
        }
        case WM_COMMAND: {
            int command = HIWORD(wParam);
            int control = LOWORD(wParam);
            if (command == CBN_SELCHANGE && control == IDC_TEXER_IMAGE) {
                int p = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
                if (p >= 0) {
                    SendMessage(
                        (HWND)lParam, CB_GETLBTEXT, p, (LPARAM)config_this->image);
                    if (config_this->load_image()) {
                        SetDlgItemText(hwndDlg, IDC_TEXER_ERROR, "");
                    } else {
                        SetDlgItemText(hwndDlg, IDC_TEXER_ERROR, "Error loading image");
                    }
                }
                return 1;
            } else if (command == BN_CLICKED) {
                if (control == IDC_TEXER_INPUT_IGNORE
                    || control == IDC_TEXER_INPUT_REPLACE) {
                    config_this->input_mode =
                        IsDlgButtonChecked(hwndDlg, IDC_TEXER_INPUT_REPLACE)
                            ? TEXER_INPUT_REPLACE
                            : TEXER_INPUT_IGNORE;
                } else if (control == IDC_TEXER_OUTPUT_NORMAL
                           || control == IDC_TEXER_OUTPUT_MASKED) {
                    config_this->output_mode =
                        IsDlgButtonChecked(hwndDlg, IDC_TEXER_OUTPUT_MASKED)
                            ? TEXER_OUTPUT_MASKED
                            : TEXER_OUTPUT_NORMAL;
                }
                return 1;
            }
            break;
        }
        case WM_HSCROLL: {
            if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_TEXER_NUM_PARTICLES)) {
                config_this->num_particles =
                    SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
                wsprintf(num_particles_label, "%d", config_this->num_particles);
                SetDlgItemText(
                    hwndDlg, IDC_TEXER_NUM_PARTICLES_LABEL, num_particles_label);
                return 1;
            }
        }
    }

    return 0;
}
