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
#include "render.h"

#include "timing.h"
#include "undo.h"
#include "wa_ipc.h"
#include "wnd.h"

AVS_Instance* g_single_instance;
Transition* g_render_transition;
C_RLibrary* g_render_library;

int is_mmx(void) {
    DWORD retval1, retval2;
    try {
#ifdef _MSC_VER
        // TODO [cleanup]: This is CPUID, maybe it wasn't available in MASM at the time?
        _asm {
			mov eax, 1           // set up CPUID to return processor version and features
            //	0 = vendor string, 1 = version info, 2 = cache info
			_emit 0x0f  // code bytes = 0fh,  0a2h
			_emit 0xa2
			mov retval1, eax		
			mov retval2, edx
        }
#else  // _MSC_VER
        __asm__ __volatile__(
            "mov %%eax, 1\n\t"
            "cpuid\n\t"
            "mov %0, %%eax\n\t"
            "mov %1, %%edx\n\t"
            : "=m"(retval1), "=m"(retval2)
            : /* no inputs */
            : "eax", "ebx", "ecx", "edx");
#endif
    } catch (...) {
        retval1 = retval2 = 0;
    }
    if (!retval1) {
        return 0;
    }
    return (retval2 & 0x800000) ? 1 : 0;
}

unsigned char g_blendtable[256][256];
unsigned int const mmx_blend4_revn[2] = {0xff00ff, 0xff00ff};  //{0x1000100,0x1000100};
                                                               //<<- this is actually
                                                               // more correct, but
                                                               // we're going for
                                                               // consistency vs. the
                                                               // non-mmx ver-jf
int const mmx_blendadj_mask[2] = {0xff00ff, 0xff00ff};
int const mmx_blend4_zero = 0;

void Render_Init() {
    timingInit();
    {
        int i, j;
        for (j = 0; j < 256; j++) {
            for (i = 0; i < 256; i++) {
                g_blendtable[i][j] = (unsigned char)((i / 255.0) * (float)j);
            }
        }
    }

    g_render_library = new C_RLibrary();
    g_single_instance = new AVS_Instance(g_path, AVS_AUDIO_INTERNAL, AVS_BEAT_INTERNAL);
    g_render_transition = new Transition();

    char INI_FILE[MAX_PATH];
    char* p = INI_FILE;
    strncpy(INI_FILE,
            (char*)SendMessage(GetWinampHwnd(), WM_WA_IPC, 0, IPC_GETINIFILE),
            MAX_PATH - 1);
    INI_FILE[MAX_PATH - 1] = '\0';
    p += strlen(INI_FILE) - 1;
    while (p >= INI_FILE && *p != '\\') {
        p--;
    }
    strcpy(p, "\\plugins\\vis_avs.dat");
    extern int g_saved_preset_dirty;
    // clear the undo stack before loading a file.
    C_UndoStack::clear();
    g_single_instance->preset_load_file(INI_FILE);
    // then add the new load to the undo stack but mark it clean if it is supposed to be
    C_UndoStack::save_undo();
    if (!g_saved_preset_dirty) {
        C_UndoStack::clear_dirty();
    }
}

void Render_Quit() {
    delete g_render_transition;
    g_render_transition = nullptr;
    if (g_single_instance) {
        char INI_FILE[MAX_PATH];
        char* p = INI_FILE;
        strncpy(INI_FILE,
                (char*)SendMessage(GetWinampHwnd(), WM_WA_IPC, 0, IPC_GETINIFILE),
                MAX_PATH - 1);
        INI_FILE[MAX_PATH - 1] = '\0';
        p += strlen(INI_FILE) - 1;
        while (p >= INI_FILE && *p != '\\') {
            p--;
        }
        strcpy(p, "\\plugins\\vis_avs.dat");
        g_single_instance->preset_save_file_legacy(INI_FILE);
    }

    delete g_single_instance;
    g_single_instance = NULL;
    if (g_render_library) {
        delete g_render_library;
    }
    g_render_library = NULL;

    timingPrint();
}
