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
#include "c_stack.h"

#include "r_defs.h"

#include "timing.h"

#define PUT_INT(y)                   \
    data[pos] = (y)&255;             \
    data[pos + 1] = (y >> 8) & 255;  \
    data[pos + 2] = (y >> 16) & 255; \
    data[pos + 3] = (y >> 24) & 255
#define GET_INT() \
    (data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24))
void C_THISCLASS::load_config(unsigned char* data, int len) {
    int pos = 0;
    if (len - pos >= 4) {
        dir = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        which = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        blend = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        adjblend_val = GET_INT();
        pos += 4;
    }

    if (which < 0) which = 0;
    if (which >= NBUF) which = NBUF - 1;
}
int C_THISCLASS::save_config(unsigned char* data) {
    int pos = 0;
    PUT_INT(dir);
    pos += 4;
    PUT_INT(which);
    pos += 4;
    PUT_INT(blend);
    pos += 4;
    PUT_INT(adjblend_val);
    pos += 4;
    return pos;
}

char* C_THISCLASS::get_desc() { return MOD_NAME; }

C_THISCLASS::C_THISCLASS() {
    adjblend_val = 128;
    dir_ch = 0;
    which = 0;
    dir = 0;
    blend = 0;
    clear = 0;
}

C_THISCLASS::~C_THISCLASS() {}

int C_THISCLASS::render(char[2][2][576],
                        int isBeat,
                        int* framebuffer,
                        int*,
                        int w,
                        int h) {
    void* thisbufptr;
    if (isBeat & 0x80000000) return 0;
    if (!(thisbufptr = getGlobalBuffer(w, h, which, dir != 1))) {
        return 0;
    }

    if (clear) {
        clear = 0;
        memset(thisbufptr, 0, sizeof(int) * w * h);
    }

    {
        int t_dir;
        t_dir = (dir < 2) ? dir : ((dir & 1) ^ dir_ch);
        dir_ch ^= 1;
        int* fbin = (int*)(t_dir == 0 ? framebuffer : thisbufptr);
        int* fbout = (int*)(t_dir != 0 ? framebuffer : thisbufptr);
        if (blend == 1) {
            mmx_avgblend_block(fbout, fbin, w * h);
        } else if (blend == 2) {
            mmx_addblend_block(fbout, fbin, w * h);
        } else if (blend == 3) {
            int r = 0;
            int y = h;
            int* bf = fbin;
            while (y-- > 0) {
                int *out, *in;
                int x = w / 2;
                out = fbout + r;
                in = bf + r;
                r ^= 1;
                while (x-- > 0) {
                    *out = *in;
                    out += 2;
                    in += 2;
                }
                fbout += w;
                bf += w;
            }
        } else if (blend == 4) {
            int x = w * h;
            int* bf = fbin;
            while (x-- > 0) {
                *fbout = BLEND_SUB(*fbout, *bf);
                fbout++;
                bf++;
            }
        } else if (blend == 5) {
            int y = h / 2;
            while (y-- > 0) {
                memcpy(fbout, fbin, w * sizeof(int));
                fbout += w * 2;
                fbin += w * 2;
            }
        } else if (blend == 6) {
            int x = w * h;
            while (x-- > 0) {
                *fbout = *fbout ^ *fbin;
                fbout++;
                fbin++;
            }
        } else if (blend == 7) {
            int x = w * h;
            int* bf = fbin;
            while (x-- > 0) {
                *fbout = BLEND_MAX(*fbout, *bf);
                fbout++;
                bf++;
            }
        } else if (blend == 8) {
            int x = w * h;
            int* bf = fbin;
            while (x-- > 0) {
                *fbout = BLEND_MIN(*fbout, *bf);
                fbout++;
                bf++;
            }
        } else if (blend == 9) {
            int x = w * h;
            int* bf = fbin;
            while (x-- > 0) {
                *fbout = BLEND_SUB(*bf, *fbout);
                fbout++;
                bf++;
            }
        } else if (blend == 10) {
            mmx_mulblend_block(fbout, fbin, w * h);
        } else if (blend == 11) {
            mmx_adjblend_block(fbout, fbin, fbout, w * h, adjblend_val);
        } else
            memcpy(fbout, fbin, w * h * sizeof(int));
        return 0;
    }

    return 0;
}

C_RBASE* R_Stack(char* desc) {
    if (desc) {
        strcpy(desc, MOD_NAME);
        return NULL;
    }
    return (C_RBASE*)new C_THISCLASS();
}
