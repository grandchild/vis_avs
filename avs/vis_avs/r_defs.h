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
#ifndef _R_DEFS_H_
#define _R_DEFS_H_

#include "../platform.h"

#include <string.h>  // Many if not all components need strcpy or memcpy.

#define M_PI 3.14159265358979323846
#define NBUF 8

// 64k is the maximum component size in AVS
#define MAX_CODE_LEN         (1 << 16)
// 1 drive letter + 1 colon + 1 backslash + 256 path + 1 terminating null
#define MAX_PATH             260
// Same as MAX_PATH now, but while MAX_PATH could be anything, LEGACY_SAVE_PATH_LEN is
// fixed as part of the legacy preset file format.
#define LEGACY_SAVE_PATH_LEN 260
// Maximum number of threads
#define MAX_SMP_THREADS      8
// Length of the APE ID string which includes trailing null-bytes if any
#define LEGACY_APE_ID_LENGTH 32

#ifndef RGB
#define RGB(r, g, b) ((((r)&0xff) << 16) | (((g)&0xff) << 8) | ((b)&0xff))
#endif

// defined in main.cpp, render.cpp
extern unsigned char g_blendtable[256][256];

extern int g_reset_vars_on_recompile;

// use this function to get a global buffer, and the last flag says whether or not to
// allocate it if it's not valid...
void* getGlobalBuffer(int w, int h, int n, int do_alloc);

// matrix.cpp
void matrixRotate(float matrix[], char m, float Deg);
void matrixTranslate(float m[], float x, float y, float z);
void matrixMultiply(float* dest, float src[]);
void matrixApply(float* m,
                 float x,
                 float y,
                 float z,
                 float* outx,
                 float* outy,
                 float* outz);

/**
 * struct line_blend_mode {
 *     uint8_t blendmode;         // 0 - 9
 *     uint8_t adjustable_value;  // 0 - 255
 *     uint8_t linesize;          // 0 - 255
 *     // (_special & 0x80) != 0 ? pre-render call on transition, skip rendering.
 *     // Within SetRenderMode, bit 32 is used to signal effect disabled (if set!)
 *     uint8_t _special;
 * }
 */
extern int g_line_blend_mode;

// linedraw.cpp
void line(int* fb,
          int x1,
          int y1,
          int x2,
          int y2,
          int width,
          int height,
          int color,
          int lw);

// inlines
static unsigned int __inline BLEND(unsigned int a, unsigned int b) {
    unsigned int r, t;
    r = (a & 0xff) + (b & 0xff);
    t = min(r, 0xff);
    r = (a & 0xff00) + (b & 0xff00);
    t |= min(r, 0xff00);
    r = (a & 0xff0000) + (b & 0xff0000);
    t |= min(r, 0xff0000);
    r = (a & 0xff000000) + (b & 0xff000000);
    return t | min(r, 0xff000000);
}

#if 1
#define FASTMAX(x, y) max(x, y)
// (x-(((x-y)>>(32-1))&(x-y))) // hmm not faster :(
#define FASTMIN(x, y) min(x, y)
//(x+(((y-x)>>(32-1))&(y-x)))
#else
#pragma warning(push, 1)

static __inline int FASTMAX(int x, int y) {
    __asm
    {
    mov ecx, [x]
    mov eax, [y]
    sub	ecx, eax
  	cmc
  	and	ecx, edx
  	add	eax, ecx
    }
}
static __inline int FASTMIN(int x, int y) {
    __asm
    {
    mov ecx, [x]
    mov eax, [y]
    sub	ecx, eax
	  sbb	edx, edx
	  and	ecx, edx
	  add	eax, ecx
    }
}
#pragma warning(pop)

#endif

static unsigned int __inline BLEND_MAX(unsigned int a, unsigned int b) {
    // TODO: Replace with SSE instructions? -> PMAXUB/PMINUB
    //       Since AVS's resolution is guaranteed to be a multiple of 4 anyway, this
    //       optimization could go further and work on 4 pixels at once by using AVX's
    //       128-bit-width VPMAXUB instruction.
    unsigned int t;
    int _a = a & 0xff;
    int _b = b & 0xff;
    t = FASTMAX(_a, _b);
    _a = a & 0xff00;
    _b = b & 0xff00;
    t |= FASTMAX(_a, _b);
    _a = a & 0xff0000;
    _b = b & 0xff0000;
    t |= FASTMAX(_a, _b);
    return t;
}

static unsigned int __inline BLEND_MIN(unsigned int a, unsigned int b) {
#if 1
    unsigned int t;
    int _a = a & 0xff;
    int _b = b & 0xff;
    t = FASTMIN(_a, _b);
    _a = a & 0xff00;
    _b = b & 0xff00;
    t |= FASTMIN(_a, _b);
    _a = a & 0xff0000;
    _b = b & 0xff0000;
    t |= FASTMIN(_a, _b);
    return t;
#else
    __asm
    {
    mov ecx, [a]
    mov eax, [b]

    and ecx, 0xff
    and eax, 0xff

    mov esi, [a]
    mov ebx, [b]

    sub	ecx, eax
	  sbb	edx, edx

    and esi, 0xff00
    and ebx, 0xff00

	  and	ecx, edx
    sub	esi, ebx

	  sbb	edx, edx
	  add	eax, ecx

	  and	esi, edx
    mov ecx, [a]

	  add	ebx, esi
    and ecx, 0xff0000

    mov esi, [b]
    or eax, ebx

    and esi, 0xff0000

    sub	ecx, esi
	  sbb	edx, edx

	  and	ecx, edx
	  add	esi, ecx
    
    or eax, esi
    }
#endif
}

#ifdef FASTMAX
#undef FASTMAX
#undef FASTMIN
#endif

static unsigned int __inline BLEND_AVG(unsigned int a, unsigned int b) {
    return ((a >> 1) & ~((1 << 7) | (1 << 15) | (1 << 23)))
           + ((b >> 1) & ~((1 << 7) | (1 << 15) | (1 << 23)));
}

static unsigned int __inline BLEND_SUB(unsigned int a, unsigned int b) {
    int r, t;
    r = (a & 0xff) - (b & 0xff);
    t = max(r, 0);
    r = (a & 0xff00) - (b & 0xff00);
    t |= max(r, 0);
    r = (a & 0xff0000) - (b & 0xff0000);
    t |= max(r, 0);
    r = (a & 0xff000000) - (b & 0xff000000);
    return t | max(r, 0);
}

#ifdef NO_MMX
#define BLEND_ADJ BLEND_ADJ_NOMMX
#endif

static unsigned int __inline BLEND_ADJ_NOMMX(unsigned int a, unsigned int b, int v) {
    int t;
    t = g_blendtable[a & 0xFF][v] + g_blendtable[b & 0xFF][0xFF - v];
    t |=
        (g_blendtable[(a & 0xFF00) >> 8][v] + g_blendtable[(b & 0xFF00) >> 8][0xFF - v])
        << 8;
    t |= (g_blendtable[(a & 0xFF0000) >> 16][v]
          + g_blendtable[(b & 0xFF0000) >> 16][0xFF - v])
         << 16;
    return t;
}

static unsigned int __inline BLEND_MUL(unsigned int a, unsigned int b) {
    int t;
    t = g_blendtable[a & 0xFF][b & 0xFF];
    t |= g_blendtable[(a & 0xFF00) >> 8][(b & 0xFF00) >> 8] << 8;
    t |= g_blendtable[(a & 0xFF0000) >> 16][(b & 0xFF0000) >> 16] << 16;
    return t;
}

static __inline void BLEND_LINE(int* fb, int color) {
    switch (g_line_blend_mode & 0xff) {
        case 1: *fb = BLEND(*fb, color); break;
        case 2: *fb = BLEND_MAX(*fb, color); break;
        case 3: *fb = BLEND_AVG(*fb, color); break;
        case 4: *fb = BLEND_SUB(*fb, color); break;
        case 5: *fb = BLEND_SUB(color, *fb); break;
        case 6: *fb = BLEND_MUL(*fb, color); break;
        case 7:
            *fb = BLEND_ADJ_NOMMX(*fb, color, (g_line_blend_mode >> 8) & 0xff);
            break;
        case 8: *fb = *fb ^ color; break;
        case 9: *fb = BLEND_MIN(*fb, color); break;
        default: *fb = color; break;
    }
}
extern unsigned int const mmx_blend4_revn[2];
extern int const mmx_blend4_zero;
extern int const mmx_blendadj_mask[2];
// NOTE. WHEN USING THIS FUNCTION, BE SURE TO DO 'if (g_mmx_available) __asm emms;'
// before calling any fpu code, or before returning.

#ifndef NO_MMX
static unsigned int __inline BLEND_ADJ(unsigned int a, unsigned int b, int v) {
#ifdef _MSC_VER  // MSVC asm
    __asm
    {
    movd mm3, [v]  // VVVVVVVV
    
    movd mm0, [a]
    packuswb mm3, mm3  // 0000HHVV

    movd mm1, [b]
    punpcklwd mm3, mm3  // HHVVHHVV
        
    movq mm4, [mmx_blend4_revn]
    punpckldq mm3, mm3  // HHVVHHVV HHVVHHVV

    punpcklbw mm0, [mmx_blend4_zero]
    pand mm3, [mmx_blendadj_mask]

    punpcklbw mm1, [mmx_blend4_zero]
    psubw mm4, mm3

    pmullw mm0, mm3      
    pmullw mm1, mm4
    
    paddw mm0, mm1

    psrlw mm0, 8
    
    packuswb mm0, mm0

    movd eax, mm0
    }
#else   // _MSC_VER, GCC asm
    unsigned int out;
    __asm__ __volatile__(
        "movd      %%mm3, [%[v]]\n\t"
        "movd      %%mm0, [%[a]]\n\t"
        "packuswb  %%mm3, %%mm3\n\t"
        "movd      %%mm1, [%[b]]\n\t"
        "punpcklwd %%mm3, %%mm3\n\t"
        "movq      %%mm4, [%[mmx_blend4_revn]]\n\t"
        "punpckldq %%mm3, %%mm3\n\t"
        "punpcklbw %%mm0, [%[mmx_blend4_zero]]\n\t"
        "pand      %%mm3, [%[mmx_blendadj_mask]]\n\t"
        "punpcklbw %%mm1, [%[mmx_blend4_zero]]\n\t"
        "psubw     %%mm4, %%mm3\n\t"
        "pmullw    %%mm0, %%mm3      \n\t"
        "pmullw    %%mm1, %%mm4\n\t"
        "paddw     %%mm0, %%mm1\n\t"
        "psrlw     %%mm0, 8\n\t"
        "packuswb  %%mm0, %%mm0\n\t"
        "movd      %[out], %%mm0\n\t"
        : [out] "=r"(out)
        : [v] "m"(v),
          [a] "m"(a),
          [b] "m"(b),
          [mmx_blend4_revn] "m"(mmx_blend4_revn),
          [mmx_blend4_zero] "m"(mmx_blend4_zero),
          [mmx_blendadj_mask] "m"(mmx_blendadj_mask)
        :);
    return out;
#endif  // _MSC_VER
}
#endif

static __inline unsigned int BLEND4(unsigned int* p1, unsigned int w, int xp, int yp) {
#ifdef NO_MMX
    int t;
    unsigned char a1, a2, a3, a4;
    a1 = g_blendtable[255 - xp][255 - yp];
    a2 = g_blendtable[xp][255 - yp];
    a3 = g_blendtable[255 - xp][yp];
    a4 = g_blendtable[xp][yp];
    t = g_blendtable[p1[0] & 0xff][a1] + g_blendtable[p1[1] & 0xff][a2]
        + g_blendtable[p1[w] & 0xff][a3] + g_blendtable[p1[w + 1] & 0xff][a4];
    t |= (g_blendtable[(p1[0] >> 8) & 0xff][a1] + g_blendtable[(p1[1] >> 8) & 0xff][a2]
          + g_blendtable[(p1[w] >> 8) & 0xff][a3]
          + g_blendtable[(p1[w + 1] >> 8) & 0xff][a4])
         << 8;
    t |=
        (g_blendtable[(p1[0] >> 16) & 0xff][a1] + g_blendtable[(p1[1] >> 16) & 0xff][a2]
         + g_blendtable[(p1[w] >> 16) & 0xff][a3]
         + g_blendtable[(p1[w + 1] >> 16) & 0xff][a4])
        << 16;
    return t;
#else
#ifdef _MSC_VER  // MSVC asm
    __asm
    {
    movd mm6, xp
    mov eax, p1

    movd mm7, yp
    mov esi, w

    movq mm4, mmx_blend4_revn
    punpcklwd mm6,mm6

    movq mm5, mmx_blend4_revn
    punpcklwd mm7,mm7

    movd mm0, [eax]
    punpckldq mm6,mm6

    movd mm1, [eax+4]
    punpckldq mm7,mm7

    movd mm2, [eax+esi*4]
    punpcklbw mm0, [mmx_blend4_zero]

    movd mm3, [eax+esi*4+4]
    psubw mm4, mm6

    punpcklbw mm1, [mmx_blend4_zero]
    pmullw mm0, mm4

    punpcklbw mm2, [mmx_blend4_zero]
    pmullw mm1, mm6

    punpcklbw mm3, [mmx_blend4_zero]
    psubw mm5, mm7

    pmullw mm2, mm4
    pmullw mm3, mm6

    paddw mm0, mm1
          // stall (mm0)

    psrlw mm0, 8
      // stall (waiting for mm3/mm2)

    paddw mm2, mm3
    pmullw mm0, mm5

    psrlw mm2, 8
      // stall (mm2)

    pmullw mm2, mm7
          // stall

          // stall (mm2)

    paddw mm0, mm2
          // stall

    psrlw mm0, 8
      // stall

    packuswb mm0, mm0
          // stall

    movd eax, mm0
    }
#else            // _MSC_VER, GCC asm
    unsigned int out;
    __asm__ __volatile__(
        "movd      %%mm6, %[xp]\n\t"
        "mov       %%eax, %[p1]\n\t"
        "movd      %%mm7, %[yp]\n\t"
        "mov       %%esi, %[w]\n\t"
        "movq      %%mm4, %[mmx_blend4_revn]\n\t"
        "punpcklwd %%mm6,%%mm6\n\t"
        "movq      %%mm5, %[mmx_blend4_revn]\n\t"
        "punpcklwd %%mm7,%%mm7\n\t"
        "movd      %%mm0, [%%eax]\n\t"
        "punpckldq %%mm6,%%mm6\n\t"
        "movd      %%mm1, [%%eax + 4]\n\t"
        "punpckldq %%mm7,%%mm7\n\t"
        "movd      %%mm2, [%%eax + %%esi * 4]\n\t"
        "punpcklbw %%mm0, [%[mmx_blend4_zero]]\n\t"
        "movd      %%mm3, [%%eax + %%esi * 4 + 4]\n\t"
        "psubw     %%mm4, %%mm6\n\t"
        "punpcklbw %%mm1, [%[mmx_blend4_zero]]\n\t"
        "pmullw    %%mm0, %%mm4\n\t"
        "punpcklbw %%mm2, [%[mmx_blend4_zero]]\n\t"
        "pmullw    %%mm1, %%mm6\n\t"
        "punpcklbw %%mm3, [%[mmx_blend4_zero]]\n\t"
        "psubw     %%mm5, %%mm7\n\t"
        "pmullw    %%mm2, %%mm4\n\t"
        "pmullw    %%mm3, %%mm6\n\t"
        "paddw     %%mm0, %%mm1\n\t"
        "psrlw     %%mm0, 8\n\t"
        "paddw     %%mm2, %%mm3\n\t"
        "pmullw    %%mm0, %%mm5\n\t"
        "psrlw     %%mm2, 8\n\t"
        "pmullw    %%mm2, %%mm7\n\t"
        "paddw     %%mm0, %%mm2\n\t"
        "psrlw     %%mm0, 8\n\t"
        "packuswb  %%mm0, %%mm0\n\t"
        "movd      %[out], %%mm0\n\t"
        : [out] "=r"(out)
        : [xp] "m"(xp),
          [p1] "m"(p1),
          [yp] "m"(yp),
          [w] "m"(w),
          [mmx_blend4_revn] "m"(mmx_blend4_revn),
          [mmx_blend4_zero] "m"(mmx_blend4_zero)
        : "eax", "esi");
    return out;
#endif           // _MSC_VER
#endif
}

static __inline unsigned int BLEND4_16(unsigned int* p1,
                                       unsigned int w,
                                       int xp,
                                       int yp) {
#ifdef NO_MMX
    int t;
    unsigned char a1, a2, a3, a4;
    xp = (xp >> 8) & 0xff;
    yp = (yp >> 8) & 0xff;
    a1 = g_blendtable[255 - xp][255 - yp];
    a2 = g_blendtable[xp][255 - yp];
    a3 = g_blendtable[255 - xp][yp];
    a4 = g_blendtable[xp][yp];
    t = g_blendtable[p1[0] & 0xff][a1] + g_blendtable[p1[1] & 0xff][a2]
        + g_blendtable[p1[w] & 0xff][a3] + g_blendtable[p1[w + 1] & 0xff][a4];
    t |= (g_blendtable[(p1[0] >> 8) & 0xff][a1] + g_blendtable[(p1[1] >> 8) & 0xff][a2]
          + g_blendtable[(p1[w] >> 8) & 0xff][a3]
          + g_blendtable[(p1[w + 1] >> 8) & 0xff][a4])
         << 8;
    t |=
        (g_blendtable[(p1[0] >> 16) & 0xff][a1] + g_blendtable[(p1[1] >> 16) & 0xff][a2]
         + g_blendtable[(p1[w] >> 16) & 0xff][a3]
         + g_blendtable[(p1[w + 1] >> 16) & 0xff][a4])
        << 16;
    return t;
#else
#ifdef _MSC_VER  // MSVC asm
    __asm
    {
    movd mm6, xp
    mov eax, p1

    movd mm7, yp
    mov esi, w

    movq mm4, mmx_blend4_revn
    psrlw mm6, 8

    movq mm5, mmx_blend4_revn
    psrlw mm7, 8

    movd mm0, [eax]
    punpcklwd mm6,mm6

    movd mm1, [eax+4]
    punpcklwd mm7,mm7

    movd mm2, [eax+esi*4]
    punpckldq mm6,mm6

    movd mm3, [eax+esi*4+4]
    punpckldq mm7,mm7

    punpcklbw mm0, [mmx_blend4_zero]
    psubw mm4, mm6

    punpcklbw mm1, [mmx_blend4_zero]
    pmullw mm0, mm4

    punpcklbw mm2, [mmx_blend4_zero]
    pmullw mm1, mm6

    punpcklbw mm3, [mmx_blend4_zero]
    psubw mm5, mm7

    pmullw mm2, mm4
    pmullw mm3, mm6

    paddw mm0, mm1
          // stall (mm0)

    psrlw mm0, 8
      // stall (waiting for mm3/mm2)

    paddw mm2, mm3
    pmullw mm0, mm5

    psrlw mm2, 8
      // stall (mm2)

    pmullw mm2, mm7
          // stall

          // stall (mm2)

    paddw mm0, mm2
          // stall

    psrlw mm0, 8
      // stall

    packuswb mm0, mm0
          // stall

    movd eax, mm0
    }
#else            // _MSC_VER, GCC asm
    unsigned int out;
    __asm__ __volatile__(
        "movd      %%mm6, %[xp]\n\t"
        "mov       %%eax, %[p1]\n\t"
        "movd      %%mm7, %[yp]\n\t"
        "mov       %%esi, %[w]\n\t"
        "movq      %%mm4, %[mmx_blend4_revn]\n\t"
        "psrlw     %%mm6, 8\n\t"
        "movq      %%mm5, %[mmx_blend4_revn]\n\t"
        "psrlw     %%mm7, 8\n\t"
        "movd      %%mm0, [%%eax]\n\t"
        "punpcklwd %%mm6,%%mm6\n\t"
        "movd      %%mm1, [%%eax + 4]\n\t"
        "punpcklwd %%mm7,%%mm7\n\t"
        "movd      %%mm2, [%%eax + %%esi * 4]\n\t"
        "punpckldq %%mm6,%%mm6\n\t"
        "movd      %%mm3, [%%eax + %%esi * 4 + 4]\n\t"
        "punpckldq %%mm7,%%mm7\n\t"
        "punpcklbw %%mm0, [%[mmx_blend4_zero]]\n\t"
        "psubw     %%mm4, %%mm6\n\t"
        "punpcklbw %%mm1, [%[mmx_blend4_zero]]\n\t"
        "pmullw    %%mm0, %%mm4\n\t"
        "punpcklbw %%mm2, [%[mmx_blend4_zero]]\n\t"
        "pmullw    %%mm1, %%mm6\n\t"
        "punpcklbw %%mm3, [%[mmx_blend4_zero]]\n\t"
        "psubw     %%mm5, %%mm7\n\t"
        "pmullw    %%mm2, %%mm4\n\t"
        "pmullw    %%mm3, %%mm6\n\t"
        "paddw     %%mm0, %%mm1\n\t"
        "psrlw     %%mm0, 8\n\t"
        "paddw     %%mm2, %%mm3\n\t"
        "pmullw    %%mm0, %%mm5\n\t"
        "psrlw     %%mm2, 8\n\t"
        "pmullw    %%mm2, %%mm7\n\t"
        "paddw     %%mm0, %%mm2\n\t"
        "psrlw     %%mm0, 8\n\t"
        "packuswb  %%mm0, %%mm0\n\t"
        "movd      %[out], %%mm0\n\t"
        : [out] "=r"(out)
        : [xp] "m"(xp),
          [p1] "m"(p1),
          [yp] "m"(yp),
          [w] "m"(w),
          [mmx_blend4_revn] "m"(mmx_blend4_revn),
          [mmx_blend4_zero] "m"(mmx_blend4_zero)
        : "eax", "esi");
    return out;
#endif           // _MSC_VER
#endif
}

static __inline void mmx_avgblend_block(int* output, int* input, int l) {
#ifdef NO_MMX
    while (l--) {
        *output = BLEND_AVG(*input++, *output);
        output++;
    }
#else
    static int mask[2] = {~((1 << 7) | (1 << 15) | (1 << 23)),
                          ~((1 << 7) | (1 << 15) | (1 << 23))};
#ifdef _MSC_VER  // MSVC asm
    __asm {
    mov eax, input
    mov edi, output
    mov ecx, l
    shr ecx, 2
    align 16
mmx_avgblend_loop:
    movq mm0, [eax]
    movq mm1, [edi]
    psrlq mm0, 1
    movq mm2, [eax+8]
    psrlq mm1, 1
    movq mm3, [edi+8]
    psrlq mm2, 1
    pand mm0, [mask]
    psrlq mm3, 1
    pand mm1, [mask]
    pand mm2, [mask]
    paddusb mm0, mm1
    pand mm3, [mask]
    add eax, 16
    paddusb mm2, mm3

    movq [edi], mm0
    movq [edi+8], mm2

    add edi, 16

    dec ecx
    jnz mmx_avgblend_loop
    emms
    }
    ;
#else            // _MSC_VER, GCC asm
    /* Remark on the use of "%=":
       This function is declared as "static inline" and so the compiler generates no
       explicit code for the function. Instead the resulting asm is copied as-is to all
       places where function is called (in the hope of a speed gain, at the cost of
       binary size). The asm contains a label, which would be an illegal duplicate in
       the complete binary, since the function is used more than once. Thankfully, GCC
       provides the special %= asm format string, which is replaced by a number that is
       unique for each copy -- thus creating unique jump labels at each call. */
    __asm__ __volatile__(
        "mov     %%eax, %[input]\n\t"
        "mov     %%edi, %[output]\n\t"
        "mov     %%ecx, %[l]\n\t"
        "shr     %%ecx, 2\n\t"
        ".align 16\n"
        "mmx_avgblend_loop%=:\n\t"
        "movq    %%mm0, [%%eax]\n\t"
        "movq    %%mm1, [%%edi]\n\t"
        "psrlq   %%mm0, 1\n\t"
        "movq    %%mm2, [%%eax + 8]\n\t"
        "psrlq   %%mm1, 1\n\t"
        "movq    %%mm3, [%%edi + 8]\n\t"
        "psrlq   %%mm2, 1\n\t"
        "pand    %%mm0, [%[mask]]\n\t"
        "psrlq   %%mm3, 1\n\t"
        "pand    %%mm1, [%[mask]]\n\t"
        "pand    %%mm2, [%[mask]]\n\t"
        "paddusb %%mm0, %%mm1\n\t"
        "pand    %%mm3, [%[mask]]\n\t"
        "add     %%eax, 16\n\t"
        "paddusb %%mm2, %%mm3\n\t"
        "movq    [%%edi], %%mm0\n\t"
        "movq    [%%edi + 8], %%mm2\n\t"
        "add     %%edi, 16\n\t"
        "dec     %%ecx\n\t"
        "jnz     mmx_avgblend_loop%=\n\t"
        "emms\n\t"
        : /* no outputs */
        : [input] "m"(input), [output] "m"(output), [l] "m"(l), [mask] "m"(mask)
        : "eax", "ecx", "edi");
#endif           // _MSC_VER
#endif
}

static __inline void mmx_addblend_block(int* output, int* input, int l) {
#ifdef NO_MMX
    while (l--) {
        *output = BLEND(*input++, *output);
        output++;
    }
#else
#ifdef _MSC_VER  // MSVC asm
    __asm {
    mov eax, input
    mov edi, output
    mov ecx, l
    shr ecx, 2
    align 16
mmx_addblend_loop:
    movq mm0, [eax]
    movq mm1, [edi]
    movq mm2, [eax+8]
    movq mm3, [edi+8]
    paddusb mm0, mm1
    paddusb mm2, mm3
    add eax, 16

    movq [edi], mm0
    movq [edi+8], mm2

    add edi, 16

    dec ecx
    jnz mmx_addblend_loop
    emms
    }
    ;
#else            // _MSC_VER, GCC asm
    /* See remark before GCC asm block in mmx_avgblend_block() above. */
    __asm__ __volatile__(
        "mov     %%eax, %[input]\n\t"
        "mov     %%edi, %[output]\n\t"
        "mov     %%ecx, %[l]\n\t"
        "shr     %%ecx, 2\n\t"
        ".align  16\n"
        "mmx_addblend_loop%=:\n\t"
        "movq    %%mm0, [%%eax]\n\t"
        "movq    %%mm1, [%%edi]\n\t"
        "movq    %%mm2, [%%eax + 8]\n\t"
        "movq    %%mm3, [%%edi + 8]\n\t"
        "paddusb %%mm0, %%mm1\n\t"
        "paddusb %%mm2, %%mm3\n\t"
        "add     %%eax, 16\n\t"
        "movq    [%%edi], %%mm0\n\t"
        "movq    [%%edi + 8], %%mm2\n\t"
        "add     %%edi, 16\n\t"
        "dec     %%ecx\n\t"
        "jnz     mmx_addblend_loop%=\n\t"
        "emms\n\t"
        : /* no outputs */
        : [input] "m"(input), [output] "m"(output), [l] "m"(l)
        : "eax", "ecx", "edi");
#endif           // _MSC_VER
#endif
}

static __inline void mmx_mulblend_block(int* output, int* input, int l) {
#ifdef NO_MMX
    while (l--) {
        *output = BLEND_MUL(*input++, *output);
        output++;
    }
#else
#ifdef _MSC_VER  // MSVC asm
    __asm {
    mov eax, input
    mov edi, output
    mov ecx, l
    shr ecx, 1
    align 16
mmx_mulblend_loop:
    movd mm0, [eax]
    movd mm1, [edi]
    movd mm2, [eax+4]
    punpcklbw mm0, [mmx_blend4_zero]
    movd mm3, [edi+4]
    punpcklbw mm1, [mmx_blend4_zero]
    punpcklbw mm2, [mmx_blend4_zero]
    pmullw mm0, mm1
    punpcklbw mm3, [mmx_blend4_zero]
    psrlw mm0, 8
    pmullw mm2, mm3
    packuswb mm0, mm0
    psrlw mm2, 8
    packuswb mm2, mm2
    add eax, 8

    movd [edi], mm0
    movd [edi+4], mm2

    add edi, 8

    dec ecx
    jnz mmx_mulblend_loop
    emms
    }
    ;
#else            // _MSC_VER, GCC asm
    /* See remark before GCC asm block in mmx_avgblend_block() above. */
    __asm__ __volatile__(
        "mov       %%eax, %[input]\n\t"
        "mov       %%edi, %[output]\n\t"
        "mov       %%ecx, %[l]\n\t"
        "shr       %%ecx, 1\n\t"
        ".align    16\n"
        "mmx_mulblend_loop%=:\n\t"
        "movd      %%mm0, [%%eax]\n\t"
        "movd      %%mm1, [%%edi]\n\t"
        "movd      %%mm2, [%%eax + 4]\n\t"
        "punpcklbw %%mm0, [%[mmx_blend4_zero]]\n\t"
        "movd      %%mm3, [%%edi+4]\n\t"
        "punpcklbw %%mm1, [%[mmx_blend4_zero]]\n\t"
        "punpcklbw %%mm2, [%[mmx_blend4_zero]]\n\t"
        "pmullw    %%mm0, %%mm1\n\t"
        "punpcklbw %%mm3, [%[mmx_blend4_zero]]\n\t"
        "psrlw     %%mm0, 8\n\t"
        "pmullw    %%mm2, %%mm3\n\t"
        "packuswb  %%mm0, %%mm0\n\t"
        "psrlw     %%mm2, 8\n\t"
        "packuswb  %%mm2, %%mm2\n\t"
        "add       %%eax, 8\n\t"
        "movd      [%%edi], %%mm0\n\t"
        "movd      [%%edi + 4], %%mm2\n\t"
        "add       %%edi, 8\n\t"
        "dec       %%ecx\n\t"
        "jnz       mmx_mulblend_loop%=\n\t"
        "emms\n\t"
        : /* no outputs */
        : [input] "m"(input),
          [output] "m"(output),
          [l] "m"(l),
          [mmx_blend4_zero] "m"(mmx_blend4_zero)
        : "eax", "ecx", "edi");
#endif           // _MSC_VER
#endif
}

static void __inline mmx_adjblend_block(int* o, int* in1, int* in2, int len, int v) {
#ifdef NO_MMX
    while (len--) {
        *o++ = BLEND_ADJ(*in1++, *in2++, v);
    }
#else
#ifdef _MSC_VER  // MSVC asm
    __asm {
    movd mm3, [v]  // VVVVVVVV
    mov ecx, len

    packuswb mm3, mm3  // 0000HHVV
    mov edx, o

    punpcklwd mm3, mm3  // HHVVHHVV
    mov esi, in1

    movq mm4, [mmx_blend4_revn]
    punpckldq mm3, mm3  // HHVVHHVV HHVVHHVV
    
    pand mm3, [mmx_blendadj_mask]
    mov edi, in2

    shr ecx, 1
    psubw mm4, mm3
    
    align 16
_mmx_adjblend_loop:
    
    movd mm0, [esi]
    
    movd mm1, [edi]
    punpcklbw mm0, [mmx_blend4_zero]
    
    movd mm6, [esi+4]
    punpcklbw mm1, [mmx_blend4_zero]
    
    movd mm7, [edi+4]        
    punpcklbw mm6, [mmx_blend4_zero]

    pmullw mm0, mm3
    punpcklbw mm7, [mmx_blend4_zero]

    pmullw mm1, mm4
    pmullw mm6, mm3

    pmullw mm7, mm4
    paddw mm0, mm1

    paddw mm6, mm7
    add edi, 8

    psrlw mm0, 8
    add esi, 8

    psrlw mm6, 8
    packuswb mm0, mm0
    
    packuswb mm6, mm6
    movd [edx], mm0

    movd [edx+4], mm6
    add edx, 8
    dec ecx
    jnz _mmx_adjblend_loop

    emms
    }
    ;
#else            // _MSC_VER, GCC asm
    /* See remark before GCC asm block in mmx_avgblend_block() above. */
    __asm__ __volatile__(
        "movd      %%mm3, [%[v]]\n\t"
        "mov       %%ecx, %[len]\n\t"
        "packuswb  %%mm3, %%mm3\n\t"
        "mov       %%edx, %[o]\n\t"
        "punpcklwd %%mm3, %%mm3\n\t"
        "mov       %%esi, %[in1]\n\t"
        "movq      %%mm4, [%[mmx_blend4_revn]]\n\t"
        "punpckldq %%mm3, %%mm3\n\t"
        "pand      %%mm3, [%[mmx_blendadj_mask]]\n\t"
        "mov       %%edi, %[in2]\n\t"
        "shr       %%ecx, 1\n\t"
        "psubw     %%mm4, %%mm3\n\t"
        ".align    16\n"
        "_mmx_adjblend_loop%=:\n\t"
        "movd      %%mm0, [%%esi]\n\t"
        "movd      %%mm1, [%%edi]\n\t"
        "punpcklbw %%mm0, [%[mmx_blend4_zero]]\n\t"
        "movd      %%mm6, [%%esi + 4]\n\t"
        "punpcklbw %%mm1, [%[mmx_blend4_zero]]\n\t"
        "movd      %%mm7, [%%edi + 4]\n\t"
        "punpcklbw %%mm6, [%[mmx_blend4_zero]]\n\t"
        "pmullw    %%mm0, %%mm3\n\t"
        "punpcklbw %%mm7, [%[mmx_blend4_zero]]\n\t"
        "pmullw    %%mm1, %%mm4\n\t"
        "pmullw    %%mm6, %%mm3\n\t"
        "pmullw    %%mm7, %%mm4\n\t"
        "paddw     %%mm0, %%mm1\n\t"
        "paddw     %%mm6, %%mm7\n\t"
        "add       %%edi, 8\n\t"
        "psrlw     %%mm0, 8\n\t"
        "add       %%esi, 8\n\t"
        "psrlw     %%mm6, 8\n\t"
        "packuswb  %%mm0, %%mm0\n\t"
        "packuswb  %%mm6, %%mm6\n\t"
        "movd      [%%edx], %%mm0\n\t"
        "movd      [%%edx + 4], %%mm6\n\t"
        "add       %%edx, 8\n\t"
        "dec       %%ecx\n\t"
        "jnz       _mmx_adjblend_loop%=\n\t"
        "emms     \n\t"
        : /* no outputs */
        : [v] "m"(v),
          [len] "m"(len),
          [o] "m"(o),
          [in1] "m"(in1),
          [in2] "m"(in2),
          [mmx_blend4_revn] "m"(mmx_blend4_revn),
          [mmx_blendadj_mask] "m"(mmx_blendadj_mask),
          [mmx_blend4_zero] "m"(mmx_blend4_zero)
        : "ecx", "edx", "esi", "edi");
#endif           // _MSC_VER
#endif
}

void doAVSEvalHighLight(void* hwndDlg, unsigned int sub, char* data);

#endif
