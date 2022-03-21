#include "c_list.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include "../platform.h"

#include <windows.h>
#include <commctrl.h>

void fill_buffer_combo(HWND dlg, int ctl) {
    int i = 0;
    char txt[64];
    for (i = 0; i < NBUF; i++) {
        wsprintf(txt, "Buffer %d", i + 1);
        SendDlgItemMessage(dlg, ctl, CB_ADDSTRING, 0, (int)txt);
    }
}

int win32_dlgproc_root_effectlist(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    C_RenderListClass* g_this = (C_RenderListClass*)g_current_render;
    switch (uMsg) {
        case WM_INITDIALOG: {
            if (g_this->clearfb()) CheckDlgButton(hwndDlg, IDC_CHECK1, BST_CHECKED);
        }
            return 1;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_CHECK1:
                    g_this->set_clearfb(IsDlgButtonChecked(hwndDlg, IDC_CHECK1));
                    break;
            }
            break;
    }
    return 0;
}

int win32_dlgproc_effectlist(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    C_RenderListClass* g_this = (C_RenderListClass*)g_current_render;
    switch (uMsg) {
        case WM_INITDIALOG: {
            {
                unsigned int x;
                for (x = 0;
                     x < sizeof(g_this->blendmodes) / sizeof(g_this->blendmodes[0]);
                     x++) {
                    SendDlgItemMessage(hwndDlg,
                                       IDC_COMBO1,
                                       CB_ADDSTRING,
                                       0,
                                       (LPARAM)g_this->blendmodes[x]);
                    SendDlgItemMessage(hwndDlg,
                                       IDC_COMBO2,
                                       CB_ADDSTRING,
                                       0,
                                       (LPARAM)g_this->blendmodes[x]);
                }
                SendDlgItemMessage(
                    hwndDlg, IDC_COMBO1, CB_SETCURSEL, (WPARAM)g_this->blendout(), 0);
                SendDlgItemMessage(
                    hwndDlg, IDC_COMBO2, CB_SETCURSEL, (WPARAM)g_this->blendin(), 0);
                ShowWindow(GetDlgItem(hwndDlg, IDC_INSLIDE),
                           (g_this->blendin() == 10) ? SW_NORMAL : SW_HIDE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_OUTSLIDE),
                           (g_this->blendout() == 10) ? SW_NORMAL : SW_HIDE);
                SendDlgItemMessage(
                    hwndDlg, IDC_INSLIDE, TBM_SETRANGE, TRUE, MAKELONG(0, 255));
                SendDlgItemMessage(
                    hwndDlg, IDC_INSLIDE, TBM_SETPOS, TRUE, (int)(g_this->inblendval));
                SendDlgItemMessage(
                    hwndDlg, IDC_OUTSLIDE, TBM_SETRANGE, TRUE, MAKELONG(0, 255));
                SendDlgItemMessage(hwndDlg,
                                   IDC_OUTSLIDE,
                                   TBM_SETPOS,
                                   TRUE,
                                   (int)(g_this->outblendval));
                ShowWindow(GetDlgItem(hwndDlg, IDC_CBBUF1),
                           (g_this->blendin() == 12) ? SW_NORMAL : SW_HIDE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_CBBUF2),
                           (g_this->blendout() == 12) ? SW_NORMAL : SW_HIDE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_INVERT1),
                           (g_this->blendin() == 12) ? SW_NORMAL : SW_HIDE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_INVERT2),
                           (g_this->blendout() == 12) ? SW_NORMAL : SW_HIDE);
                fill_buffer_combo(hwndDlg, IDC_CBBUF1);
                fill_buffer_combo(hwndDlg, IDC_CBBUF2);
                SendDlgItemMessage(
                    hwndDlg, IDC_CBBUF1, CB_SETCURSEL, (WPARAM)g_this->bufferin, 0);
                SendDlgItemMessage(
                    hwndDlg, IDC_CBBUF2, CB_SETCURSEL, (WPARAM)g_this->bufferout, 0);
                if (g_this->ininvert) CheckDlgButton(hwndDlg, IDC_INVERT1, BST_CHECKED);
                if (g_this->outinvert)
                    CheckDlgButton(hwndDlg, IDC_INVERT2, BST_CHECKED);
            }
            if (g_this->clearfb()) CheckDlgButton(hwndDlg, IDC_CHECK1, BST_CHECKED);
            g_this->isstart = 1;
            SetDlgItemText(hwndDlg, IDC_EDIT4, (char*)g_this->effect_exp[0].c_str());
            SetDlgItemText(hwndDlg, IDC_EDIT5, (char*)g_this->effect_exp[1].c_str());
            g_this->isstart = 0;

            if (((g_this->mode & 2) ^ 2))
                CheckDlgButton(hwndDlg, IDC_CHECK2, BST_CHECKED);
            if (g_this->beat_render)
                CheckDlgButton(hwndDlg, IDC_CHECK3, BST_CHECKED);
            else
                EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT1), FALSE);
            if (g_this->use_code) CheckDlgButton(hwndDlg, IDC_CHECK4, BST_CHECKED);
            char buf[999];
            wsprintf(buf, "%d", g_this->beat_render_frames);
            SetDlgItemText(hwndDlg, IDC_EDIT1, buf);
        }
            return 1;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_EDIT4:
                case IDC_EDIT5:
                    if (!g_this->isstart && HIWORD(wParam) == EN_CHANGE) {
                        lock_lock(g_this->code_lock);
                        g_this->effect_exp[0] = string_from_dlgitem(hwndDlg, IDC_EDIT4);
                        g_this->effect_exp[1] = string_from_dlgitem(hwndDlg, IDC_EDIT5);
                        g_this->need_recompile = 1;
                        if (LOWORD(wParam) == IDC_EDIT4) g_this->inited = 0;
                        lock_unlock(g_this->code_lock);
                    }
                    break;
                case IDC_COMBO1:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        int r =
                            SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0);
                        if (r != CB_ERR) {
                            g_this->set_blendout(r);
                            ShowWindow(GetDlgItem(hwndDlg, IDC_OUTSLIDE),
                                       (r == 10) ? SW_NORMAL : SW_HIDE);
                            ShowWindow(
                                GetDlgItem(hwndDlg, IDC_CBBUF2),
                                (g_this->blendout() == 12) ? SW_NORMAL : SW_HIDE);
                            ShowWindow(
                                GetDlgItem(hwndDlg, IDC_INVERT2),
                                (g_this->blendout() == 12) ? SW_NORMAL : SW_HIDE);
                        }
                    }
                    break;
                case IDC_COMBO2:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        int r =
                            SendDlgItemMessage(hwndDlg, IDC_COMBO2, CB_GETCURSEL, 0, 0);
                        if (r != CB_ERR) {
                            g_this->set_blendin(r);
                            ShowWindow(GetDlgItem(hwndDlg, IDC_INSLIDE),
                                       (r == 10) ? SW_NORMAL : SW_HIDE);
                            ShowWindow(GetDlgItem(hwndDlg, IDC_CBBUF1),
                                       (g_this->blendin() == 12) ? SW_NORMAL : SW_HIDE);
                            ShowWindow(GetDlgItem(hwndDlg, IDC_INVERT1),
                                       (g_this->blendin() == 12) ? SW_NORMAL : SW_HIDE);
                        }
                    }
                    break;
                case IDC_CBBUF1:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                        g_this->bufferin =
                            SendDlgItemMessage(hwndDlg, IDC_CBBUF1, CB_GETCURSEL, 0, 0);
                    break;
                case IDC_CBBUF2:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                        g_this->bufferout =
                            SendDlgItemMessage(hwndDlg, IDC_CBBUF2, CB_GETCURSEL, 0, 0);
                    break;
                case IDC_CHECK1:
                    g_this->set_clearfb(IsDlgButtonChecked(hwndDlg, IDC_CHECK1));
                    break;
                case IDC_CHECK2:
                    g_this->set_enabled(IsDlgButtonChecked(hwndDlg, IDC_CHECK2));
                    break;
                case IDC_INVERT1:
                    g_this->ininvert = IsDlgButtonChecked(hwndDlg, IDC_INVERT1);
                    break;
                case IDC_INVERT2:
                    g_this->outinvert = IsDlgButtonChecked(hwndDlg, IDC_INVERT2);
                    break;
                case IDC_CHECK3:
                    g_this->beat_render = IsDlgButtonChecked(hwndDlg, IDC_CHECK3);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT1), g_this->beat_render);
                    break;
                case IDC_CHECK4:
                    g_this->use_code = !!IsDlgButtonChecked(hwndDlg, IDC_CHECK4);
                    break;
                case IDC_EDIT1:
                    if (HIWORD(wParam) == EN_CHANGE) {
                        char buf[999] = "1";
                        GetDlgItemText(hwndDlg, IDC_EDIT1, buf, 999);
                        buf[998] = 0;
                        g_this->beat_render_frames = atoi(buf);
                    }
                    break;
                case IDC_BUTTON2: {
                    char* text =
                        "Read/write 'enabled' to get/set whether the effect list is "
                        "enabled for this frame\r\n"
                        "Read/write 'beat' to get/set whether there is currently a "
                        "beat\r\n"
                        "Read/write 'clear' to get/set whether to clear the "
                        "framebuffer\r\n"
                        "If the input blend is set to adjustable, 'alphain' can be set "
                        "from 0.0-1.0\r\n"
                        "If the output blend is set to adjustable, 'alphaout' can be "
                        "set from 0.0-1.0\r\n"
                        "'w' and 'h' are set with the current width and height of the "
                        "frame\r\n";
                    compilerfunctionlist(hwndDlg, "Effect List", text);
                } break;
            }
            break;
        case WM_NOTIFY:
            if (LOWORD(wParam) == IDC_INSLIDE)
                g_this->inblendval =
                    SendDlgItemMessage(hwndDlg, IDC_INSLIDE, TBM_GETPOS, 0, 0);
            if (LOWORD(wParam) == IDC_OUTSLIDE)
                g_this->outblendval =
                    SendDlgItemMessage(hwndDlg, IDC_OUTSLIDE, TBM_GETPOS, 0, 0);
            break;
    }
    return 0;
}
