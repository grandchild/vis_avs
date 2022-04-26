#include "e_dynamicmovement.h"

#include "g__defs.h"
#include "g__lib.h"

#include "c__defs.h"
#include "resource.h"

#include <windows.h>

int win32_dlgproc_dynamicmovement(HWND hwndDlg,
                                  UINT uMsg,
                                  WPARAM wParam,
                                  LPARAM lParam) {
    E_DynamicMovement* g_this = (E_DynamicMovement*)g_current_render;
    AVS_Parameter_Handle p_init = g_this->info.parameters[0].handle;
    AVS_Parameter_Handle p_frame = g_this->info.parameters[1].handle;
    AVS_Parameter_Handle p_beat = g_this->info.parameters[2].handle;
    AVS_Parameter_Handle p_point = g_this->info.parameters[3].handle;
    AVS_Parameter_Handle p_bilinear = g_this->info.parameters[4].handle;
    AVS_Parameter_Handle p_coordinates = g_this->info.parameters[5].handle;
    AVS_Parameter_Handle p_grid_w = g_this->info.parameters[6].handle;
    AVS_Parameter_Handle p_grid_h = g_this->info.parameters[7].handle;
    AVS_Parameter_Handle p_blend = g_this->info.parameters[8].handle;
    AVS_Parameter_Handle p_wrap = g_this->info.parameters[9].handle;
    const Parameter& p_buffer = g_this->info.parameters[10];
    AVS_Parameter_Handle p_alpha_only = g_this->info.parameters[11].handle;
    const Parameter& p_example = g_this->info.parameters[12];
    AVS_Parameter_Handle p_load_example = g_this->info.parameters[13].handle;

    static int isstart;
    switch (uMsg) {
        case WM_INITDIALOG: {
            isstart = 1;
            SetDlgItemText(hwndDlg, IDC_EDIT4, g_this->get_string(p_init));
            SetDlgItemText(hwndDlg, IDC_EDIT2, g_this->get_string(p_frame));
            SetDlgItemText(hwndDlg, IDC_EDIT3, g_this->get_string(p_beat));
            SetDlgItemText(hwndDlg, IDC_EDIT1, g_this->get_string(p_point));
            CheckDlgButton(hwndDlg, IDC_CHECK1, g_this->get_bool(p_blend));
            CheckDlgButton(hwndDlg, IDC_CHECK2, g_this->get_bool(p_bilinear));
            CheckDlgButton(hwndDlg,
                           IDC_CHECK3,
                           g_this->get_int(p_coordinates) == COORDS_CARTESIAN);
            CheckDlgButton(hwndDlg, IDC_WRAP, g_this->get_bool(p_wrap));
            CheckDlgButton(hwndDlg, IDC_NOMOVEMENT, g_this->get_bool(p_alpha_only));

            SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_ADDSTRING, 0, (int)"Current");
            char txt[16];
            for (int i = 0; i < p_buffer.int_max; i++) {
                wsprintf(txt, "Buffer %d", i + 1);
                SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_ADDSTRING, 0, (int)txt);
            }
            auto buffer = g_this->get_int(p_buffer.handle);
            SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_SETCURSEL, (WPARAM)buffer, 0);

            SetDlgItemInt(hwndDlg, IDC_EDIT5, g_this->get_int(p_grid_w), false);
            SetDlgItemInt(hwndDlg, IDC_EDIT6, g_this->get_int(p_grid_h), false);
            isstart = 0;
            return 1;
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_BUTTON1) {
                compilerfunctionlist(
                    hwndDlg, g_this->info.get_name(), g_this->info.get_help());
            }

            if (LOWORD(wParam) == IDC_CHECK1) {
                g_this->set_bool(p_blend, IsDlgButtonChecked(hwndDlg, IDC_CHECK1));
            }
            if (LOWORD(wParam) == IDC_CHECK2) {
                g_this->set_bool(p_bilinear, IsDlgButtonChecked(hwndDlg, IDC_CHECK2));
            }
            if (LOWORD(wParam) == IDC_CHECK3) {
                g_this->set_int(p_coordinates,
                                IsDlgButtonChecked(hwndDlg, IDC_CHECK3) ? 1 : 0);
            }
            if (LOWORD(wParam) == IDC_WRAP) {
                g_this->set_bool(p_wrap, IsDlgButtonChecked(hwndDlg, IDC_WRAP));
            }
            if (LOWORD(wParam) == IDC_NOMOVEMENT) {
                g_this->set_bool(p_alpha_only,
                                 IsDlgButtonChecked(hwndDlg, IDC_NOMOVEMENT));
            }
            // Load preset examples from the examples table.
            if (LOWORD(wParam) == IDC_BUTTON4) {
                RECT r;
                HMENU hMenu;
                MENUITEMINFO i = {};
                i.cbSize = sizeof(i);
                hMenu = CreatePopupMenu();
                int x;

                int64_t options_length;
                const char* const* options = p_example.get_options(&options_length);
                const int index_offset = 16;
                for (x = 0; x < options_length; x++) {
                    i.fMask = MIIM_TYPE | MIIM_DATA | MIIM_ID;
                    i.fType = MFT_STRING;
                    i.wID = x + index_offset;
                    i.dwTypeData = (char*)options[x];
                    i.cch = strnlen(options[x], 128);
                    InsertMenuItem(hMenu, x, true, &i);
                }
                GetWindowRect(GetDlgItem(hwndDlg, IDC_BUTTON1), &r);
                x = TrackPopupMenu(hMenu,
                                   TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD
                                       | TPM_RIGHTBUTTON | TPM_LEFTBUTTON
                                       | TPM_NONOTIFY,
                                   r.right,
                                   r.top,
                                   0,
                                   hwndDlg,
                                   NULL);
                if (x >= index_offset && x < index_offset + options_length) {
                    g_this->set_int(p_example.handle, x - index_offset);
                    g_this->run_action(p_load_example);
                    SetDlgItemText(hwndDlg, IDC_EDIT1, g_this->get_string(p_point));
                    SetDlgItemText(hwndDlg, IDC_EDIT2, g_this->get_string(p_frame));
                    SetDlgItemText(hwndDlg, IDC_EDIT3, g_this->get_string(p_beat));
                    SetDlgItemText(hwndDlg, IDC_EDIT4, g_this->get_string(p_init));
                    CheckDlgButton(hwndDlg,
                                   IDC_CHECK3,
                                   g_this->get_int(p_coordinates) == COORDS_CARTESIAN);
                    CheckDlgButton(hwndDlg, IDC_WRAP, g_this->get_bool(p_wrap));
                    SetDlgItemInt(hwndDlg, IDC_EDIT5, g_this->get_int(p_grid_w), false);
                    SetDlgItemInt(hwndDlg, IDC_EDIT6, g_this->get_int(p_grid_h), false);
                }
                DestroyMenu(hMenu);
            }

            if (!isstart && HIWORD(wParam) == CBN_SELCHANGE
                && LOWORD(wParam) == IDC_COMBO1)  // handle clicks to combo box
                g_this->set_int(
                    p_buffer.handle,
                    SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0));

            if (!isstart && HIWORD(wParam) == EN_CHANGE) {
                int t;
                if (LOWORD(wParam) == IDC_EDIT5) {
                    g_this->set_int(p_grid_w, GetDlgItemInt(hwndDlg, IDC_EDIT5, &t, 0));
                }
                if (LOWORD(wParam) == IDC_EDIT6) {
                    g_this->set_int(p_grid_h, GetDlgItemInt(hwndDlg, IDC_EDIT6, &t, 0));
                }

                if (LOWORD(wParam) == IDC_EDIT1 || LOWORD(wParam) == IDC_EDIT2
                    || LOWORD(wParam) == IDC_EDIT3 || LOWORD(wParam) == IDC_EDIT4) {
                    int length = 0;
                    char* buf = NULL;
                    switch (LOWORD(wParam)) {
                        case IDC_EDIT1:
                            length = GetWindowTextLength((HWND)lParam) + 1;
                            buf = new char[length];
                            GetDlgItemText(hwndDlg, IDC_EDIT1, buf, length);
                            g_this->set_string(p_point, buf);
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
                        case IDC_EDIT4:
                            length = GetWindowTextLength((HWND)lParam) + 1;
                            buf = new char[length];
                            GetDlgItemText(hwndDlg, IDC_EDIT4, buf, length);
                            g_this->set_string(p_init, buf);
                            delete[] buf;
                            break;
                    }
                }
            }
            return 0;
    }
    return 0;
}
