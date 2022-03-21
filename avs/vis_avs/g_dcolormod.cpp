#include "c_dcolormod.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_colormodifier(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    C_THISCLASS* g_this = (C_THISCLASS*)g_current_render;

    static int isstart;
    switch (uMsg) {
        case WM_INITDIALOG:
            isstart = 1;
            SetDlgItemText(hwndDlg, IDC_EDIT1, (char*)g_this->effect_exp[0].c_str());
            SetDlgItemText(hwndDlg, IDC_EDIT2, (char*)g_this->effect_exp[1].c_str());
            SetDlgItemText(hwndDlg, IDC_EDIT3, (char*)g_this->effect_exp[2].c_str());
            SetDlgItemText(hwndDlg, IDC_EDIT4, (char*)g_this->effect_exp[3].c_str());
            if (g_this->m_recompute) CheckDlgButton(hwndDlg, IDC_CHECK1, BST_CHECKED);

            isstart = 0;

            return 1;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_BUTTON1) {
                compilerfunctionlist(hwndDlg, "Color Modifier", g_this->help_text);
            }

            if (LOWORD(wParam) == IDC_CHECK1) {
                g_this->m_recompute = IsDlgButtonChecked(hwndDlg, IDC_CHECK1) ? 1 : 0;
            }

            if (LOWORD(wParam) == IDC_BUTTON4) {
                RECT r;
                HMENU hMenu;
                MENUITEMINFO i = {};
                i.cbSize = sizeof(i);
                hMenu = CreatePopupMenu();
                unsigned int x;
                for (x = 0; x < sizeof(g_this->presets) / sizeof(g_this->presets[0]);
                     x++) {
                    i.fMask = MIIM_TYPE | MIIM_DATA | MIIM_ID;
                    i.fType = MFT_STRING;
                    i.wID = x + 16;
                    i.dwTypeData = g_this->presets[x].name;
                    i.cch = strlen(g_this->presets[x].name);
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
                if (x >= 16
                    && x < 16 + sizeof(g_this->presets) / sizeof(g_this->presets[0])) {
                    isstart = 1;
                    SetDlgItemText(hwndDlg, IDC_EDIT1, g_this->presets[x - 16].point);
                    SetDlgItemText(hwndDlg, IDC_EDIT2, g_this->presets[x - 16].frame);
                    SetDlgItemText(hwndDlg, IDC_EDIT3, g_this->presets[x - 16].beat);
                    SetDlgItemText(hwndDlg, IDC_EDIT4, g_this->presets[x - 16].init);
                    g_this->m_recompute = g_this->presets[x - 16].recompute;
                    CheckDlgButton(
                        hwndDlg, IDC_CHECK1, g_this->m_recompute ? BST_CHECKED : 0);
                    isstart = 0;

                    SendMessage(
                        hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_EDIT4, EN_CHANGE), 0);
                }
                DestroyMenu(hMenu);
            }
            if (!isstart && HIWORD(wParam) == EN_CHANGE) {
                if (LOWORD(wParam) == IDC_EDIT1 || LOWORD(wParam) == IDC_EDIT2
                    || LOWORD(wParam) == IDC_EDIT3 || LOWORD(wParam) == IDC_EDIT4) {
                    lock_lock(g_this->code_lock);
                    g_this->effect_exp[0] = string_from_dlgitem(hwndDlg, IDC_EDIT1);
                    g_this->effect_exp[1] = string_from_dlgitem(hwndDlg, IDC_EDIT2);
                    g_this->effect_exp[2] = string_from_dlgitem(hwndDlg, IDC_EDIT3);
                    g_this->effect_exp[3] = string_from_dlgitem(hwndDlg, IDC_EDIT4);
                    g_this->need_recompile = 1;
                    if (LOWORD(wParam) == IDC_EDIT4) g_this->inited = 0;
                    g_this->m_tab_valid = 0;
                    lock_unlock(g_this->code_lock);
                }
            }
            return 0;
    }
    return 0;
}
