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
#include "e_colorreduction.h"

#include "r_defs.h"

#include "../util.h"  // ssizeof32()

constexpr Parameter ColorReduction_Info::parameters[];

E_ColorReduction::E_ColorReduction() {}
E_ColorReduction::~E_ColorReduction() {}

int E_ColorReduction::render(char[2][2][576],
                             int isBeat,
                             int* framebuffer,
                             int*,
                             int w,
                             int h) {
    if (isBeat & 0x80000000) return 0;

    int a, b, c;
    a = 8 - config.levels;
    b = 0xFF;
    while (a--) b = (b << 1) & 0xFF;
    b |= (b << 16) | (b << 8);
    c = w * h;
#ifdef _MSC_VER
    __asm {
		mov ebx, framebuffer;
		mov ecx, c;
		mov edx, b;
		lp:
		sub ecx, 4;
		test ecx, ecx;
		jz end;
		and dword ptr [ebx+ecx*4], edx;
		and dword ptr [ebx+ecx*4+4], edx;
		and dword ptr [ebx+ecx*4+8], edx;
		and dword ptr [ebx+ecx*4+12], edx;
		jmp lp;
		end:
    }
#else  // _MSC_VER
    __asm__ __volatile__(
        "mov %%ebx, %0\n\t"
        "mov %%ecx, %1\n\t"
        "mov %%edx, %2\n"
        "lp:\n\t"
        "sub %%ecx, 4\n\t"
        "test %%ecx, %%ecx\n\t"
        "jz end\n\t"
        // TODO [performance]: could be put into one or two packed instructions?
        "and dword ptr [%%ebx + %%ecx * 4], %%edx\n\t"
        "and dword ptr [%%ebx + %%ecx * 4 + 4], %%edx\n\t"
        "and dword ptr [%%ebx + %%ecx * 4 + 8], %%edx\n\t"
        "and dword ptr [%%ebx + %%ecx * 4 + 12], %%edx\n\t"
        "jmp lp\n"
        "end:"
        : /* no outputs */
        : "m"(framebuffer), "m"(c), "m"(b)
        : "ebx", "ecx", "edx");
#endif
    return 0;
}

void E_ColorReduction::load_legacy(unsigned char* data, int len) {
    if (len - LEGACY_SAVE_PATH_LEN >= ssizeof32(uint32_t)) {
        this->config.levels = *(uint32_t*)&data[LEGACY_SAVE_PATH_LEN];
    } else {
        this->config.levels = COLRED_LEVELS_256;
    }
}

int E_ColorReduction::save_legacy(unsigned char* data) {
    memset(data, '\0', LEGACY_SAVE_PATH_LEN);
    *(uint32_t*)&data[LEGACY_SAVE_PATH_LEN] = (uint32_t)this->config.levels;
    return LEGACY_SAVE_PATH_LEN + sizeof(uint32_t);
}

Effect_Info* create_ColorReduction_Info() { return new ColorReduction_Info(); }
Effect* create_ColorReduction() { return new E_ColorReduction(); }
void set_ColorReduction_desc(char* desc) { E_ColorReduction::set_desc(desc); }
