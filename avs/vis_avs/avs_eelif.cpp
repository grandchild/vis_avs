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
#include "avs_eelif.h"

#include "../3rdparty/WDL-EEL2/eel2/ns-eel-addfuncs.h"
#include "../platform.h"

#include <stdlib.h>
#include <string.h>

#ifdef CAN_TALK_TO_WINAMP
#include <windows.h>
#endif

char last_error_string[1024];
int g_log_errors;
lock_t* g_eval_cs;
static char* g_evallib_visdata;

/////////////////////// begin AVS specific script functions

static double getvis(unsigned char* visdata, int bc, int bw, int ch, int xorv) {
    int x;
    int accum = 0;
    if (ch && ch != 1 && ch != 2) {
        return 0.0;
    }

    if (bw < 1) {
        bw = 1;
    }
    bc -= bw / 2;
    if (bc < 0) {
        bw += bc;
        bc = 0;
    }
    if (bc > 575) {
        bc = 575;
    }
    if (bc + bw > 576) {
        bw = 576 - bc;
    }

    if (!ch) {
        for (x = 0; x < bw; x++) {
            accum += (visdata[bc] ^ xorv) - xorv;
            accum += (visdata[bc + 576] ^ xorv) - xorv;
            bc++;
        }
        return (double)accum / ((double)bw * 255.0);
    } else {
        if (ch == 2) {
            visdata += 576;
        }
        for (x = 0; x < bw; x++) {
            accum += (visdata[bc++] ^ xorv) - xorv;
        }
        return (double)accum / ((double)bw * 127.5);
    }
}

static double getspec(double band, double bandw, double chan) {
    if (!g_evallib_visdata) {
        return 0.0;
    }
    return getvis((unsigned char*)g_evallib_visdata,
                  (int)(band * 576.0),
                  (int)(bandw * 576.0),
                  (int)(chan + 0.5),
                  0)
           * 0.5;
}

static double getosc(double band, double bandw, double chan) {
    if (!g_evallib_visdata) {
        return 0.0;
    }
    return getvis((unsigned char*)g_evallib_visdata + 576 * 2,
                  (int)(band * 576.0),
                  (int)(bandw * 576.0),
                  (int)(chan + 0.5),
                  128);
}

static double gettime(double sc) {
#ifdef CAN_TALK_TO_WINAMP
    int ispos;
    if ((ispos = (sc > -1.001 && sc < -0.999)) || (sc > -2.001 && sc < -1.999)) {
        int pos = 0;

        extern HWND hwnd_WinampParent;
        if (IsWindow(hwnd_WinampParent)) {
            if (!SendMessageTimeout(hwnd_WinampParent,
                                    WM_USER,
                                    (WPARAM)!ispos,
                                    (LPARAM)105,
                                    SMTO_BLOCK,
                                    50,
                                    (LPDWORD)&pos)) {
                pos = 0;
            }
        }
        if (!ispos) {
            return (double)pos;
        }
        return pos / 1000.0;
    }
#endif  // CAN_TALK_TO_WINAMP
    return timer_ms() / 1000.0 - sc;
}

static double setmousepos(double x, double y) {
    // fucko: implement me
    (void)x, (void)y;
    return 0.0;
}

static double getmouse(double which) {
    int w = (int)(which + 0.5);

#ifdef CAN_TALK_TO_WINAMP
    if (w > 5) {
        return (GetAsyncKeyState(w) & 0x8000) ? 1.0 : 0.0;
    }

    if (w == 1 || w == 2) {
        POINT p;
        GetCursorPos(&p);
        extern double DDraw_translatePoint(POINT p, int isY);
        return DDraw_translatePoint(p, w == 2);
    }
    if (w == 3) {
        return (GetAsyncKeyState(MK_LBUTTON) & 0x8000) ? 1.0 : 0.0;
    }
    if (w == 4) {
        return (GetAsyncKeyState(MK_RBUTTON) & 0x8000) ? 1.0 : 0.0;
    }
    if (w == 5) {
        return (GetAsyncKeyState(MK_MBUTTON) & 0x8000) ? 1.0 : 0.0;
    }
#endif  // CAN_TALK_TO_WINAMP
    return 0.0;
}

/////////////////////// end AVS specific script functions

void AVS_EEL_IF_init() {
    g_eval_cs = lock_init();
    NSEEL_init();
    NSEEL_addfunc_retval("getosc", 3, NSEEL_PProc_THIS, (void*)getosc);
    NSEEL_addfunc_retval("getspec", 3, NSEEL_PProc_THIS, (void*)getspec);
    NSEEL_addfunc_retval("gettime", 1, NSEEL_PProc_THIS, (void*)gettime);
    NSEEL_addfunc_retval("getkbmouse", 1, NSEEL_PProc_THIS, (void*)getmouse);
    NSEEL_addfunc_retval("setmousepos", 2, NSEEL_PProc_THIS, (void*)setmousepos);
}
void AVS_EEL_IF_quit() {
    lock_destroy(g_eval_cs);
    NSEEL_quit();
}

static void movestringover(char* str, int amount) {
    char tmp[1024 + 8];

    // l = min(1024-amount-1, strlen(str));
    int l = strnlen(str, 1024 - amount - 1);

    memcpy(tmp, str, l + 1);

    while (l >= 0 && tmp[l] != '\n') {
        l--;
    }
    l++;

    tmp[l] = 0;  // ensure we null terminate

    memcpy(str + amount, tmp, l + 1);
}

int AVS_EEL_IF_Compile(int context, char* code) {
    NSEEL_CODEHANDLE ret;
    lock_lock(g_eval_cs);
    ret = NSEEL_code_compile((NSEEL_VMCTX)context, code, 0);
    if (!ret) {
        if (g_log_errors) {
            char* expr = NSEEL_code_getcodeerror((NSEEL_VMCTX)context);
            if (expr) {
                int l = strlen(expr);
                if (l > 512) {
                    l = 512;
                }
                movestringover(last_error_string, l + 2);
                memcpy(last_error_string, expr, l);
                last_error_string[l] = '\r';
                last_error_string[l + 1] = '\n';
            }
        }
    }
    lock_unlock(g_eval_cs);
    return (int)ret;
}

void AVS_EEL_IF_Execute(void* handle, char visdata[2][2][576]) {
    if (handle) {
        lock_lock(g_eval_cs);
        g_evallib_visdata = (char*)visdata;
        NSEEL_code_execute((NSEEL_CODEHANDLE)handle);
        g_evallib_visdata = NULL;
        lock_unlock(g_eval_cs);
    }
}

void AVS_EEL_IF_Free(int handle) { NSEEL_code_free((NSEEL_CODEHANDLE)handle); }

void AVS_EEL_IF_resetvars(NSEEL_VMCTX ctx) { NSEEL_VM_remove_all_nonreg_vars(ctx); }

void AVS_EEL_IF_VM_free(NSEEL_VMCTX ctx) { NSEEL_VM_free(ctx); }

void NSEEL_HOSTSTUB_EnterMutex() {}
void NSEEL_HOSTSTUB_LeaveMutex() {}
