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

#include "e_blur.h"

#include <immintrin.h>

#define PUT_INT(y)                   \
    data[pos] = (y) & 255;           \
    data[pos + 1] = (y >> 8) & 255;  \
    data[pos + 2] = (y >> 16) & 255; \
    data[pos + 3] = (y >> 24) & 255
#define GET_INT() \
    (data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24))

#define DIV_2(x)  (((x) & 0xfefefeff) >> 1)
#define DIV_4(x)  (((x) & 0xfcfcfcff) >> 2)
#define DIV_8(x)  (((x) & 0xf8f8f8ff) >> 3)
#define DIV_16(x) (((x) & 0xf0f0f0ff) >> 4)

constexpr Parameter Blur_Info::parameters[];

E_Blur::E_Blur(AVS_Instance* avs) : Configurable_Effect(avs) {}
E_Blur::~E_Blur() {}

void E_Blur::smp_render(int this_thread,
                        int max_threads,
                        char[2][2][576],
                        int,
                        int* framebuffer,
                        int* fbout,
                        int w,
                        int h) {
    uint32_t* f = (uint32_t*)framebuffer;
    uint32_t* of = (uint32_t*)fbout;

    if (max_threads < 1) {
        max_threads = 1;
    }

    int start_l = (this_thread * h) / max_threads;
    int end_l;

    if (this_thread >= max_threads - 1) {
        end_l = h;
    } else {
        end_l = ((this_thread + 1) * h) / max_threads;
    }

    int outh = end_l - start_l;
    if (outh < 1) {
        return;
    }

    int skip_pix = start_l * w;

    f += skip_pix;
    of += skip_pix;

    int at_top = 0, at_bottom = 0;

    if (!this_thread) {
        at_top = 1;
    }
    if (this_thread >= max_threads - 1) {
        at_bottom = 1;
    }

    if (this->config.level == BLUR_LIGHT) {
        /* #LIGHT
         *  o  1  o
         *  1 12  1
         *  o  1  o
         *  /16
         *
         *  o  1  o    o  1 |
         *  1  5  1    1  6 |
         *  -------    -----
         *  /8         /8
         */

        // top line
        if (at_top) {
            uint32_t* f_below = f + w;
            int x;
            int adj_tl = 0, adj_tl2 = 0;
            if (this->config.round == BLUR_ROUND_UP) {
                adj_tl = 0x03030303;
                adj_tl2 = 0x04040404;
            }
            // top left
            *of++ =
                DIV_2(f[0]) + DIV_4(f[0]) + DIV_8(f[1]) + DIV_8(f_below[0]) + adj_tl;
            f++;
            f_below++;
            // top center
            x = (w - 2) / 4;
            while (x--) {
                of[0] = DIV_2(f[0]) + DIV_8(f[0]) + DIV_8(f[1]) + DIV_8(f[-1])
                        + DIV_8(f_below[0]) + adj_tl2;
                of[1] = DIV_2(f[1]) + DIV_8(f[1]) + DIV_8(f[2]) + DIV_8(f[0])
                        + DIV_8(f_below[1]) + adj_tl2;
                of[2] = DIV_2(f[2]) + DIV_8(f[2]) + DIV_8(f[3]) + DIV_8(f[1])
                        + DIV_8(f_below[2]) + adj_tl2;
                of[3] = DIV_2(f[3]) + DIV_8(f[3]) + DIV_8(f[4]) + DIV_8(f[2])
                        + DIV_8(f_below[3]) + adj_tl2;
                f += 4;
                f_below += 4;
                of += 4;
            }
            x = (w - 2) & 3;
            while (x--) {
                *of++ = DIV_2(f[0]) + DIV_8(f[0]) + DIV_8(f[1]) + DIV_8(f[-1])
                        + DIV_8(f_below[0]) + adj_tl2;
                f++;
                f_below++;
            }
            // top right
            *of++ =
                DIV_2(f[0]) + DIV_4(f[0]) + DIV_8(f[-1]) + DIV_8(f_below[0]) + adj_tl;
            f++;
            f_below++;
        }

        // middle block
        {
            int y = outh - at_top - at_bottom;
            uint32_t adj_tl1 = 0, adj_tl2 = 0;
            if (this->config.round == BLUR_ROUND_UP) {
                adj_tl1 = 0x00040404;
                adj_tl2 = 0x00050505;
            }
            while (y--) {
                int x;
                uint32_t* f_below = f + w;
                uint32_t* f_above = f - w;

                // left edge
                *of++ = DIV_2(f[0]) + DIV_8(f[0]) + DIV_8(f[1]) + DIV_8(f_below[0])
                        + DIV_8(f_above[0]) + adj_tl1;
                f++;
                f_below++;
                f_above++;

                // middle of line
                x = (w - 2) / 4;
#ifdef SIMD_MODE_X86_SSE
                __m128i round_up_or_zero = this->config.round == BLUR_ROUND_UP
                                               ? _mm_set1_epi32(adj_tl2)
                                               : _mm_setzero_si128();
                __m128i strip_high_1bit_mask = _mm_set1_epi32(0x7f7f7f7f);
                __m128i strip_high_2bit_mask = _mm_set1_epi32(0x1f3f3f3f);
                __m128i strip_high_4bit_mask = _mm_set1_epi32(0x0f0f0f0f);
                while (x--) {
                    __m128i four_px_above = _mm_loadu_si128((__m128i*)(f_above));
                    __m128i four_px_below = _mm_loadu_si128((__m128i*)(f_below));
                    __m128i four_px_left = _mm_loadu_si128((__m128i*)(f - 1));
                    __m128i four_px_right = _mm_loadu_si128((__m128i*)(f + 1));
                    __m128i four_px_center = _mm_loadu_si128((__m128i*)f);

                    four_px_above = _mm_and_si128(_mm_srli_epi32(four_px_above, 4),
                                                  strip_high_4bit_mask);
                    four_px_below = _mm_and_si128(_mm_srli_epi32(four_px_below, 4),
                                                  strip_high_4bit_mask);
                    four_px_left = _mm_and_si128(_mm_srli_epi32(four_px_left, 4),
                                                 strip_high_4bit_mask);
                    four_px_right = _mm_and_si128(_mm_srli_epi32(four_px_right, 4),
                                                  strip_high_4bit_mask);

                    __m128i vertical = _mm_add_epi8(four_px_above, four_px_below);
                    __m128i horizontal = _mm_add_epi8(four_px_left, four_px_right);
                    __m128i cross = _mm_add_epi8(vertical, horizontal);

                    four_px_center =
                        _mm_add_epi8(_mm_and_si128(_mm_srli_epi32(four_px_center, 1),
                                                   strip_high_1bit_mask),
                                     _mm_and_si128(_mm_srli_epi32(four_px_center, 2),
                                                   strip_high_2bit_mask));
                    four_px_center = _mm_add_epi8(four_px_center, cross);
                    four_px_center = _mm_add_epi8(four_px_center, round_up_or_zero);
                    _mm_storeu_si128((__m128i*)(of), four_px_center);
                    f += 4;
                    f_below += 4;
                    f_above += 4;
                    of += 4;
                }
#else
                if (this->config.round == BLUR_ROUND_UP) {
                    while (x--) {
                        of[0] = DIV_2(f[0]) + DIV_4(f[0]) + DIV_16(f[1]) + DIV_16(f[-1])
                                + DIV_16(f_below[0]) + DIV_16(f_above[0]) + adj_tl2;
                        of[1] = DIV_2(f[1]) + DIV_4(f[1]) + DIV_16(f[2]) + DIV_16(f[0])
                                + DIV_16(f_below[1]) + DIV_16(f_above[1]) + adj_tl2;
                        of[2] = DIV_2(f[2]) + DIV_4(f[2]) + DIV_16(f[3]) + DIV_16(f[1])
                                + DIV_16(f_below[2]) + DIV_16(f_above[2]) + adj_tl2;
                        of[3] = DIV_2(f[3]) + DIV_4(f[3]) + DIV_16(f[4]) + DIV_16(f[2])
                                + DIV_16(f_below[3]) + DIV_16(f_above[3]) + adj_tl2;
                        f += 4;
                        f_below += 4;
                        f_above += 4;
                        of += 4;
                    }
                } else {
                    while (x--) {
                        of[0] = DIV_2(f[0]) + DIV_4(f[0]) + DIV_16(f[1]) + DIV_16(f[-1])
                                + DIV_16(f_below[0]) + DIV_16(f_above[0]);
                        of[1] = DIV_2(f[1]) + DIV_4(f[1]) + DIV_16(f[2]) + DIV_16(f[0])
                                + DIV_16(f_below[1]) + DIV_16(f_above[1]);
                        of[2] = DIV_2(f[2]) + DIV_4(f[2]) + DIV_16(f[3]) + DIV_16(f[1])
                                + DIV_16(f_below[2]) + DIV_16(f_above[2]);
                        of[3] = DIV_2(f[3]) + DIV_4(f[3]) + DIV_16(f[4]) + DIV_16(f[2])
                                + DIV_16(f_below[3]) + DIV_16(f_above[3]);
                        f += 4;
                        f_below += 4;
                        f_above += 4;
                        of += 4;
                    }
                }
#endif
                x = (w - 2) & 0b11;
                while (x--) {
                    *of++ = DIV_2(f[0]) + DIV_4(f[0]) + DIV_16(f[1]) + DIV_16(f[-1])
                            + DIV_16(f_below[0]) + DIV_16(f_above[0]) + adj_tl2;
                    f++;
                    f_below++;
                    f_above++;
                }

                // right block
                *of++ = DIV_2(f[0]) + DIV_8(f[0]) + DIV_8(f[-1]) + DIV_8(f_below[0])
                        + DIV_8(f_above[0]) + adj_tl1;
                f++;
            }
        }
        // bottom block
        if (at_bottom) {
            uint32_t* f_above = f - w;
            int x;
            int adj_tl = 0, adj_tl2 = 0;
            if (this->config.round == BLUR_ROUND_UP) {
                adj_tl = 0x03030303;
                adj_tl2 = 0x04040404;
            }
            // bottom left
            *of++ =
                DIV_2(f[0]) + DIV_4(f[0]) + DIV_8(f[1]) + DIV_8(f_above[0]) + adj_tl;
            f++;
            f_above++;
            // bottom center
            x = (w - 2) / 4;
            while (x--) {
                of[0] = DIV_2(f[0]) + DIV_8(f[0]) + DIV_8(f[1]) + DIV_8(f[-1])
                        + DIV_8(f_above[0]) + adj_tl2;
                of[1] = DIV_2(f[1]) + DIV_8(f[1]) + DIV_8(f[2]) + DIV_8(f[0])
                        + DIV_8(f_above[1]) + adj_tl2;
                of[2] = DIV_2(f[2]) + DIV_8(f[2]) + DIV_8(f[3]) + DIV_8(f[1])
                        + DIV_8(f_above[2]) + adj_tl2;
                of[3] = DIV_2(f[3]) + DIV_8(f[3]) + DIV_8(f[4]) + DIV_8(f[2])
                        + DIV_8(f_above[3]) + adj_tl2;
                f += 4;
                f_above += 4;
                of += 4;
            }
            x = (w - 2) & 3;
            while (x--) {
                *of++ = DIV_2(f[0]) + DIV_8(f[0]) + DIV_8(f[1]) + DIV_8(f[-1])
                        + DIV_8(f_above[0]) + adj_tl2;
                f++;
                f_above++;
            }
            // bottom right
            *of++ =
                DIV_2(f[0]) + DIV_4(f[0]) + DIV_8(f[-1]) + DIV_8(f_above[0]) + adj_tl;
            f++;
            f_above++;
        }
    } else if (this->config.level == BLUR_HEAVY) {
        /* #HEAVY
         *  o 1 o
         *  1 o 1
         *  o 1 o
         *  /4
         *
         *  o 2 o   o 1 |
         *  1 o 1   1 o |
         *  -----   ----
         *  /4      /2
         */

        // top line
        if (at_top) {
            uint32_t* f_below = f + w;
            int x;
            uint32_t adj_tl = 0;
            uint32_t adj_tl2 = 0;
            if (this->config.round == BLUR_ROUND_UP) {
                adj_tl = 0x02020202;
                adj_tl2 = 0x01010101;
            }
            // top left
            *of++ = DIV_2(f[1]) + DIV_2(f_below[0]) + adj_tl2;
            f++;
            f_below++;
            // top center
            x = (w - 2) / 4;
            while (x--) {
                of[0] = DIV_4(f[1]) + DIV_4(f[-1]) + DIV_2(f_below[0]) + adj_tl;
                of[1] = DIV_4(f[2]) + DIV_4(f[0]) + DIV_2(f_below[1]) + adj_tl;
                of[2] = DIV_4(f[3]) + DIV_4(f[1]) + DIV_2(f_below[2]) + adj_tl;
                of[3] = DIV_4(f[4]) + DIV_4(f[2]) + DIV_2(f_below[3]) + adj_tl;
                f += 4;
                f_below += 4;
                of += 4;
            }
            x = (w - 2) & 3;
            while (x--) {
                *of++ = DIV_4(f[1]) + DIV_4(f[-1]) + DIV_2(f_below[0]) + adj_tl;
                f++;
                f_below++;
            }
            // top right
            *of++ = DIV_2(f[-1]) + DIV_2(f_below[0]) + adj_tl2;
            f++;
            f_below++;
        }

        // middle block
        {
            int y = outh - at_top - at_bottom;
            uint32_t adj_tl1 = 0;
            uint32_t adj_tl2 = 0;
            if (this->config.round == BLUR_ROUND_UP) {
                adj_tl1 = 0x00020202;
                adj_tl2 = 0x00030303;
            }

            while (y--) {
                int x;
                uint32_t* f_below = f + w;
                uint32_t* f_above = f - w;

                // left edge
                *of++ = DIV_2(f[1]) + DIV_4(f_below[0]) + DIV_4(f_above[0]) + adj_tl1;
                f++;
                f_below++;
                f_above++;

                // middle of line
                x = (w - 2) / 4;
#ifdef SIMD_MODE_X86_SSE
                __m128i round_up_or_zero = this->config.round == BLUR_ROUND_UP
                                               ? _mm_set1_epi32(adj_tl2)
                                               : _mm_setzero_si128();
                __m128i strip_high_2bit_mask = _mm_set1_epi32(0x3f3f3f3f);
                while (x--) {
                    __m128i four_px_above = _mm_loadu_si128((__m128i*)(f_above));
                    __m128i four_px_left = _mm_loadu_si128((__m128i*)(f - 1));
                    __m128i four_px_right = _mm_loadu_si128((__m128i*)(f + 1));
                    __m128i four_px_below = _mm_loadu_si128((__m128i*)(f_below));
                    four_px_above = _mm_and_si128(_mm_srli_epi32(four_px_above, 2),
                                                  strip_high_2bit_mask);
                    four_px_below = _mm_and_si128(_mm_srli_epi32(four_px_below, 2),
                                                  strip_high_2bit_mask);
                    four_px_left = _mm_and_si128(_mm_srli_epi32(four_px_left, 2),
                                                 strip_high_2bit_mask);
                    four_px_right = _mm_and_si128(_mm_srli_epi32(four_px_right, 2),
                                                  strip_high_2bit_mask);
                    __m128i vertical = _mm_add_epi8(four_px_above, four_px_below);
                    __m128i horizontal = _mm_add_epi8(four_px_left, four_px_right);
                    __m128i cross = _mm_add_epi8(vertical, horizontal);
                    cross = _mm_add_epi8(cross, round_up_or_zero);
                    _mm_storeu_si128((__m128i*)(of), cross);
                    f += 4;
                    f_below += 4;
                    f_above += 4;
                    of += 4;
                }
#else
                if (this->config.round == BLUR_ROUND_UP) {
                    while (x--) {
                        of[0] = DIV_4(f[1]) + DIV_4(f[-1]) + DIV_4(f_below[0])
                                + DIV_4(f_above[0]) + 0x03030303;
                        of[1] = DIV_4(f[2]) + DIV_4(f[0]) + DIV_4(f_below[1])
                                + DIV_4(f_above[1]) + 0x03030303;
                        of[2] = DIV_4(f[3]) + DIV_4(f[1]) + DIV_4(f_below[2])
                                + DIV_4(f_above[2]) + 0x03030303;
                        of[3] = DIV_4(f[4]) + DIV_4(f[2]) + DIV_4(f_below[3])
                                + DIV_4(f_above[3]) + 0x03030303;
                        f += 4;
                        f_below += 4;
                        f_above += 4;
                        of += 4;
                    }
                } else {
                    while (x--) {
                        of[0] = DIV_4(f[1]) + DIV_4(f[-1]) + DIV_4(f_below[0])
                                + DIV_4(f_above[0]);
                        of[1] = DIV_4(f[2]) + DIV_4(f[0]) + DIV_4(f_below[1])
                                + DIV_4(f_above[1]);
                        of[2] = DIV_4(f[3]) + DIV_4(f[1]) + DIV_4(f_below[2])
                                + DIV_4(f_above[2]);
                        of[3] = DIV_4(f[4]) + DIV_4(f[2]) + DIV_4(f_below[3])
                                + DIV_4(f_above[3]);
                        f += 4;
                        f_below += 4;
                        f_above += 4;
                        of += 4;
                    }
                }
#endif
                x = (w - 2) & 3;
                while (x--) {
                    *of++ = DIV_4(f[1]) + DIV_4(f[-1]) + DIV_4(f_below[0])
                            + DIV_4(f_above[0]) + adj_tl2;
                    f++;
                    f_below++;
                    f_above++;
                }

                // right block
                *of++ = DIV_2(f[-1]) + DIV_4(f_below[0]) + DIV_4(f_above[0]) + adj_tl1;
                f++;
            }
        }

        // bottom block
        if (at_bottom) {
            uint32_t* f_above = f - w;
            int x;
            int adj_tl = 0, adj_tl2 = 0;
            if (this->config.round == BLUR_ROUND_UP) {
                adj_tl = 0x02020202;
                adj_tl2 = 0x01010101;
            }
            // bottom left
            *of++ = DIV_2(f[1]) + DIV_2(f_above[0]) + adj_tl2;
            f++;
            f_above++;
            // bottom center
            x = (w - 2) / 4;
            while (x--) {
                of[0] = DIV_4(f[1]) + DIV_4(f[-1]) + DIV_2(f_above[0]) + adj_tl;
                of[1] = DIV_4(f[2]) + DIV_4(f[0]) + DIV_2(f_above[1]) + adj_tl;
                of[2] = DIV_4(f[3]) + DIV_4(f[1]) + DIV_2(f_above[2]) + adj_tl;
                of[3] = DIV_4(f[4]) + DIV_4(f[2]) + DIV_2(f_above[3]) + adj_tl;
                f += 4;
                f_above += 4;
                of += 4;
            }
            x = (w - 2) & 3;
            while (x--) {
                *of++ = DIV_4(f[1]) + DIV_4(f[-1]) + DIV_2(f_above[0]) + adj_tl;
                f++;
                f_above++;
            }
            // bottom right
            *of++ = DIV_2(f[-1]) + DIV_2(f_above[0]) + adj_tl2;
            f++;
            f_above++;
        }
    } else {  // REGULAR

        /* #REGULAR
         *  o 1 o
         *  1 4 1
         *  o 1 o
         *  /8
         *
         *  o 1 o   o 1 |
         *  1 1 1   1 2 |
         *  -----   ----
         *  /4      /4
         */

        // top line
        if (at_top) {
            uint32_t* f_below = f + w;
            int x;
            int adj_tl = 0, adj_tl2 = 0;
            if (this->config.round == BLUR_ROUND_UP) {
                adj_tl = 0x02020202;
                adj_tl2 = 0x03030303;
            }
            // top left
            *of++ = DIV_2(f[0]) + DIV_4(f[1]) + DIV_4(f_below[0]) + adj_tl;
            f++;
            f_below++;
            // top center
            x = (w - 2) / 4;
            while (x--) {
                of[0] = DIV_4(f[0]) + DIV_4(f[1]) + DIV_4(f[-1]) + DIV_4(f_below[0])
                        + adj_tl2;
                of[1] = DIV_4(f[1]) + DIV_4(f[2]) + DIV_4(f[0]) + DIV_4(f_below[1])
                        + adj_tl2;
                of[2] = DIV_4(f[2]) + DIV_4(f[3]) + DIV_4(f[1]) + DIV_4(f_below[2])
                        + adj_tl2;
                of[3] = DIV_4(f[3]) + DIV_4(f[4]) + DIV_4(f[2]) + DIV_4(f_below[3])
                        + adj_tl2;
                f += 4;
                f_below += 4;
                of += 4;
            }
            x = (w - 2) & 3;
            while (x--) {
                *of++ = DIV_4(f[0]) + DIV_4(f[1]) + DIV_4(f[-1]) + DIV_4(f_below[0])
                        + adj_tl2;
                f++;
                f_below++;
            }
            // top right
            *of++ = DIV_2(f[0]) + DIV_4(f[-1]) + DIV_4(f_below[0]) + adj_tl;
            f++;
            f_below++;
        }

        // middle block
        {
            int y = outh - at_top - at_bottom;
            uint32_t adj_tl1 = 0;
            uint32_t adj_tl2 = 0;
            if (this->config.round == BLUR_ROUND_UP) {
                adj_tl1 = 0x00030303;
                adj_tl2 = 0x00040404;
            }
            while (y--) {
                int x;
                uint32_t* f_below = f + w;
                uint32_t* f_above = f - w;

                // left edge
                *of++ = DIV_4(f[0]) + DIV_4(f[1]) + DIV_4(f_below[0])
                        + DIV_4(f_above[0]) + adj_tl1;
                f++;
                f_below++;
                f_above++;

                // middle of line
                x = (w - 2) / 4;
#ifdef SIMD_MODE_X86_SSE
                __m128i round_up_or_zero = this->config.round == BLUR_ROUND_UP
                                               ? _mm_set1_epi32(adj_tl2)
                                               : _mm_setzero_si128();
                __m128i strip_high_1bit_mask = _mm_set1_epi32(0x7f7f7f7f);
                __m128i strip_high_3bit_mask = _mm_set1_epi32(0x1f1f1f1f);
                while (x--) {
                    __m128i four_px_above = _mm_loadu_si128((__m128i*)(f_above));
                    __m128i four_px_left = _mm_loadu_si128((__m128i*)(f - 1));
                    __m128i four_px_right = _mm_loadu_si128((__m128i*)(f + 1));
                    __m128i four_px_below = _mm_loadu_si128((__m128i*)(f_below));
                    __m128i four_px_center = _mm_loadu_si128((__m128i*)f);
                    four_px_center = _mm_and_si128(_mm_srli_epi32(four_px_center, 1),
                                                   strip_high_1bit_mask);
                    four_px_above = _mm_and_si128(_mm_srli_epi32(four_px_above, 3),
                                                  strip_high_3bit_mask);
                    four_px_below = _mm_and_si128(_mm_srli_epi32(four_px_below, 3),
                                                  strip_high_3bit_mask);
                    four_px_left = _mm_and_si128(_mm_srli_epi32(four_px_left, 3),
                                                 strip_high_3bit_mask);
                    four_px_right = _mm_and_si128(_mm_srli_epi32(four_px_right, 3),
                                                  strip_high_3bit_mask);

                    __m128i vertical = _mm_add_epi8(four_px_above, four_px_below);
                    __m128i horizontal = _mm_add_epi8(four_px_left, four_px_right);
                    __m128i cross = _mm_add_epi8(vertical, horizontal);

                    four_px_center = _mm_add_epi8(four_px_center, cross);
                    four_px_center = _mm_add_epi8(four_px_center, round_up_or_zero);
                    _mm_storeu_si128((__m128i*)(of), four_px_center);
                    f += 4;
                    f_below += 4;
                    f_above += 4;
                    of += 4;
                }
#else
                if (this->config.round == BLUR_ROUND_UP) {
                    while (x--) {
                        of[0] = DIV_2(f[0]) + DIV_8(f[1]) + DIV_8(f[-1])
                                + DIV_8(f_below[0]) + DIV_8(f_above[0]) + adj_tl2;
                        of[1] = DIV_2(f[1]) + DIV_8(f[2]) + DIV_8(f[0])
                                + DIV_8(f_below[1]) + DIV_8(f_above[1]) + adj_tl2;
                        of[2] = DIV_2(f[2]) + DIV_8(f[3]) + DIV_8(f[1])
                                + DIV_8(f_below[2]) + DIV_8(f_above[2]) + adj_tl2;
                        of[3] = DIV_2(f[3]) + DIV_8(f[4]) + DIV_8(f[2])
                                + DIV_8(f_below[3]) + DIV_8(f_above[3]) + adj_tl2;
                        f += 4;
                        f_below += 4;
                        f_above += 4;
                        of += 4;
                    }
                } else {
                    while (x--) {
                        of[0] = DIV_2(f[0]) + DIV_8(f[1]) + DIV_8(f[-1])
                                + DIV_8(f_below[0]) + DIV_8(f_above[0]);
                        of[1] = DIV_2(f[1]) + DIV_8(f[2]) + DIV_8(f[0])
                                + DIV_8(f_below[1]) + DIV_8(f_above[1]);
                        of[2] = DIV_2(f[2]) + DIV_8(f[3]) + DIV_8(f[1])
                                + DIV_8(f_below[2]) + DIV_8(f_above[2]);
                        of[3] = DIV_2(f[3]) + DIV_8(f[4]) + DIV_8(f[2])
                                + DIV_8(f_below[3]) + DIV_8(f_above[3]);
                        f += 4;
                        f_below += 4;
                        f_above += 4;
                        of += 4;
                    }
                }
#endif
                x = (w - 2) & 3;
                while (x--) {
                    *of++ = DIV_2(f[0]) + DIV_8(f[1]) + DIV_8(f[-1]) + DIV_8(f_below[0])
                            + DIV_8(f_above[0]) + adj_tl2;
                    f++;
                    f_below++;
                    f_above++;
                }

                // right block
                *of++ = DIV_4(f[0]) + DIV_4(f[-1]) + DIV_4(f_below[0])
                        + DIV_4(f_above[0]) + adj_tl1;
                f++;
            }
        }

        // bottom block
        if (at_bottom) {
            uint32_t* f_above = f - w;
            int adj_tl = 0, adj_tl2 = 0;
            if (this->config.round == BLUR_ROUND_UP) {
                adj_tl = 0x02020202;
                adj_tl2 = 0x03030303;
            }
            int x;
            // bottom left
            *of++ = DIV_2(f[0]) + DIV_4(f[1]) + DIV_4(f_above[0]) + adj_tl;
            f++;
            f_above++;
            // bottom center
            x = (w - 2) / 4;
            while (x--) {
                of[0] = DIV_4(f[0]) + DIV_4(f[1]) + DIV_4(f[-1]) + DIV_4(f_above[0])
                        + adj_tl2;
                of[1] = DIV_4(f[1]) + DIV_4(f[2]) + DIV_4(f[0]) + DIV_4(f_above[1])
                        + adj_tl2;
                of[2] = DIV_4(f[2]) + DIV_4(f[3]) + DIV_4(f[1]) + DIV_4(f_above[2])
                        + adj_tl2;
                of[3] = DIV_4(f[3]) + DIV_4(f[4]) + DIV_4(f[2]) + DIV_4(f_above[3])
                        + adj_tl2;
                f += 4;
                f_above += 4;
                of += 4;
            }
            x = (w - 2) & 3;
            while (x--) {
                *of++ = DIV_4(f[0]) + DIV_4(f[1]) + DIV_4(f[-1]) + DIV_4(f_above[0])
                        + adj_tl2;
                f++;
                f_above++;
            }
            // bottom right
            *of++ = DIV_2(f[0]) + DIV_4(f[-1]) + DIV_4(f_above[0]) + adj_tl;
            f++;
            f_above++;
        }
    }
}

int E_Blur::smp_begin(int max_threads, char[2][2][576], int, int*, int*, int, int) {
    return max_threads;
}

int E_Blur::smp_finish(char[2][2][576], int, int*, int*, int, int) { return 1; }

int E_Blur::render(char visdata[2][2][576],
                   int is_beat,
                   int* framebuffer,
                   int* fbout,
                   int w,
                   int h) {
    this->smp_begin(1, visdata, is_beat, framebuffer, fbout, w, h);
    if (is_beat & 0x80000000) {
        return 0;
    }

    this->smp_render(0, 1, visdata, is_beat, framebuffer, fbout, w, h);
    return this->smp_finish(visdata, is_beat, framebuffer, fbout, w, h);
}

void E_Blur::load_legacy(unsigned char* data, int len) {
    int pos = 0;
    uint32_t blur_level = BLUR_MEDIUM;
    if (len - pos >= 4) {
        blur_level = GET_INT();
        this->enabled = true;
        // Note that in the legacy save format Medium is 1 and Light is 2.
        // This is not a mistake.
        switch (blur_level) {
            case 0: this->enabled = false; break;
            default:
            case 1: this->config.level = BLUR_MEDIUM; break;
            case 2: this->config.level = BLUR_LIGHT; break;
            case 3: this->config.level = BLUR_HEAVY; break;
        }
        pos += 4;
    }
    if (len - pos >= 4) {
        this->config.round = GET_INT() == 1 ? BLUR_ROUND_UP : BLUR_ROUND_DOWN;
        pos += 4;
    } else {
        this->config.round = BLUR_ROUND_DOWN;
    }
}

int E_Blur::save_legacy(unsigned char* data) {
    int pos = 0;
    uint32_t blur_level = 0;
    if (this->enabled) {
        switch (this->config.level) {
            case BLUR_LIGHT: blur_level = 2; break;
            default:
            case BLUR_MEDIUM: blur_level = 1; break;
            case BLUR_HEAVY: blur_level = 3; break;
        }
    }
    PUT_INT(blur_level);
    pos += 4;
    PUT_INT(this->config.round == BLUR_ROUND_UP ? 1 : 0);
    pos += 4;
    return pos;
}

Effect_Info* create_Blur_Info() { return new Blur_Info(); }
Effect* create_Blur(AVS_Instance* avs) { return new E_Blur(avs); }
void set_Blur_desc(char* desc) { E_Blur::set_desc(desc); }
