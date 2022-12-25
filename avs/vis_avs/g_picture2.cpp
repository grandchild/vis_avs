#include "e_picture2.h"

#include "g__defs.h"
#include "g__lib.h"

#include "c__defs.h"
#include "files.h"
#include "resource.h"

#include <windows.h>
#include <commctrl.h>
#include <thread>

int win32_dlgproc_picture2(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto g_this = (E_Picture2*)g_current_render;
    const Parameter& p_image = Picture2_Info::parameters[0];
    const Parameter& p_blend_mode = Picture2_Info::parameters[1];
    const Parameter& p_on_beat_blend_mode = Picture2_Info::parameters[2];
    AVS_Parameter_Handle p_bilinear = Picture2_Info::parameters[3].handle;
    AVS_Parameter_Handle p_on_beat_bilinear = Picture2_Info::parameters[4].handle;
    const Parameter& p_adjust_blend = Picture2_Info::parameters[5];
    const Parameter& p_on_beat_adjust_blend = Picture2_Info::parameters[6];
    AVS_Parameter_Handle p_error_msg = Picture2_Info::parameters[7].handle;

    switch (uMsg) {
        case WM_INITDIALOG: {
            auto image = g_this->get_string(p_image.handle);
            init_resource(p_image, image, hwndDlg, IDC_PICTUREII_SOURCE, MAX_PATH);
            if (g_this->get_int(p_bilinear)) {
                CheckDlgButton(hwndDlg, IDC_PICTUREII_BILINEAR, BST_CHECKED);
            }
            if (g_this->get_int(p_on_beat_bilinear)) {
                CheckDlgButton(hwndDlg, IDC_PICTUREII_BEAT_BILINEAR, BST_CHECKED);
            }
            auto blend_mode = g_this->get_int(p_blend_mode.handle);
            init_select(p_blend_mode, blend_mode, hwndDlg, IDC_PICTUREII_OUTPUT);
            auto on_beat_blend_mode = g_this->get_int(p_on_beat_blend_mode.handle);
            init_select(p_on_beat_blend_mode,
                        on_beat_blend_mode,
                        hwndDlg,
                        IDC_PICTUREII_BEAT_OUTPUT);

            CheckDlgButton(
                hwndDlg, IDC_PICTUREII_BILINEAR, g_this->get_bool(p_bilinear));
            CheckDlgButton(hwndDlg,
                           IDC_PICTUREII_BEAT_BILINEAR,
                           g_this->get_bool(p_on_beat_bilinear));

            auto adjust_blend = g_this->get_int(p_adjust_blend.handle);
            init_ranged_slider(
                p_adjust_blend, adjust_blend, hwndDlg, IDC_PICTUREII_ADJUSTABLE);
            auto on_beat_adjust_blend = g_this->get_int(p_on_beat_adjust_blend.handle);
            init_ranged_slider(p_on_beat_adjust_blend,
                               on_beat_adjust_blend,
                               hwndDlg,
                               IDC_PICTUREII_BEAT_ADJUSTABLE);

            EnableWindow(GetDlgItem(hwndDlg, IDC_PICTUREII_ADJUSTABLE),
                         g_this->get_int(p_blend_mode.handle) == P2_BLEND_ADJUSTABLE);
            EnableWindow(
                GetDlgItem(hwndDlg, IDC_PICTUREII_BEAT_ADJUSTABLE),
                g_this->get_int(p_on_beat_blend_mode.handle) == P2_BLEND_ADJUSTABLE);
            return 1;
        }
        case WM_COMMAND:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                HWND h = (HWND)lParam;
                switch (LOWORD(wParam)) {
                    case IDC_PICTUREII_OUTPUT:
                        g_this->set_int(p_blend_mode.handle,
                                        SendMessage(h, CB_GETCURSEL, 0, 0));
                        EnableWindow(GetDlgItem(hwndDlg, IDC_PICTUREII_ADJUSTABLE),
                                     g_this->get_int(p_blend_mode.handle)
                                         == P2_BLEND_ADJUSTABLE);
                        break;

                    case IDC_PICTUREII_BEAT_OUTPUT:
                        g_this->set_int(p_on_beat_blend_mode.handle,
                                        SendMessage(h, CB_GETCURSEL, 0, 0));
                        EnableWindow(GetDlgItem(hwndDlg, IDC_PICTUREII_BEAT_ADJUSTABLE),
                                     g_this->get_int(p_on_beat_blend_mode.handle)
                                         == P2_BLEND_ADJUSTABLE);
                        break;
                    case IDC_PICTUREII_SOURCE: {
                        int sel = SendMessage(h, CB_GETCURSEL, 0, 0);
                        if (sel < 0) {
                            break;
                        }
                        size_t filename_length =
                            SendDlgItemMessage(
                                hwndDlg, IDC_PICTUREII_SOURCE, CB_GETLBTEXTLEN, sel, 0)
                            + 1;
                        char* buf = (char*)calloc(filename_length, sizeof(char));
                        SendDlgItemMessage(hwndDlg,
                                           IDC_PICTUREII_SOURCE,
                                           CB_GETLBTEXT,
                                           sel,
                                           (LPARAM)buf);
                        g_this->set_string(p_image.handle, buf);
                        free(buf);

                        SetDlgItemText(hwndDlg,
                                       IDC_PICTUREII_ERROR,
                                       g_this->get_string(p_error_msg));
                        break;
                    }
                }
            } else if (HIWORD(wParam) == BN_CLICKED) {
                HWND h = (HWND)lParam;
                switch (LOWORD(wParam)) {
                    case IDC_PICTUREII_BILINEAR:
                        g_this->set_bool(p_bilinear, SendMessage(h, BM_GETCHECK, 0, 0));
                        break;
                    case IDC_PICTUREII_BEAT_BILINEAR:
                        g_this->set_bool(p_on_beat_bilinear,
                                         SendMessage(h, BM_GETCHECK, 0, 0));
                        break;
                }
            }
            return 1;

        case WM_HSCROLL:
            HWND control = (HWND)lParam;
            auto value = SendMessage(control, TBM_GETPOS, 0, 0);
            if (control == GetDlgItem(hwndDlg, IDC_PICTUREII_ADJUSTABLE)) {
                g_this->set_int(p_adjust_blend.handle, value);
            } else if (control == GetDlgItem(hwndDlg, IDC_PICTUREII_BEAT_ADJUSTABLE)) {
                g_this->set_int(p_on_beat_adjust_blend.handle, value);
            }
            return 1;
    }
    return 0;
}
