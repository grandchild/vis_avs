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
#include "c_bpm.h"

#include "r_defs.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define SLIDER_CTRL_IN  1
#define SLIDER_CTRL_OUT 2

C_THISCLASS::~C_THISCLASS() {}

C_THISCLASS::C_THISCLASS()  // set up default configuration
{
    skipfirst = 0;
    inSlide = 0;
    outSlide = 0;
    oldInSlide = 0;
    oldOutSlide = 0;
    enabled = 1;
    arbLastTC = timer_ms();
    arbitrary = 1;
    arbVal = 500;
    skipVal = 1;
    skipCount = 0;
    skip = 0;
    invert = 0;
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
        arbitrary = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        skip = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        invert = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        arbVal = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        skipVal = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        skipfirst = GET_INT();
        pos += 4;
    }
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
    PUT_INT(arbitrary);
    pos += 4;
    PUT_INT(skip);
    pos += 4;
    PUT_INT(invert);
    pos += 4;
    PUT_INT(arbVal);
    pos += 4;
    PUT_INT(skipVal);
    pos += 4;
    PUT_INT(skipfirst);
    pos += 4;
    return pos;
}

void C_THISCLASS::SliderStep(int Ctl, int* slide) {
    *slide += Ctl == SLIDER_CTRL_IN ? inInc : outInc;
    if (!*slide || *slide == 8) {
        (Ctl == SLIDER_CTRL_IN ? inInc : outInc) *= -1;
    }
}

// render function
// render should return 0 if it only used framebuffer, or 1 if the new output data is in
// fbout. this is used when you want to do something that you'd otherwise need to make a
// copy of the framebuffer. w and h are the width and height of the screen, in pixels.
// isBeat is 1 if a beat has been detected.
// visdata is in the format of [spectrum:0,wave:1][channel][band].

int C_THISCLASS::render(char[2][2][576], int isBeat, int*, int*, int, int) {
    if (!enabled) {
        return 0;
    }
    if (isBeat & 0x80000000) {
        return 0;
    }

    if (isBeat)  // Show the beat received from AVS
    {
        SliderStep(SLIDER_CTRL_IN, &inSlide);
        count++;
    }

    if (skipfirst != 0 && count <= skipfirst) {
        return isBeat ? CLR_BEAT : 0;
    }

    if (arbitrary) {
        uint64_t TCNow = timer_ms();
        if (TCNow > arbLastTC + arbVal) {
            arbLastTC = TCNow;
            SliderStep(SLIDER_CTRL_OUT, &outSlide);
            return SET_BEAT;
        }
        return CLR_BEAT;
    }

    if (skip) {
        if (isBeat && ++skipCount >= skipVal + 1) {
            skipCount = 0;
            SliderStep(SLIDER_CTRL_OUT, &outSlide);
            return SET_BEAT;
        }
        return CLR_BEAT;
    }

    if (invert) {
        if (isBeat) {
            return CLR_BEAT;
        } else {
            SliderStep(SLIDER_CTRL_OUT, &outSlide);
            return SET_BEAT;
        }
    }
    return 0;
}

C_RBASE* R_Bpm(char* desc)  // creates a new effect object if desc is NULL, otherwise
                            // fills in desc with description
{
    if (desc) {
        strcpy(desc, MOD_NAME);
        return NULL;
    }
    return (C_RBASE*)new C_THISCLASS();
}
