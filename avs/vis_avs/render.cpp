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

#include <immintrin.h>

int is_mmx(void) { return _may_i_use_cpu_feature(_FEATURE_MMX); }

void Render_Init() {
    timingInit();

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
    C_UndoStack::save_undo(g_single_instance);
    if (!g_saved_preset_dirty) {
        C_UndoStack::clear_dirty();
    }
}

void Render_Quit() {
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

    timingPrint();
}
