#include "e_texer2.h"

#include "g__defs.h"
#include "g__lib.h"

#include "c__defs.h"
#include "resource.h"

#include <windows.h>

#define TEXERII_EXAMPLES_FIRST_ID 31337

void load_examples(E_Texer2* texer2, HWND dialog, HWND button);

int win32_dlgproc_texer2(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    E_Texer2* g_this = (E_Texer2*)g_current_render;
    const Parameter& p_image = g_this->info.parameters[1];
    const AVS_Parameter_Handle p_resize = g_this->info.parameters[2].handle;
    const AVS_Parameter_Handle p_wrap = g_this->info.parameters[3].handle;
    const AVS_Parameter_Handle p_colorize = g_this->info.parameters[4].handle;
    const AVS_Parameter_Handle p_init = g_this->info.parameters[5].handle;
    const AVS_Parameter_Handle p_frame = g_this->info.parameters[6].handle;
    const AVS_Parameter_Handle p_beat = g_this->info.parameters[7].handle;
    const AVS_Parameter_Handle p_point = g_this->info.parameters[8].handle;
    const Parameter& p_example = g_this->info.parameters[9];
    const AVS_Parameter_Handle p_load_example = g_this->info.parameters[10].handle;

    int64_t options_length;
    const char* const* options;

    switch (uMsg) {
        case WM_COMMAND: {
            int wNotifyCode = HIWORD(wParam);
            HWND h = (HWND)lParam;

            if (wNotifyCode == CBN_SELCHANGE) {
                switch (LOWORD(wParam)) {
                    case IDC_TEXERII_TEXTURE:
                        HWND h = (HWND)lParam;
                        int selection = SendMessage(h, CB_GETCURSEL, 0, 0);
                        if (selection >= 1) {
                            g_this->set_int(p_image.handle, selection);
                            g_this->load_image();
                        } else {
                            g_this->set_int(p_image.handle, 0);
                            g_this->load_image();
                        }
                        break;
                }
            } else if (wNotifyCode == BN_CLICKED) {
                g_this->set_bool(p_resize,
                                 IsDlgButtonChecked(hwndDlg, IDC_TEXERII_RESIZE));
                g_this->set_bool(p_wrap, IsDlgButtonChecked(hwndDlg, IDC_TEXERII_WRAP));
                g_this->set_bool(p_colorize,
                                 IsDlgButtonChecked(hwndDlg, IDC_TEXERII_MASK));

                if (LOWORD(wParam) == IDC_TEXERII_ABOUT) {
                    compilerfunctionlist(
                        hwndDlg, g_this->info.get_name(), g_this->info.get_help());
                } else if (LOWORD(wParam) == IDC_TEXERII_EXAMPLE) {
                    RECT r;
                    GetWindowRect(GetDlgItem(hwndDlg, IDC_TEXERII_EXAMPLE), &r);

                    HMENU m = CreatePopupMenu();
                    if (m == NULL) {
                        return 1;
                    }
                    const char* const* options = p_example.get_options(&options_length);
                    for (int i = 0; i < options_length; i++) {
                        AppendMenu(
                            m, MF_STRING, TEXERII_EXAMPLES_FIRST_ID + i, options[i]);
                    }
                    int ret = TrackPopupMenu(
                        m, TPM_RETURNCMD, r.left + 1, r.bottom + 1, 0, hwndDlg, 0);
                    if (ret < TEXERII_EXAMPLES_FIRST_ID
                        || ret >= (TEXERII_EXAMPLES_FIRST_ID + options_length)) {
                        return 1;
                    }
                    g_this->set_int(p_example.handle, ret - TEXERII_EXAMPLES_FIRST_ID);
                    g_this->run_action(p_load_example);
                    SetDlgItemText(
                        hwndDlg, IDC_TEXERII_INIT, g_this->get_string(p_init));
                    SetDlgItemText(
                        hwndDlg, IDC_TEXERII_FRAME, g_this->get_string(p_frame));
                    SetDlgItemText(
                        hwndDlg, IDC_TEXERII_BEAT, g_this->get_string(p_beat));
                    SetDlgItemText(
                        hwndDlg, IDC_TEXERII_POINT, g_this->get_string(p_point));
                    CheckDlgButton(
                        hwndDlg, IDC_TEXERII_RESIZE, g_this->get_bool(p_resize));
                    CheckDlgButton(hwndDlg, IDC_TEXERII_WRAP, g_this->get_bool(p_wrap));
                    CheckDlgButton(
                        hwndDlg, IDC_TEXERII_MASK, g_this->get_bool(p_colorize));

                    // select the default texture image
                    g_this->set_int(p_image.handle, 0);
                    SendDlgItemMessage(hwndDlg,
                                       IDC_TEXERII_TEXTURE,
                                       CB_SETCURSEL,
                                       g_this->get_int(p_image.handle),
                                       0);
                    DestroyMenu(m);
                }
            } else if (wNotifyCode == EN_CHANGE) {
                char* buf;
                int l = GetWindowTextLength(h) + 1;
                buf = new char[l];
                GetWindowText(h, buf, l);

                switch (LOWORD(wParam)) {
                    case IDC_TEXERII_INIT: g_this->set_string(p_init, buf); break;
                    case IDC_TEXERII_FRAME: g_this->set_string(p_frame, buf); break;
                    case IDC_TEXERII_BEAT: g_this->set_string(p_beat, buf); break;
                    case IDC_TEXERII_POINT: g_this->set_string(p_point, buf); break;
                    default: break;
                }
                delete[] buf;
            }
            return 1;
        }

        case WM_INITDIALOG: {
            options = p_image.get_options(&options_length);
            for (unsigned int i = 0; i < options_length; i++) {
                SendDlgItemMessage(
                    hwndDlg, IDC_TEXERII_TEXTURE, CB_ADDSTRING, 0, (LPARAM)options[i]);
            }
            int64_t filename_sel = g_this->get_int(p_image.handle);
            if (filename_sel >= 0) {
                SendDlgItemMessage(
                    hwndDlg, IDC_TEXERII_TEXTURE, CB_SETCURSEL, filename_sel, 0);
            } else {
                SendDlgItemMessage(hwndDlg, IDC_TEXERII_TEXTURE, CB_SETCURSEL, 0, 0);
            }
            SetDlgItemText(hwndDlg, IDC_TEXERII_INIT, g_this->get_string(p_init));
            SetDlgItemText(hwndDlg, IDC_TEXERII_FRAME, g_this->get_string(p_frame));
            SetDlgItemText(hwndDlg, IDC_TEXERII_BEAT, g_this->get_string(p_beat));
            SetDlgItemText(hwndDlg, IDC_TEXERII_POINT, g_this->get_string(p_point));
            CheckDlgButton(hwndDlg, IDC_TEXERII_RESIZE, g_this->get_bool(p_resize));
            CheckDlgButton(hwndDlg, IDC_TEXERII_WRAP, g_this->get_bool(p_wrap));
            CheckDlgButton(hwndDlg, IDC_TEXERII_MASK, g_this->get_bool(p_colorize));
            return 1;
        }

        case WM_DESTROY: return 1;
    }
    return 0;
}
