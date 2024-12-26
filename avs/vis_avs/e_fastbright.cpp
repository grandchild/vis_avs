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
#include "e_fastbright.h"

#include "timing.h"

#define GET_INT() \
    (data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24))
#define PUT_INT(y)                   \
    data[pos] = (y) & 255;           \
    data[pos + 1] = (y >> 8) & 255;  \
    data[pos + 2] = (y >> 16) & 255; \
    data[pos + 3] = (y >> 24) & 255

constexpr Parameter FastBright_Info::parameters[];

E_FastBright::E_FastBright(AVS_Instance* avs) : Configurable_Effect(avs) {
#ifdef NO_MMX
    int x;
    for (x = 0; x < 128; x++) {
        tab[0][x] = x + x;
        tab[1][x] = x << 9;
        tab[2][x] = x << 17;
    }
    for (; x < 256; x++) {
        tab[0][x] = 255;
        tab[1][x] = 255 << 8;
        tab[2][x] = 255 << 16;
    }
#endif
}

E_FastBright::~E_FastBright() {}

int E_FastBright::render(char[2][2][576],
                         int is_beat,
                         int* framebuffer,
                         int*,
                         int w,
                         int h) {
    if (is_beat & 0x80000000) {
        return 0;
    }
#ifdef NO_MMX
    // the non mmx x2 version really isn't in any terms faster than normal brightness
    // with no exclusions turned on
    {
        unsigned int* t = (unsigned int*)framebuffer;
        int x;
        unsigned int mask = 0x7F7F7F7F;

        x = w * h / 2;
        if (this->config.multiply == FASTBRIGHT_MULTIPLY_DOUBLE) {
            while (x--) {
                unsigned int v1 = t[0];
                unsigned int v2 = t[1];
                v1 = tab[0][v1 & 0xff] | tab[1][(v1 >> 8) & 0xff]
                     | tab[2][(v1 >> 16) & 0xff] | (v1 & 0xff000000);
                v2 = tab[0][v2 & 0xff] | tab[1][(v2 >> 8) & 0xff]
                     | tab[2][(v2 >> 16) & 0xff] | (v2 & 0xff000000);
                t[0] = v1;
                t[1] = v2;
                t += 2;
            }
        } else if (this->config.multiply == FASTBRIGHT_MULTIPLY_HALF) {
            while (x--) {
                unsigned int v1 = t[0] >> 1;
                unsigned int v2 = t[1] >> 1;
                t[0] = v1 & mask;
                t[1] = v2 & mask;
                t += 2;
            }
        }
    }
#else
    int mask[2] = {
        0x7F7F7F7F,
        0x7F7F7F7F,
    };
    int l = (w * h);
    if (this->config.multiply == FASTBRIGHT_MULTIPLY_DOUBLE) {
#ifdef _MSC_VER  // MSVC asm
        __asm {
			mov edx, l
			mov edi, framebuffer
      shr edx, 3  // 8 pixels at a time
      align 16
		fastbright_l1:
			movq mm0, [edi]
			movq mm1, [edi+8]
			movq mm2, [edi+16]
      paddusb mm0, mm0
			movq mm3, [edi+24]
      paddusb mm1, mm1
      paddusb mm2, mm2
      movq [edi], mm0
      paddusb mm3, mm3
      movq [edi+8], mm1
      movq [edi+16], mm2
      movq [edi+24], mm3

      add edi, 32

			dec edx
			jnz fastbright_l1

			mov edx, l
			and edx, 7
      shr edx, 1  // up the last 7 pixels (two at a time)
			jz fastbright_l3

		fastbright_l2:
			movq mm3, [edi]
      paddusb mm3, mm3
      movq [edi], mm3
			add edi, 8
			dec edx
			jnz fastbright_l2
		fastbright_l3:
			emms
        }
#else            // _MSC_VER, GCC asm
        __asm__ __volatile__(
            "mov     %%edx, %[l]\n\t"
            "mov     %%edi, %[framebuffer]\n\t"
            "shr     %%edx, 3\n\t"
            ".align  16\n"
            "fastbright_l1:\n\t"
            "movq    %%mm0, [%%edi]\n\t"
            "movq    %%mm1, [%%edi + 8]\n\t"
            "movq    %%mm2, [%%edi + 16]\n\t"
            "paddusb %%mm0, %%mm0\n\t"
            "movq    %%mm3, [%%edi+24]\n\t"
            "paddusb %%mm1, %%mm1\n\t"
            "paddusb %%mm2, %%mm2\n\t"
            "movq    [%%edi], %%mm0\n\t"
            "paddusb %%mm3, %%mm3\n\t"
            "movq    [%%edi + 8], %%mm1\n\t"
            "movq    [%%edi + 16], %%mm2\n\t"
            "movq    [%%edi + 24], %%mm3\n\t"
            "add     %%edi, 32\n\t"
            "dec     %%edx\n\t"
            "jnz     fastbright_l1\n\t"
            "mov     %%edx, %[l]\n\t"
            "and     %%edx, 7\n\t"
            "shr     %%edx, 1\n\t"
            "jz      fastbright_l3\n"
            "fastbright_l2:\n\t"
            "movq    %%mm3, [%%edi]\n\t"
            "paddusb %%mm3, %%mm3\n\t"
            "movq    [%%edi], %%mm3\n\t"
            "add     %%edi, 8\n\t"
            "dec     %%edx\n\t"
            "jnz     fastbright_l2\n"
            "fastbright_l3:\n\t"
            "emms\n\t"
            : /* no outputs */
            : [l] "m"(l), [framebuffer] "m"(framebuffer)
            : "edx", "esi");
#endif           // _MSC_VER
    } else if (this->config.multiply == FASTBRIGHT_MULTIPLY_HALF) {
#ifdef _MSC_VER  // MSVC asm
        __asm {
			mov edx, l
      movq mm7, [mask]
			mov edi, framebuffer
      shr edx, 3  // 8 pixels at a time
      align 16
		_lr1:
			movq mm0, [edi]
			movq mm1, [edi+8]

			movq mm2, [edi+16]
      psrl mm0, 1

			movq mm3, [edi+24]
      pand mm0, mm7
      
      psrl mm1, 1
      movq [edi], mm0

      psrl mm2, 1
      pand mm1, mm7

      movq [edi+8], mm1
      pand mm2, mm7

      psrl mm3, 1
      movq [edi+16], mm2

      pand mm3, mm7
      movq [edi+24], mm3

      add edi, 32

			dec edx
			jnz _lr1

			mov edx, l
			and edx, 7
      shr edx, 1  // up the last 7 pixels (two at a time)
			jz _lr3

		_lr2:
			movq mm3, [edi]
      psrl mm3, 1
      pand mm3, mm7
      movq [edi], mm3
			add edi, 8
			dec edx
			jnz _lr2
		_lr3:
			emms
        }
#else            // _MSC_VER, GCC asm
        __asm__ __volatile__(
            "mov   %%edx, %[l]\n\t"
            "movq  %%mm7, [%[mask]]\n\t"
            "mov   %%edi, %[framebuffer]\n\t"
            "shr   %%edx, 3\n\t"
            ".align 16\n"
            "_lr1:\n\t"
            "movq  %%mm0, [%%edi]\n\t"
            "movq  %%mm1, [%%edi + 8]\n\t"
            "movq  %%mm2, [%%edi + 16]\n\t"
            "psrlq %%mm0, 1\n\t"
            "movq  %%mm3, [%%edi + 24]\n\t"
            "pand  %%mm0, %%mm7\n\t"
            "psrlq %%mm1, 1\n\t"
            "movq  [%%edi], %%mm0\n\t"
            "psrlq %%mm2, 1\n\t"
            "pand  %%mm1, %%mm7\n\t"
            "movq  [%%edi + 8], %%mm1\n\t"
            "pand  %%mm2, %%mm7\n\t"
            "psrlq %%mm3, 1\n\t"
            "movq  [%%edi + 16], %%mm2\n\t"
            "pand  %%mm3, %%mm7\n\t"
            "movq  [%%edi + 24], %%mm3\n\t"
            "add   %%edi, 32\n\t"
            "dec   %%edx\n\t"
            "jnz   _lr1\n\t"
            "mov   %%edx, %[l]\n\t"
            "and   %%edx, 7\n\t"
            "shr   %%edx, 1\n\t"
            "jz    _lr3\n"
            "_lr2:\n\t"
            "movq  %%mm3, [%%edi]\n\t"
            "psrlq %%mm3, 1\n\t"
            "pand  %%mm3, %%mm7\n\t"
            "movq  [%%edi], %%mm3\n\t"
            "add   %%edi, 8\n\t"
            "dec   %%edx\n\t"
            "jnz   _lr2\n"
            "_lr3:\n\t"
            "emms\n\t"
            :
            : [l] "m"(l), [mask] "m"(mask), [framebuffer] "m"(framebuffer)
            : "edx", "edi");
#endif           // _MSC_VER
    }
#endif
    return 0;
}

void E_FastBright::load_legacy(unsigned char* data, int len) {
    int pos = 0;
    this->config.multiply = FASTBRIGHT_MULTIPLY_DOUBLE;
    if (len - pos >= 4) {
        int64_t multiply = GET_INT();
        if (multiply == 2) {
            this->enabled = false;
        } else if (multiply == 1) {
            this->config.multiply = FASTBRIGHT_MULTIPLY_HALF;
        }
        pos += 4;
    }
}

int E_FastBright::save_legacy(unsigned char* data) {
    int pos = 0;
    if (this->enabled) {
        PUT_INT(this->config.multiply);
    } else {
        PUT_INT(2);
    }
    pos += 4;
    return pos;
}

Effect_Info* create_FastBright_Info() { return new FastBright_Info(); }
Effect* create_FastBright(AVS_Instance* avs) { return new E_FastBright(avs); }
void set_FastBright_desc(char* desc) { E_FastBright::set_desc(desc); }
