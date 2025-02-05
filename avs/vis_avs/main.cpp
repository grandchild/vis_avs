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
#include "e_effectlist.h"

#include "blend.h"
#include "bpm.h"
#include "cfgwin.h"
#include "draw.h"
#include "render.h"
#include "resource.h"
#include "vis.h"
#include "wnd.h"

#include "../platform.h"

#include <windows.h>
#include <math.h>
#include <process.h>
#include <stdio.h>

#ifdef WA3_COMPONENT
#include "wasabicfg.h"

#include "../studio/studio/api.h"
#endif

#include "avs_eelif.h"

extern void GetClientRect_adj(HWND hwnd, RECT* r);

static unsigned char g_logtab[256];
HINSTANCE g_hInstance;

char* verstr = "Advanced Visualization Studio v2.81d";

static unsigned int WINAPI RenderThread(LPVOID a);

static void config(struct winampVisModule* this_mod);
static int init(struct winampVisModule* this_mod);
static int render(struct winampVisModule* this_mod);
static void quit(struct winampVisModule* this_mod);

HANDLE g_hThread;
volatile int g_ThreadQuit;

#ifndef WA3_COMPONENT
static lock_t* g_cs;
#endif

static unsigned char g_visdata[2][2][576];
static int g_visdata_pstat;

#ifndef WA3_COMPONENT

static winampVisModule* getModule(int which);
static winampVisHeader hdr = {VIS_HDRVER, verstr, getModule};

extern "C" {
__declspec(dllexport) winampVisHeader* winampVisGetHeader() { return &hdr; }
}

static winampVisModule* getModule(int which) {
    static winampVisModule mod = {"Advanced Visualization Studio",
                                  NULL,       // hwndParent
                                  NULL,       // hDllInstance
                                  0,          // sRate
                                  0,          // nCh
                                  1000 / 70,  // latencyMS
                                  1000 / 70,  // delayMS
                                  2,          // spectrumNch
                                  2,          // waveformNch
                                  {
                                      0,
                                  },  // spectrumData
                                  {
                                      0,
                                  },  // waveformData
                                  config,
                                  init,
                                  render,
                                  quit,
                                  NULL};
    if (which == 0) {
        return &mod;
    }
    return 0;
}
#endif

BOOL CALLBACK aboutProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    switch (uMsg) {
        case WM_INITDIALOG: SetDlgItemText(hwndDlg, IDC_VERSTR, verstr); return 1;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                case IDCANCEL: EndDialog(hwndDlg, 0); return 0;
            }
            return 0;
    }
    return 0;
}

static void config(struct winampVisModule* this_mod) {
    if (!g_hwnd || !IsWindow(g_hwnd)) {
        DialogBox(this_mod->hDllInstance,
                  MAKEINTRESOURCE(IDD_DIALOG2),
                  this_mod->hwndParent,
                  aboutProc);
    } else {
        SendMessage(g_hwnd, WM_USER + 33, 0, 0);
    }
}

static int g_is_beat;
char g_path[1024];
AVS_Instance* g_single_instance;

int beat_peak1, beat_peak2, beat_cnt, beat_peak1_peak;

void main_setRenderThreadPriority() {
    int prios[] = {
        GetThreadPriority(GetCurrentThread()),
        THREAD_PRIORITY_IDLE,
        THREAD_PRIORITY_LOWEST,
        THREAD_PRIORITY_NORMAL,
        THREAD_PRIORITY_HIGHEST,
    };
    SetThreadPriority(g_hThread, prios[cfg_render_prio]);
}

extern void previous_preset(HWND hwnd);
extern void next_preset(HWND hwnd);
extern void random_preset(HWND hwnd);

#if 0  // syntax highlighting
HINSTANCE hRich;
#endif

static int init(struct winampVisModule* this_mod) {
    DWORD id;
    FILETIME ft;
#if 0  // syntax highlighting
  if (!hRich) hRich=LoadLibrary("RICHED32.dll");
#endif
    GetSystemTimeAsFileTime(&ft);
    srand(ft.dwLowDateTime | (ft.dwHighDateTime ^ GetCurrentThreadId()));
    g_hInstance = this_mod->hDllInstance;
#ifdef WA3_COMPONENT
    GetModuleFileName(GetModuleHandle(NULL), g_path, MAX_PATH);
#else
    GetModuleFileName(g_hInstance, g_path, MAX_PATH);
#endif
    char* p = g_path + strlen(g_path);
    while (p > g_path && *p != '\\') {
        p--;
    }
    *p = 0;

#ifdef WA2_EMBED
    if (SendMessage(this_mod->hwndParent, WM_USER, 0, 0) < 0x2900) {
        MessageBox(this_mod->hwndParent,
                   "This version of AVS requires Winamp 2.9+",
                   "AVS ERROR",
                   MB_OK | MB_ICONSTOP);
        return 1;
    }
#endif

#ifdef WA3_COMPONENT
    strcat(g_path, "\\wacs\\data");
#endif

    strcat(g_path, "\\avs");
    CreateDirectory(g_path, NULL);

#ifndef WA3_COMPONENT
    g_cs = lock_init();
#endif
    g_ThreadQuit = 0;
    g_visdata_pstat = 1;

    g_single_instance = new AVS_Instance(g_path, AVS_AUDIO_INTERNAL, AVS_BEAT_INTERNAL);

    AVS_EEL_IF_init(g_single_instance);

    make_blend_LUTs();

    if (Wnd_Init(this_mod)) {
        return 1;
    }

    {
        int x;
        for (x = 0; x < 256; x++) {
            double a = log(x * 60.0 / 255.0 + 1.0) / log(60.0);
            int t = (int)(a * 255.0);
            if (t < 0) {
                t = 0;
            }
            if (t > 255) {
                t = 255;
            }
            g_logtab[x] = (unsigned char)t;
        }
    }

    initBpm();

    Render_Init();

    CfgWnd_Create(this_mod);

    g_hThread = (HANDLE)_beginthreadex(NULL, 0, RenderThread, 0, 0, (unsigned int*)&id);
    main_setRenderThreadPriority();
    SetForegroundWindow(g_hwnd);
    SetFocus(g_hwnd);

    return 0;
}

static int render(struct winampVisModule* this_mod) {
#ifndef WA3_COMPONENT
    int x, avs_beat = 0, b;
    if (g_ThreadQuit) {
        return 1;
    }
    lock_lock(g_cs);
    if (g_ThreadQuit) {
        lock_unlock(g_cs);
        return 1;
    }
    if (g_visdata_pstat) {
        // TODO [bugfix][cleanup]:
        //    The loop conditions are x<576*2, but spectrumData has size [2][576], so
        //    the code probably relied on the two channel arrays lying consecutively in
        //    memory. Make it explicit (fixes a warning), but investigate more later.
        for (x = 0; x < 576 * 2; x++) {
            g_visdata[0][x / 576][x % 576] =
                g_logtab[(unsigned char)this_mod->spectrumData[x / 576][x % 576]];
        }
    } else {
        for (x = 0; x < 576 * 2; x++) {
            int t = g_logtab[(unsigned char)this_mod->spectrumData[x / 576][x % 576]];
            if (g_visdata[0][x / 576][x % 576] < t) {
                g_visdata[0][x / 576][x % 576] = t;
            }
        }
    }
    memcpy(&g_visdata[1][0][0], this_mod->waveformData, 576 * 2);
    {
        int lt[2] = {0, 0};
        int x;
        int ch;
        for (ch = 0; ch < 2; ch++) {
            unsigned char* f = (unsigned char*)&this_mod->waveformData[ch][0];
            for (x = 0; x < 576; x++) {
                int r = *f++ ^ 128;
                r -= 128;
                if (r < 0) {
                    r = -r;
                }
                lt[ch] += r;
            }
        }
        lt[0] = max(lt[0], lt[1]);

        beat_peak1 = (beat_peak1 * 125 + beat_peak2 * 3) / 128;

        beat_cnt++;

        if (lt[0] >= (beat_peak1 * 34) / 32 && lt[0] > (576 * 16)) {
            if (beat_cnt > 0) {
                beat_cnt = 0;
                avs_beat = 1;
            }
            beat_peak1 = (lt[0] + beat_peak1_peak) / 2;
            beat_peak1_peak = lt[0];
        } else if (lt[0] > beat_peak2) {
            beat_peak2 = lt[0];
        } else {
            beat_peak2 = (beat_peak2 * 14) / 16;
        }
    }
    b = refineBeat(avs_beat);
    if (b) {
        g_is_beat = 1;
    }
    g_visdata_pstat = 0;
    lock_unlock(g_cs);
#endif
    return 0;
}

static void quit(struct winampVisModule*) {
#define DS(x)
    if (g_hThread) {
        DS("Waitin for thread to quit\n");
        g_ThreadQuit = 1;
        if (WaitForSingleObject(g_hThread, 10000) != WAIT_OBJECT_0) {
            DS("Terminated thread (BAD!)\n");
            TerminateThread(g_hThread, 0);
        }
        DS("Thread done... calling ddraw_quit\n");
        DDraw_Quit();

        DS("Calling cfgwnd_destroy\n");
        CfgWnd_Destroy();
        DS("Calling render_quit\n");
        Render_Quit();

        DS("Calling wnd_quit\n");
        Wnd_Quit();

        DS("closing thread handle\n");
        CloseHandle(g_hThread);
        g_hThread = NULL;

        DS("calling eel quit\n");
        AVS_EEL_IF_quit(g_single_instance);

        DS("cleaning up critsections\n");
#ifndef WA3_COMPONENT
        lock_destroy(g_cs);
#endif

        DS("smp_cleanupthreads\n");
        E_EffectList::smp_cleanup_threads();
    }
#undef DS
#if 0  // syntax highlighting
  if (hRich) FreeLibrary(hRich);
  hRich=0;
#endif

    delete g_single_instance;
    g_single_instance = NULL;
}

#ifdef WA3_COMPONENT
static winampVisModule dummyMod;

void init3(void) {
    extern HWND g_wndparent;
    dummyMod.hwndParent = g_wndparent;
    dummyMod.hDllInstance = g_hInstance;
    init(&dummyMod);
}

void quit3(void) {
    extern HWND last_parent;
    if (last_parent) {
        ShowWindow(GetParent(last_parent), SW_SHOWNA);
    }
    quit(NULL);
}
#endif

#define FPS_NF 64

static unsigned int WINAPI RenderThread(LPVOID) {
    int framedata[FPS_NF] = {
        0,
    };
    unsigned int framedata_pos = 0;
    int s = 0;
    char vis_data[2][2][576];
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    srand(ft.dwLowDateTime | (ft.dwHighDateTime ^ GetCurrentThreadId()));
    while (!g_ThreadQuit) {
        int w, h, *fb = NULL, *fb2 = NULL, beat = 0;
#ifdef WA3_COMPONENT
        char visdata[576 * 2 * 2];
        int ret = api->core_getVisData(0, visdata, sizeof(visdata));

        if (!ret) {
            memset(&vis_data[0][0][0], 0, 576 * 2 * 2);
            beat = 0;
        } else {
            int x;
            unsigned char* v = (unsigned char*)visdata;
            for (x = 0; x < 576 * 2; x++) {
                vis_data[0][0][x] = g_logtab[*v++];
            }
            for (x = 0; x < 576 * 2; x++) {
                ((unsigned char*)vis_data[1][0])[x] = *v++;
            }

            v = (unsigned char*)visdata + 1152;
            {
                int lt[2] = {0, 0};
                int ch;
                for (ch = 0; ch < 2; ch++) {
                    for (x = 0; x < 576; x++) {
                        int r = *v++ ^ 128;
                        r -= 128;
                        if (r < 0) {
                            r = -r;
                        }
                        lt[ch] += r;
                    }
                }
                lt[0] = max(lt[0], lt[1]);

                beat_peak1 = (beat_peak1 * 125 + beat_peak2 * 3) / 128;
                beat_cnt++;

                if (lt[0] >= (beat_peak1 * 34) / 32 && lt[0] > (576 * 16)) {
                    if (beat_cnt > 0) {
                        beat_cnt = 0;
                        beat = 1;
                    }
                    beat_peak1 = (lt[0] + beat_peak1_peak) / 2;
                    beat_peak1_peak = lt[0];
                } else if (lt[0] > beat_peak2) {
                    beat_peak2 = lt[0];
                } else {
                    beat_peak2 = (beat_peak2 * 14) / 16;
                }
            }
            //     lock_lock(g_title_cs);
            beat = refineBeat(beat);
            //      lock_unlock(g_title_cs);
        }

#else
        lock_lock(g_cs);
        memcpy(&vis_data[0][0][0], &g_visdata[0][0][0], 576 * 2 * 2);
        g_visdata_pstat = 1;
        beat = g_is_beat;
        g_is_beat = 0;
        lock_unlock(g_cs);
#endif

        if (!g_ThreadQuit) {
            if (IsWindow(g_hwnd) && !g_in_destroy) {
                DDraw_Enter(&w, &h, &fb, &fb2);
            } else {
                break;
            }
            if (fb && fb2) {
                extern int g_dlg_w, g_dlg_h, g_dlg_fps;

                lock_lock(g_single_instance->render_lock);
                g_single_instance->init_global_buffers_if_needed(
                    w, h, AVS_PIXEL_RGB0_8);
                int t = g_single_instance->root.render(
                    vis_data, beat, s ? fb2 : fb, s ? fb : fb2, w, h);
                lock_unlock(g_single_instance->render_lock);
                if (t & 1) {
                    s ^= 1;
                }

                if (IsWindow(g_hwnd)) {
                    DDraw_Exit(s);
                }

                int lastt = framedata[framedata_pos];
                int thist = GetTickCount();
                framedata[framedata_pos] = thist;
                g_dlg_w = w;
                g_dlg_h = h;
                if (lastt) {
                    g_dlg_fps = MulDiv(
                        sizeof(framedata) / sizeof(framedata[0]), 10000, thist - lastt);
                }
                framedata_pos++;
                if (framedata_pos >= sizeof(framedata) / sizeof(framedata[0])) {
                    framedata_pos = 0;
                }
            }
            int fs = DDraw_IsFullScreen();
            int sv = (fs ? (cfg_speed >> 8) : cfg_speed) & 0xff;
            Sleep(min(max(sv, 1), 100));
        }
    }
    _endthreadex(0);
    return 0;
}
