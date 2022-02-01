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
#pragma once

#include "r_defs.h"

#include "undo.h"

class C_RenderTransitionClass {
   protected:
    int* fbs[4];
    int ep[2];
    int l_w, l_h;
    int enabled;
    uint64_t start_time;
    int curtrans;
    int mask;
    void* initThread;
    char last_file[MAX_PATH];
    int last_which;
    int _dotransitionflag;
    bool prev_renders_need_cleanup;

   public:
    static unsigned int __stdcall m_initThread(void* p);

    int LoadPreset(char* file, int which, C_UndoItem* item = 0);  // 0 on success
    void clean_prev_renders_if_needed();
    C_RenderTransitionClass();
    virtual ~C_RenderTransitionClass();

    virtual int render(char visdata[2][2][576],
                       int isBeat,
                       int* framebuffer,
                       int* fbout,
                       int w,
                       int h);

    char* transitionmodes[15] = {
        "Random",
        "Cross dissolve",
        "L/R Push",
        "R/L Push",
        "T/B Push",
        "B/T Push",
        "9 Random Blocks",
        "Split L/R Push",
        "L/R to Center Push",
        "L/R to Center Squeeze",
        "L/R Wipe",
        "R/L Wipe",
        "T/B Wipe",
        "B/T Wipe",
        "Dot Dissolve",
    };
};
