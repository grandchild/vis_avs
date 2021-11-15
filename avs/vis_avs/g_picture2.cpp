#include "c_picture2.h"

#include "avs.h"
#include "c__defs.h"
#include "files.h"
#include "g__defs.h"
#include "g__lib.h"
#include "resource.h"

#include <commctrl.h>
#include <thread>
#include <windows.h>

int win32_dlgproc_picture2(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    C_Picture2* config_this = (C_Picture2*)g_current_render;
    unsigned int i;

    switch (uMsg) {
        case WM_COMMAND:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                HWND h = (HWND)lParam;
                switch (LOWORD(wParam)) {
                    case IDC_PICTUREII_OUTPUT:
                        config_this->config.output = SendMessage(h, CB_GETCURSEL, 0, 0);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_PICTUREII_ADJUSTABLE),
                                     config_this->config.output == OUT_ADJUSTABLE);
                        break;
                    case IDC_PICTUREII_BEAT_OUTPUT:
                        config_this->config.outputbeat =
                            SendMessage(h, CB_GETCURSEL, 0, 0);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_PICTUREII_BEAT_ADJUSTABLE),
                                     config_this->config.outputbeat == OUT_ADJUSTABLE);
                        break;
                    case IDC_PICTUREII_SOURCE:
                        int p = SendMessage(h, CB_GETCURSEL, 0, 0);
                        if (p >= 0) {
                            SendMessage(
                                h, CB_GETLBTEXT, p, (LPARAM)config_this->config.image);
                            if (config_this->load_image()) {
                                SetDlgItemText(hwndDlg, IDC_PICTUREII_ERROR, "");
                            } else {
                                SetDlgItemText(hwndDlg,
                                               IDC_PICTUREII_ERROR,
                                               "Error loading image");
                            }
                        }
                        break;
                }
            } else if (HIWORD(wParam) == BN_CLICKED) {
                HWND h = (HWND)lParam;
                switch (LOWORD(wParam)) {
                    case IDC_PICTUREII_BILINEAR:
                        config_this->config.bilinear =
                            (SendMessage(h, BM_GETCHECK, 0, 0) == BST_CHECKED);
                        break;
                    case IDC_PICTUREII_BEAT_BILINEAR:
                        config_this->config.bilinearbeat =
                            (SendMessage(h, BM_GETCHECK, 0, 0) == BST_CHECKED);
                        break;
                }
            }
            return 1;

        case WM_HSCROLL: {
            HWND h = (HWND)lParam;
            bool beat = h == GetDlgItem(hwndDlg, IDC_PICTUREII_BEAT_ADJUSTABLE);
            (beat ? config_this->config.adjustablebeat
                  : config_this->config.adjustable) =
                min(255,
                    max(0,
                        SendDlgItemMessage(hwndDlg,
                                           beat ? IDC_PICTUREII_BEAT_ADJUSTABLE
                                                : IDC_PICTUREII_ADJUSTABLE,
                                           TBM_GETPOS,
                                           0,
                                           0)));
            return 1;
        }

        case WM_INITDIALOG: {
            if (config_this->config.bilinear) {
                CheckDlgButton(hwndDlg, IDC_PICTUREII_BILINEAR, BST_CHECKED);
            }
            if (config_this->config.bilinearbeat) {
                CheckDlgButton(hwndDlg, IDC_PICTUREII_BEAT_BILINEAR, BST_CHECKED);
            }

            // Fill in outputlist
            for (i = 0; i < sizeof(config_this->blendmodes) / sizeof(char*); ++i) {
                SendDlgItemMessage(hwndDlg,
                                   IDC_PICTUREII_OUTPUT,
                                   CB_ADDSTRING,
                                   0,
                                   (LPARAM)config_this->blendmodes[i]);
                SendDlgItemMessage(hwndDlg,
                                   IDC_PICTUREII_BEAT_OUTPUT,
                                   CB_ADDSTRING,
                                   0,
                                   (LPARAM)config_this->blendmodes[i]);
            }
            SendDlgItemMessage(hwndDlg,
                               IDC_PICTUREII_OUTPUT,
                               CB_SETCURSEL,
                               config_this->config.output,
                               0);
            SendDlgItemMessage(hwndDlg,
                               IDC_PICTUREII_BEAT_OUTPUT,
                               CB_SETCURSEL,
                               config_this->config.outputbeat,
                               0);

            EnableWindow(GetDlgItem(hwndDlg, IDC_PICTUREII_ADJUSTABLE),
                         config_this->config.output == OUT_ADJUSTABLE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_PICTUREII_BEAT_ADJUSTABLE),
                         config_this->config.outputbeat == OUT_ADJUSTABLE);

            SendDlgItemMessage(
                hwndDlg, IDC_PICTUREII_ADJUSTABLE, TBM_SETRANGE, 1, MAKELONG(0, 255));
            SendDlgItemMessage(hwndDlg,
                               IDC_PICTUREII_ADJUSTABLE,
                               TBM_SETPOS,
                               1,
                               config_this->config.adjustable);

            SendDlgItemMessage(hwndDlg,
                               IDC_PICTUREII_BEAT_ADJUSTABLE,
                               TBM_SETRANGE,
                               1,
                               MAKELONG(0, 255));
            SendDlgItemMessage(hwndDlg,
                               IDC_PICTUREII_BEAT_ADJUSTABLE,
                               TBM_SETPOS,
                               1,
                               config_this->config.adjustablebeat);

            for (auto file : config_this->file_list) {
                int p = SendDlgItemMessage(
                    hwndDlg, IDC_PICTUREII_SOURCE, CB_ADDSTRING, 0, (LPARAM)file);

                if (strncmp(file, config_this->config.image, MAX_PATH) == 0) {
                    SendDlgItemMessage(
                        hwndDlg, IDC_PICTUREII_SOURCE, CB_SETCURSEL, p, 0);
                }
            }
            return 1;
        }

        case WM_DESTROY:
            KillTimer(hwndDlg, 1);
            return 1;
    }
    return 0;
}
