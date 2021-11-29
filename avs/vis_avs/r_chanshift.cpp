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
#include "c_chanshift.h"

#include "r_defs.h"

#include "../util.h"

#include <time.h>

// historically, these were the radio buttons' resource IDs, hence the weird numbers.
#define CHANSHIFT_MODE_RBG 1020
#define CHANSHIFT_MODE_BRG 1019
#define CHANSHIFT_MODE_BGR 1021
#define CHANSHIFT_MODE_GBR 1018
#define CHANSHIFT_MODE_GRB 1022
#define CHANSHIFT_MODE_RGB 1183

// set up default configuration
C_THISCLASS::C_THISCLASS() {
    memset(&config, 0, sizeof(channelShiftConfig));
    config.mode = CHANSHIFT_MODE_RBG;
    config.onbeat = 1;
}

// virtual destructor
C_THISCLASS::~C_THISCLASS() {}

int C_THISCLASS::render(char[2][2][576],
                        int isBeat,
                        int* framebuffer,
                        int*,
                        int w,
                        int h) {
    if (isBeat & 0x80000000) return 0;

    int c;
    int modes[] = {CHANSHIFT_MODE_RGB,
                   CHANSHIFT_MODE_RBG,
                   CHANSHIFT_MODE_GBR,
                   CHANSHIFT_MODE_GRB,
                   CHANSHIFT_MODE_BRG,
                   CHANSHIFT_MODE_BGR};

    if (isBeat && config.onbeat) {
        config.mode = modes[rand() % 6];
    }

    c = w * h;

    // TODO [performance]: All of these byte-swapping sections just smell like PSHUFB
    switch (config.mode) {
        default:
        case CHANSHIFT_MODE_RGB:
            return 0;
        case CHANSHIFT_MODE_RBG:
#ifdef _MSC_VER
            __asm {
			mov ebx, framebuffer;
			mov ecx, c;
			lp1:
			sub ecx, 4;

			mov eax, dword ptr [ebx+ecx*4];
			xchg ah, al;
			mov [ebx+ecx*4], eax;

			mov eax, dword ptr [ebx+ecx*4+4];
			xchg ah, al;
			mov [ebx+ecx*4+4], eax;

			mov eax, dword ptr [ebx+ecx*4+8];
			xchg ah, al;
			mov [ebx+ecx*4+8], eax;

			mov eax, dword ptr [ebx+ecx*4+12];
			xchg ah, al;
			mov [ebx+ecx*4+12], eax;

			test ecx, ecx;
			jnz lp1;
            }
#else  // _MSC_VER
            __asm__ __volatile__(
                "mov %%ebx, %0\n\t"
                "mov %%ecx, %1\n"
                "lp1:\n\t"
                "sub %%ecx, 4\n\t"

                "mov %%eax, dword ptr [%%ebx + %%ecx * 4]\n\t"
                "xchg %%ah, %%al\n\t"
                "mov [%%ebx + %%ecx * 4], %%eax\n\t"

                "mov %%eax, dword ptr [%%ebx + %%ecx * 4 + 4]\n\t"
                "xchg %%ah, %%al\n\t"
                "mov [%%ebx + %%ecx * 4 + 4], %%eax\n\t"

                "mov %%eax, dword ptr [%%ebx + %%ecx * 4 + 8]\n\t"
                "xchg %%ah, %%al\n\t"
                "mov [%%ebx + %%ecx * 4 + 8], %%eax\n\t"

                "mov %%eax, dword ptr [%%ebx + %%ecx * 4 + 12]\n\t"
                "xchg %%ah, %%al\n\t"
                "mov [%%ebx + %%ecx * 4 + 12], %%eax\n\t"

                "test %%ecx, %%ecx\n\t"
                "jnz lp1\n\t"
                : /* no outputs */
                : "m"(framebuffer), "m"(c)
                : "eax", "ebx", "ecx");
#endif
            break;

        case CHANSHIFT_MODE_BRG:
#ifdef _MSC_VER
            __asm {
			mov ebx, framebuffer;
			mov ecx, c;
			lp2:
			sub ecx, 4;
			
			mov eax, dword ptr [ebx+ecx*4];
			mov dl, al;
			shr eax, 8;
			bswap eax;
			mov ah, dl;
			bswap eax;
			mov [ebx+ecx*4], eax;
			
			mov eax, dword ptr [ebx+ecx*4+4];
			mov dl, al;
			shr eax, 8;
			bswap eax;
			mov ah, dl;
			bswap eax;
			mov [ebx+ecx*4+4], eax;

			mov eax, dword ptr [ebx+ecx*4+8];
			mov dl, al;
			shr eax, 8;
			bswap eax;
			mov ah, dl;
			bswap eax;
			mov [ebx+ecx*4+8], eax;

			mov eax, dword ptr [ebx+ecx*4+12];
			mov dl, al;
			shr eax, 8;
			bswap eax;
			mov ah, dl;
			bswap eax;
			mov [ebx+ecx*4+12], eax;

			test ecx, ecx;
			jnz lp2;
            }
#else  // _MSC_VER
            __asm__ __volatile__(
                "mov %%ebx, %0\n\t"
                "mov %%ecx, %1\n"
                "lp2:\n\t"
                "sub %%ecx, 4\n\t"

                "mov %%eax, dword ptr [%%ebx + %%ecx * 4]\n\t"
                "mov %%dl, %%al\n\t"
                "shr %%eax, 8\n\t"
                "bswap %%eax\n\t"
                "mov %%ah, %%dl\n\t"
                "bswap %%eax\n\t"
                "mov [%%ebx + %%ecx * 4], %%eax\n\t"

                "mov %%eax, dword ptr [%%ebx + %%ecx * 4 + 4]\n\t"
                "mov %%dl, %%al\n\t"
                "shr %%eax, 8\n\t"
                "bswap %%eax\n\t"
                "mov %%ah, %%dl\n\t"
                "bswap %%eax\n\t"
                "mov [%%ebx + %%ecx * 4 + 4], %%eax\n\t"

                "mov %%eax, dword ptr [%%ebx + %%ecx * 4 + 8]\n\t"
                "mov %%dl, %%al\n\t"
                "shr %%eax, 8\n\t"
                "bswap %%eax\n\t"
                "mov %%ah, %%dl\n\t"
                "bswap %%eax\n\t"
                "mov [%%ebx + %%ecx * 4 + 8], %%eax\n\t"

                "mov %%eax, dword ptr [%%ebx + %%ecx * 4 + 12]\n\t"
                "mov %%dl, %%al\n\t"
                "shr %%eax, 8\n\t"
                "bswap %%eax\n\t"
                "mov %%ah, %%dl\n\t"
                "bswap %%eax\n\t"
                "mov [%%ebx + %%ecx * 4 + 12], %%eax\n\t"

                "test %%ecx, %%ecx\n\t"
                "jnz lp2\n\t"
                : /* no outputs */
                : "m"(framebuffer), "m"(c)
                : "eax", "ebx", "ecx");
#endif
            break;

        case CHANSHIFT_MODE_BGR:
#ifdef _MSC_VER
            __asm {
			mov ebx, framebuffer;
			mov ecx, c;
			lp3:
			sub ecx, 4;

			mov eax, dword ptr [ebx+ecx*4];
			bswap eax;
			shr eax, 8;
			mov [ebx+ecx*4], eax;

			mov eax, dword ptr [ebx+ecx*4+4];
			bswap eax;
			shr eax, 8;
			mov [ebx+ecx*4+4], eax;

			mov eax, dword ptr [ebx+ecx*4+8];
			bswap eax;
			shr eax, 8;
			mov [ebx+ecx*4+8], eax;

			mov eax, dword ptr [ebx+ecx*4+12];
			bswap eax;
			shr eax, 8;
			mov [ebx+ecx*4+12], eax;

			test ecx, ecx;
			jnz lp3;
            }
#else  // _MSC_VER
            __asm__ __volatile__(
                "mov %%ebx, %0\n\t"
                "mov %%ecx, %1\n"
                "lp3:\n\t"
                "sub %%ecx, 4\n\t"

                "mov %%eax, dword ptr [%%ebx + %%ecx * 4]\n\t"
                "bswap %%eax\n\t"
                "shr %%eax, 8\n\t"
                "mov [%%ebx + %%ecx * 4], %%eax\n\t"

                "mov %%eax, dword ptr [%%ebx + %%ecx * 4 + 4]\n\t"
                "bswap %%eax\n\t"
                "shr %%eax, 8\n\t"
                "mov [%%ebx + %%ecx * 4 + 4], %%eax\n\t"

                "mov %%eax, dword ptr [%%ebx + %%ecx * 4 + 8]\n\t"
                "bswap %%eax\n\t"
                "shr %%eax, 8\n\t"
                "mov [%%ebx + %%ecx * 4 + 8], %%eax\n\t"

                "mov %%eax, dword ptr [%%ebx + %%ecx * 4 + 12]\n\t"
                "bswap %%eax\n\t"
                "shr %%eax, 8\n\t"
                "mov [%%ebx + %%ecx * 4 + 12], %%eax\n\t"

                "test %%ecx, %%ecx\n\t"
                "jnz lp3\n\t"
                : /* no outputs */
                : "m"(framebuffer), "m"(c)
                : "eax", "ebx", "ecx");
#endif
            break;

        case CHANSHIFT_MODE_GBR:
#ifdef _MSC_VER
            __asm {
			mov ebx, framebuffer;
			mov ecx, c;
			lp4:
			sub ecx, 4;

			mov eax, dword ptr [ebx+ecx*4];
			mov edx, eax;
			bswap edx;
			shl eax, 8;
			mov al, dh;
			mov [ebx+ecx*4], eax;

			mov eax, dword ptr [ebx+ecx*4+4];
			mov edx, eax;
			bswap edx;
			shl eax, 8;
			mov al, dh;
			mov [ebx+ecx*4+4], eax;

			mov eax, dword ptr [ebx+ecx*4+8];
			mov edx, eax;
			bswap edx;
			shl eax, 8;
			mov al, dh;
			mov [ebx+ecx*4+8], eax;

			mov eax, dword ptr [ebx+ecx*4+12];
			mov edx, eax;
			bswap edx;
			shl eax, 8;
			mov al, dh;
			mov [ebx+ecx*4+12], eax;

			test ecx, ecx;
			jnz lp4;
            }
#else  // _MSC_VER
            __asm__ __volatile__(
                "mov %%ebx, %0\n\t"
                "mov %%ecx, %1\n"
                "lp4:\n\t"
                "sub %%ecx, 4\n\t"

                "mov %%eax, dword ptr [%%ebx + %%ecx * 4]\n\t"
                "mov %%edx, %%eax\n\t"
                "bswap %%edx\n\t"
                "shl %%edx, 8\n\t"
                "mov %%al, %%dh\n\t"
                "mov [%%ebx + %%ecx * 4], %%eax\n\t"

                "mov %%eax, dword ptr [%%ebx + %%ecx * 4 + 4]\n\t"
                "mov %%edx, %%eax\n\t"
                "bswap %%edx\n\t"
                "shl %%edx, 8\n\t"
                "mov %%al, %%dh\n\t"
                "mov [%%ebx + %%ecx * 4 + 4], %%eax\n\t"

                "mov %%eax, dword ptr [%%ebx + %%ecx * 4 + 8]\n\t"
                "mov %%edx, %%eax\n\t"
                "bswap %%edx\n\t"
                "shl %%edx, 8\n\t"
                "mov %%al, %%dh\n\t"
                "mov [%%ebx + %%ecx * 4 + 8], %%eax\n\t"

                "mov %%eax, dword ptr [%%ebx + %%ecx * 4 + 12]\n\t"
                "mov %%edx, %%eax\n\t"
                "bswap %%edx\n\t"
                "shl %%edx, 8\n\t"
                "mov %%al, %%dh\n\t"
                "mov [%%ebx + %%ecx * 4 + 12], %%eax\n\t"

                "test %%ecx, %%ecx\n\t"
                "jnz lp4\n\t"
                : /* no outputs */
                : "m"(framebuffer), "m"(c)
                : "eax", "ebx", "ecx", "edx");
#endif
            break;

        case CHANSHIFT_MODE_GRB:
#ifdef _MSC_VER
            __asm {
			mov ebx, framebuffer;
			mov ecx, c;
			lp5:
			sub ecx, 4;

			mov eax, dword ptr [ebx+ecx*4];
			shl eax, 8;
			bswap eax;
			xchg ah, al;
			bswap eax;
			shr eax, 8;
			mov [ebx+ecx*4], eax;

			mov eax, dword ptr [ebx+ecx*4+4];
			shl eax, 8;
			bswap eax;
			xchg ah, al;
			bswap eax;
			shr eax, 8;
			mov [ebx+ecx*4+4], eax;

			mov eax, dword ptr [ebx+ecx*4+8];
			shl eax, 8;
			bswap eax;
			xchg ah, al;
			bswap eax;
			shr eax, 8;
			mov [ebx+ecx*4+8], eax;

			mov eax, dword ptr [ebx+ecx*4+12];
			shl eax, 8;
			bswap eax;
			xchg ah, al;
			bswap eax;
			shr eax, 8;
			mov [ebx+ecx*4+12], eax;

			test ecx, ecx;
			jnz lp5;
            }
#else  // _MSC_VER
            __asm__ __volatile__(
                "mov %%ebx, %0\n\t"
                "mov %%ecx, %1\n"
                "lp5:\n\t"
                "sub %%ecx, 4\n\t"

                "mov %%eax, dword ptr [%%ebx + %%ecx * 4]\n\t"
                "shl %%eax, 8\n\t"
                "bswap %%eax\n\t"
                "xchg %%ah, %%al\n\t"
                "bswap %%eax\n\t"
                "shr %%eax, 8\n\t"
                "mov [%%ebx + %%ecx * 4], %%eax\n\t"

                "mov %%eax, dword ptr [%%ebx + %%ecx * 4 + 4]\n\t"
                "shl %%eax, 8\n\t"
                "bswap %%eax\n\t"
                "xchg %%ah, %%al\n\t"
                "bswap %%eax\n\t"
                "shr %%eax, 8\n\t"
                "mov [%%ebx + %%ecx * 4 + 4], %%eax\n\t"

                "mov %%eax, dword ptr [%%ebx + %%ecx * 4 + 8]\n\t"
                "shl %%eax, 8\n\t"
                "bswap %%eax\n\t"
                "xchg %%ah, %%al\n\t"
                "bswap %%eax\n\t"
                "shr %%eax, 8\n\t"
                "mov [%%ebx + %%ecx * 4 + 8], %%eax\n\t"

                "mov %%eax, dword ptr [%%ebx + %%ecx * 4 + 12]\n\t"
                "shl %%eax, 8\n\t"
                "bswap %%eax\n\t"
                "xchg %%ah, %%al\n\t"
                "bswap %%eax\n\t"
                "shr %%eax, 8\n\t"
                "mov [%%ebx + %%ecx * 4 + 12], %%eax\n\t"

                "test %%ecx, %%ecx\n\t"
                "jnz lp5\n\t"
                : /* no outputs */
                : "m"(framebuffer), "m"(c)
                : "eax", "ebx", "ecx");
#endif
            break;
    }
    return 0;
}

char* C_THISCLASS::get_desc(void) { return MOD_NAME; }

void C_THISCLASS::load_config(unsigned char* data, int len) {
    srand(time(0));
    if (len <= ssizeof32(channelShiftConfig)) memcpy(&this->config, data, len);
}

int C_THISCLASS::save_config(unsigned char* data) {
    memcpy(data, &this->config, sizeof(channelShiftConfig));
    return sizeof(channelShiftConfig);
}

C_RBASE* R_ChannelShift(char* desc) {
    if (desc) {
        strcpy(desc, MOD_NAME);
        return NULL;
    }
    return (C_RBASE*)new C_THISCLASS();
}
