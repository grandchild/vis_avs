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
#include "e_invert.h"

#include "r_defs.h"

#include <stdlib.h>

E_Invert::E_Invert() { enabled = 1; }
E_Invert::~E_Invert() {}

int E_Invert::render(char[2][2][576],
                     int isBeat,
                     int* framebuffer,
                     int*,
                     int w,
                     int h) {
    int i = w * h;
    int* p = framebuffer;

    if (isBeat & 0x80000000) {
        return 0;
    }
    if (!enabled) {
        return 0;
    }

#ifndef NO_MMX
    int a[2] = {0xffffff, 0xffffff};
#ifdef _MSC_VER  // MSVC asm
    __asm {
      mov ecx, i
      shr ecx, 3
      movq mm0, [a]
      mov edi, p
      align 16
_mmx_invert_loop:
      movq mm1, [edi]
      movq mm2, [edi+8]
      pxor mm1, mm0
      movq mm3, [edi+16]
      pxor mm2, mm0
      movq mm4, [edi+24]
      pxor mm3, mm0
      movq [edi], mm1
      pxor mm4, mm0
      movq [edi+8], mm2
      movq [edi+16], mm3
      movq [edi+24], mm4
      add edi, 32
      dec ecx
      jnz _mmx_invert_loop
      mov ecx, i
      shr ecx, 1
      and ecx, 3
      jz _mmx_invert_noendloop
_mmx_invert_endloop:
      movq mm1, [edi]
      pxor mm1, mm0
      movq [edi], mm1
      add edi, 8
      dec ecx
      jnz _mmx_invert_endloop

_mmx_invert_noendloop:
      emms
    }
#else   // _MSC_VER, GCC asm
    __asm__ __volatile__(
        "mov    %%ecx, %[i]\n\t"
        "shr    %%ecx, 3\n\t"
        "movq   %%mm0, [%[a]]\n\t"
        "mov    %%edi, %[p]\n\t"
        ".align 16\n"
        "_mmx_invert_loop:\n\t"
        "movq   %%mm1, [%%edi]\n\t"
        "movq   %%mm2, [%%edi + 8]\n\t"
        "pxor   %%mm1, %%mm0\n\t"
        "movq   %%mm3, [%%edi + 16]\n\t"
        "pxor   %%mm2, %%mm0\n\t"
        "movq   %%mm4, [%%edi + 24]\n\t"
        "pxor   %%mm3, %%mm0\n\t"
        "movq   [%%edi], %%mm1\n\t"
        "pxor   %%mm4, %%mm0\n\t"
        "movq   [%%edi + 8], %%mm2\n\t"
        "movq   [%%edi + 16], %%mm3\n\t"
        "movq   [%%edi + 24], %%mm4\n\t"
        "add    %%edi, 32\n\t"
        "dec    %%ecx\n\t"
        "jnz    _mmx_invert_loop\n\t"
        "mov    %%ecx, %[i]\n\t"
        "shr    %%ecx, 1\n\t"
        "and    %%ecx, 3\n\t"
        "jz     _mmx_invert_noendloop\n"
        "_mmx_invert_endloop:\n\t"
        "movq   %%mm1, [%%edi]\n\t"
        "pxor   %%mm1, %%mm0\n\t"
        "movq   [%%edi], %%mm1\n\t"
        "add    %%edi, 8\n\t"
        "dec    %%ecx\n\t"
        "jnz    _mmx_invert_endloop\n"
        "_mmx_invert_noendloop:\n\t"
        "emms\n\t"
        : /* no outputs */
        : [i] "m"(i), [a] "m"(a), [p] "m"(p)
        : "ecx", "edi");
#endif  // _MSC_VER
#else
    while (i--) {
        *p++ = 0xFFFFFF ^ *p;
    }
#endif
    return 0;
}

#define GET_INT() \
    (data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24))
void E_Invert::load_legacy(unsigned char* data, int len) {
    int pos = 0;
    if (len - pos >= 4) {
        enabled = GET_INT();
        pos += 4;
    }
}

#define PUT_INT(y)                   \
    data[pos] = (y)&255;             \
    data[pos + 1] = (y >> 8) & 255;  \
    data[pos + 2] = (y >> 16) & 255; \
    data[pos + 3] = (y >> 24) & 255
int E_Invert::save_legacy(unsigned char* data) {
    int pos = 0;
    PUT_INT(enabled);
    pos += 4;
    return pos;
}

Effect_Info* create_Invert_Info() { return new Invert_Info(); }
Effect* create_Invert() { return new E_Invert(); }
void set_Invert_desc(char* desc) { E_Invert::set_desc(desc); }
