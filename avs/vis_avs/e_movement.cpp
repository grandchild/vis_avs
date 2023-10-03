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

#include "e_movement.h"

#include "r_defs.h"

#include "avs_eelif.h"

#include <math.h>
#include <mmintrin.h>

#define REFFECT_MIN 2
#define REFFECT_MAX 22

#define PUT_INT(y)                     \
    data[pos] = (y)&255;               \
    data[pos + 1] = ((y) >> 8) & 255;  \
    data[pos + 2] = ((y) >> 16) & 255; \
    data[pos + 3] = ((y) >> 24) & 255
#define GET_INT() \
    (data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24))

#define MAKE_EFFECT(n, expr)                                                         \
    void _effect_##n(double& r, double& d, double max_d, int& xo, int& yo) {         \
        /* any parameter unused in at least one expression, avoid "unused parameter" \
         * warnings */                                                               \
        (void)r, (void)d, (void)max_d, (void)xo, (void)yo;                           \
        expr                                                                         \
    }

typedef void effect_func_t(double& r, double& d, double max_d, int& xo, int& yo);

MAKE_EFFECT(2, r += 0.1 - 0.2 * (d / max_d); d *= 0.96;)
MAKE_EFFECT(3, d *= 0.99 * (1.0 - sin(r) / 32.0);
            r += 0.03 * sin(d / max_d * M_PI * 4);)
MAKE_EFFECT(4, d *= 0.94 + (cos(r * 32.0) * 0.06);)
MAKE_EFFECT(5, d *= 1.01 + (cos(r * 4.0) * 0.04);
            r += 0.03 * sin(d / max_d * M_PI * 4);)
MAKE_EFFECT(7, r += 0.1 * sin(d / max_d * M_PI * 5);)
MAKE_EFFECT(8, double t; t = sin(d / max_d * M_PI); d -= 8 * t * t * t * t * t;)
MAKE_EFFECT(9, double t; t = sin(d / max_d * M_PI); d -= 8 * t * t * t * t * t;
            t = cos((d / max_d) * M_PI / 2.0);
            r += 0.1 * t * t * t;)
MAKE_EFFECT(10, d *= 0.95 + (cos(r * 5.0 - M_PI / 2.50) * 0.03);)
MAKE_EFFECT(11, r += 0.04; d *= 0.96 + cos(d / max_d * M_PI) * 0.05;)
MAKE_EFFECT(12, double t; t = cos(d / max_d * M_PI); r += 0.07 * t;
            d *= 0.98 + t * 0.10;)
MAKE_EFFECT(13, r += 0.1 - 0.2 * (d / max_d); d *= 0.96; xo = 8;)
MAKE_EFFECT(14, d = max_d * 0.15;)
MAKE_EFFECT(15, r = cos(r * 3);)
MAKE_EFFECT(16, d *= (1 - (((d / max_d) - .35) * .5)); r += .1;)

static effect_func_t* builtin_effects[REFFECT_MAX - REFFECT_MIN + 1] = {
    _effect_2,   _effect_3,   _effect_4,   _effect_5,   NULL,        _effect_7,
    _effect_8,   _effect_9,   _effect_10,  _effect_11,  _effect_12,  _effect_13,
    _effect_14,  _effect_15,  _effect_16,  NULL /*18*/, NULL /*19*/, NULL /*20*/,
    NULL /*21*/, NULL /*22*/, NULL /*23*/,
};

constexpr Movement_Effect Movement_Info::effects[];
constexpr Parameter Movement_Info::parameters[];

void Movement_Info::recreate_tab(Effect* component,
                                 const Parameter*,
                                 const std::vector<int64_t>&) {
    E_Movement* movement = (E_Movement*)component;
    lock_lock(movement->transform.lock);
    movement->transform.need_regen = true;
    lock_unlock(movement->transform.lock);
}

void Movement_Info::recompile(Effect* component,
                              const Parameter*,
                              const std::vector<int64_t>&) {
    E_Movement* movement = (E_Movement*)component;
    lock_lock(movement->transform.lock);
    movement->use_eel = true;
    movement->config.effect = -1;
    movement->config.use_custom_code = true;
    movement->transform.need_regen = true;
    lock_unlock(movement->transform.lock);
}

void Movement_Info::load_effect(Effect* component,
                                const Parameter*,
                                const std::vector<int64_t>&) {
    E_Movement* movement = (E_Movement*)component;
    const Movement_Effect& to_load = movement->info.effects[movement->config.effect];
    movement->config.init = to_load.code;
    movement->config.wrap = to_load.wrap;
    movement->config.coordinates = to_load.coordinates;
    movement->use_eel = to_load.use_eel;
    movement->config.use_custom_code = false;
    movement->transform.need_regen = true;
}

E_Movement::E_Movement(AVS_Instance* avs) : Programmable_Effect(avs) {}
E_Movement::~E_Movement() {}
E_Movement::Transform::Transform() : need_regen(true), lock(lock_init()){};
E_Movement::Transform::~Transform() {
    free(this->table);
    lock_destroy(this->lock);
}

void E_Movement::regenerate_transform_table(int effect,
                                            char visdata[2][2][576],
                                            int w,
                                            int h) {
    lock_lock(this->transform.lock);
    int p;
    int* trans_p;
    int x;
    free(this->transform.table);
    this->transform.w = w;
    this->transform.h = h;
    this->transform.table =
        (int*)malloc(this->transform.w * this->transform.h * sizeof(int));
    this->transform.bilinear =
        (this->config.bilinear && this->transform.w * this->transform.h < (1 << 22)
         && ((effect >= REFFECT_MIN && effect <= REFFECT_MAX && effect != 0
              && effect != 1 && effect != 6)
             || this->config.use_custom_code));

    trans_p = this->transform.table;
    x = w * h;
    p = 0;

    if (effect == 0) {  // slight fuzzify
        while (x--) {
            int r = (p++) + (rand() % 3) - 1 + ((rand() % 3) - 1) * w;
            *trans_p++ = min(w * h - 1, max(r, 0));
        }
    } else if (effect == 1) {  // shift rotate left
        int y = h;
        while (y--) {
            int x = w;
            int lp = w / 64;
            while (x--) {
                *trans_p++ = p + lp++;
                if (lp >= w) {
                    lp -= w;
                }
            }
            p += w;
        }
    } else if (effect == 6) {  // blocky partial out
        int y;
        for (y = 0; y < h; y++) {
            for (x = 0; x < w; x++) {
                if (x & 2 || y & 2) {
                    *trans_p++ = x + y * w;
                } else {
                    int xp = w / 2 + (((x & ~1) - w / 2) * 7) / 8;
                    int yp = h / 2 + (((y & ~1) - h / 2) * 7) / 8;
                    *trans_p++ = xp + yp * w;
                }
            }
        }
    } else if (effect >= REFFECT_MIN && effect <= REFFECT_MAX
               && !this->info.effects[effect].use_eel) {
        double max_d = sqrt((w * w + h * h) / 4.0);
        int y;
        effect_func_t* effect_func = builtin_effects[effect - REFFECT_MIN];
        if (effect_func) {
            for (y = 0; y < h; y++) {
                for (x = 0; x < w; x++) {
                    double r, d;
                    double xd, yd;
                    int ow, oh, xo = 0, yo = 0;
                    xd = x - (w / 2);
                    yd = y - (h / 2);
                    d = sqrt(xd * xd + yd * yd);
                    r = atan2(yd, xd);

                    effect_func(r, d, max_d, xo, yo);

                    double tmp1, tmp2;
                    tmp1 = ((h / 2) + sin(r) * d + 0.5) + (yo * h) * (1.0 / 256.0);
                    tmp2 = ((w / 2) + cos(r) * d + 0.5) + (xo * w) * (1.0 / 256.0);
                    oh = (int)tmp1;
                    ow = (int)tmp2;
                    if (this->transform.bilinear) {
                        int xpartial = (int)(32.0 * (tmp2 - ow));
                        int ypartial = (int)(32.0 * (tmp1 - oh));
                        if (this->config.wrap) {
                            ow %= (w - 1);
                            oh %= (h - 1);
                            if (ow < 0) {
                                ow += w - 1;
                            }
                            if (oh < 0) {
                                oh += h - 1;
                            }
                        } else {
                            if (ow < 0) {
                                xpartial = 0;
                                ow = 0;
                            }
                            if (ow >= w - 1) {
                                xpartial = 31;
                                ow = w - 2;
                            }
                            if (oh < 0) {
                                ypartial = 0;
                                oh = 0;
                            }
                            if (oh >= h - 1) {
                                ypartial = 31;
                                oh = h - 2;
                            }
                        }
                        *trans_p++ =
                            (oh * w + ow) | (ypartial << 22) | (xpartial << 27);
                    } else {
                        if (this->config.wrap) {
                            ow %= (w);
                            oh %= (h);
                            if (ow < 0) {
                                ow += w;
                            }
                            if (oh < 0) {
                                oh += h;
                            }
                        } else {
                            if (ow < 0) {
                                ow = 0;
                            }
                            if (ow >= w) {
                                ow = w - 1;
                            }
                            if (oh < 0) {
                                oh = 0;
                            }
                            if (oh >= h) {
                                oh = h - 1;
                            }
                        }
                        *trans_p++ = ow + oh * w;
                    }
                }
            }
        }
    } else if (this->config.use_custom_code || this->info.effects[effect].use_eel) {
        double max_d = sqrt((double)(w * w + h * h)) / 2.0;
        double i_max_d = 1.0 / max_d;
        int y;
        this->reset_code_context();
        this->code_init.need_recompile = true;
        Coordinates coords = this->config.use_custom_code
                                 ? (Coordinates)this->config.coordinates
                                 : this->info.effects[effect].coordinates;
        this->recompile_if_needed();
        this->init_variables(w, h, /* is_beat unused */ false);
        if (this->code_init.is_valid()) {
            double w2 = w / 2;
            double h2 = h / 2;
            double xsc = 1.0 / w2;
            double ysc = 1.0 / h2;

            for (y = 0; y < h; y++) {
                for (x = 0; x < w; x++) {
                    double xd, yd;
                    int ow, oh;
                    xd = x - w2;
                    yd = y - h2;
                    *this->vars.x = xd * xsc;
                    *this->vars.y = yd * ysc;
                    *this->vars.d = sqrt(xd * xd + yd * yd) * i_max_d;
                    *this->vars.r = atan2(yd, xd) + M_PI * 0.5;

                    this->code_init.exec(visdata);

                    double tmp1, tmp2;
                    if (coords == COORDS_POLAR) {
                        *this->vars.d *= max_d;
                        *this->vars.r -= M_PI / 2.0;
                        tmp1 = ((h / 2) + sin(*this->vars.r) * *this->vars.d);
                        tmp2 = ((w / 2) + cos(*this->vars.r) * *this->vars.d);
                    } else {
                        tmp1 = ((*this->vars.y + 1.0) * h2);
                        tmp2 = ((*this->vars.x + 1.0) * w2);
                    }
                    if (this->transform.bilinear) {
                        oh = (int)tmp1;
                        ow = (int)tmp2;
                        int xpartial = (int)(32.0 * (tmp2 - ow));
                        int ypartial = (int)(32.0 * (tmp1 - oh));
                        if (this->config.wrap) {
                            ow %= (w - 1);
                            oh %= (h - 1);
                            if (ow < 0) {
                                ow += w - 1;
                            }
                            if (oh < 0) {
                                oh += h - 1;
                            }
                        } else {
                            if (ow < 0) {
                                xpartial = 0;
                                ow = 0;
                            }
                            if (ow >= w - 1) {
                                xpartial = 31;
                                ow = w - 2;
                            }
                            if (oh < 0) {
                                ypartial = 0;
                                oh = 0;
                            }
                            if (oh >= h - 1) {
                                ypartial = 31;
                                oh = h - 2;
                            }
                        }
                        *trans_p++ =
                            (oh * w + ow) | (ypartial << 22) | (xpartial << 27);
                    } else {
                        tmp1 += 0.5;
                        tmp2 += 0.5;
                        oh = (int)tmp1;
                        ow = (int)tmp2;
                        if (this->config.wrap) {
                            ow %= (w);
                            oh %= (h);
                            if (ow < 0) {
                                ow += w;
                            }
                            if (oh < 0) {
                                oh += h;
                            }
                        } else {
                            if (ow < 0) {
                                ow = 0;
                            }
                            if (ow >= w) {
                                ow = w - 1;
                            }
                            if (oh < 0) {
                                oh = 0;
                            }
                            if (oh >= h) {
                                oh = h - 1;
                            }
                        }
                        *trans_p++ = ow + oh * w;
                    }
                }
            }
        } else {
            trans_p = this->transform.table;
            this->transform.bilinear = false;
            for (x = 0; x < w * h; x++) {
                *trans_p++ = x;
            }
        }
    }
    this->transform.need_regen = false;
    lock_unlock(this->transform.lock);
}

int E_Movement::smp_begin(int max_threads,
                          char visdata[2][2][576],
                          int is_beat,
                          int* framebuffer,
                          int* fbout,
                          int w,
                          int h) {
    if (!this->transform.table || this->transform.w != w || this->transform.h != h
        || this->transform.need_regen) {
        this->regenerate_transform_table(this->config.effect, visdata, w, h);
    }

    if (!(is_beat & 0x80000000)) {
        if (this->config.on_beat_toggle_source_map && is_beat) {
            this->config.source_map = !this->config.source_map;
        }
        if (this->config.source_map) {
            if (this->config.blend_mode == BLEND_SIMPLE_REPLACE) {
                memset(fbout, 0, w * h * sizeof(int));
            } else {
                memcpy(fbout, framebuffer, w * h * sizeof(int));
            }
        }
    }
    return max_threads;
}

void E_Movement::smp_render(int this_thread,
                            int max_threads,
                            char[2][2][576],
                            int,
                            int* framebuffer,
                            int* fbout,
                            int w,
                            int h) {
#define OFFSET_MASK ((1 << 22) - 1)

    unsigned int* in_p = (unsigned int*)framebuffer;
    unsigned int* out_p;
    int* trans_p;
    int x;

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

    int out_h = end_l - start_l;
    if (out_h < 1) {
        return;
    }

    int skip_pix = start_l * w;
    lock_lock(this->transform.lock);
    trans_p = this->transform.table;

    out_p = (unsigned int*)fbout;
    x = (w * out_h) / 4;
    if (this->config.source_map) {
        in_p += skip_pix;
        trans_p += skip_pix;
        if (this->transform.bilinear) {
            while (x--) {
                fbout[trans_p[0] & OFFSET_MASK] =
                    BLEND_MAX(in_p[0], fbout[trans_p[0] & OFFSET_MASK]);
                fbout[trans_p[1] & OFFSET_MASK] =
                    BLEND_MAX(in_p[1], fbout[trans_p[1] & OFFSET_MASK]);
                fbout[trans_p[2] & OFFSET_MASK] =
                    BLEND_MAX(in_p[2], fbout[trans_p[2] & OFFSET_MASK]);
                fbout[trans_p[3] & OFFSET_MASK] =
                    BLEND_MAX(in_p[3], fbout[trans_p[3] & OFFSET_MASK]);
                in_p += 4;
                trans_p += 4;
            }
            x = (w * out_h) & 3;
            if (x > 0) {
                while (x--) {
                    fbout[trans_p[0] & OFFSET_MASK] =
                        BLEND_MAX(in_p++[0], fbout[trans_p[0] & OFFSET_MASK]);
                    trans_p++;
                }
            }
        } else {
            {
                while (x--) {
                    fbout[trans_p[0]] = BLEND_MAX(in_p[0], fbout[trans_p[0]]);
                    fbout[trans_p[1]] = BLEND_MAX(in_p[1], fbout[trans_p[1]]);
                    fbout[trans_p[2]] = BLEND_MAX(in_p[2], fbout[trans_p[2]]);
                    fbout[trans_p[3]] = BLEND_MAX(in_p[3], fbout[trans_p[3]]);
                    in_p += 4;
                    trans_p += 4;
                }
                x = (w * out_h) & 3;
                if (x > 0) {
                    while (x--) {
                        fbout[trans_p[0]] = BLEND_MAX(in_p++[0], fbout[trans_p[0]]);
                        trans_p++;
                    }
                }
            }
        }
        if (this->config.blend_mode == BLEND_SIMPLE_5050) {
            framebuffer += skip_pix;
            fbout += skip_pix;
            x = (w * out_h) / 4;
            while (x--) {
                fbout[0] = BLEND_AVG(fbout[0], framebuffer[0]);
                fbout[1] = BLEND_AVG(fbout[1], framebuffer[1]);
                fbout[2] = BLEND_AVG(fbout[2], framebuffer[2]);
                fbout[3] = BLEND_AVG(fbout[3], framebuffer[3]);
                fbout += 4;
                framebuffer += 4;
            }
            x = (w * out_h) & 3;
            while (x--) {
                fbout[0] = BLEND_AVG(fbout[0], framebuffer[0]);
                fbout++;
                framebuffer++;
            }
        }
    } else {
        in_p += skip_pix;
        out_p += skip_pix;
        trans_p += skip_pix;
        if (this->transform.bilinear && this->config.blend_mode == BLEND_SIMPLE_5050) {
            while (x--) {
                int offs = trans_p[0] & OFFSET_MASK;
                out_p[0] = BLEND_AVG(in_p[0],
                                     BLEND4((unsigned int*)framebuffer + offs,
                                            w,
                                            ((trans_p[0] >> 24) & (31 << 3)),
                                            ((trans_p[0] >> 19) & (31 << 3))));
                offs = trans_p[1] & OFFSET_MASK;
                out_p[1] = BLEND_AVG(in_p[1],
                                     BLEND4((unsigned int*)framebuffer + offs,
                                            w,
                                            ((trans_p[1] >> 24) & (31 << 3)),
                                            ((trans_p[1] >> 19) & (31 << 3))));
                offs = trans_p[2] & OFFSET_MASK;
                out_p[2] = BLEND_AVG(in_p[2],
                                     BLEND4((unsigned int*)framebuffer + offs,
                                            w,
                                            ((trans_p[2] >> 24) & (31 << 3)),
                                            ((trans_p[2] >> 19) & (31 << 3))));
                offs = trans_p[3] & OFFSET_MASK;
                out_p[3] = BLEND_AVG(in_p[3],
                                     BLEND4((unsigned int*)framebuffer + offs,
                                            w,
                                            ((trans_p[3] >> 24) & (31 << 3)),
                                            ((trans_p[3] >> 19) & (31 << 3))));
                trans_p += 4;
                out_p += 4;
                in_p += 4;
            }
            x = (w * out_h) & 3;
            while (x--) {
                int offs = trans_p[0] & OFFSET_MASK;
                out_p++[0] = BLEND_AVG(in_p[0],
                                       BLEND4((unsigned int*)framebuffer + offs,
                                              w,
                                              ((trans_p[0] >> 24) & (31 << 3)),
                                              ((trans_p[0] >> 19) & (31 << 3))));
                trans_p++;
                in_p++;
            }
#ifndef NO_MMX
            _mm_empty();
#endif
        } else if (this->transform.bilinear) {
            while (x--) {
                int offs = trans_p[0] & OFFSET_MASK;
                out_p[0] = BLEND4((unsigned int*)framebuffer + offs,
                                  w,
                                  ((trans_p[0] >> 24) & (31 << 3)),
                                  ((trans_p[0] >> 19) & (31 << 3)));
                offs = trans_p[1] & OFFSET_MASK;
                out_p[1] = BLEND4((unsigned int*)framebuffer + offs,
                                  w,
                                  ((trans_p[1] >> 24) & (31 << 3)),
                                  ((trans_p[1] >> 19) & (31 << 3)));
                offs = trans_p[2] & OFFSET_MASK;
                out_p[2] = BLEND4((unsigned int*)framebuffer + offs,
                                  w,
                                  ((trans_p[2] >> 24) & (31 << 3)),
                                  ((trans_p[2] >> 19) & (31 << 3)));
                offs = trans_p[3] & OFFSET_MASK;
                out_p[3] = BLEND4((unsigned int*)framebuffer + offs,
                                  w,
                                  ((trans_p[3] >> 24) & (31 << 3)),
                                  ((trans_p[3] >> 19) & (31 << 3)));
                trans_p += 4;
                out_p += 4;
            }
            x = (w * out_h) & 3;
            while (x--) {
                int offs = trans_p[0] & OFFSET_MASK;
                out_p++[0] = BLEND4((unsigned int*)framebuffer + offs,
                                    w,
                                    ((trans_p[0] >> 24) & (31 << 3)),
                                    ((trans_p[0] >> 19) & (31 << 3)));
                trans_p++;
            }
#ifndef NO_MMX
            _mm_empty();
#endif
        } else if (this->config.blend_mode == BLEND_SIMPLE_5050) {
            while (x--) {
                out_p[0] = BLEND_AVG(in_p[0], framebuffer[trans_p[0]]);
                out_p[1] = BLEND_AVG(in_p[1], framebuffer[trans_p[1]]);
                out_p[2] = BLEND_AVG(in_p[2], framebuffer[trans_p[2]]);
                out_p[3] = BLEND_AVG(in_p[3], framebuffer[trans_p[3]]);
                out_p += 4;
                in_p += 4;
                trans_p += 4;
            }
            x = (w * out_h) & 3;
            if (x > 0) {
                while (x--) {
                    out_p++[0] = BLEND_AVG(in_p++[0], framebuffer[trans_p++[0]]);
                }
            }
        } else {
            while (x--) {
                out_p[0] = framebuffer[trans_p[0]];
                out_p[1] = framebuffer[trans_p[1]];
                out_p[2] = framebuffer[trans_p[2]];
                out_p[3] = framebuffer[trans_p[3]];
                out_p += 4;
                trans_p += 4;
            }
            x = (w * out_h) & 3;
            if (x > 0) {
                while (x--) {
                    out_p++[0] = framebuffer[trans_p++[0]];
                }
            }
        }
    }
    lock_unlock(this->transform.lock);
}

int E_Movement::smp_finish(char[2][2][576], int, int*, int*, int, int) {
    return this->enabled;
}

int E_Movement::render(char visdata[2][2][576],
                       int is_beat,
                       int* framebuffer,
                       int* fbout,
                       int w,
                       int h) {
    smp_begin(1, visdata, is_beat, framebuffer, fbout, w, h);
    if (is_beat & 0x80000000) {
        return 0;
    }

    smp_render(0, 1, visdata, is_beat, framebuffer, fbout, w, h);
    return smp_finish(visdata, is_beat, framebuffer, fbout, w, h);
}

void Movement_Vars::register_(void* vm_context) {
    this->d = NSEEL_VM_regvar(vm_context, "d");
    this->r = NSEEL_VM_regvar(vm_context, "r");
    this->x = NSEEL_VM_regvar(vm_context, "x");
    this->y = NSEEL_VM_regvar(vm_context, "y");
    this->w = NSEEL_VM_regvar(vm_context, "sw");
    this->h = NSEEL_VM_regvar(vm_context, "sh");
}

void Movement_Vars::init(int w, int h, int, va_list) {
    *this->w = w;
    *this->h = h;
}

void E_Movement::load_legacy(unsigned char* data, int len) {
    int pos = 0;
    char* str_data = (char*)data;
    int effect = 0;
    if (len - pos >= 4) {
        effect = GET_INT();
        pos += 4;
    }
    if (effect == LEGACY_SAVE_USE_CUSTOM_CODE) {
        this->config.use_custom_code = true;
        if (!memcmp(&data[pos], "!rect ", 6)) {
            // Ancient AVS versions marked cartesian coordinates with this prefix.
            pos += 6;
            this->config.coordinates = COORDS_CARTESIAN;
        }
        if (data[pos] == 1) {
            pos += 1;  // Skip version marker
            pos += string_load_legacy(&str_data[pos], this->config.init, len - pos);
        } else if (len - pos >= 256) {
            char buf[257];
            int l = 256 - (this->config.coordinates == COORDS_CARTESIAN ? 6 : 0);
            memcpy(buf, &data[pos], l);
            buf[l] = 0;
            this->config.init.assign(buf);
            pos += l;
        }
    } else {
        this->config.use_custom_code = false;
    }
    if (len - pos >= 4) {
        this->config.blend_mode = GET_INT() ? BLEND_SIMPLE_5050 : BLEND_SIMPLE_REPLACE;
        pos += 4;
    }
    if (len - pos >= 4) {
        int32_t sourcemapped = GET_INT();
        this->config.source_map = bool(sourcemapped & 0b1);
        this->config.on_beat_toggle_source_map = bool(sourcemapped & 0b10);
        pos += 4;
    }
    if (len - pos >= 4) {
        this->config.coordinates = GET_INT() ? COORDS_CARTESIAN : COORDS_POLAR;
        pos += 4;
    }
    if (len - pos >= 4) {
        this->config.bilinear = GET_INT();
        pos += 4;
    } else {
        this->config.bilinear = 0;
    }
    if (len - pos >= 4) {
        this->config.wrap = GET_INT();
        pos += 4;
    }
    if (effect == 0 && len - pos >= 4) {
        effect = GET_INT();
        pos += 4;
    }

    if ((effect != LEGACY_SAVE_USE_CUSTOM_CODE && effect > (REFFECT_MAX + 1))
        || effect < 0) {
        effect = -1;
    }
    this->enabled = effect >= 0;
    if (effect > 0) {
        this->config.effect =
            effect != LEGACY_SAVE_USE_CUSTOM_CODE ? effect - 1 : effect;
    }
}

int E_Movement::save_legacy(unsigned char* data) {
    int pos = 0;
    int32_t effect = 0;
    if (this->enabled) {
        if (this->config.use_custom_code) {
            effect = LEGACY_SAVE_USE_CUSTOM_CODE;
        } else {
            effect = this->config.effect + 1;
        }
    }
    // In case of higher effect indices, saving a zero here and the actual value later
    // are a safeguard for loading the preset with ancient versions of AVS which cannot
    // handle effect > LEGACY_ANCIENT_SAVE_MAX_EFFECT.
    if (effect > LEGACY_ANCIENT_SAVE_MAX_EFFECT && !this->config.use_custom_code) {
        PUT_INT(0);
        pos += 4;
    } else {
        PUT_INT(effect);
        pos += 4;
    }
    if (this->config.use_custom_code) {
        data[pos] = 1;
        pos += 1;
        char* str_data = (char*)data;
        pos += this->string_save_legacy(this->config.init,
                                        &str_data[pos],
                                        MAX_CODE_LEN - 1 - pos,
                                        /*with_nt*/ true);
    }
    PUT_INT(this->config.blend_mode == BLEND_SIMPLE_5050);
    pos += 4;
    int32_t source_map = 0b00;
    if (this->config.on_beat_toggle_source_map) {
        source_map = 0b10;
    } else if (this->config.source_map) {
        source_map = 0b01;
    }
    PUT_INT(source_map);
    pos += 4;
    PUT_INT(this->config.coordinates == COORDS_CARTESIAN);
    pos += 4;
    PUT_INT(this->config.bilinear);
    pos += 4;
    PUT_INT(this->config.wrap);
    pos += 4;

    // See note above. This field back here is ignored in ancient AVS versions.
    if (effect > LEGACY_ANCIENT_SAVE_MAX_EFFECT && !this->config.use_custom_code) {
        PUT_INT(effect);
        pos += 4;
    }
    return pos;
}

Effect_Info* create_Movement_Info() { return new Movement_Info(); }
Effect* create_Movement(AVS_Instance* avs) { return new E_Movement(avs); }
void set_Movement_desc(char* desc) { E_Movement::set_desc(desc); }
