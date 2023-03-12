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
#include "c_timescope.h"

#include "r_defs.h"

#include <stdio.h>
#include <stdlib.h>

C_THISCLASS::~C_THISCLASS() {}

// configuration read/write

C_THISCLASS::C_THISCLASS()  // set up default configuration
{
    x = 0;
    color = 0xFFFFFF;
    blend = 2;
    blendavg = 0;
    enabled = 1;
    nbands = 576;
    oldh = -1;
    which_ch = 2;
}

#define GET_INT() \
    (data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24))
void C_THISCLASS::load_config(unsigned char* data, int len)  // read configuration of
                                                             // max length "len" from
                                                             // data.
{
    int pos = 0;
    if (len - pos >= 4) {
        enabled = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        color = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        blend = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        blendavg = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        which_ch = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        nbands = GET_INT();
        pos += 4;
    }
    oldh = -1;
}

#define PUT_INT(y)                   \
    data[pos] = (y)&255;             \
    data[pos + 1] = (y >> 8) & 255;  \
    data[pos + 2] = (y >> 16) & 255; \
    data[pos + 3] = (y >> 24) & 255
int C_THISCLASS::save_config(unsigned char* data)  // write configuration to data,
                                                   // return length. config data should
                                                   // not exceed 64k.
{
    int pos = 0;
    PUT_INT(enabled);
    pos += 4;
    PUT_INT(color);
    pos += 4;
    PUT_INT(blend);
    pos += 4;
    PUT_INT(blendavg);
    pos += 4;
    PUT_INT(which_ch);
    pos += 4;
    PUT_INT(nbands);
    pos += 4;
    return pos;
}

// render function
// render should return 0 if it only used framebuffer, or 1 if the new output data is in
// fbout. this is used when you want to do something that you'd otherwise need to make a
// copy of the framebuffer. w and h are the width and height of the screen, in pixels.
// isBeat is 1 if a beat has been detected.
// visdata is in the format of [spectrum:0,wave:1][channel][band].

int C_THISCLASS::render(char visdata[2][2][576],
                        int isBeat,
                        int* framebuffer,
                        int*,
                        int w,
                        int h) {
    int i;
    int c;

    if (!enabled) {
        return 0;
    }
    if (isBeat & 0x80000000) {
        return 0;
    }

    x++;
    x %= w;
    framebuffer += x;
    int r, g, b;
    r = color & 0xff;
    g = (color >> 8) & 0xff;
    b = (color >> 16) & 0xff;
    for (i = 0; i < h; i++) {
        c = visdata[0][0][(i * nbands) / h] & 0xFF;
        c = (r * c) / 256 + (((g * c) / 256) << 8) + (((b * c) / 256) << 16);
        if (blend == 2) {
            BLEND_LINE(framebuffer, c);
        } else if (blend == 1) {
            framebuffer[0] = BLEND(framebuffer[0], c);
        } else if (blendavg) {
            framebuffer[0] = BLEND_AVG(framebuffer[0], c);
        } else {
            framebuffer[0] = c;
        }
        framebuffer += w;
    }

    return 0;
}

C_RBASE* R_Timescope(char* desc)  // creates a new effect object if desc is NULL,
                                  // otherwise fills in desc with description
{
    if (desc) {
        strcpy(desc, MOD_NAME);
        return NULL;
    }
    return (C_RBASE*)new C_THISCLASS();
}
