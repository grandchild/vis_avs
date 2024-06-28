/*
  LICENSE
  -------
Copyright 2005 Nullsoft, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

  * Neither the name of Nullsoft nor the names of its contributors may be used to
    endorse or promote products derived from this software without specific prior
    written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#include "c_transition.h"
#include "e_effectlist.h"
#include "e_unknown.h"

#include "r_defs.h"
#include "g__defs.h"
#include "g__lib.h"

#include "avs_editor.h"
#include "avs_eelif.h"
#include "bpm.h"
#include "c__defs.h"
#include "cfgwin.h"
#include "draw.h"
#include "effect_library.h"
#include "render.h"
#include "resource.h"
#include "undo.h"
#include "vis.h"
#include "wnd.h"

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

static void _do_add(HWND hwnd, HTREEITEM h, Effect* list);
static int treeview_hack;
static HTREEITEM g_hroot;

extern int g_config_smp_mt, g_config_smp;
extern struct winampVisModule* g_mod;
extern int cfg_cancelfs_on_deactivate;

HWND g_debugwnd;

C_GLibrary* g_ui_library = nullptr;

char g_noeffectstr[] = "No effect/setting selected";
// extern char *verstr;
static HWND cur_hwnd;
int is_aux_wnd = 0;
int config_prompt_save_preset = 1;
int g_config_reuseonresize = 1;
// int g_preset_dirty;

extern BOOL CALLBACK aboutProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern int win32_dlgproc_transition(HWND hwndDlg,
                                    UINT uMsg,
                                    WPARAM wParam,
                                    LPARAM lParam);
extern int readyToLoadPreset(HWND parent, int isnew);

extern char* extension(char* fn);

int g_dlg_fps, g_dlg_w, g_dlg_h;

int cfg_cfgwnd_x = 50, cfg_cfgwnd_y = 50, cfg_cfgwnd_open = 0;

int cfg_fs_w = 0, cfg_fs_h = 0, cfg_fs_d = 2, cfg_fs_bpp = 0, cfg_fs_fps = 0,
    cfg_fs_rnd = 1, cfg_fs_flip = 0, cfg_fs_height = 80, cfg_speed = 5,
    cfg_fs_rnd_time = 10, cfg_fs_use_overlay = 0;
int cfg_trans = 0, cfg_trans_amount = 128;
int cfg_dont_min_avs = 0;
int cfg_bkgnd_render = 0, cfg_bkgnd_render_color = 0x1F000F;
int cfg_render_prio = 0;

char config_pres_subdir[MAX_PATH];
char last_preset[2048];

static BOOL CALLBACK dlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
HWND g_hwndDlg;

#ifdef WA2_EMBED
#include "wa_ipc.h"
extern embedWindowState myWindowState;
#endif

static unsigned int ExtractWindowsVersion() {
    // Get major version number of Windows
    return (DWORD)(LOBYTE(LOWORD(GetVersion())));
}

void CfgWnd_Create(struct winampVisModule* this_mod) {
    g_ui_library = new C_GLibrary();
    CreateDialog(this_mod->hDllInstance,
                 MAKEINTRESOURCE(IDD_DIALOG1),
                 this_mod->hwndParent,
                 dlgProc);
}

void CfgWnd_Destroy() {
    if (g_hwndDlg && IsWindow(g_hwndDlg)) {
        RECT r;
        GetWindowRect(g_hwndDlg, &r);
        cfg_cfgwnd_x = r.left;
        cfg_cfgwnd_y = r.top;
        DestroyWindow(g_hwndDlg);
    }
    g_hwndDlg = nullptr;
    if (g_debugwnd) {
        DestroyWindow(g_debugwnd);
    }
    /*
    if (hcfgThread)
    {
            SendMessage(g_hwndDlg,WM_USER+6,0,0);
            g_hwndDlg=0;
      WaitForSingleObject(hcfgThread,INFINITE);
      CloseHandle(hcfgThread);
      hcfgThread=0;
    }
    */
    delete g_ui_library;
}

static void recursive_add_dir_list(HMENU menu, UINT* id, char* path, int pathlen) {
    HANDLE h;
    WIN32_FIND_DATA d;
    char dirmask[4096];
    wsprintf(dirmask, "%s\\*.*", path);

    h = FindFirstFile(dirmask, &d);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (d.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY
                && d.cFileName[0] != '.') {
                wsprintf(dirmask, "%s\\%s", path, d.cFileName);

                MENUITEMINFO i = {};
                i.cbSize = sizeof(i);
                i.fMask = MIIM_TYPE | MIIM_ID;
                i.fType = MFT_STRING;
                i.wID = *id;
                i.dwTypeData = dirmask + pathlen + 1;
                i.cch = strlen(i.dwTypeData);
                InsertMenuItem(menu, *id + 2 - 1025, true, &i);
                (*id)++;

                recursive_add_dir_list(menu, id, dirmask, pathlen);
            }
        } while (FindNextFile(h, &d));
        FindClose(h);
    }
}

static BOOL CALLBACK DlgProc_Preset(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    switch (uMsg) {
        case WM_INITDIALOG: {
            int x;
            for (x = 0; x < 12; x++) {
                char s[123];
                wsprintf(s, "F%d", x + 1);
                SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)s);
            }
            for (x = 0; x < 10; x++) {
                char s[123];
                wsprintf(s, "%d", x);
                SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)s);
            }
            for (x = 0; x < 10; x++) {
                char s[123];
                wsprintf(s, "Shift+%d", x);
                SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)s);
            }

            CheckDlgButton(
                hwndDlg, IDC_CHECK3, cfg_fs_rnd ? BST_CHECKED : BST_UNCHECKED);
            SetDlgItemInt(hwndDlg, IDC_EDIT1, cfg_fs_rnd_time, false);
            if (config_prompt_save_preset) {
                CheckDlgButton(hwndDlg, IDC_CHECK1, BST_CHECKED);
            }

            if (config_pres_subdir[0]) {
                SetDlgItemText(hwndDlg, IDC_BUTTON3, config_pres_subdir);
            } else {
                SetDlgItemText(hwndDlg, IDC_BUTTON3, "All");
            }
            return 1;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_BUTTON3: {
                    MENUITEMINFO i = {};
                    i.cbSize = sizeof(i);

                    HMENU hMenu;
                    hMenu = CreatePopupMenu();
                    i.fMask = MIIM_TYPE | MIIM_ID;
                    i.fType = MFT_STRING;
                    i.wID = 1024;
                    i.dwTypeData = "All";
                    i.cch = strlen("All");
                    InsertMenuItem(hMenu, 0, true, &i);
                    i.wID = 0;
                    i.fType = MFT_SEPARATOR;
                    InsertMenuItem(hMenu, 1, true, &i);

                    UINT id = 1025;
                    recursive_add_dir_list(hMenu, &id, g_path, strlen(g_path));

                    RECT r;
                    GetWindowRect(GetDlgItem(hwndDlg, IDC_BUTTON3), &r);

                    int x = TrackPopupMenu(hMenu,
                                           TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD
                                               | TPM_RIGHTBUTTON | TPM_LEFTBUTTON
                                               | TPM_NONOTIFY,
                                           r.left,
                                           r.bottom,
                                           0,
                                           hwndDlg,
                                           nullptr);
                    if (x == 1024) {
                        config_pres_subdir[0] = 0;
                        SetDlgItemText(hwndDlg, IDC_BUTTON3, "All");
                    } else if (x >= 1025) {
                        MENUITEMINFO mi = {};
                        mi.cbSize = sizeof(mi);
                        mi.fMask = MIIM_TYPE;
                        mi.dwTypeData = config_pres_subdir;
                        mi.cch = sizeof(config_pres_subdir);
                        GetMenuItemInfo(hMenu, x, false, &mi);
                        SetDlgItemText(hwndDlg, IDC_BUTTON3, config_pres_subdir);
                    }
                    DestroyMenu(hMenu);
                }
                    return 0;
                case IDC_CHECK1:
                    config_prompt_save_preset =
                        IsDlgButtonChecked(hwndDlg, IDC_CHECK1) ? 1 : 0;
                    return 0;
                case IDC_CHECK3:
                    cfg_fs_rnd = IsDlgButtonChecked(hwndDlg, IDC_CHECK3) ? 1 : 0;
#ifdef WA2_EMBED
                    SendMessage(
                        g_mod->hwndParent, WM_WA_IPC, cfg_fs_rnd, IPC_CB_VISRANDOM);
#endif
                    return 0;
                case IDC_EDIT1:
                    if (HIWORD(wParam) == EN_CHANGE) {
                        BOOL f;
                        int r = GetDlgItemInt(hwndDlg, IDC_EDIT1, &f, 0);
                        if (f) {
                            cfg_fs_rnd_time = r;
                        }
                    }
                    return 0;
                case IDC_BUTTON1: {
                    int w = SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0);
                    if (w != CB_ERR) {
                        extern void WritePreset(int preset);
                        WritePreset(w);
                    }
                }
                    return 0;
                case IDC_BUTTON2:
                    if (readyToLoadPreset(hwndDlg, 0)) {
                        int w =
                            SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0);
                        if (w != CB_ERR) {
                            extern int LoadPreset(int preset);
                            LoadPreset(w);
                        }
                    }
                    return 0;
            }
    }
    return 0;
}

static BOOL CALLBACK DlgProc_Disp(HWND hwndDlg,
                                  UINT uMsg,
                                  WPARAM wParam,
                                  LPARAM lParam) {
    extern void GetClientRect_adj(HWND hwnd, RECT * r);
    extern void main_setRenderThreadPriority();
    switch (uMsg) {
        case WM_INITDIALOG: {
            if (ExtractWindowsVersion() < 5) {
                EnableWindow(GetDlgItem(hwndDlg, IDC_TRANS_CHECK), 0);
                EnableWindow(GetDlgItem(hwndDlg, IDC_TRANS_SLIDER), 0);
            } else {
                CheckDlgButton(
                    hwndDlg, IDC_TRANS_CHECK, cfg_trans ? BST_CHECKED : BST_UNCHECKED);
                SendDlgItemMessage(hwndDlg,
                                   IDC_TRANS_SLIDER,
                                   TBM_SETRANGE,
                                   (WPARAM) true,
                                   (LPARAM)MAKELONG(16, 255));
                SendDlgItemMessage(
                    hwndDlg, IDC_TRANS_SLIDER, TBM_SETTICFREQ, 10, (LPARAM)0);
                SendDlgItemMessage(hwndDlg,
                                   IDC_TRANS_SLIDER,
                                   TBM_SETPOS,
                                   (WPARAM) true,
                                   (LPARAM)cfg_trans_amount);
            }
            CheckDlgButton(hwndDlg, IDC_CHECK4, g_config_smp ? BST_CHECKED : 0);
            SetDlgItemInt(hwndDlg, IDC_EDIT1, g_config_smp_mt, false);
        }
#ifdef WA2_EMBED
            {
                HWND w = myWindowState.me;
                while (GetWindowLong(w, GWL_STYLE) & WS_CHILD) {
                    w = GetParent(w);
                }
                char classname[256];
                GetClassName(w, classname, 255);
                classname[255] = 0;
                if (!stricmp(classname, "BaseWindow_RootWnd")) {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_STATIC_ALPHA), false);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_TRANS_CHECK), false);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_TRANS_SLIDER), false);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_STATIC_TRANS_TOTAL), false);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_STATIC_TRANS_NONE), false);
                }
            }
#endif
            CheckDlgButton(
                hwndDlg, IDC_CHECK1, (cfg_fs_d & 2) ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(
                hwndDlg, IDC_CHECK6, (cfg_fs_flip & 4) ? BST_UNCHECKED : BST_CHECKED);
            CheckDlgButton(
                hwndDlg, IDC_CHECK3, (cfg_fs_fps & 2) ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(
                hwndDlg, IDC_CHECK5, (cfg_fs_fps & 4) ? BST_CHECKED : BST_UNCHECKED);
            //			CheckDlgButton(hwndDlg,IDC_DONT_MIN_AVS,cfg_dont_min_avs?BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hwndDlg,
                           IDC_CHECK2,
                           g_config_reuseonresize ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwndDlg,
                           IDC_BKGND_RENDER,
                           (cfg_bkgnd_render & 1) ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwndDlg,
                           IDC_SETDESKTOPCOLOR,
                           (cfg_bkgnd_render & 2) ? BST_CHECKED : BST_UNCHECKED);
            SendDlgItemMessage(hwndDlg, IDC_SLIDER1, TBM_SETRANGEMIN, 0, 0);
            SendDlgItemMessage(hwndDlg, IDC_SLIDER1, TBM_SETRANGEMAX, 0, 80);
            SendDlgItemMessage(hwndDlg, IDC_SLIDER1, TBM_SETPOS, 1, cfg_speed & 0xff);

            SendDlgItemMessage(
                hwndDlg, IDC_THREAD_PRIORITY, CB_ADDSTRING, 0, (int)"Same as winamp");
            SendDlgItemMessage(
                hwndDlg, IDC_THREAD_PRIORITY, CB_ADDSTRING, 0, (int)"Idle");
            SendDlgItemMessage(
                hwndDlg, IDC_THREAD_PRIORITY, CB_ADDSTRING, 0, (int)"Lowest");
            SendDlgItemMessage(
                hwndDlg, IDC_THREAD_PRIORITY, CB_ADDSTRING, 0, (int)"Normal");
            SendDlgItemMessage(
                hwndDlg, IDC_THREAD_PRIORITY, CB_ADDSTRING, 0, (int)"Highest");
            SendDlgItemMessage(
                hwndDlg, IDC_THREAD_PRIORITY, CB_SETCURSEL, cfg_render_prio, 0);
            return 1;
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* di = (DRAWITEMSTRUCT*)lParam;
            switch (di->CtlID) {
                case IDC_OVERLAYCOLOR:
                    GR_DrawColoredButton(di, cfg_bkgnd_render_color);
                    break;
            }
            return 0;
        }
        case WM_HSCROLL: {
            HWND swnd = (HWND)lParam;
            int t = (int)SendMessage(swnd, TBM_GETPOS, 0, 0);
            if (swnd == GetDlgItem(hwndDlg, IDC_SLIDER1)) {
                cfg_speed &= ~0xff;
                cfg_speed |= t;
            }
            if (swnd == GetDlgItem(hwndDlg, IDC_TRANS_SLIDER)) {
                cfg_trans_amount = t;
                SetTransparency(g_hwnd, cfg_trans, cfg_trans_amount);
            }
            return 0;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_CHECK4:
                    g_config_smp = !!IsDlgButtonChecked(hwndDlg, IDC_CHECK4);
                    return 0;
                case IDC_EDIT1: {
                    BOOL t;
                    g_config_smp_mt = GetDlgItemInt(hwndDlg, IDC_EDIT1, &t, false);
                }
                    return 0;
                case IDC_TRANS_CHECK:
                    cfg_trans = IsDlgButtonChecked(hwndDlg, IDC_TRANS_CHECK) ? 1 : 0;
                    SetTransparency(g_hwnd, cfg_trans, cfg_trans_amount);
                    return 0;
                case IDC_CHECK1:
                    cfg_fs_d &= ~2;
                    cfg_fs_d |= IsDlgButtonChecked(hwndDlg, IDC_CHECK1) ? 2 : 0;
                    {
                        RECT r;
                        GetClientRect_adj(g_hwnd, &r);
                        DDraw_Resize(r.right, r.bottom, cfg_fs_d & 2);
                    }

                    return 0;
                case IDC_CHECK6:
                    cfg_fs_flip &= ~4;
                    cfg_fs_flip |= IsDlgButtonChecked(hwndDlg, IDC_CHECK6) ? 0 : 4;
                    return 0;
                case IDC_CHECK3:
                    cfg_fs_fps &= ~2;
                    cfg_fs_fps |= IsDlgButtonChecked(hwndDlg, IDC_CHECK3) ? 2 : 0;
                    return 0;
                case IDC_CHECK5:
                    cfg_fs_fps &= ~4;
                    cfg_fs_fps |= IsDlgButtonChecked(hwndDlg, IDC_CHECK5) ? 4 : 0;
                    return 0;
                case IDC_CHECK2:
                    g_config_reuseonresize = !!IsDlgButtonChecked(hwndDlg, IDC_CHECK2);
                    return 0;
                    // case IDC_DONT_MIN_AVS:
                    // cfg_dont_min_avs=IsDlgButtonChecked(hwndDlg,IDC_DONT_MIN_AVS)?1:0;
                case IDC_DEFOVERLAYCOLOR:
                    cfg_bkgnd_render_color = 0x1F000F;
                    InvalidateRect(
                        GetDlgItem(hwndDlg, IDC_OVERLAYCOLOR), nullptr, false);
                    goto update_overlayshit;
                case IDC_OVERLAYCOLOR:
                    GR_SelectColor(hwndDlg, &cfg_bkgnd_render_color);
                    InvalidateRect(
                        GetDlgItem(hwndDlg, IDC_OVERLAYCOLOR), nullptr, false);
                    goto update_overlayshit;
                case IDC_SETDESKTOPCOLOR:
                case IDC_BKGND_RENDER:
                    cfg_bkgnd_render =
                        (IsDlgButtonChecked(hwndDlg, IDC_BKGND_RENDER) ? 1 : 0)
                        | (IsDlgButtonChecked(hwndDlg, IDC_SETDESKTOPCOLOR) ? 2 : 0);
                update_overlayshit: {
                    RECT r;
                    GetClientRect_adj(g_hwnd, &r);
                    DDraw_Resize(r.right, r.bottom, cfg_fs_d & 2);
                    return 0;
                }
                case IDC_THREAD_PRIORITY:
                    cfg_render_prio = SendDlgItemMessage(
                        hwndDlg, IDC_THREAD_PRIORITY, CB_GETCURSEL, 0, 0);
                    main_setRenderThreadPriority();
                    return 0;
            }
            return 0;
    }
    return 0;
}

static void enableFSWindows(HWND hwndDlg, int v) {
    EnableWindow(GetDlgItem(hwndDlg, IDC_BPP_CONV), v);
    // EnableWindow(GetDlgItem(hwndDlg,IDC_EDIT1),v);
    EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK4), v);
    EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK2), v);
    EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK3), v);
    EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK5), v);
}

extern int cfg_fs_dblclk;

static BOOL CALLBACK DlgProc_FS(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG: {
            int l;
            DDraw_EnumDispModes(GetDlgItem(hwndDlg, IDC_COMBO1));
            l = SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_GETCOUNT, 0, 0);
            if (l < 1 || l == CB_ERR) {
                SendDlgItemMessage(hwndDlg,
                                   IDC_COMBO1,
                                   CB_ADDSTRING,
                                   0,
                                   (long)"no suitable modes found");
                SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_SETCURSEL, 0, 0);
            } else {
                int x;
                for (x = 0; x < l; x++) {
                    char b[256], *p = b;
                    SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_GETLBTEXT, x, (long)b);
                    int w, h, bpp;
                    w = atoi(p);
                    while (*p >= '0' && *p <= '9') {
                        p++;
                    }
                    if (!*p) {
                        continue;
                    }
                    h = atoi(++p);
                    while (*p >= '0' && *p <= '9') {
                        p++;
                    }
                    if (!*p) {
                        continue;
                    }
                    bpp = atoi(++p);
                    if (w == cfg_fs_w && h == cfg_fs_h && bpp == cfg_fs_bpp) {
                        break;
                    }
                }
                if (x != l) {
                    SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_SETCURSEL, x, 0);
                }
            }
            CheckDlgButton(hwndDlg,
                           IDC_USE_OVERLAY,
                           cfg_fs_use_overlay ? BST_CHECKED : BST_UNCHECKED);
            enableFSWindows(hwndDlg, !cfg_fs_use_overlay);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK8), cfg_fs_use_overlay);
            CheckDlgButton(
                hwndDlg, IDC_CHECK1, (cfg_fs_d & 1) ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(
                hwndDlg, IDC_CHECK2, (cfg_fs_fps & 1) ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(
                hwndDlg, IDC_CHECK3, (cfg_fs_fps & 8) ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(
                hwndDlg, IDC_CHECK5, (cfg_fs_fps & 16) ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(
                hwndDlg, IDC_CHECK4, (cfg_fs_flip & 1) ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(
                hwndDlg, IDC_CHECK7, (cfg_fs_flip & 2) ? BST_UNCHECKED : BST_CHECKED);
            CheckDlgButton(hwndDlg,
                           IDC_CHECK8,
                           (cfg_cancelfs_on_deactivate) ? BST_UNCHECKED : BST_CHECKED);
            CheckDlgButton(
                hwndDlg, IDC_BPP_CONV, (cfg_fs_flip & 8) ? BST_UNCHECKED : BST_CHECKED);
            SetDlgItemInt(hwndDlg, IDC_EDIT1, cfg_fs_height, false);
            SendDlgItemMessage(hwndDlg, IDC_SLIDER1, TBM_SETRANGEMIN, 0, 0);
            SendDlgItemMessage(hwndDlg, IDC_SLIDER1, TBM_SETRANGEMAX, 0, 80);
            SendDlgItemMessage(
                hwndDlg, IDC_SLIDER1, TBM_SETPOS, 1, (cfg_speed >> 8) & 0xff);
            CheckDlgButton(
                hwndDlg, IDC_CHECK6, cfg_fs_dblclk ? BST_CHECKED : BST_UNCHECKED);
        }
            return 1;
        case WM_HSCROLL: {
            HWND swnd = (HWND)lParam;
            int t = (int)SendMessage(swnd, TBM_GETPOS, 0, 0);
            if (swnd == GetDlgItem(hwndDlg, IDC_SLIDER1)) {
                cfg_speed &= ~0xff00;
                cfg_speed |= (t << 8);
            }
        }
            return 0;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_BUTTON1:
                    if (IsDlgButtonChecked(hwndDlg, IDC_USE_OVERLAY)
                        || DDraw_IsMode(cfg_fs_w, cfg_fs_h, cfg_fs_bpp)) {
                        SetForegroundWindow(g_hwnd);
                        PostMessage(g_hwnd, WM_USER + 32, 0, 0);
                    } else {
                        MessageBox(hwndDlg, "Choose a video mode", "Fullscreen", MB_OK);
                    }
                    return 0;
                case IDC_BPP_CONV:
                    cfg_fs_flip &= ~8;
                    cfg_fs_flip |= IsDlgButtonChecked(hwndDlg, IDC_BPP_CONV) ? 0 : 8;
                    return 0;
                case IDC_CHECK6:
                    cfg_fs_dblclk = !!IsDlgButtonChecked(hwndDlg, IDC_CHECK6);
                    return 0;
                case IDC_CHECK1:
                    cfg_fs_d &= ~1;
                    cfg_fs_d |= IsDlgButtonChecked(hwndDlg, IDC_CHECK1) ? 1 : 0;
                    return 0;
                case IDC_CHECK2:
                    cfg_fs_fps &= ~1;
                    cfg_fs_fps |= IsDlgButtonChecked(hwndDlg, IDC_CHECK2) ? 1 : 0;
                    return 0;
                case IDC_CHECK3:
                    cfg_fs_fps &= ~8;
                    cfg_fs_fps |= IsDlgButtonChecked(hwndDlg, IDC_CHECK3) ? 8 : 0;
                    return 0;
                case IDC_CHECK5:
                    cfg_fs_fps &= ~16;
                    cfg_fs_fps |= IsDlgButtonChecked(hwndDlg, IDC_CHECK5) ? 16 : 0;
                    return 0;
                case IDC_CHECK4:
                    cfg_fs_flip &= ~1;
                    cfg_fs_flip |= IsDlgButtonChecked(hwndDlg, IDC_CHECK4) ? 1 : 0;
                    return 0;
                case IDC_CHECK7:
                    cfg_fs_flip &= ~2;
                    cfg_fs_flip |= IsDlgButtonChecked(hwndDlg, IDC_CHECK7) ? 0 : 2;
                    return 0;
                case IDC_CHECK8:
                    cfg_cancelfs_on_deactivate =
                        IsDlgButtonChecked(hwndDlg, IDC_CHECK8) ? 0 : 1;
                    return 0;
                case IDC_EDIT1:
                    if (HIWORD(wParam) == EN_CHANGE) {
                        BOOL t;
                        int r = GetDlgItemInt(hwndDlg, IDC_EDIT1, &t, false);
                        if (r > 0 && r <= 100 && t) {
                            cfg_fs_height = r;
                        }
                    }
                    return 0;
                case IDC_COMBO1:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        int bps = -1;
                        char b[256], *p = b;
                        int l =
                            SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0);
                        if (l == CB_ERR) {
                            return 0;
                        }
                        SendDlgItemMessage(
                            hwndDlg, IDC_COMBO1, CB_GETLBTEXT, l, (long)b);
                        int w, h;
                        while (*p >= '0' && *p <= '9') {
                            p++;
                        }
                        if (!*p) {
                            return 0;
                        }
                        *p++ = 0;
                        w = atoi(b);
                        while (*p < '0' && *p > '9' && *p) {
                            p++;
                        }
                        h = atoi(p);
                        while (*p >= '0' && *p <= '9') {
                            p++;
                        }
                        if (!*p) {
                            return 0;
                        }
                        p++;
                        bps = atoi(p);
                        if (w < 1 || h < 1 || bps < 1) {
                            return 0;
                        }
                        cfg_fs_h = h;
                        cfg_fs_w = w;
                        cfg_fs_bpp = bps;
                    }
                    return 0;
                case IDC_USE_OVERLAY:
                    cfg_fs_use_overlay =
                        IsDlgButtonChecked(hwndDlg, IDC_USE_OVERLAY) ? 1 : 0;
                    enableFSWindows(hwndDlg, !cfg_fs_use_overlay);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK8), cfg_fs_use_overlay);
                    return 0;
            }
            return 0;
    }
    return 0;
}

static void _insertintomenu2(HMENU hMenu, int wid, Effect_Info* id, char* str) {
    int x;
    for (x = 0; x < 4096; x++) {
        MENUITEMINFO mi = {};
        mi.cbSize = sizeof(mi);
        mi.fMask = MIIM_DATA | MIIM_TYPE | MIIM_SUBMENU;
        mi.fType = MFT_STRING;
        char c[512];
        mi.dwTypeData = c;
        mi.cch = 512;
        if (!GetMenuItemInfo(hMenu, x, true, &mi)) {
            break;
        }
        if (strcmp(str, c) < 0 && !mi.hSubMenu) {
            break;
        }
    }

    MENUITEMINFO i = {};
    i.cbSize = sizeof(i);
    i.fMask = MIIM_TYPE | MIIM_DATA | MIIM_ID;
    i.fType = MFT_STRING;
    i.wID = wid;
    i.dwItemData = (uint32_t)id;
    i.dwTypeData = str;
    i.cch = strlen(str);
    InsertMenuItem(hMenu, x, true, &i);
}

static HMENU _findsubmenu(HMENU hmenu, char* str) {
    int x;
    for (x = 0; x < 4096; x++) {
        MENUITEMINFO mi = {};
        mi.cbSize = sizeof(mi);
        mi.fMask = MIIM_DATA | MIIM_TYPE | MIIM_SUBMENU;
        mi.fType = MFT_STRING;
        char c[512];
        mi.dwTypeData = c;
        mi.cch = 512;
        if (!GetMenuItemInfo(hmenu, x, true, &mi)) {
            break;
        }
        if (!strcmp(str, c) && mi.hSubMenu) {
            return mi.hSubMenu;
        }
    }
    return nullptr;
}

static void _insertintomenu(HMENU hMenu, int wid, Effect_Info* id, char* str) {
    char ostr[1024];
    strncpy(ostr, str, 1023);
    char* first = str;
    char* second = str;
    while (*second && *second != '/') {
        second++;
    }
    if (*second) {
        *second++ = 0;
    }
    if (*second) {
        while (*second == ' ' || *second == '/') {
            second++;
        }
        if (*second) {
            HMENU hs;

            if (!(hs = _findsubmenu(hMenu, first))) {
                MENUITEMINFO i = {};
                i.cbSize = sizeof(i);
                i.fMask = MIIM_TYPE | MIIM_SUBMENU | MIIM_DATA | MIIM_ID;
                i.fType = MFT_STRING;
                i.dwTypeData = first;
                i.cch = strlen(first);
                i.hSubMenu = hs = CreatePopupMenu();
                i.wID = 0;
                InsertMenuItem(hMenu, 0, true, &i);
            }
            _insertintomenu2(hs, wid, id, second);
            return;
        }
    }
    _insertintomenu2(hMenu, wid, id, ostr);
}
static HTREEITEM g_drag_source, g_dragsource_parent, g_drag_last_dest,
    g_drag_placeholder;
static AVS_Component_Position g_drag_insert_direction;
extern int findInMenu(HMENU parent, HMENU sub, UINT id, char* buf, int buf_len);

#define UNDO_TIMER_INTERVAL 333

static WNDPROC sniffConfigWindow_oldProc;
static BOOL CALLBACK sniffConfigWindow_newProc(HWND hwndDlg,
                                               UINT uMsg,
                                               WPARAM wParam,
                                               LPARAM lParam) {
    bool dirty = false;
    if (uMsg == WM_COMMAND || uMsg == WM_HSCROLL || uMsg == WM_VSCROLL) {
        if (uMsg != WM_COMMAND || HIWORD(wParam) == EN_CHANGE
            || HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == LBN_SELCHANGE
            || HIWORD(wParam) == CBN_SELCHANGE

        ) {
            dirty = true;
        }
    }

    BOOL retval =
        CallWindowProc(sniffConfigWindow_oldProc, hwndDlg, uMsg, wParam, lParam);

    // Don't save the new state until the window proc handles it.  :)
    if (dirty) {
        KillTimer(GetParent(hwndDlg), 69);
        SetTimer(GetParent(hwndDlg), 69, UNDO_TIMER_INTERVAL, nullptr);

        //      g_preset_dirty=1;
    }

    return retval;
}

int dosavePreset(HWND hwndDlg) {
    int r = 1;
    char temp[2048];
    OPENFILENAME l = {};
    l.lStructSize = sizeof(l);
    char buf1[2048], buf2[2048];
    temp[0] = 0;
    GetCurrentDirectory(sizeof(buf2), buf2);
    strcpy(buf1, g_path);
    l.hwndOwner = hwndDlg;
    l.lpstrFilter = "AVS presets\0*.avs\0All files\0*.*\0";
    l.lpstrFile = temp;
    strcpy(temp, last_preset);
    l.nMaxFile = 2048 - 1;
    l.lpstrTitle = "Save Preset";
    l.lpstrDefExt = "AVS";
    l.lpstrInitialDir = buf1;
    l.Flags = OFN_HIDEREADONLY | OFN_EXPLORER | OFN_OVERWRITEPROMPT;
    if (GetSaveFileName(&l)) {
        strcpy(last_preset, temp);
        r = g_single_instance->preset_save_file_legacy(temp);
        if (r == 1) {
            MessageBox(hwndDlg, "Error saving preset", "Save Preset", MB_OK);
        } else if (r == 2) {
            MessageBox(hwndDlg, "Preset too large", "Save Preset", MB_OK);
        } else if (r == -1) {
            MessageBox(hwndDlg, "Out of memory", "Save Preset", MB_OK);
        } else {
            C_UndoStack::clear_dirty();
            // g_preset_dirty=0;
        }
    }
    SetCurrentDirectory(buf2);
    return r;
}

extern int g_config_seh;

static BOOL CALLBACK debugProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    extern int debug_reg[8];
    switch (uMsg) {
        case WM_INITDIALOG: {
            int x;
            for (x = 0; x < 8; x++) {
                SetDlgItemInt(hwndDlg, IDC_DEBUGREG_1 + x * 2, debug_reg[x], false);
            }
            SetTimer(hwndDlg, 1, 250, nullptr);
        }
            if (g_log_errors) {
                CheckDlgButton(hwndDlg, IDC_CHECK1, BST_CHECKED);
            }
            if (g_reset_vars_on_recompile) {
                CheckDlgButton(hwndDlg, IDC_CHECK2, BST_CHECKED);
            }
            if (!g_config_seh) {
                CheckDlgButton(hwndDlg, IDC_CHECK3, BST_CHECKED);
            }

            return 0;
        case WM_TIMER:
            if (wParam == 1) {
                int x;
                for (x = 0; x < 8; x++) {
                    char buf[128];
                    int v = debug_reg[x];
                    if (v >= 0 && v < 100) {
                        sprintf(buf, "%.14f", NSEEL_getglobalregs()[v]);
                    } else {
                        strcpy(buf, "?");
                    }
                    SetDlgItemText(hwndDlg, IDC_DEBUGREG_1 + x * 2 + 1, buf);
                }

                if (g_log_errors) {
                    // IDC_EDIT1
                    lock_lock(g_eval_cs);
                    char buf[1025];
                    GetDlgItemText(hwndDlg, IDC_EDIT1, buf, sizeof(buf) - 1);
                    buf[sizeof(buf) - 1] = 0;
                    if (strncmp(buf, last_error_string, sizeof(buf)) != 0) {
                        SetDlgItemText(hwndDlg, IDC_EDIT1, last_error_string);
                    }
                    lock_unlock(g_eval_cs);
                }

                {
                    char buf[512];
                    int* g_evallib_stats = NSEEL_getstats();
                    wsprintf(buf,
                             "Eval code stats: %d segments, %d bytes source, %d+%d "
                             "bytes code, %d bytes data",
                             g_evallib_stats[4],
                             g_evallib_stats[0],
                             g_evallib_stats[1],
                             g_evallib_stats[2],
                             g_evallib_stats[3]);
                    SetDlgItemText(hwndDlg, IDC_EDIT2, buf);
                }
            }
            return 0;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_CHECK1:
                    g_log_errors = !!IsDlgButtonChecked(hwndDlg, IDC_CHECK1);
                    return 0;
                case IDC_CHECK2:
                    g_reset_vars_on_recompile =
                        !!IsDlgButtonChecked(hwndDlg, IDC_CHECK2);
                    return 0;
                case IDC_CHECK3:
                    g_config_seh = !IsDlgButtonChecked(hwndDlg, IDC_CHECK3);
                    return 0;
                case IDC_BUTTON1:
                    lock_lock(g_eval_cs);
                    last_error_string[0] = 0;
                    SetDlgItemText(hwndDlg, IDC_EDIT1, "");
                    lock_unlock(g_eval_cs);
                    return 0;
                case IDOK:
                case IDCANCEL: DestroyWindow(hwndDlg); return 0;
                default:
                    if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) >= IDC_DEBUGREG_1
                        && LOWORD(wParam) <= IDC_DEBUGREG_16) {
                        int x = LOWORD(wParam) - IDC_DEBUGREG_1;
                        if (!(x & 1)) {
                            x /= 2;
                            if (x > 7) {
                                x = 7;
                            }
                            BOOL t;
                            int v = GetDlgItemInt(
                                hwndDlg, IDC_DEBUGREG_1 + x * 2, &t, false);
                            if (t) {
                                debug_reg[x] = v;
                            }
                        }
                    }
                    break;
            }
            return 0;
        case WM_DESTROY: g_debugwnd = 0; return 0;
    }
    return 0;
}

static BOOL CALLBACK dlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HMENU presetTreeMenu;
    static int presetTreeCount;
    extern int need_redock;
    extern int inWharf;
    extern void toggleWharfAmpDock(HWND hwnd);

    switch (uMsg) {
        case WM_SHOWWINDOW: ShowWindow(cur_hwnd, wParam ? SW_SHOW : SW_HIDE); break;
        case WM_INITMENU:
            EnableMenuItem(
                (HMENU)wParam,
                IDM_UNDO,
                MF_BYCOMMAND | (C_UndoStack::can_undo() ? MF_ENABLED : MF_GRAYED));
            EnableMenuItem(
                (HMENU)wParam,
                IDM_REDO,
                MF_BYCOMMAND | (C_UndoStack::can_redo() ? MF_ENABLED : MF_GRAYED));
            return 0;
        case WM_INITMENUPOPUP:
            if (!HIWORD(lParam) && presetTreeMenu && !GetMenuItemCount((HMENU)wParam)) {
                char buf[2048];
                buf[0] = 0;
                if (findInMenu(presetTreeMenu, (HMENU)wParam, 0, buf, 2048)) {
                    HANDLE h;
                    WIN32_FIND_DATA d;
                    char dirmask[4096];
                    wsprintf(dirmask, "%s%s\\*.*", g_path, buf);
                    int directory_pos = 0, insert_pos = 0;
                    // build menu
                    h = FindFirstFile(dirmask, &d);
                    if (h != INVALID_HANDLE_VALUE) {
                        do {
                            if (d.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY
                                && d.cFileName[0] != '.') {
                                MENUITEMINFO mi = {};
                                mi.cbSize = sizeof(mi);
                                mi.fMask = MIIM_SUBMENU | MIIM_TYPE;
                                mi.fType = MFT_STRING;
                                mi.fState = MFS_DEFAULT;
                                mi.hSubMenu = CreatePopupMenu();
                                mi.dwTypeData = d.cFileName;
                                mi.cch = strlen(d.cFileName);
                                InsertMenuItem(
                                    (HMENU)wParam, directory_pos++, true, &mi);
                                insert_pos++;
                            } else if (!stricmp(extension(d.cFileName), "avs")) {
                                extension(d.cFileName)[-1] = 0;
                                MENUITEMINFO i = {};
                                i.cbSize = sizeof(i);
                                i.fMask = MIIM_TYPE | MIIM_DATA | MIIM_ID;
                                i.fType = MFT_STRING;
                                i.fState = MFS_DEFAULT;
                                i.wID = presetTreeCount++;
                                i.dwItemData = 0xFFFFFFFF;  // preset
                                i.dwTypeData = d.cFileName;
                                i.cch = strlen(d.cFileName);
                                InsertMenuItem((HMENU)wParam, insert_pos++, true, &i);
                            }
                        } while (FindNextFile(h, &d));
                        FindClose(h);
                    }
                }
            }
            return 0;
        case WM_USER + 20:
            CfgWnd_Unpopulate();
            g_single_instance->transition.clean_prev_renders_if_needed();
            CfgWnd_Populate();
            return 0;
        case WM_CLOSE:
            if (inWharf) {
#ifdef WA2_EMBED
                toggleWharfAmpDock(g_hwnd);
            }
#else
                PostMessage(g_hwnd, WM_CLOSE, 0, 0);
            } else
#endif
            {
                cfg_cfgwnd_open = 0;
                ShowWindow(hwndDlg, SW_HIDE);
            }
            return 0;
        case WM_DESTROY: return 0;
        case WM_INITDIALOG: {
            g_hwndDlg = hwndDlg;
            // SetDlgItemText(hwndDlg,IDC_AVS_VER,verstr);
        }
            TreeView_SetIndent(GetDlgItem(hwndDlg, IDC_TREE1), 8);
            SetTimer(hwndDlg, 1, 250, nullptr);
            if (cfg_cfgwnd_open) {
                ShowWindow(hwndDlg, SW_SHOWNA);
            }
            CfgWnd_Populate();
            SetWindowPos(hwndDlg,
                         nullptr,
                         cfg_cfgwnd_x,
                         cfg_cfgwnd_y,
                         0,
                         0,
                         SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
            if (need_redock) {
                need_redock = 0;
                toggleWharfAmpDock(g_hwnd);
            } else {
                RECT r, r2;
                // hide IDC_RRECT, resize IDC_TREE1 up
                GetWindowRect(GetDlgItem(g_hwndDlg, IDC_RRECT), &r);
                GetWindowRect(GetDlgItem(g_hwndDlg, IDC_TREE1), &r2);
                ShowWindow(GetDlgItem(g_hwndDlg, IDC_RRECT), SW_HIDE);
                SetWindowPos(GetDlgItem(g_hwndDlg, IDC_TREE1),
                             nullptr,
                             0,
                             0,
                             r2.right - r2.left,
                             r.bottom - r2.top - 2,
                             SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
            return true;
        case WM_TIMER:
            if (wParam == 1) {
                char s[1024];
                char* tp = last_preset;
                while (*tp) {
                    tp++;
                }
                while (tp >= last_preset && *tp != '\\') {
                    tp--;
                }
                tp++;
                wsprintf(s,
                         "%d.%d FPS @ %dx%d%s%s",
                         g_dlg_fps / 10,
                         g_dlg_fps % 10,
                         g_dlg_w,
                         g_dlg_h,
                         *tp ? " - " : ".",
                         tp);
                tp = s;
                while (*tp) {
                    tp++;
                }
                while (tp > s && *tp != '.' && *tp != '-') {
                    tp--;
                }
                if (*tp == '.') {
                    *tp = 0;
                }
                SetDlgItemText(hwndDlg, IDC_FPS, s);
            }
            if (wParam == 69) {
                KillTimer(hwndDlg, 69);
                C_UndoStack::save_undo(g_single_instance);
            }
            return false;
        case WM_MOUSEMOVE:
            if (g_drag_source) {
                TVHITTESTINFO hti = {};
                HWND hwnd = GetDlgItem(hwndDlg, IDC_TREE1);
                hti.pt.x = (int)LOWORD(lParam);
                hti.pt.y = (int)HIWORD(lParam);
                ClientToScreen(hwndDlg, &hti.pt);
                ScreenToClient(hwnd, &hti.pt);
                HTREEITEM h = TreeView_HitTest(hwnd, &hti);
                if (hti.flags & TVHT_ABOVE) {
                    SendMessage(hwnd, WM_VSCROLL, SB_LINEUP, 0);
                }
                if (hti.flags & TVHT_BELOW) {
                    SendMessage(hwnd, WM_VSCROLL, SB_LINEDOWN, 0);
                }
                if (hti.flags & TVHT_NOWHERE) {
                    h = g_hroot;
                }
                if ((hti.flags
                     & (TVHT_NOWHERE | TVHT_ONITEMINDENT | TVHT_ONITEMRIGHT
                        | TVHT_ONITEM | TVHT_ONITEMBUTTON))
                    && h) {
                    HTREEITEM temp = h;
                    while (temp && temp != TVI_ROOT) {
                        if (temp == g_drag_source) {
                            h = g_drag_source;
                            break;
                        }
                        temp = TreeView_GetParent(hwnd, temp);
                    }
                    if (h == g_drag_source) {
                        SetCursor(LoadCursor(nullptr, IDC_NO));
                        if (g_drag_placeholder) {
                            TreeView_DeleteItem(hwnd, g_drag_placeholder);
                        }
                        g_drag_placeholder = nullptr;
                    } else {
                        SetCursor(LoadCursor(nullptr, IDC_ARROW));
                        TV_ITEM i = {
                            TVIF_HANDLE | TVIF_PARAM, h, 0, 0, nullptr, 0, 0, 0, 0, 0};
                        TreeView_GetItem(hwnd, &i);
                        if (i.lParam) {
                            RECT r;
                            TreeView_GetItemRect(hwnd, h, &r, false);
                            if (hti.pt.y > r.bottom - (r.bottom - r.top) / 2) {
                                g_drag_insert_direction = AVS_COMPONENT_POSITION_AFTER;
                            } else {
                                g_drag_insert_direction = AVS_COMPONENT_POSITION_BEFORE;
                            }
                            HTREEITEM parenth;
                            g_drag_last_dest = h;
                            auto it = (Effect*)i.lParam;
                            if (it->can_have_child_components()
                                && (hti.flags & (TVHT_ONITEMINDENT | TVHT_ONITEMBUTTON)
                                    || it == Root_Info())) {
                                if (g_drag_placeholder
                                    && (TreeView_GetParent(hwnd, g_drag_placeholder)
                                            != h
                                        || TreeView_GetNextSibling(
                                            hwnd, g_drag_placeholder))) {
                                    TreeView_DeleteItem(hwnd, g_drag_placeholder);
                                    g_drag_placeholder = nullptr;
                                }

                                g_drag_insert_direction = AVS_COMPONENT_POSITION_CHILD;
                                parenth = h;
                                h = TVI_LAST;
                            } else {
                                parenth = TreeView_GetParent(hwnd, h);
                                if (g_drag_placeholder
                                    && ((g_drag_insert_direction
                                         == AVS_COMPONENT_POSITION_AFTER)
                                            ? TreeView_GetNextSibling(hwnd, h)
                                                  != g_drag_placeholder
                                            : TreeView_GetPrevSibling(hwnd, h)
                                                  != g_drag_placeholder)) {
                                    TreeView_DeleteItem(hwnd, g_drag_placeholder);
                                    g_drag_placeholder = nullptr;
                                }
                                if (g_drag_insert_direction
                                    == AVS_COMPONENT_POSITION_BEFORE) {
                                    h = TreeView_GetPrevSibling(hwnd, h);
                                    if (!h) {
                                        h = TVI_FIRST;
                                    }
                                }
                            }
                            if (!g_drag_placeholder) {
                                TV_INSERTSTRUCT is = {};
                                is.hParent = parenth;
                                is.hInsertAfter = h;
                                is.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_CHILDREN;
                                is.item.pszText = "<move here>";
                                g_drag_placeholder = TreeView_InsertItem(hwnd, &is);
                                if (g_drag_insert_direction
                                    == AVS_COMPONENT_POSITION_CHILD) {
                                    SendMessage(
                                        hwnd, TVM_EXPAND, TVE_EXPAND, (long)parenth);
                                }
                            }

                            // TreeView_Select(hwnd,g_drag_placeholder,TVGN_DROPHILITE);
                        }
                    }
                }
                return 0;
            }
            break;
        case WM_LBUTTONUP:
            if (g_drag_source) {
                HWND hwnd = GetDlgItem(hwndDlg, IDC_TREE1);
                SetCursor(LoadCursor(nullptr, IDC_ARROW));
                if (g_drag_placeholder) {
                    TreeView_DeleteItem(hwnd, g_drag_placeholder);
                    g_drag_placeholder = nullptr;
                }
                if (g_drag_last_dest) {
                    TV_ITEM dest_item{};
                    dest_item.mask = TVIF_HANDLE | TVIF_PARAM;
                    dest_item.hItem = g_drag_last_dest;
                    TreeView_GetItem(hwnd, &dest_item);
                    auto dest = (Effect*)dest_item.lParam;
                    HTREEITEM dest_handle = g_drag_last_dest;
                    HTREEITEM dest_parent_handle =
                        TreeView_GetParent(hwnd, g_drag_last_dest);
                    if (dest->can_have_child_components()
                        && (!dest_parent_handle
                            || g_drag_insert_direction
                                   == AVS_COMPONENT_POSITION_CHILD)) {
                        dest_parent_handle = dest_handle;
                    }

                    TV_ITEM source_item{};
                    source_item.mask = TVIF_HANDLE | TVIF_PARAM | TVIF_STATE;
                    source_item.hItem = g_drag_source;
                    source_item.stateMask = TVIS_EXPANDED;
                    TreeView_GetItem(hwnd, &source_item);
                    unsigned int expand = source_item.state & TVIS_EXPANDED;
                    auto source = (Effect*)source_item.lParam;

                    Effect::Insert_Direction direction;
                    switch (g_drag_insert_direction) {
                        case AVS_COMPONENT_POSITION_BEFORE:
                            direction = Effect::INSERT_BEFORE;
                            break;
                        default:
                        case AVS_COMPONENT_POSITION_AFTER:
                            direction = Effect::INSERT_AFTER;
                            break;
                        case AVS_COMPONENT_POSITION_CHILD:
                            direction = Effect::INSERT_CHILD;
                            break;
                    }

                    lock_lock(g_single_instance->render_lock);
                    auto success = g_single_instance->root.move(
                        source,
                        dest,
                        direction,
                        true /* insert even if not found in preset */);
                    lock_unlock(g_single_instance->render_lock);

                    if (success) {
                        treeview_hack = 1;
                        TreeView_DeleteItem(hwnd, g_drag_source);

                        TV_INSERTSTRUCT new_item = {};
                        new_item.hParent = dest_parent_handle;
                        new_item.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_CHILDREN;
                        new_item.item.pszText = source->get_desc();
                        new_item.item.cChildren = source->can_have_child_components();
                        new_item.item.lParam = (int)source;
                        new_item.hInsertAfter = TVI_LAST;

                        if (dest_handle) {
                            switch (g_drag_insert_direction) {
                                case AVS_COMPONENT_POSITION_AFTER:
                                    new_item.hInsertAfter = dest_handle;
                                    break;
                                case AVS_COMPONENT_POSITION_BEFORE:
                                    new_item.hInsertAfter =
                                        TreeView_GetPrevSibling(hwnd, dest_handle);
                                    if (!new_item.hInsertAfter) {
                                        new_item.hInsertAfter = TVI_FIRST;
                                    }
                                    break;
                                case AVS_COMPONENT_POSITION_CHILD:
                                case AVS_COMPONENT_POSITION_DONTCARE: break;
                            }
                        }

                        HTREEITEM newi = TreeView_InsertItem(hwnd, &new_item);
                        if (source->can_have_child_components()) {
                            _do_add(hwnd, newi, source);
                            if (expand) {
                                SendMessage(hwnd, TVM_EXPAND, TVE_EXPAND, (long)newi);
                            }
                        }
                        TreeView_Select(hwnd, newi, TVGN_CARET);
                        treeview_hack = 0;

                        // After everything is changed, then save the undo and set
                        // the dirty bit.
                        KillTimer(hwndDlg, 69);
                        SetTimer(hwndDlg, 69, UNDO_TIMER_INTERVAL, nullptr);
                        // g_preset_dirty=1;
                    } else {
                        log_err("failed to move effect %s", source->get_desc());
                    }
                }
                TreeView_Select(hwnd, nullptr, TVGN_DROPHILITE);
                ReleaseCapture();
                g_drag_source = nullptr;
                return 0;
            }
            break;
        case WM_NOTIFY: {
            NM_TREEVIEW* p;
            p = (NM_TREEVIEW*)lParam;
            if (p->hdr.hwndFrom == GetDlgItem(hwndDlg, IDC_TREE1)) {
                if (p->hdr.code == TVN_BEGINDRAG) {
                    if (p->itemNew.hItem != g_hroot) {
                        g_drag_last_dest = nullptr;
                        g_dragsource_parent =
                            TreeView_GetParent(p->hdr.hwndFrom, p->itemNew.hItem);
                        if (g_dragsource_parent) {
                            SetCapture(hwndDlg);
                            g_drag_source = p->itemNew.hItem;
                            g_drag_placeholder = nullptr;

                            TreeView_Select(p->hdr.hwndFrom, g_drag_source, TVGN_CARET);
                            SetCursor(LoadCursor(nullptr, IDC_APPSTARTING));
                        } else {
                            g_drag_source = nullptr;
                        }
                    } else {
                        g_drag_source = nullptr;
                    }
                }
                if (p->hdr.code == TVN_SELCHANGED && !treeview_hack) {
                    HTREEITEM hTreeItem = TreeView_GetSelection(p->hdr.hwndFrom);
                    if (hTreeItem) {
                        TV_ITEM i = {TVIF_HANDLE | TVIF_PARAM,
                                     hTreeItem,
                                     0,
                                     0,
                                     nullptr,
                                     0,
                                     0,
                                     0,
                                     0,
                                     0};
                        TreeView_GetItem(p->hdr.hwndFrom, &i);
                        auto tp = (Effect*)i.lParam;

                        is_aux_wnd = 0;
                        if (tp) {
                            SetDlgItemText(hwndDlg, IDC_EFNAME, tp->get_desc());
                            if (cur_hwnd) {
                                DestroyWindow(cur_hwnd);
                            }
                            g_current_render = (void*)tp;
                            C_Win32GuiComponent* ui_component =
                                g_ui_library->get((tp->get_legacy_id() == -1)
                                                      ? (int32_t)tp->get_legacy_ape_id()
                                                      : tp->get_legacy_id());
                            if (ui_component != nullptr) {
                                if (ui_component->uiprep != nullptr) {
                                    ui_component->uiprep(g_hInstance);
                                }
                                cur_hwnd = CreateDialog(
                                    g_hInstance,
                                    MAKEINTRESOURCE(ui_component->dialog_resource_id),
                                    hwndDlg,
                                    ui_component->ui_handler);
                            }
                            if (cur_hwnd) {
                                sniffConfigWindow_oldProc = (WNDPROC)SetWindowLong(
                                    cur_hwnd,
                                    GWL_WNDPROC,
                                    (LONG)sniffConfigWindow_newProc);
                            }
                        }
                        if (cur_hwnd) {
                            RECT r;
                            GetWindowRect(GetDlgItem(hwndDlg, IDC_EFFECTRECT), &r);
                            ScreenToClient(hwndDlg, (LPPOINT)&r);
                            SetWindowPos(cur_hwnd,
                                         nullptr,
                                         r.left,
                                         r.top,
                                         0,
                                         0,
                                         SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
                            ShowWindow(cur_hwnd, SW_SHOWNA);
                        } else {
                            SetDlgItemText(hwndDlg, IDC_EFNAME, g_noeffectstr);
                        }
                    }
                }
            }
            break;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_RRECT:
                    if (HIWORD(wParam) == 1) {
                        toggleWharfAmpDock(g_hwnd);
                    }
                    return 0;
                case IDM_HELP_DEBUGWND:
                    if (!g_debugwnd) {
                        g_debugwnd = CreateDialog(
                            g_hInstance, MAKEINTRESOURCE(IDD_DEBUG), g_hwnd, debugProc);
                    }
                    ShowWindow(g_debugwnd, SW_SHOW);
                    return 0;
                case IDM_ABOUT:
                    DialogBox(
                        g_hInstance, MAKEINTRESOURCE(IDD_DIALOG2), hwndDlg, aboutProc);
                    return 0;
                case IDM_DISPLAY:
                case IDM_FULLSCREEN:
                case IDM_PRESETS:
                case IDM_BPM:
                case IDM_TRANSITIONS: {
                    char* names[5] = {"Display",
                                      "Fullscreen Settings",
                                      "Presets/Hotkeys",
                                      "Beat Detection",
                                      "Transitions"};
                    int x = 0;
                    if (LOWORD(wParam) == IDM_DISPLAY) {
                        x = 1;
                    }
                    if (LOWORD(wParam) == IDM_FULLSCREEN) {
                        x = 2;
                    }
                    if (LOWORD(wParam) == IDM_PRESETS) {
                        x = 3;
                    }
                    if (LOWORD(wParam) == IDM_BPM) {
                        x = 4;
                    }
                    if (LOWORD(wParam) == IDM_TRANSITIONS) {
                        x = 5;
                    }

                    if (x >= 1 && x <= 5) {
                        SetDlgItemText(hwndDlg, IDC_EFNAME, names[x - 1]);
                        TreeView_Select(
                            GetDlgItem(hwndDlg, IDC_TREE1), nullptr, TVGN_CARET);
                        if (cur_hwnd) {
                            DestroyWindow(cur_hwnd);
                        }
                        if (x == 1) {
                            cur_hwnd = CreateDialog(g_hInstance,
                                                    MAKEINTRESOURCE(IDD_GCFG_DISP),
                                                    hwndDlg,
                                                    DlgProc_Disp);
                        }
                        if (x == 2) {
                            cur_hwnd = CreateDialog(g_hInstance,
                                                    MAKEINTRESOURCE(IDD_GCFG_FS),
                                                    hwndDlg,
                                                    DlgProc_FS);
                        }
                        if (x == 3) {
                            cur_hwnd = CreateDialog(g_hInstance,
                                                    MAKEINTRESOURCE(IDD_GCFG_PRESET),
                                                    hwndDlg,
                                                    DlgProc_Preset);
                        }
                        if (x == 4) {
                            cur_hwnd = CreateDialog(g_hInstance,
                                                    MAKEINTRESOURCE(IDD_GCFG_BPM),
                                                    hwndDlg,
                                                    DlgProc_Bpm);
                        }
                        if (x == 5) {
                            cur_hwnd =
                                CreateDialog(g_hInstance,
                                             MAKEINTRESOURCE(IDD_GCFG_TRANSITIONS),
                                             hwndDlg,
                                             (DLGPROC)win32_dlgproc_transition);
                        }
                        if (cur_hwnd) {
                            RECT r;
                            is_aux_wnd = 1;
                            GetWindowRect(GetDlgItem(hwndDlg, IDC_EFFECTRECT), &r);
                            ScreenToClient(hwndDlg, (LPPOINT)&r);
                            SetWindowPos(cur_hwnd,
                                         nullptr,
                                         r.left,
                                         r.top,
                                         0,
                                         0,
                                         SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
                            ShowWindow(cur_hwnd, SW_SHOWNA);
                        } else {
                            SetDlgItemText(hwndDlg, IDC_EFNAME, g_noeffectstr);
                        }
                    }
                    return 0;
                }
                case IDC_ADD: {
                    RECT r;
                    presetTreeMenu = CreatePopupMenu();

                    HMENU hAddMenu = CreatePopupMenu();

                    int p = 64;
                    Effect_Info* effectlist_info = nullptr;
                    for (const auto& effect : g_effect_lib) {
                        auto info = effect.second;
                        if (!info->is_createable_by_user()) {
                            continue;
                        }
                        if (info == EffectList_Info()) {
                            effectlist_info = info;
                            continue;
                        }
                        std::string display = info->get_group();
                        display += "/";
                        display += info->get_name();
                        char* display_str =
                            (char*)calloc(display.size() + 1, sizeof(char));
                        strncpy(display_str, display.c_str(), display.size());
                        _insertintomenu(hAddMenu, p++, info, display_str);
                        free(display_str);
                    }
                    if (effectlist_info) {
                        _insertintomenu(hAddMenu, p++, effectlist_info, "Effect List");
                    }

                    presetTreeCount = p;
                    int preset_base = p;
                    // add presets
                    MENUITEMINFO i = {};
                    i.cbSize = sizeof(i);
                    i.hSubMenu = presetTreeMenu;
                    i.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_ID;
                    i.fType = MFT_STRING;
                    i.dwTypeData = "Presets";
                    i.cch = strlen((char*)i.dwTypeData);
                    InsertMenuItem(hAddMenu, 0, true, &i);
                    i.hSubMenu = nullptr;
                    i.fMask = MIIM_TYPE | MIIM_ID;
                    i.fType = MFT_SEPARATOR;
                    InsertMenuItem(hAddMenu, 1, true, &i);

                    HANDLE h;
                    WIN32_FIND_DATA d;
                    char dirmask[1024];
                    wsprintf(dirmask, "%s\\*.*", g_path);

                    int directory_pos = 0;
                    int insert_pos = 0;

                    h = FindFirstFile(dirmask, &d);
                    if (h != INVALID_HANDLE_VALUE) {
                        do {
                            if (d.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY
                                && d.cFileName[0] != '.') {
                                MENUITEMINFO mi = {};
                                mi.cbSize = sizeof(mi);
                                mi.fMask = MIIM_SUBMENU | MIIM_TYPE;
                                mi.fType = MFT_STRING;
                                mi.fState = MFS_DEFAULT;
                                mi.hSubMenu = CreatePopupMenu();
                                mi.dwTypeData = d.cFileName;
                                mi.cch = strlen(d.cFileName);
                                InsertMenuItem(
                                    presetTreeMenu, directory_pos++, true, &mi);
                                insert_pos++;
                            } else if (!stricmp(extension(d.cFileName), "avs")) {
                                extension(d.cFileName)[-1] = 0;
                                MENUITEMINFO i = {};
                                i.cbSize = sizeof(i);
                                i.fMask = MIIM_DATA | MIIM_TYPE | MIIM_ID;
                                i.fType = MFT_STRING;
                                i.fState = MFS_DEFAULT;
                                i.wID = presetTreeCount++;
                                i.dwItemData = 0xffffffff;
                                i.dwTypeData = d.cFileName;
                                i.cch = strlen(d.cFileName);
                                InsertMenuItem(presetTreeMenu, insert_pos++, true, &i);
                            }
                        } while (FindNextFile(h, &d));
                        FindClose(h);
                    }

                    GetWindowRect(GetDlgItem(hwndDlg, IDC_ADD), &r);
                    int t = TrackPopupMenu(hAddMenu,
                                           TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD
                                               | TPM_RIGHTBUTTON | TPM_LEFTBUTTON,
                                           r.right,
                                           r.top,
                                           0,
                                           hwndDlg,
                                           nullptr);
                    if (t) {
                        Effect* new_component = nullptr;
                        char buf[2048];
                        buf[0] = 0;
                        if (t >= preset_base) {
                            if (findInMenu(presetTreeMenu, nullptr, t, buf, 2048)) {
                                // preset
                                new_component = new E_EffectList(g_single_instance);

                                char temp[4096];
                                wsprintf(temp, "%s%s.avs", g_path, buf);
                                AVS_Instance loader(
                                    g_path, AVS_AUDIO_EXTERNAL, AVS_BEAT_EXTERNAL);
                                if (loader.preset_load_file(temp)) {
                                    new_component->children =
                                        std::move(loader.root.children);
                                } else {
                                    delete new_component;
                                    new_component = nullptr;
                                }
                            }
                        } else {
                            MENUITEMINFO mi = {};
                            mi.cbSize = sizeof(mi);
                            mi.fMask = MIIM_DATA;
                            GetMenuItemInfo(hAddMenu, t, false, &mi);
                            if (mi.dwItemData != 0xffffffff) {
                                auto effect_info = (Effect_Info*)mi.dwItemData;
                                new_component =
                                    component_factory(effect_info, g_single_instance);
                            }
                        }

                        if (new_component) {
                            HTREEITEM h_selected =
                                TreeView_GetSelection(GetDlgItem(hwndDlg, IDC_TREE1));
                            Effect* parent = &g_single_instance->root;
                            HTREEITEM h_parent = g_hroot;
                            Effect* selected = nullptr;
                            if (h_selected) {
                                TV_ITEM i = {TVIF_HANDLE | TVIF_PARAM,
                                             h_selected,
                                             0,
                                             0,
                                             nullptr,
                                             0,
                                             0,
                                             0,
                                             0,
                                             0};
                                TreeView_GetItem(GetDlgItem(hwndDlg, IDC_TREE1), &i);
                                selected = (Effect*)i.lParam;
                                if (selected->can_have_child_components()) {
                                    parent = selected;
                                    h_parent = h_selected;
                                } else {
                                    HTREEITEM h_parent2 = TreeView_GetParent(
                                        GetDlgItem(hwndDlg, IDC_TREE1), h_selected);
                                    if (h_parent2 && h_parent2 != TVI_ROOT) {
                                        TV_ITEM i2 = {TVIF_HANDLE | TVIF_PARAM,
                                                      h_parent2,
                                                      0,
                                                      0,
                                                      nullptr,
                                                      0,
                                                      0,
                                                      0,
                                                      0,
                                                      0};
                                        TreeView_GetItem(GetDlgItem(hwndDlg, IDC_TREE1),
                                                         &i2);
                                        parent = (Effect*)i2.lParam;
                                        h_parent = h_parent2;
                                    }
                                }
                            }

                            lock_lock(g_single_instance->render_lock);
                            parent->insert(new_component,
                                           selected ? selected : parent,
                                           Effect::INSERT_CHILD);
                            lock_unlock(g_single_instance->render_lock);
                            TV_INSERTSTRUCT is = {};
                            is.hParent = h_parent;
                            is.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_CHILDREN;
                            is.item.pszText = new_component->get_desc();
                            is.item.cChildren =
                                new_component->can_have_child_components();
                            is.item.lParam = (int)new_component;
                            if (!h_selected || h_parent == h_selected) {
                                is.hInsertAfter = TVI_FIRST;
                            } else {
                                is.hInsertAfter = TreeView_GetPrevSibling(
                                    GetDlgItem(hwndDlg, IDC_TREE1), h_selected);
                                if (!is.hInsertAfter) {
                                    is.hInsertAfter = TVI_FIRST;
                                }
                            }
                            HTREEITEM newh = TreeView_InsertItem(
                                GetDlgItem(hwndDlg, IDC_TREE1), &is);
                            TreeView_Select(
                                GetDlgItem(hwndDlg, IDC_TREE1), newh, TVGN_CARET);
                            if (new_component->can_have_child_components()) {
                                _do_add(GetDlgItem(hwndDlg, IDC_TREE1),
                                        newh,
                                        new_component);
                            }

                            // Always do undo last.
                            KillTimer(hwndDlg, 69);
                            SetTimer(hwndDlg, 69, UNDO_TIMER_INTERVAL, nullptr);
                            // g_preset_dirty=1;
                        }
                    }
                    DestroyMenu(hAddMenu);
                    presetTreeMenu = nullptr;
                    return 0;
                }
                case IDC_CLEAR:
                    if (readyToLoadPreset(hwndDlg, 1)) {
                        if (g_single_instance->transition.load_preset(
                                "", TRANSITION_SWITCH_LOAD)
                            != 2) {
                            last_preset[0] = 0;
                        }
                    }
                    return 0;
                case IDC_REMSEL: {
                    HTREEITEM hTreeItem =
                        TreeView_GetSelection(GetDlgItem(hwndDlg, IDC_TREE1));
                    if (hTreeItem == g_hroot) {
                        CfgWnd_Unpopulate();
                        lock_lock(g_single_instance->render_lock);
                        g_single_instance->clear();
                        lock_unlock(g_single_instance->render_lock);
                        CfgWnd_Populate();
                    } else if (hTreeItem) {
                        TV_ITEM i = {};
                        i.mask = TVIF_HANDLE | TVIF_PARAM;
                        i.hItem = hTreeItem;
                        if (TreeView_GetItem(GetDlgItem(hwndDlg, IDC_TREE1), &i)) {
                            auto tp = (Effect*)i.lParam;
                            HTREEITEM hParent = TreeView_GetParent(
                                GetDlgItem(hwndDlg, IDC_TREE1), hTreeItem);
                            if (hParent != nullptr) {
                                TV_ITEM i2 = {};
                                i2.mask = TVIF_HANDLE | TVIF_PARAM;
                                i2.hItem = hParent;
                                TreeView_GetItem(GetDlgItem(hwndDlg, IDC_TREE1), &i2);
                                auto parent = (Effect*)i2.lParam;
                                lock_lock(g_single_instance->render_lock);
                                parent->remove(tp);
                                TreeView_DeleteItem(GetDlgItem(hwndDlg, IDC_TREE1),
                                                    hTreeItem);
                                lock_unlock(g_single_instance->render_lock);
                            }
                        }
                    }
                    // Always save undo last.
                    KillTimer(hwndDlg, 69);
                    SetTimer(hwndDlg, 69, UNDO_TIMER_INTERVAL, nullptr);
                    // g_preset_dirty=1;
                    return 0;
                }
                case IDC_CLONESEL: {
                    Effect* new_component = nullptr;

                    HTREEITEM hTreeItem =
                        TreeView_GetSelection(GetDlgItem(hwndDlg, IDC_TREE1));
                    if (hTreeItem && hTreeItem != g_hroot) {
                        TV_ITEM i = {};
                        i.mask = TVIF_HANDLE | TVIF_PARAM;
                        i.hItem = hTreeItem;
                        TreeView_GetItem(GetDlgItem(hwndDlg, IDC_TREE1), &i);
                        E_EffectList* tp = (E_EffectList*)i.lParam;
                        auto effect_info = tp->get_info();
                        new_component =
                            component_factory(effect_info, g_single_instance);
                        if (new_component) {
                            HTREEITEM hParent = TreeView_GetParent(
                                GetDlgItem(hwndDlg, IDC_TREE1), hTreeItem);

                            if (hParent && hParent != TVI_ROOT) {
                                TV_ITEM i2 = {};
                                i2.mask = TVIF_HANDLE | TVIF_PARAM;
                                i2.hItem = hParent;
                                TreeView_GetItem(GetDlgItem(hwndDlg, IDC_TREE1), &i2);
                                auto tparent = (E_EffectList*)i2.lParam;
                                auto parentrender = (E_EffectList*)tparent;
                                auto buf = (uint8_t*)calloc(
                                    MAX_LEGACY_PRESET_FILESIZE_BYTES, sizeof(uint8_t));
                                if (buf) {
                                    int len = tp->save_legacy(buf);
                                    new_component->load_legacy(buf, len);
                                    free(buf);
                                }
                                lock_lock(g_single_instance->render_lock);
                                parentrender->insert(
                                    new_component, tp, Effect::INSERT_AFTER);
                                lock_unlock(g_single_instance->render_lock);

                                TV_INSERTSTRUCT is = {};
                                is.hParent = hParent;
                                is.hInsertAfter = hTreeItem;
                                is.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_CHILDREN;
                                is.item.pszText = new_component->get_desc();
                                is.item.cChildren =
                                    new_component->can_have_child_components();
                                is.item.lParam = (int)new_component;
                                HTREEITEM newh = TreeView_InsertItem(
                                    GetDlgItem(hwndDlg, IDC_TREE1), &is);
                                TreeView_Select(
                                    GetDlgItem(hwndDlg, IDC_TREE1), newh, TVGN_CARET);
                                if (new_component->can_have_child_components()) {
                                    _do_add(GetDlgItem(hwndDlg, IDC_TREE1),
                                            newh,
                                            new_component);
                                }
                            }
                        }
                        // Always save undo last.
                        KillTimer(hwndDlg, 69);
                        SetTimer(hwndDlg, 69, UNDO_TIMER_INTERVAL, nullptr);
                        // g_preset_dirty=1;
                    }
                    return 0;
                }
                case IDC_LOAD: {
                    char temp[2048];
                    OPENFILENAME l = {};
                    l.lStructSize = sizeof(l);
                    char buf1[2048], buf2[2048];
                    GetCurrentDirectory(sizeof(buf2), buf2);
                    strcpy(buf1, g_path);
                    temp[0] = 0;
                    l.lpstrInitialDir = buf1;
                    l.hwndOwner = hwndDlg;
                    l.lpstrFilter = "AVS presets\0*.avs\0All files\0*.*\0";
                    l.lpstrFile = temp;
                    l.nMaxFile = 2048 - 1;
                    l.lpstrTitle = "Load Preset";
                    l.lpstrDefExt = "AVS";
                    l.Flags = OFN_HIDEREADONLY | OFN_EXPLORER;
                    if (readyToLoadPreset(hwndDlg, 0) && GetOpenFileName(&l)) {
                        int x = g_single_instance->transition.load_preset(
                            temp, TRANSITION_SWITCH_LOAD);
                        if (x == 2) {
                            MessageBox(hwndDlg,
                                       "Still initializing previous preset",
                                       "Load Preset",
                                       MB_OK);
                        } else {
                            lstrcpyn(last_preset, temp, sizeof(last_preset));
                        }
                    }
                    SetCurrentDirectory(buf2);
                    return 0;
                }
                case IDC_SAVE: dosavePreset(hwndDlg); return 0;
                case IDM_UNDO: C_UndoStack::undo(g_single_instance); return 0;
                case IDM_REDO: C_UndoStack::redo(g_single_instance); return 0;
            }
            return 0;
    }
    return 0;
}

static void _do_add(HWND hwnd, HTREEITEM h, Effect* list) {
    for (auto& child : list->children) {
        TV_INSERTSTRUCT is;
        memset(&is, 0, sizeof(is));

        is.hParent = h;
        is.hInsertAfter = TVI_LAST;
        is.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_CHILDREN;
        is.item.pszText = child->get_desc();
        is.item.cChildren = child->can_have_child_components();
        is.item.lParam = (int)child;

        HTREEITEM h2 = TreeView_InsertItem(hwnd, &is);
        if (child->can_have_child_components()) {
            _do_add(hwnd, h2, child);
        }
    }
    SendMessage(hwnd, TVM_EXPAND, TVE_EXPAND, (long)h);
}

static void _do_free(HWND hwnd, HTREEITEM h) {
    while (h) {
        TV_ITEM i = {};
        i.mask = TVIF_HANDLE | TVIF_PARAM;
        i.hItem = h;
        TreeView_GetItem(hwnd, &i);
        if (i.lParam) {
            free((void*)i.lParam);
        }
        HTREEITEM h2 = TreeView_GetChild(hwnd, h);
        if (h2) {
            _do_free(hwnd, h2);
        }
        h = TreeView_GetNextSibling(hwnd, h);
    }
}

int need_repop;

void CfgWnd_RePopIfNeeded() {
    if (need_repop) {
        CfgWnd_Unpopulate(1);
        CfgWnd_Populate(1);
        need_repop = 0;
    }
}

void CfgWnd_Unpopulate(int force) {
    if (force || (IsWindowVisible(g_hwndDlg) && !DDraw_IsFullScreen())) {
        HWND hwnd = GetDlgItem(g_hwndDlg, IDC_TREE1);
        if (!is_aux_wnd) {
            if (cur_hwnd) {
                DestroyWindow(cur_hwnd);
            }
            cur_hwnd = 0;
            SetDlgItemText(g_hwndDlg, IDC_EFNAME, g_noeffectstr);
        }
        treeview_hack = 1;
        _do_free(hwnd, TreeView_GetChild(hwnd, TVI_ROOT));
        TreeView_DeleteAllItems(hwnd);
        treeview_hack = 0;
    } else {
        need_repop = 1;
    }
}

void CfgWnd_Populate(int force) {
    if (force || (IsWindowVisible(g_hwndDlg) && !DDraw_IsFullScreen())) {
        treeview_hack = 1;
        HWND hwnd = GetDlgItem(g_hwndDlg, IDC_TREE1);
        Effect* newt = &g_single_instance->root;
        TV_INSERTSTRUCT is;
        memset(&is, 0, sizeof(is));
        is.hParent = TVI_ROOT;
        is.hInsertAfter = TVI_LAST;
        is.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_CHILDREN;
        is.item.pszText = "Main";
        is.item.cChildren = 1;
        is.item.lParam = (int)newt;
        g_hroot = TreeView_InsertItem(hwnd, &is);
        if (g_hroot) {
            _do_add(hwnd, g_hroot, newt);
            SendMessage(hwnd, TVM_EXPAND, TVE_EXPAND, (long)g_hroot);
        }
        treeview_hack = 0;
    } else {
        need_repop = 1;
    }
}
