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
#include "e_multiplier.h"

#include "r_defs.h"

constexpr Parameter Multiplier_Info::parameters[];

E_Multiplier::E_Multiplier(AVS_Instance* avs) : Configurable_Effect(avs) {}

E_Multiplier::~E_Multiplier() {}

int E_Multiplier::render(char[2][2][576],
                         int is_beat,
                         int* framebuffer,
                         int*,
                         int w,
                         int h) {
    if (is_beat & 0x80000000) {
        return 0;
    }

    int /*b,*/ c;  // TODO [cleanup]: see below
    __int64 mask;

    c = w * h;
    switch (config.multiply) {
        case MULTIPLY_XI:
#ifdef _MSC_VER
            __asm {
			mov ebx, framebuffer;
			mov ecx, c;
                // mov	edx, b; // doesn't do anything. a leftover?
			lp0:
			xor eax, eax;
			dec ecx;
			test ecx, ecx;
			jz end;
			mov eax, dword ptr [ebx+ecx*4];
			test eax, eax;
			jz sk0;
			mov eax, 0xFFFFFF;
			sk0:
			mov [ebx+ecx*4], eax;
			jmp lp0;
            }
#else  // _MSC_VER
            __asm__ goto(
                "mov %%ebx, %0\n\t"
                "mov %%ecx, %1\n"
                "lp0:\n\t"
                "xor %%eax, %%eax\n\t"
                "dec %%ecx\n\t"
                "test %%ecx, %%ecx\n\t"
                "jz %l[end]\n\t"
                "mov %%eax, dword ptr [%%ebx + %%ecx * 4]\n\t"
                "test %%eax, %%eax\n\t"
                "jz sk0\n\t"
                "mov %%eax, 0xFFFFFF\n"
                "sk0:\n\t"
                "mov [%%ebx + %%ecx * 4], %%eax\n\t"
                "jmp lp0\n\t"
                : /* no outputs */
                : "m"(framebuffer), "m"(c)
                : "eax", "ebx", "ecx"
                : end);
#endif
            break;
        case MULTIPLY_XS:
#ifdef _MSC_VER
            __asm {
			mov ebx, framebuffer;
			mov ecx, c;
                // mov	edx, b; // doesn't do anything. a leftover?
			lp9:
			xor eax, eax;
			dec ecx;
			test ecx, ecx;
			jz end;
			mov eax, dword ptr [ebx+ecx*4];
			cmp eax, 0xFFFFFF;
			je sk9;  // TODO [bugfix]: this could be jae, to account for garbage in
                         // MSByte
                // TODO [performance]: the next _3_ lines could be compressed into just
                //          mov [ebx+ecx*4], 0x000000
                //          sk9:
                //      since the white pixel (= 0xFFFFFF) doesn't need to be rewritten
			mov eax, 0x000000;
			sk9:
			mov [ebx+ecx*4], eax;
			jmp lp9;
                }
#else  // _MSC_VER
            __asm__ goto(
                "mov %%ebx, %0\n\t"
                "mov %%ecx, %1\n"
                "lp9:\n\t"
                "xor %%eax, %%eax\n\t"
                "dec %%ecx\n\t"
                "test %%ecx, %%ecx\n\t"
                "jz %l[end]\n\t"
                "mov %%eax, dword ptr [%%ebx + %%ecx * 4]\n\t"
                "cmp %%eax, 0xFFFFFF\n\t"
                "je sk9\n\t"
                "mov %%eax, 0x000000\n"
                "sk9:\n\t"
                "mov [%%ebx + %%ecx * 4], %%eax\n\t"
                "jmp lp9\n\t"
                : /* no outputs */
                : "m"(framebuffer), "m"(c)
                : "eax", "ebx", "ecx"
                : end);
#endif
            break;
        case MULTIPLY_X8: c = w * h / 2;
#ifdef _MSC_VER
            __asm {
			mov ebx, framebuffer;
			mov ecx, c;
			lp1:
			movq mm0, [ebx];
			paddusb mm0, mm0;
			paddusb mm0, mm0;
			paddusb mm0, mm0;
			movq [ebx], mm0;
			add ebx, 8;
			dec ecx;
			test ecx, ecx;
			jz end;
			jmp lp1;
            }
#else  // _MSC_VER
            __asm__ goto(
                "mov %%ebx, %0\n\t"
                "mov %%ecx, %1\n"
                "lp1:\n\t"
                "movq %%mm0, [%%ebx]\n\t"
                "paddusb %%mm0, %%mm0\n\t"
                "paddusb %%mm0, %%mm0\n\t"
                "paddusb %%mm0, %%mm0\n\t"
                "movq [%%ebx], %%mm0\n\t"
                "add %%ebx, 8\n\t"
                "dec %%ecx\n\t"
                "test %%ecx, %%ecx\n\t"
                "jz %l[end]\n\t"
                "jmp lp1\n\t"
                : /* no outputs */
                : "m"(framebuffer), "m"(c)
                : "ebx", "ecx"
                : end);
#endif
            break;
        case MULTIPLY_X4: c = w * h / 2;
#ifdef _MSC_VER
            __asm {
			mov ebx, framebuffer;
			mov ecx, c;
			lp2:
			movq mm0, [ebx];
			paddusb mm0, mm0;
			paddusb mm0, mm0;
			movq [ebx], mm0;
			add ebx, 8;
			dec ecx;
			test ecx, ecx;
			jz end;
			jmp lp2;
            }
#else  // _MSC_VER
            __asm__ goto(
                "mov %%ebx, %0\n\t"
                "mov %%ecx, %1\n"
                "lp2:\n\t"
                "movq %%mm0, [%%ebx]\n\t"
                "paddusb %%mm0, %%mm0\n\t"
                "paddusb %%mm0, %%mm0\n\t"
                "movq [%%ebx], %%mm0\n\t"
                "add %%ebx, 8\n\t"
                "dec %%ecx\n\t"
                "test %%ecx, %%ecx\n\t"
                "jz %l[end]\n\t"
                "jmp lp2\n\t"
                : /* no outputs */
                : "m"(framebuffer), "m"(c)
                : "ebx", "ecx"
                : end);
#endif
            break;
        case MULTIPLY_X2: c = w * h / 2;
#ifdef _MSC_VER
            __asm {
			mov ebx, framebuffer;
			mov ecx, c;
			lp3:
			movq mm0, [ebx];
			paddusb mm0, mm0;
			movq [ebx], mm0;
			add ebx, 8;
			dec ecx;
			test ecx, ecx;
			jz end;
			jmp lp3;
            }
#else  // _MSC_VER
            __asm__ goto(
                "mov %%ebx, %0\n\t"
                "mov %%ecx, %1\n"
                "lp3:\n\t"
                "movq %%mm0, [%%ebx]\n\t"
                "paddusb %%mm0, %%mm0\n\t"
                "movq [%%ebx], %%mm0\n\t"
                "add %%ebx, 8\n\t"
                "dec %%ecx\n\t"
                "test %%ecx, %%ecx\n\t"
                "jz %l[end]\n\t"
                "jmp lp3\n\t"
                : /* no outputs */
                : "m"(framebuffer), "m"(c)
                : "ebx", "ecx"
                : end);
#endif
            break;
        case MULTIPLY_X05: c = w * h / 2; mask = 0x7F7F7F7F7F7F7F7F;
#ifdef _MSC_VER
            __asm {
			mov ebx, framebuffer;
			mov ecx, c;
			movq mm1, mask;
			lp4:
			movq mm0, [ebx];
			psrlq mm0, 1;
			pand mm0, mm1;
			movq [ebx], mm0;
			add ebx, 8;
			dec ecx;
			test ecx, ecx;
			jz end;
			jmp lp4;
            }
#else  // _MSC_VER
            __asm__ goto(
                "mov %%ebx, %0\n\t"
                "mov %%ecx, %1\n\t"
                "movq %%mm1, %2\n"
                "lp4:\n\t"
                "movq %%mm0, [%%ebx]\n\t"
                "psrlq %%mm0, 1\n\t"
                "pand %%mm0, %%mm1\n\t"
                "movq [%%ebx], %%mm0\n\t"
                "add %%ebx, 8\n\t"
                "dec %%ecx\n\t"
                "test %%ecx, %%ecx\n\t"
                "jz %l[end]\n\t"
                "jmp lp4\n\t"
                : /* no outputs */
                : "m"(framebuffer), "m"(c), "m"(mask)
                : "ebx", "ecx"
                : end);
#endif
            break;
        case MULTIPLY_X025: c = w * h / 2; mask = 0x3F3F3F3F3F3F3F3F;
#ifdef _MSC_VER
            __asm {
			mov ebx, framebuffer;
			mov ecx, c;
			movq mm1, mask;
			lp5:
			movq mm0, [ebx];
			psrlq mm0, 2;
			pand mm0, mm1;
			movq [ebx], mm0;
			add ebx, 8;
			dec ecx;
			test ecx, ecx;
			jz end;
			jmp lp5;
            }
#else  // _MSC_VER
            __asm__ goto(
                "mov %%ebx, %0\n\t"
                "mov %%ecx, %1\n\t"
                "movq %%mm1, %2\n"
                "lp5:\n\t"
                "movq %%mm0, [%%ebx]\n\t"
                "psrlq %%mm0, 2\n\t"
                "pand %%mm0, %%mm1\n\t"
                "movq [%%ebx], %%mm0\n\t"
                "add %%ebx, 8\n\t"
                "dec %%ecx\n\t"
                "test %%ecx, %%ecx\n\t"
                "jz %l[end]\n\t"
                "jmp lp5\n\t"
                : /* no outputs */
                : "m"(framebuffer), "m"(c), "m"(mask)
                : "ebx", "ecx"
                : end);
#endif
            break;
        case MULTIPLY_X0125: c = w * h / 2; mask = 0x1F1F1F1F1F1F1F1F;
#ifdef _MSC_VER
            __asm {
			mov ebx, framebuffer;
			mov ecx, c;
			movq mm1, mask;
			lp6:
			movq mm0, [ebx];
			psrlq mm0, 3;
			pand mm0, mm1;
			movq [ebx], mm0;
			add ebx, 8;
			dec ecx;
			test ecx, ecx;
			jz end;
			jmp lp6;
            }
#else  // _MSC_VER
            __asm__ goto(
                "mov %%ebx, %0\n\t"
                "mov %%ecx, %1\n\t"
                "movq %%mm1, %2\n"
                "lp6:\n\t"
                "movq %%mm0, [%%ebx]\n\t"
                "psrlq %%mm0, 3\n\t"
                "pand %%mm0, %%mm1\n\t"
                "movq [%%ebx], %%mm0\n\t"
                "add %%ebx, 8\n\t"
                "dec %%ecx\n\t"
                "test %%ecx, %%ecx\n\t"
                "jz %l[end]\n\t"
                "jmp lp6\n\t"
                : /* no outputs */
                : "m"(framebuffer), "m"(c), "m"(mask)
                : "ebx", "ecx"
                : end);
#endif
            break;
    }
end:
#ifdef _MSC_VER
    __asm emms;
#else  // _MSC_VER
    __asm__ __volatile__("emms");
#endif
    return 0;
}

void E_Multiplier::load_legacy(unsigned char* data, int len) {
    if (len == sizeof(uint32_t)) {
        this->config.multiply = *(uint32_t*)data;
    } else {
        this->config.multiply = MULTIPLY_X2;
    }
}

int E_Multiplier::save_legacy(unsigned char* data) {
    *(uint32_t*)data = (uint32_t)this->config.multiply;
    return sizeof(uint32_t);
}

Effect_Info* create_Multiplier_Info() { return new Multiplier_Info(); }
Effect* create_Multiplier(AVS_Instance* avs) { return new E_Multiplier(avs); }
void set_Multiplier_desc(char* desc) { E_Multiplier::set_desc(desc); }
