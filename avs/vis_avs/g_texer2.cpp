#include "c_texer2.h"

#include "g__defs.h"
#include "g__lib.h"

#include "c__defs.h"
#include "resource.h"

#include <windows.h>

void load_examples(C_Texer2* texer2, HWND dialog, HWND button);

int win32_dlgproc_texer2(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    C_Texer2* g_ConfigThis = (C_Texer2*)g_current_render;
    char buf[MAX_PATH];
    switch (uMsg) {
        case WM_COMMAND: {
            int wNotifyCode = HIWORD(wParam);
            HWND h = (HWND)lParam;

            if (wNotifyCode == CBN_SELCHANGE) {
                switch (LOWORD(wParam)) {
                    case IDC_TEXERII_TEXTURE:
                        HWND h = (HWND)lParam;
                        int p = SendMessage(h, CB_GETCURSEL, 0, 0);
                        if (p >= 1) {
                            SendMessage(
                                h, CB_GETLBTEXT, p, (LPARAM)g_ConfigThis->config.img);
                            g_ConfigThis->InitTexture();
                        } else {
                            g_ConfigThis->config.img[0] = 0;
                            g_ConfigThis->InitTexture();
                        }
                        break;
                }
            } else if (wNotifyCode == BN_CLICKED) {
                g_ConfigThis->config.wrap =
                    IsDlgButtonChecked(hwndDlg, IDC_TEXERII_WRAP) == BST_CHECKED;
                g_ConfigThis->config.resize =
                    IsDlgButtonChecked(hwndDlg, IDC_TEXERII_RESIZE) == BST_CHECKED;
                g_ConfigThis->config.mask =
                    IsDlgButtonChecked(hwndDlg, IDC_TEXERII_MASK) == BST_CHECKED;

                if (LOWORD(wParam) == IDC_TEXERII_ABOUT) {
                    compilerfunctionlist(hwndDlg, g_ConfigThis->help_text);
                } else if (LOWORD(wParam) == IDC_TEXERII_EXAMPLE) {
                    HWND examplesButton = GetDlgItem(hwndDlg, IDC_TEXERII_EXAMPLE);
                    load_examples(g_ConfigThis, hwndDlg, examplesButton);
                }
            } else if (wNotifyCode == EN_CHANGE) {
                char* buf;
                int l = GetWindowTextLength(h);
                buf = new char[l + 1];
                GetWindowText(h, buf, l + 1);

                switch (LOWORD(wParam)) {
                    case IDC_TEXERII_INIT:
                        g_ConfigThis->code.init.set(buf, l + 1);
                        break;
                    case IDC_TEXERII_FRAME:
                        g_ConfigThis->code.frame.set(buf, l + 1);
                        break;
                    case IDC_TEXERII_BEAT:
                        g_ConfigThis->code.beat.set(buf, l + 1);
                        break;
                    case IDC_TEXERII_POINT:
                        g_ConfigThis->code.point.set(buf, l + 1);
                        break;
                    default:
                        break;
                }
                delete buf;
            }
            return 1;
        }

        case WM_INITDIALOG: {
            WIN32_FIND_DATA wfd;
            HANDLE h;

            wsprintf(buf, "%s\\*.bmp", g_path);

            bool found = false;
            SendDlgItemMessage(hwndDlg,
                               IDC_TEXERII_TEXTURE,
                               CB_ADDSTRING,
                               0,
                               (LPARAM) "(default image)");
            h = FindFirstFile(buf, &wfd);
            if (h != INVALID_HANDLE_VALUE) {
                bool rep = true;
                while (rep) {
                    int p = SendDlgItemMessage(hwndDlg,
                                               IDC_TEXERII_TEXTURE,
                                               CB_ADDSTRING,
                                               0,
                                               (LPARAM)wfd.cFileName);
                    if (stricmp(wfd.cFileName, g_ConfigThis->config.img) == 0) {
                        SendDlgItemMessage(
                            hwndDlg, IDC_TEXERII_TEXTURE, CB_SETCURSEL, p, 0);
                        found = true;
                    }
                    if (!FindNextFile(h, &wfd)) {
                        rep = false;
                    }
                };
                FindClose(h);
            }
            if (!found)
                SendDlgItemMessage(hwndDlg, IDC_TEXERII_TEXTURE, CB_SETCURSEL, 0, 0);
        }

            SetDlgItemText(hwndDlg, IDC_TEXERII_INIT, g_ConfigThis->code.init.string);
            SetDlgItemText(hwndDlg, IDC_TEXERII_FRAME, g_ConfigThis->code.frame.string);
            SetDlgItemText(hwndDlg, IDC_TEXERII_BEAT, g_ConfigThis->code.beat.string);
            SetDlgItemText(hwndDlg, IDC_TEXERII_POINT, g_ConfigThis->code.point.string);
            CheckDlgButton(hwndDlg, IDC_TEXERII_WRAP, g_ConfigThis->config.wrap);
            CheckDlgButton(hwndDlg, IDC_TEXERII_MASK, g_ConfigThis->config.mask);
            CheckDlgButton(hwndDlg, IDC_TEXERII_RESIZE, g_ConfigThis->config.resize);

            return 1;

        case WM_DESTROY:
            return 1;
    }
    return 0;
}

void load_examples(C_Texer2* texer2, HWND dialog, HWND button) {
    RECT r;
    if (GetWindowRect(button, &r) == 0) {
        return;
    }

    HMENU m = CreatePopupMenu();
    if (m == NULL) {
        return;
    }
    for (int i = 0; i < TEXERII_NUM_EXAMPLES; i++) {
        AppendMenu(
            m, MF_STRING, TEXERII_EXAMPLES_FIRST_ID + i, texer2->examples[i].name);
    }
    int ret = TrackPopupMenu(m, TPM_RETURNCMD, r.left + 1, r.bottom + 1, 0, button, 0);
    if (ret < TEXERII_EXAMPLES_FIRST_ID
        || ret >= (TEXERII_EXAMPLES_FIRST_ID + TEXERII_NUM_EXAMPLES)) {
        return;
    }
    Texer2Example* example = &(texer2->examples[ret - TEXERII_EXAMPLES_FIRST_ID]);
    texer2->code.init.set(example->init, strnlen(example->init, 65534) + 1);
    texer2->code.frame.set(example->frame, strnlen(example->frame, 65534) + 1);
    texer2->code.beat.set(example->beat, strnlen(example->beat, 65534) + 1);
    texer2->code.point.set(example->point, strnlen(example->point, 65534) + 1);
    texer2->config.resize = example->resize;
    texer2->config.wrap = example->wrap;
    texer2->config.mask = example->mask;
    CheckDlgButton(dialog, IDC_TEXERII_WRAP, texer2->config.wrap);
    CheckDlgButton(dialog, IDC_TEXERII_MASK, texer2->config.mask);
    CheckDlgButton(dialog, IDC_TEXERII_RESIZE, texer2->config.resize);
    SetDlgItemText(dialog, IDC_TEXERII_INIT, texer2->code.init.string);
    SetDlgItemText(dialog, IDC_TEXERII_FRAME, texer2->code.frame.string);
    SetDlgItemText(dialog, IDC_TEXERII_BEAT, texer2->code.beat.string);
    SetDlgItemText(dialog, IDC_TEXERII_POINT, texer2->code.point.string);
    // select the default texture image
    SendDlgItemMessage(dialog, IDC_TEXERII_TEXTURE, CB_SETCURSEL, 0, 0);
    texer2->InitTexture();
}
