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
// highly optimized on 10/10/00 JF. MMX.

#include "e_blitterfeedback.h"

#include "blend.h"

#define PUT_INT(y)                     \
    data[pos] = (y) & 255;             \
    data[pos + 1] = ((y) >> 8) & 255;  \
    data[pos + 2] = ((y) >> 16) & 255; \
    data[pos + 3] = ((y) >> 24) & 255

#define GET_INT() \
    (data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24))

constexpr Parameter BlitterFeedback_Info::parameters[];

void BlitterFeedback_Info::on_zoom_change(Effect* component,
                                          const Parameter* parameter,
                                          const std::vector<int64_t>&) {
    auto blitter = (E_BlitterFeedback*)component;
    blitter->set_current_zoom((int32_t)blitter->get_int(parameter));
}

// {0x1000100,0x1000100}; <<- this is actually more correct, but we're going for
// consistency vs. the non-mmx ver
// -jf
static const uint64_t revn = 0x00ff00ff00ff00ff;
static const int zero = 0;

E_BlitterFeedback::E_BlitterFeedback(AVS_Instance* avs)
    : Configurable_Effect(avs), current_zoom(this->config.zoom) {}

void E_BlitterFeedback::set_current_zoom(int32_t zoom) { this->current_zoom = zoom; }

int E_BlitterFeedback::blitter_out(uint32_t* framebuffer,
                                   uint32_t* fbout,
                                   int w,
                                   int h,
                                   int32_t zoom) {
    const int32_t adj = 7;
    int32_t ds_x = ((zoom + (1 << adj)) << (16 - adj));
    int32_t x_len = ((w << 16) / ds_x) & ~3;
    int32_t y_len = (h << 16) / ds_x;

    if (x_len >= w || y_len >= h) {
        return 0;
    }

    int32_t start_x = (w - x_len) / 2;
    int32_t start_y = (h - y_len) / 2;
    int32_t s_y = 32768;

    int32_t* dest = (int32_t*)framebuffer + start_y * w + start_x;
    int32_t* src = (int32_t*)fbout + start_y * w + start_x;
    int32_t y;

    fbout += start_y * w;
    for (y = 0; y < y_len; y++) {
        int32_t s_x = 32768;
        uint32_t* src = framebuffer + (s_y >> 16) * w;
        uint32_t* old_dest = fbout + start_x;
        s_y += ds_x;
        if (this->config.blend_mode == BLITTER_BLEND_REPLACE) {
            int32_t x = x_len / 4;
            while (x--) {
                old_dest[0] = src[s_x >> 16];
                s_x += ds_x;
                old_dest[1] = src[s_x >> 16];
                s_x += ds_x;
                old_dest[2] = src[s_x >> 16];
                s_x += ds_x;
                old_dest[3] = src[s_x >> 16];
                s_x += ds_x;
                old_dest += 4;
            }
        } else {  // BLITTER_BLEND_5050
            uint32_t* s2 = (uint32_t*)framebuffer + ((y + start_y) * w) + start_x;
            int32_t x = x_len / 4;
            while (x--) {
                blend_5050_1px(&src[s_x >> 16], &s2[0], &old_dest[0]);
                s_x += ds_x;
                blend_5050_1px(&src[s_x >> 16], &s2[1], &old_dest[1]);
                s_x += ds_x;
                blend_5050_1px(&src[s_x >> 16], &s2[2], &old_dest[2]);
                s_x += ds_x;
                blend_5050_1px(&src[s_x >> 16], &s2[3], &old_dest[3]);
                s2 += 4;
                old_dest += 4;
                s_x += ds_x;
            }
        }
        fbout += w;
    }
    for (y = 0; y < y_len; y++) {
        memcpy(dest, src, x_len * sizeof(int32_t));
        dest += w;
        src += w;
    }

    return 0;
}

int E_BlitterFeedback::blitter_in(uint32_t* framebuffer,
                                  uint32_t* fbout,
                                  int w,
                                  int h,
                                  int32_t zoom) {
    int32_t ds_x = ((zoom + 64) << 16) / 64;
    int32_t isx = (((w << 16) - ((ds_x * w))) / 2);
    int32_t s_y = (((h << 16) - ((ds_x * h))) / 2);

    if (this->config.bilinear) {
        int32_t y;
        for (y = 0; y < h; y++) {
            int32_t s_x = isx;
            uint32_t* src = ((uint32_t*)framebuffer) + (s_y >> 16) * w;
            int32_t ypart = (s_y >> 8) & 0xff;
            s_y += ds_x;
            ypart = (ypart * 255) >> 8;
            int32_t x = w / 4;
            while (x--) {
                fbout[0] =
                    blend_bilinear_2x2(src + (s_x >> 16), w, (s_x >> 8) & 0xff, ypart);
                s_x += ds_x;
                fbout[1] =
                    blend_bilinear_2x2(src + (s_x >> 16), w, (s_x >> 8) & 0xff, ypart);
                s_x += ds_x;
                fbout[2] =
                    blend_bilinear_2x2(src + (s_x >> 16), w, (s_x >> 8) & 0xff, ypart);
                s_x += ds_x;
                fbout[3] =
                    blend_bilinear_2x2(src + (s_x >> 16), w, (s_x >> 8) & 0xff, ypart);
                s_x += ds_x;
                fbout += 4;
            }
            if (this->config.blend_mode == BLITTER_BLEND_5050) {
                // reblend this scanline with the original
                fbout -= w;
                int32_t x = w / 4;
                uint32_t* s2 = (uint32_t*)framebuffer + y * w;
                while (x--) {
                    blend_5050_1px(&s2[0], &fbout[0], &fbout[0]);
                    blend_5050_1px(&s2[1], &fbout[1], &fbout[1]);
                    blend_5050_1px(&s2[2], &fbout[2], &fbout[2]);
                    blend_5050_1px(&s2[3], &fbout[3], &fbout[3]);
                    fbout += 4;
                    s2 += 4;
                }
            }
        }
    } else {  // !this->config.bilinear
        int32_t y;
        for (y = 0; y < h; y++) {
            int32_t s_x = isx;
            uint32_t* src = ((uint32_t*)framebuffer) + (s_y >> 16) * w;
            s_y += ds_x;
            if (this->config.blend_mode == BLITTER_BLEND_REPLACE) {
                int32_t x = w / 4;
                while (x--) {
                    fbout[0] = src[s_x >> 16];
                    s_x += ds_x;
                    fbout[1] = src[s_x >> 16];
                    s_x += ds_x;
                    fbout[2] = src[s_x >> 16];
                    s_x += ds_x;
                    fbout[3] = src[s_x >> 16];
                    s_x += ds_x;
                    fbout += 4;
                }
            } else {
                uint32_t* s2 = (uint32_t*)framebuffer + (y * w);
                int32_t x = w / 4;
                while (x--) {
                    blend_5050_1px(&src[s_x >> 16], &s2[0], &fbout[0]);
                    s_x += ds_x;
                    blend_5050_1px(&src[s_x >> 16], &s2[1], &fbout[1]);
                    s_x += ds_x;
                    blend_5050_1px(&src[s_x >> 16], &s2[2], &fbout[2]);
                    s_x += ds_x;
                    blend_5050_1px(&src[s_x >> 16], &s2[3], &fbout[3]);
                    s2 += 4;
                    fbout += 4;
                    s_x += ds_x;
                }
            }
        }
    }
    return 1;
}

int E_BlitterFeedback::render(char[2][2][576],
                              int is_beat,
                              int* framebuffer,
                              int* fbout,
                              int w,
                              int h) {
    if (is_beat & 0x80000000) {
        return 0;
    }

    if (is_beat && this->config.on_beat) {
        this->current_zoom = this->config.on_beat_zoom;
    }

    int32_t target_zoom;
    if (this->config.zoom < this->config.on_beat_zoom) {
        target_zoom = max(this->current_zoom, (int32_t)this->config.zoom);
        this->current_zoom -= 3;
    } else {
        target_zoom = min(this->current_zoom, (int32_t)this->config.zoom);
        this->current_zoom += 3;
    }

    if (target_zoom < this->info.parameters[0].int_min) {
        target_zoom = 0;
    }

    if (target_zoom < 0) {
        return blitter_in((uint32_t*)framebuffer, (uint32_t*)fbout, w, h, target_zoom);
    }
    if (target_zoom > 0) {
        return blitter_out((uint32_t*)framebuffer, (uint32_t*)fbout, w, h, target_zoom);
    }
    return 0;
}

void E_BlitterFeedback::load_legacy(unsigned char* data, int len) {
    int pos = 0;
    if (len - pos >= 4) {
        this->config.zoom = GET_INT() - 32;
        pos += 4;
    }
    if (len - pos >= 4) {
        this->config.on_beat_zoom = GET_INT() - 32;
        pos += 4;
    }
    if (len - pos >= 4) {
        this->config.blend_mode = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        this->config.on_beat_zoom = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        this->config.bilinear = GET_INT();
        pos += 4;
    }

    this->current_zoom = (int32_t)this->config.zoom;
}

int E_BlitterFeedback::save_legacy(unsigned char* data) {
    int pos = 0;
    PUT_INT(this->config.zoom + 32);
    pos += 4;
    PUT_INT(this->config.on_beat_zoom + 32);
    pos += 4;
    PUT_INT(this->config.blend_mode);
    pos += 4;
    PUT_INT(this->config.on_beat_zoom);
    pos += 4;
    PUT_INT(this->config.bilinear);
    pos += 4;
    return pos;
}

Effect_Info* create_BlitterFeedback_Info() { return new BlitterFeedback_Info(); }
Effect* create_BlitterFeedback(AVS_Instance* avs) { return new E_BlitterFeedback(avs); }
void set_BlitterFeedback_desc(char* desc) { E_BlitterFeedback::set_desc(desc); }
