#include "e_colormodifier.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_colormodifier(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    E_ColorModifier* g_this = (E_ColorModifier*)g_current_render;
    const AVS_Parameter_Handle p_init = g_this->info.parameters[0].handle;
    const AVS_Parameter_Handle p_frame = g_this->info.parameters[1].handle;
    const AVS_Parameter_Handle p_beat = g_this->info.parameters[2].handle;
    const AVS_Parameter_Handle p_point = g_this->info.parameters[3].handle;
    const AVS_Parameter_Handle p_recompute = g_this->info.parameters[4].handle;
    const Parameter& p_example = g_this->info.parameters[5];
    AVS_Parameter_Handle p_load_example = g_this->info.parameters[6].handle;

    static int isstart;
    switch (uMsg) {
        case WM_INITDIALOG:
            isstart = 1;
            SetDlgItemText(hwndDlg, IDC_EDIT1, (char*)g_this->get_string(p_point));
            SetDlgItemText(hwndDlg, IDC_EDIT2, (char*)g_this->get_string(p_frame));
            SetDlgItemText(hwndDlg, IDC_EDIT3, (char*)g_this->get_string(p_beat));
            SetDlgItemText(hwndDlg, IDC_EDIT4, (char*)g_this->get_string(p_init));
            CheckDlgButton(hwndDlg, IDC_CHECK1, g_this->get_bool(p_recompute));
            isstart = 0;
            return 1;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_BUTTON1) {
                compilerfunctionlist(
                    hwndDlg, g_this->info.get_name(), g_this->info.get_help());
            }

            if (LOWORD(wParam) == IDC_CHECK1) {
                g_this->set_bool(p_recompute, IsDlgButtonChecked(hwndDlg, IDC_CHECK1));
            }

            if (LOWORD(wParam) == IDC_BUTTON4) {
                RECT r;
                HMENU hMenu;
                MENUITEMINFO i = {};
                i.cbSize = sizeof(i);
                hMenu = CreatePopupMenu();
                unsigned int x;
                int64_t num_examples;
                const char* const* example_names = p_example.get_options(&num_examples);
                const int index_offset = 16;
                for (x = 0; x < num_examples; x++) {
                    i.fMask = MIIM_TYPE | MIIM_DATA | MIIM_ID;
                    i.fType = MFT_STRING;
                    i.wID = x + 16;
                    i.dwTypeData = (char*)example_names[x];
                    i.cch = strnlen(example_names[x], 128);
                    InsertMenuItem(hMenu, x, TRUE, &i);
                }
                GetWindowRect(GetDlgItem(hwndDlg, IDC_BUTTON4), &r);
                x = TrackPopupMenu(hMenu,
                                   TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD
                                       | TPM_RIGHTBUTTON | TPM_LEFTBUTTON
                                       | TPM_NONOTIFY,
                                   r.right,
                                   r.top,
                                   0,
                                   hwndDlg,
                                   NULL);
                if (x >= index_offset && x < index_offset + num_examples) {
                    g_this->set_int(p_example.handle, x - index_offset);
                    g_this->run_action(p_load_example);
                    SetDlgItemText(
                        hwndDlg, IDC_EDIT1, (char*)g_this->get_string(p_point));
                    SetDlgItemText(
                        hwndDlg, IDC_EDIT2, (char*)g_this->get_string(p_frame));
                    SetDlgItemText(
                        hwndDlg, IDC_EDIT3, (char*)g_this->get_string(p_beat));
                    SetDlgItemText(
                        hwndDlg, IDC_EDIT4, (char*)g_this->get_string(p_init));
                    CheckDlgButton(hwndDlg, IDC_CHECK1, g_this->get_bool(p_recompute));

                    // SendMessage(
                    //     hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_EDIT4, EN_CHANGE), 0);
                }
                DestroyMenu(hMenu);
            }
            if (!isstart && HIWORD(wParam) == EN_CHANGE) {
                int length = 0;
                char* buf = NULL;
                switch (LOWORD(wParam)) {
                    case IDC_EDIT1:
                        length = GetWindowTextLength((HWND)lParam);
                        buf = (char*)calloc(length, sizeof(char));
                        GetDlgItemText(hwndDlg, IDC_EDIT1, buf, length + 1);
                        g_this->set_string(p_point, buf);
                        free(buf);
                        break;
                    case IDC_EDIT2:
                        length = GetWindowTextLength((HWND)lParam);
                        buf = (char*)calloc(length, sizeof(char));
                        GetDlgItemText(hwndDlg, IDC_EDIT2, buf, length + 1);
                        g_this->set_string(p_frame, buf);
                        free(buf);
                        break;
                    case IDC_EDIT3:
                        length = GetWindowTextLength((HWND)lParam);
                        buf = (char*)calloc(length, sizeof(char));
                        GetDlgItemText(hwndDlg, IDC_EDIT3, buf, length + 1);
                        g_this->set_string(p_beat, buf);
                        free(buf);
                        break;
                    case IDC_EDIT4:
                        length = GetWindowTextLength((HWND)lParam);
                        buf = (char*)calloc(length, sizeof(char));
                        GetDlgItemText(hwndDlg, IDC_EDIT4, buf, length + 1);
                        g_this->set_string(p_init, buf);
                        free(buf);
                        break;
                }
            }
            return 0;
    }
    return 0;
}
