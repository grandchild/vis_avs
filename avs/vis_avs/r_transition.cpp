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

#include "r_defs.h"

#include "cfgwin.h"
#include "draw.h"
#include "render.h"

#include <windows.h>
#include <math.h>
#include <process.h>
#include <stdio.h>
extern char* scanstr_back(char* str, char* toscan, char* defval);

Transition::Transition()
    : l_w(0),
      l_h(0),
      enabled(0),
      start_time(0),
      initThread(nullptr),
      do_transition_flag(0),
      prev_renders_need_cleanup(false) {
    last_file[0] = 0;
    memset(this->fbs, 0, sizeof(this->fbs));
}

Transition::~Transition() {
    if (initThread) {
        WaitForSingleObject(initThread, INFINITE);
        CloseHandle(initThread);
        initThread = nullptr;
    }
    for (auto& fb : this->fbs) {
        free(fb);
        fb = nullptr;
    }
}

unsigned int WINAPI Transition::m_initThread(LPVOID p) {
    auto transition = (Transition*)p;
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    srand(ft.dwLowDateTime | (ft.dwHighDateTime ^ GetCurrentThreadId()));
    if (cfg_transitions2 & 32) {
        extern HANDLE g_hThread;

        int d = GetThreadPriority(g_hThread);
        if (d == THREAD_PRIORITY_TIME_CRITICAL) {
            d = THREAD_PRIORITY_HIGHEST;
        } else if (d == THREAD_PRIORITY_HIGHEST) {
            d = THREAD_PRIORITY_ABOVE_NORMAL;
        } else if (d == THREAD_PRIORITY_ABOVE_NORMAL) {
            d = THREAD_PRIORITY_NORMAL;
        } else if (d == THREAD_PRIORITY_NORMAL) {
            d = THREAD_PRIORITY_BELOW_NORMAL;
        } else if (d == THREAD_PRIORITY_BELOW_NORMAL) {
            d = THREAD_PRIORITY_LOWEST;
        } else if (d == THREAD_PRIORITY_LOWEST) {
            d = THREAD_PRIORITY_IDLE;
        }
        SetThreadPriority(GetCurrentThread(), d);
    }
    int* fb = (int*)calloc(transition->l_w * transition->l_h, sizeof(int));
    char last_visdata[2][2][576] = {{{0}}};
    g_single_instance->root_secondary.render(
        last_visdata, (int)0x80000000, fb, fb, transition->l_w, transition->l_h);
    free(fb);

    transition->do_transition_flag = 2;

    _endthreadex(0);
    return 0;
}

int Transition::load_preset(char* file, int which, C_UndoItem* item) {
    if (this->initThread) {
        if (WaitForSingleObject(this->initThread, 0) == WAIT_TIMEOUT) {
            DDraw_SetStatusText("loading [wait]...", 1000 * 100);
            return 2;
        }
        CloseHandle(this->initThread);
        this->initThread = nullptr;
    }

    if (this->enabled) {
        this->enabled = false;
    }

    int r = 0;

    if (item) {
        if (item->size() < MAX_LEGACY_PRESET_FILESIZE_BYTES) {
            auto data = (uint8_t*)calloc(item->size(), 1);
            memcpy(data, item->get(), item->size());
            g_single_instance->preset_load_legacy(
                data, item->size(), /*with_transition*/ true);
            free(data);
            this->last_which = which;
            this->do_transition_flag = 2;
        }
    } else {
        lstrcpyn(last_file, file, sizeof(last_file));
        if (file[0]) {
            r = g_single_instance->preset_load_file(file);
        } else {
            g_single_instance->clear_secondary();
        }
        if (!r && l_w && l_h && (cfg_transitions2 & which)
            && ((cfg_transitions2 & 128) || DDraw_IsFullScreen())) {
            DWORD id;
            this->last_which = which;
            this->do_transition_flag = 1;
            this->initThread = (HANDLE)_beginthreadex(
                nullptr, 0, m_initThread, (LPVOID)this, 0, (unsigned int*)&id);
            DDraw_SetStatusText("loading...", 1000 * 100);
        } else {
            this->last_which = which;
            this->do_transition_flag = 2;
        }

        if (r) {
            char s[MAX_PATH * 2];
            wsprintf(s,
                     "error loading: %s",
                     scanstr_back(last_file, "\\", last_file - 1) + 1);
            DDraw_SetStatusText(s);
            this->do_transition_flag = 3;
        }
        C_UndoStack::clear();
        C_UndoStack::save_undo(false);
        C_UndoStack::clear_dirty();
    }

    return !!r;
}

void Transition::clean_prev_renders_if_needed() {
    if (this->prev_renders_need_cleanup) {
        g_single_instance->clear_secondary();
        this->prev_renders_need_cleanup = false;
    }
}

#define PI 3.14159265358979323846
// 264338327950288419716939937510582097494459230781640628620899862803482534211706798214808651328230664709384460955058223172535940812848...

extern int g_rnd_cnt;

int Transition::render(char visdata[2][2][576],
                       int is_beat,
                       int* framebuffer,
                       int* fbout,
                       int w,
                       int h) {
    if (this->do_transition_flag || this->enabled) {
        g_rnd_cnt = 0;
    }
    if (this->do_transition_flag == 2 || this->do_transition_flag == 3) {
        int notext = this->do_transition_flag == 3;
        this->do_transition_flag = 0;
        if (cfg_transitions & this->last_which) {
            curtrans =
                (cfg_transition_mode & 0x7fff)
                    ? (cfg_transition_mode & 0x7fff)
                    : (rand()
                       % ((sizeof(transitionmodes) / sizeof(transitionmodes[0])) - 1))
                          + 1;
            if (cfg_transition_mode & 0x8000) {
                curtrans |= 0x8000;
            }
            ep[0] = 0;
            ep[1] = 2;
            mask = 0;
            start_time = 0;
            this->enabled = 1;
        }
        E_Root&& temp = std::move(g_single_instance->root);
        g_single_instance->root = g_single_instance->root_secondary;
        g_single_instance->root_secondary = temp;
        extern int need_repop;
        extern char* extension(char* fn);
        need_repop = 1;
        PostMessage(g_hwndDlg, WM_USER + 20, 0, 0);
        if (!notext && stricmp("aph", extension(last_file))) {
            char buf[512];
            strncpy(buf, scanstr_back(last_file, "\\", last_file - 1) + 1, 510);
            buf[510] = 0;
            scanstr_back(buf, ".", buf + strlen(buf))[0] = 0;
            strcat(buf, " ");
            DDraw_SetStatusText(buf);
        }
    }

    if (!this->enabled) {
        int x;
        l_w = w;
        l_h = h;
        if (fbs[0]) {
            for (x = 0; x < 4; x++) {
                if (fbs[x]) {
                    free(fbs[x]);
                    fbs[x] = nullptr;
                }
            }
        }
        if (!this->initThread && !g_single_instance->root_secondary.children.empty()) {
            this->prev_renders_need_cleanup = true;
        }
        return g_single_instance->root.render(
            visdata, is_beat, framebuffer, fbout, w, h);
    }

    // handle resize
    if (l_w != w || l_h != h || !fbs[0]) {
        l_w = w;
        l_h = h;
        int x;
        for (x = 0; x < 4; x++) {
            if (fbs[x]) {
                free(fbs[x]);
            }
            fbs[x] = (int*)calloc(l_w * l_h, sizeof(int));
        }
    }

    if (start_time == 0) {
        memcpy(fbs[ep[0]], framebuffer, sizeof(int) * l_w * l_h);
        memcpy(fbs[ep[1]], framebuffer, sizeof(int) * l_w * l_h);
    }

    // maybe there's a faster way than using 3 more buffers without screwing
    // any effect... justin ?
    if (curtrans & 0x8000) {
        ep[1] ^= g_single_instance->root_secondary.render(
                     visdata, is_beat, fbs[ep[1]], fbs[ep[1] ^ 1], w, h)
                 & 1;
    }
    ep[0] ^= g_single_instance->root.render(
                 visdata, is_beat, fbs[ep[0]], fbs[ep[0] ^ 1], w, h)
             & 1;

    int* p = fbs[ep[1]];
    int* d = fbs[ep[0]];
    int* o = framebuffer;
    int x = w * h;

    int ttime = 250 * cfg_transitions_speed;
    if (ttime < 100) {
        ttime = 100;
    }

    int n;
    if (!start_time) {
        n = 0;
        start_time = timer_ms();
    } else {
        n = (timer_ms() - start_time) * 256 / ttime;
    }

    if (n >= 255) {
        n = 255;
    }

    // used for smoothing transitions
    // now sintrans does a smooth curve from 0 to 1
    float sintrans = (float)(sin(((float)n / 255) * PI - PI / 2) / 2 + 0.5);
    switch (curtrans & 0x7fff) {
        case 1:  // Crossfade
            mmx_adjblend_block(o, d, p, x, n);
            break;
        case 2: {  // Left to right push
            int i = (int)(sintrans * w);
            int j;
            for (j = 0; j < h; j++) {
                memcpy(framebuffer + (j * w), d + (j * w) + (w - i), i * 4);
                memcpy(framebuffer + (j * w) + i, p + (j * w), (w - i) * 4);
            }
        } break;
        case 3: {  // Right to left push
            int i = (int)(sintrans * w);
            int j;
            for (j = 0; j < h; j++) {
                memcpy(framebuffer + (j * w), p + (i + j * w), (w - i) * 4);
                memcpy(framebuffer + (j * w) + (w - i), d + (j * w), i * 4);
            }
        } break;
        case 4: {  // Top to bottom push
            int i = (int)(sintrans * h);
            memcpy(framebuffer, d + (h - i) * w, w * i * 4);
            memcpy(framebuffer + w * i, p, w * (h - i) * 4);
        } break;
        case 5: {  // Bottom to Top push
            int i = (int)(sintrans * h);
            memcpy(framebuffer, p + i * w, w * (h - i) * 4);
            memcpy(framebuffer + w * (h - i), d, w * i * 4);
        } break;
        case 6: {  // 9 random blocks
            if (!(mask & (1 << (10 + n / 28)))) {
                int r = 0;
                if ((mask & 0x1ff) != 0x1ff) {
                    do {
                        r = rand() % 9;
                    } while ((1 << r) & mask);
                }
                mask |= (1 << r) | (1 << (10 + n / 28));
            }
            int j;
            int tw = w / 3, th = h / 3;
            int twr = w - 2 * tw;
            memcpy(framebuffer, p, w * h * 4);
            int i;
            for (i = 0; i < 9; i++) {
                if (mask & (1 << i)) {
                    int end = i / 3 * th + th;
                    if (i > 5) {
                        end = h;
                    }
                    for (j = i / 3 * th; j < end; j++) {
                        memcpy(framebuffer + (j * w) + (i % 3) * tw,
                               d + (j * w) + (i % 3) * tw,
                               (i % 3 == 2) ? twr * 4 : tw * 4);
                    }
                }
            }
        } break;
        case 7: {  // Left/Right to Right/Left
            int i = (int)(sintrans * w);
            int j;
            for (j = 0; j < h / 2; j++) {
                memcpy(framebuffer + (i + j * w), p + (j * w), (w - i) * 4);
                memcpy(framebuffer + (j * w), d + ((j + 1) * w) - i, i * 4);
            }
            for (j = h / 2; j < h; j++) {
                memcpy(framebuffer + (j * w), p + (i + j * w), (w - i) * 4);
                memcpy(framebuffer + (j * w) + (w - i), d + (j * w), i * 4);
            }
        } break;
        case 8: {  // Left/Right to Center
            int i = (int)(sintrans * w / 2);
            int j;
            for (j = 0; j < h; j++) {
                memcpy(framebuffer + (j * w), d + ((j + 1) * w - i - w / 2), i * 4);
                memcpy(framebuffer + ((j + 1) * w - i), d + (j * w + w / 2), i * 4);
                memcpy(framebuffer + (j * w) + i, p + (j * w) + i, (w - i * 2) * 4);
            }
        } break;
        case 9: {  // Left/Right to Center, squeeze
            int i = (int)(sintrans * w / 2);
            int j;
            for (j = 0; j < h; j++) {
                if (i) {
                    int xl = i;
                    int xp = 0;
                    int dxp = ((w / 2) << 16) / xl;
                    int* ot = framebuffer + (j * w);
                    int* it = d + (j * w);
                    while (xl--) {
                        *ot++ = it[xp >> 16];
                        xp += dxp;
                    }
                }

                if (i * 2 != w) {
                    int xl = w - i * 2;
                    int xp = 0;
                    int dxp = (w << 16) / xl;
                    int* ot = framebuffer + (j * w) + i;
                    int* it = p + (j * w);
                    while (xl--) {
                        *ot++ = it[xp >> 16];
                        xp += dxp;
                    }
                }
                if (i) {
                    int xl = i;
                    int xp = 0;
                    int dxp = ((w / 2) << 16) / xl;
                    int* ot = framebuffer + (j * w) + w - i;
                    int* it = d + (j * w) + w / 2;
                    while (xl--) {
                        *ot++ = it[xp >> 16];
                        xp += dxp;
                    }
                }
            }
        } break;
        case 10: {  // Left to right wipe
            int i = (int)(sintrans * w);
            int j;
            for (j = 0; j < h; j++) {
                memcpy(framebuffer + (i + j * w), p + (j * w) + i, (w - i) * 4);
                memcpy(framebuffer + (j * w), d + (j * w), i * 4);
            }
        } break;
        case 11: {  // Right to left wipe
            int i = (int)(sintrans * w);
            int j;
            for (j = 0; j < h; j++) {
                memcpy(framebuffer + (j * w), p + (j * w), (w - i) * 4);
                memcpy(framebuffer + (j * w) + (w - i), d + (j * w) + (w - i), i * 4);
            }
        } break;
        case 12: {  // Top to bottom wipe
            int i = (int)(sintrans * h);
            memcpy(framebuffer, d, w * i * 4);
            memcpy(framebuffer + w * i, p + w * i, w * (h - i) * 4);
        } break;
        case 13: {  // Bottom to top wipe
            int i = (int)(sintrans * h);
            memcpy(framebuffer, p, w * (h - i) * 4);
            memcpy(framebuffer + w * (h - i), d + w * (h - i), w * i * 4);
        } break;
        case 14: {  // dot dissolve
            int i = ((int)(sintrans * 5)) - 5;
            int j;
            int t = 0;
            int dir = 1;

            if (i < 0) {
                dir = !dir;
                i++;
                i = -i;
            }
            i = 1 << i;
            for (j = 0; j < h; j++) {
                if (t++ == i) {
                    int x = w;
                    t = 0;
                    int t2 = 0;
                    int* of = framebuffer + j * w;
                    int* p2 = (dir ? p : d) + j * w;
                    int* d2 = (dir ? d : p) + j * w;
                    while (x--) {
                        if (t2++ == i) {
                            of[0] = p2[0];
                            t2 = 0;
                        } else {
                            of[0] = d2[0];
                        }
                        p2++;
                        d2++;
                        of++;
                    }
                } else {
                    memcpy(framebuffer + j * w, (dir ? d : p) + j * w, w * sizeof(int));
                }
            }
        } break;
        default: break;
    }

    if (n == 255) {
        int x;
        this->enabled = 0;
        start_time = 0;
        for (x = 0; x < 4; x++) {
            if (fbs[x]) {
                free(fbs[x]);
            }
            fbs[x] = nullptr;
        }
        g_single_instance->clear_secondary();
    }
    return 0;
}
