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
// alphachannel safe 11/21/99
#include "e_fadeout.h"

#include "r_defs.h"

#include "timing.h"

constexpr Parameter Fadeout_Info::parameters[];

void maketab(Effect* component, const Parameter*, std::vector<int64_t>) {
    E_Fadeout* fadeout = (E_Fadeout*)component;
    fadeout->maketab();
}

void E_Fadeout::maketab(void) {
    int rseek = this->config.color & 0xff;
    int gseek = (this->config.color >> 8) & 0xff;
    int bseek = (this->config.color >> 16) & 0xff;
    int x;
    for (x = 0; x < 256; x++) {
        int r = x;
        int g = x;
        int b = x;
        if (r <= rseek - this->config.fadelen)
            r += this->config.fadelen;
        else if (r >= rseek + this->config.fadelen)
            r -= this->config.fadelen;
        else
            r = rseek;

        if (g <= gseek - this->config.fadelen)
            g += this->config.fadelen;
        else if (g >= gseek + this->config.fadelen)
            g -= this->config.fadelen;
        else
            g = gseek;
        if (b <= bseek - this->config.fadelen)
            b += this->config.fadelen;
        else if (b >= bseek + this->config.fadelen)
            b -= this->config.fadelen;
        else
            b = bseek;

        fadtab[0][x] = r;
        fadtab[1][x] = g;
        fadtab[2][x] = b;
    }
}

#define PUT_INT(y)                   \
    data[pos] = (y)&255;             \
    data[pos + 1] = (y >> 8) & 255;  \
    data[pos + 2] = (y >> 16) & 255; \
    data[pos + 3] = (y >> 24) & 255
#define GET_INT() \
    (data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24))
void E_Fadeout::load_legacy(unsigned char* data, int len) {
    int pos = 0;
    if (len - pos >= 4) {
        this->config.fadelen = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        this->config.color = GET_INT();
        pos += 4;
    }
    maketab();
}

int E_Fadeout::save_legacy(unsigned char* data) {
    int pos = 0;
    PUT_INT(this->config.fadelen);
    pos += 4;
    PUT_INT(this->config.color);
    pos += 4;
    return pos;
}

E_Fadeout::E_Fadeout() {
    this->config.color = 0;
    this->config.fadelen = 16;
    maketab();
}

E_Fadeout::~E_Fadeout() {}

int E_Fadeout::render(char[2][2][576],
                      int isBeat,
                      int* framebuffer,
                      int*,
                      int w,
                      int h) {
    if (isBeat & 0x80000000) return 0;
    if (!this->config.fadelen) return 0;
    timingEnter(1);
    if (
#ifdef NO_MMX
        1
#else
        this->config.color
#endif
    ) {
        unsigned char* t = (unsigned char*)framebuffer;
        int x = w * h;
        while (x--) {
            t[0] = fadtab[0][t[0]];
            t[1] = fadtab[1][t[1]];
            t[2] = fadtab[2][t[2]];
            t += 4;
        }
    }
#ifndef NO_MMX
    else {
        int l = (w * h);
        char fadj[8];
        int x;
        unsigned char* t = fadtab[0];
        for (x = 0; x < 8; x++) fadj[x] = this->config.fadelen;
#ifdef _MSC_VER  // MSVC asm
        __asm {
			mov edx, l
			mov edi, framebuffer
			movq mm7, [fadj]
			shr edx, 3
      align 16
		fadeout_l1:
			movq mm0, [edi]

			movq mm1, [edi+8]

			movq mm2, [edi+16]
			psubusb mm0, mm7

			movq mm3, [edi+24]
			psubusb mm1, mm7

			movq [edi], mm0
			psubusb mm2, mm7

			movq [edi+8], mm1
			psubusb mm3, mm7

			movq [edi+16], mm2

			movq [edi+24], mm3

			add edi, 8*4

			dec edx
			jnz fadeout_l1

			mov edx, l
			sub eax, eax

			and edx, 7
			jz _l3

			sub ebx, ebx
			sub ecx, ecx

			mov esi, t
		fadeout_l2:
			mov al, [edi]
			mov bl, [edi+1]
			mov cl, [edi+2]
			sub al, [esi+eax]
			sub bl, [esi+ebx]
			sub cl, [esi+ecx]
			mov [edi], al
			mov [edi+1], bl
			mov [edi+2], cl
			add edi, 4
			dec edx
			jnz fadeout_l2
		_l3:
			emms
        }
#else   // _MSC_VER, GCC asm
        __asm__ __volatile__(
            "mov     %%edx, %[l]\n\t"
            "mov     %%edi, %[framebuffer]\n\t"
            "movq    %%mm7, [%[fadj]]\n\t"
            "shr     %%edx, 3\n\t"
            ".align  16\n"
            "fadeout_l1:\n\t"
            "movq    %%mm0, [%%edi]\n\t"
            "movq    %%mm1, [%%edi + 8]\n\t"
            "movq    %%mm2, [%%edi + 16]\n\t"
            "psubusb %%mm0, %%mm7\n\t"
            "movq    %%mm3, [%%edi + 24]\n\t"
            "psubusb %%mm1, %%mm7\n\t"
            "movq    [%%edi], %%mm0\n\t"
            "psubusb %%mm2, %%mm7\n\t"
            "movq    [%%edi + 8], %%mm1\n\t"
            "psubusb %%mm3, %%mm7\n\t"
            "movq    [%%edi + 16], %%mm2\n\t"
            "movq    [%%edi + 24], %%mm3\n\t"
            "add     %%edi, 8 * 4\n\t"
            "dec     %%edx\n\t"
            "jnz     fadeout_l1\n\t"
            "mov     %%edx, %[l]\n\t"
            "sub     %%eax, %%eax\n\t"
            "and     %%edx, 7\n\t"
            "jz      _l3\n\t"
            "sub     %%ebx, %%ebx\n\t"
            "sub     %%ecx, %%ecx\n\t"
            "mov     %%esi, %[t]\n"
            "fadeout_l2:\n\t"
            "mov     al, [%%edi]\n\t"
            "mov     bl, [%%edi + 1]\n\t"
            "mov     cl, [%%edi + 2]\n\t"
            "sub     al, [%%esi + %%eax]\n\t"
            "sub     bl, [%%esi + %%ebx]\n\t"
            "sub     cl, [%%esi + %%ecx]\n\t"
            "mov     [%%edi], %%al\n\t"
            "mov     [%%edi + 1], %%bl\n\t"
            "mov     [%%edi + 2], %%cl\n\t"
            "add     %%edi, 4\n\t"
            "dec     %%edx\n\t"
            "jnz     fadeout_l2\n"
            "_l3:\n\t"
            "emms\n\t"
            : /* no outputs */
            : [l] "m"(l), [framebuffer] "m"(framebuffer), [fadj] "m"(fadj), [t] "m"(t)
            : "eax", "ebx", "ecx", "edx", "edi", "esi");
#endif  // _MSC_VER
    }
#endif
    timingLeave(1);
    return 0;
}

Effect_Info* create_Fadeout_Info() { return new Fadeout_Info(); }
Effect* create_Fadeout() { return new E_Fadeout(); }
