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
#include "c_list.h"
#include "e_unknown.h"

#include "r_defs.h"

#include "avs_eelif.h"
#include "effect_library.h"
#include "render.h"
#include "undo.h"

#include "../platform.h"

#include <stdio.h>

#define PUT_INT(y)                   \
    data[pos] = (y)&255;             \
    data[pos + 1] = (y >> 8) & 255;  \
    data[pos + 2] = (y >> 16) & 255; \
    data[pos + 3] = (y >> 24) & 255
#define GET_INT() \
    (data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24))

int g_config_seh = 1;
extern int g_config_smp_mt, g_config_smp;

constexpr Parameter EffectList_Info::parameters[];

void EffectList_Info::recompile(Effect* component,
                                const Parameter* parameter,
                                const std::vector<int64_t>&) {
    auto effectlist = (E_EffectList*)component;
    if (std::string("Init") == parameter->name) {
        effectlist->code_init.need_recompile = true;
        effectlist->need_init = true;
    } else if (std::string("Frame") == parameter->name) {
        effectlist->code_frame.need_recompile = true;
    }
    effectlist->recompile_if_needed();
}

E_EffectList::E_EffectList()
    : list_framebuffer(nullptr), last_w(0), last_h(0), on_beat_frames_cooldown(0) {}

void E_EffectList::smp_cleanup_threads() {
    if (E_EffectList::smp.num_threads > 0) {
        if (E_EffectList::smp.quit_signal) {
            signal_set(E_EffectList::smp.quit_signal);
        }

        thread_join_all(
            E_EffectList::smp.threads, E_EffectList::smp.num_threads, WAIT_INFINITE);
        for (int x = 0; x < E_EffectList::smp.num_threads; x++) {
            thread_destroy(E_EffectList::smp.threads[x]);
            signal_destroy(E_EffectList::smp.thread_done[x]);
            signal_destroy(E_EffectList::smp.thread_start[x]);
        }
    }

    if (E_EffectList::smp.quit_signal) {
        signal_destroy(E_EffectList::smp.quit_signal);
    }

    memset(&E_EffectList::smp, 0, sizeof(E_EffectList::smp));
}

static int __inline depthof(int c, int i) {
    int r = max(max((c & 0xFF), ((c & 0xFF00) >> 8)), (c & 0xFF0000) >> 16);
    return i ? 255 - r : r;
}

int E_EffectList::render(char visdata[2][2][576],
                         int is_beat,
                         int* framebuffer,
                         int* fbout,
                         int w,
                         int h) {
    int is_preinit = (is_beat & 0x80000000);

    if (is_beat && this->config.on_beat) {
        this->on_beat_frames_cooldown = this->config.on_beat_frames;
    }

    bool enabled_this_frame = this->enabled || this->on_beat_frames_cooldown > 0;
    int64_t input_blend_this_frame = this->config.output_blend_adjustable;
    int64_t output_blend_this_frame = this->config.output_blend_adjustable;
    bool clear_this_frame = this->config.clear_every_frame;

    if (this->config.use_code) {
        this->recompile_if_needed();
        this->init_variables(w,
                             h,
                             is_beat,
                             enabled_this_frame,
                             clear_this_frame,
                             this->config.input_blend_adjustable,
                             this->config.output_blend_adjustable);
        if (this->need_init) {
            this->code_init.exec(visdata);
            this->need_init = false;
        }
        this->code_frame.exec(visdata);

        if (!is_preinit) {
            is_beat = *this->vars.beat > 0.1 || *this->vars.beat < -0.1;
        }
        input_blend_this_frame = (int)(*this->vars.alpha_in * 255.0);
        if (input_blend_this_frame < 0) {
            input_blend_this_frame = 0;
        } else if (input_blend_this_frame > 255) {
            input_blend_this_frame = 255;
        }
        output_blend_this_frame = (int)(*this->vars.alpha_out * 255.0);
        if (output_blend_this_frame < 0) {
            output_blend_this_frame = 0;
        } else if (output_blend_this_frame > 255) {
            output_blend_this_frame = 255;
        }

        enabled_this_frame = *this->vars.enabled > 0.1 || *this->vars.enabled < -0.1;
        clear_this_frame = *this->vars.clear > 0.1 || *this->vars.clear < -0.1;

        // code execute
    }

    // root/replaceinout special cases
    if (enabled_this_frame && this->config.input_blend_mode == LIST_BLEND_REPLACE
        && this->config.output_blend_mode == LIST_BLEND_REPLACE) {
        int s = 0;
        int line_blend_mode_save = g_line_blend_mode;
        if (this->list_framebuffer) {
            free(this->list_framebuffer);
        }
        this->list_framebuffer = nullptr;
        if (clear_this_frame && (this->config.input_blend_mode != 1)) {
            memset(framebuffer, 0, w * h * sizeof(int));
        }
        if (!is_preinit) {
            g_line_blend_mode = 0;
            // set_n_Context();
        }
        for (auto& child : this->children) {
            int t = 0;
            int smp_max_threads;

            if (g_config_smp && child->can_multithread() && g_config_smp_mt > 1) {
                smp_max_threads = g_config_smp;
                if (smp_max_threads > MAX_SMP_THREADS) {
                    smp_max_threads = MAX_SMP_THREADS;
                }

                int nt = smp_max_threads;
                nt = child->smp_begin(nt,
                                      visdata,
                                      is_beat,
                                      s ? fbout : framebuffer,
                                      s ? framebuffer : fbout,
                                      w,
                                      h);
                if (!is_preinit && nt > 0) {
                    if (nt > smp_max_threads) {
                        nt = smp_max_threads;
                    }

                    // launch threads
                    this->smp_render_list(nt,
                                          child,
                                          visdata,
                                          is_beat,
                                          s ? fbout : framebuffer,
                                          s ? framebuffer : fbout,
                                          w,
                                          h);

                    t = child->smp_finish(visdata,
                                          is_beat,
                                          s ? fbout : framebuffer,
                                          s ? framebuffer : fbout,
                                          w,
                                          h);
                }

            } else {
                if (g_config_seh && child->get_legacy_id() != LIST_ID) {
                    try {
                        t = child->render(visdata,
                                          is_beat,
                                          s ? fbout : framebuffer,
                                          s ? framebuffer : fbout,
                                          w,
                                          h);
                    } catch (...) {
                        t = 0;
                    }
                } else {
                    t = child->render(visdata,
                                      is_beat,
                                      s ? fbout : framebuffer,
                                      s ? framebuffer : fbout,
                                      w,
                                      h);
                }
            }

            if (t & 1) {
                s ^= 1;
            }
            if (!is_preinit) {
                if (t & 0x10000000) {
                    is_beat = 1;
                }
                if (t & 0x20000000) {
                    is_beat = 0;
                }
            }
        }
        if (!is_preinit) {
            g_line_blend_mode = line_blend_mode_save;
            // unset_n_Context();
        }
        this->on_beat_frames_cooldown--;
        return s;
    }

    if (!enabled_this_frame) {
        if (this->list_framebuffer) {
            free(this->list_framebuffer);
        }
        this->list_framebuffer = NULL;
        return 0;
    }

    this->on_beat_frames_cooldown--;

    // handle resize
    if (this->last_w != w || this->last_h != h || !this->list_framebuffer) {
        extern int config_reuseonresize;
        int do_resize = config_reuseonresize && !!this->list_framebuffer && this->last_w
                        && this->last_h && !clear_this_frame;

        int* newfb;
        if (!do_resize) {
            newfb = (int*)calloc(w * h, sizeof(int));
        } else {
            newfb = (int*)malloc(w * h * sizeof(int));
            if (newfb) {
                int dxpos = (this->last_w << 16) / w;
                int ypos = 0;
                int dypos = (this->last_h << 16) / h;
                int* out = newfb;
                for (int y = 0; y < h; y++) {
                    int* p = this->list_framebuffer + this->last_w * (ypos >> 16);
                    int xpos = 0;
                    for (int x = 0; x < w; x++) {
                        *out++ = p[xpos >> 16];
                        xpos += dxpos;
                    }
                    ypos += dypos;
                }
            }
        }
        // TODO [bug]: What happens here if newfb alloc failed?
        this->last_w = w;
        this->last_h = h;
        if (this->list_framebuffer) {
            free(this->list_framebuffer);
        }
        this->list_framebuffer = newfb;
    }
    // handle clear mode
    if (clear_this_frame) {
        memset(this->list_framebuffer, 0, w * h * sizeof(int));
    }

    // blend parent framebuffer into current, if necessary
    if (!is_preinit) {
        int x = w * h;
        int* tfb = framebuffer;
        int* o = this->list_framebuffer;
        // set_n_Context();
        int64_t use_blendin = this->config.input_blend_mode;
        if (use_blendin == LIST_BLEND_ADJUSTABLE && input_blend_this_frame >= 255) {
            use_blendin = LIST_BLEND_REPLACE;
        }

        switch (use_blendin) {
            case LIST_BLEND_REPLACE: memcpy(o, tfb, w * h * sizeof(int)); break;
            case LIST_BLEND_5050: mmx_avgblend_block(o, tfb, x); break;
            case LIST_BLEND_MAXIMUM:
                while (x--) {
                    *o = BLEND_MAX(*o, *tfb++);
                    o++;
                }
                break;
            case LIST_BLEND_ADDITIVE: mmx_addblend_block(o, tfb, x); break;
            case LIST_BLEND_SUB_1:
                while (x--) {
                    *o = BLEND_SUB(*o, *tfb++);
                    o++;
                }
                break;
            case LIST_BLEND_SUB_2:
                while (x--) {
                    *o = BLEND_SUB(*tfb++, *o);
                    o++;
                }
                break;
            case LIST_BLEND_EVERY_OTHER_LINE: {
                int y = h / 2;
                while (y-- > 0) {
                    memcpy(o, tfb, w * sizeof(int));
                    tfb += w * 2;
                    o += w * 2;
                }
            } break;
            case LIST_BLEND_EVERY_OTHER_PIXEL: {
                int r = 0;
                int y = h;
                while (y-- > 0) {
                    int *out, *in;
                    int x = w / 2;
                    out = o + r;
                    in = tfb + r;
                    r ^= 1;
                    while (x-- > 0) {
                        *out = *in;
                        out += 2;
                        in += 2;
                    }
                    o += w;
                    tfb += w;
                }
            } break;
            case LIST_BLEND_XOR:
                while (x--) {
                    *o = *o ^ *tfb++;
                    o++;
                }
                break;
            case LIST_BLEND_ADJUSTABLE:
                mmx_adjblend_block(o, tfb, o, x, input_blend_this_frame);
                break;
            case LIST_BLEND_MULTIPLY: mmx_mulblend_block(o, tfb, x); break;
            case LIST_BLEND_MINIMUM:
                while (x--) {
                    *o = BLEND_MIN(*o, *tfb++);
                    o++;
                }
                break;
            default: break;
            case LIST_BLEND_BUFFER: {
                int* buf =
                    (int*)getGlobalBuffer(w, h, this->config.input_blend_buffer, 0);
                if (!buf) {
                    break;
                }
                while (x--) {
                    *o = BLEND_ADJ(
                        *tfb++,
                        *o,
                        depthof(*buf, this->config.input_blend_buffer_invert));
                    o++;
                    buf++;
                }
#ifndef NO_MMX
#ifdef _MSC_VER  // MSVC asm
                __asm emms;
#else   // _MSC_VER, GCC asm
                __asm__ __volatile__("emms");
#endif  // _MSC_VER
#endif
                break;
            }
        }
        // unset_n_Context();
    }

    int s = 0;
    int x;
    int line_blend_mode_save = g_line_blend_mode;
    if (!is_preinit) {
        g_line_blend_mode = 0;
    }

    for (auto& child : this->children) {
        int t = 0;

        int smp_max_threads;

        if (g_config_smp && child->can_multithread() && g_config_smp_mt > 1) {
            smp_max_threads = g_config_smp_mt;
            if (smp_max_threads > MAX_SMP_THREADS) {
                smp_max_threads = MAX_SMP_THREADS;
            }

            int nt = smp_max_threads;
            nt = child->smp_begin(nt,
                                  visdata,
                                  is_beat,
                                  s ? fbout : framebuffer,
                                  s ? framebuffer : fbout,
                                  w,
                                  h);
            if (!is_preinit && nt > 0) {
                if (nt > smp_max_threads) {
                    nt = smp_max_threads;
                }

                // launch threads
                this->smp_render_list(nt,
                                      child,
                                      visdata,
                                      is_beat,
                                      s ? fbout : this->list_framebuffer,
                                      s ? this->list_framebuffer : fbout,
                                      w,
                                      h);

                t = child->smp_finish(visdata,
                                      is_beat,
                                      s ? fbout : framebuffer,
                                      s ? framebuffer : fbout,
                                      w,
                                      h);
            }

        } else if (g_config_seh && child->get_legacy_id() != LIST_ID) {
            try {
                t = child->render(visdata,
                                  is_beat,
                                  s ? fbout : this->list_framebuffer,
                                  s ? this->list_framebuffer : fbout,
                                  w,
                                  h);
            } catch (...) {
                t = 0;
            }
        } else {
            t = child->render(visdata,
                              is_beat,
                              s ? fbout : this->list_framebuffer,
                              s ? this->list_framebuffer : fbout,
                              w,
                              h);
        }

        if (t & 1) {
            s ^= 1;
        }
        if (!is_preinit) {
            if (t & 0x10000000) {
                is_beat = 1;
            }
            if (t & 0x20000000) {
                is_beat = 0;
            }
        }
    }
    if (!is_preinit) {
        g_line_blend_mode = line_blend_mode_save;
    }

    // if s==1 at this point, data we want is in fbout.

    if (!is_preinit) {
        if (s) {
            memcpy(this->list_framebuffer, fbout, w * h * sizeof(int));
        }

        int* tfb = s ? fbout : this->list_framebuffer;
        int* o = framebuffer;
        x = w * h;
        // set_n_Context();

        int64_t use_blendout = this->config.output_blend_mode;
        if (use_blendout == LIST_BLEND_ADJUSTABLE && output_blend_this_frame >= 255) {
            use_blendout = 1;
        }
        switch (use_blendout) {
            case LIST_BLEND_REPLACE:
                if (s) {
                    // unset_n_Context();
                    return 1;
                }
                memcpy(o, tfb, x * sizeof(int));
                break;
            case LIST_BLEND_5050: mmx_avgblend_block(o, tfb, x); break;
            case LIST_BLEND_MAXIMUM:
                while (x--) {
                    *o = BLEND_MAX(*o, *tfb++);
                    o++;
                }
                break;
            case LIST_BLEND_ADDITIVE: mmx_addblend_block(o, tfb, x); break;
            case LIST_BLEND_SUB_1:
                while (x--) {
                    *o = BLEND_SUB(*o, *tfb++);
                    o++;
                }
                break;
            case LIST_BLEND_SUB_2:
                while (x--) {
                    *o = BLEND_SUB(*tfb++, *o);
                    o++;
                }
                break;
            case LIST_BLEND_EVERY_OTHER_LINE: {
                int y = h / 2;
                while (y-- > 0) {
                    memcpy(o, tfb, w * sizeof(int));
                    tfb += w * 2;
                    o += w * 2;
                }
            } break;
            case LIST_BLEND_EVERY_OTHER_PIXEL: {
                int r = 0;
                int y = h;
                while (y-- > 0) {
                    int *out, *in;
                    int x = w / 2;
                    out = o + r;
                    in = tfb + r;
                    r ^= 1;
                    while (x-- > 0) {
                        *out = *in;
                        out += 2;
                        in += 2;
                    }
                    o += w;
                    tfb += w;
                }
            } break;
            case LIST_BLEND_XOR:
                while (x--) {
                    *o = *o ^ *tfb++;
                    o++;
                }
                break;
            case LIST_BLEND_ADJUSTABLE:
                mmx_adjblend_block(o, tfb, o, x, output_blend_this_frame);
                break;
            case LIST_BLEND_MULTIPLY: mmx_mulblend_block(o, tfb, x); break;
            case LIST_BLEND_MINIMUM:
                while (x--) {
                    *o = BLEND_MIN(*o, *tfb++);
                    o++;
                }
                break;
            case LIST_BLEND_BUFFER: {
                int* buf =
                    (int*)getGlobalBuffer(w, h, this->config.output_blend_buffer, 0);
                if (!buf) {
                    break;
                }
                while (x--) {
                    *o = BLEND_ADJ(
                        *tfb++,
                        *o,
                        depthof(*buf, this->config.output_blend_buffer_invert));
                    o++;
                    buf++;
                }
#ifndef NO_MMX
#ifdef _MSC_VER  // MSVC asm
                __asm emms;
#else   // _MSC_VER, GCC asm
                __asm__ __volatile__("emms");
#endif  // _MSC_VER
#endif
                break;
            }
            default: break;
        }
        // unset_n_Context();
    }
    return 0;
}

void EffectList_Vars::register_(void* vm_context) {
    this->enabled = NSEEL_VM_regvar(vm_context, "enabled");
    this->clear = NSEEL_VM_regvar(vm_context, "clear");
    this->beat = NSEEL_VM_regvar(vm_context, "beat");
    this->alpha_in = NSEEL_VM_regvar(vm_context, "alphain");
    this->alpha_out = NSEEL_VM_regvar(vm_context, "alphaout");
    this->w = NSEEL_VM_regvar(vm_context, "w");
    this->h = NSEEL_VM_regvar(vm_context, "h");
}

// extra_args must contain in this order:
//  int enabled
//  int clear
//  int alpha_in
//  int alpha_out
void EffectList_Vars::init(int w, int h, int is_beat, va_list extra_args) {
    *this->beat = is_beat ? 1.0 : 0.0;
    *this->w = w;
    *this->h = h;
    *this->enabled = va_arg(extra_args, int) ? 1.0 : 0.0;
    *this->clear = va_arg(extra_args, int) ? 1.0 : 0.0;
    *this->alpha_in = va_arg(extra_args, int) / 255.0;
    *this->alpha_out = va_arg(extra_args, int) / 255.0;
}

Effect* E_EffectList::get_child(int index) {
    if (index >= 0 && index < (int)this->children.size()) {
        return this->children[index];
    }
    return NULL;
}

int64_t E_EffectList::find(Effect* effect) {
    auto result = std::find(this->children.begin(), this->children.end(), effect);
    if (result != this->children.end()) {
        return result - this->children.begin();
    }
    return -1;
}

bool E_EffectList::remove_render_from(Effect* r, int del) {
    if (!r) {
        return false;
    }
    int index = 0;
    for (; index < (int)this->children.size() && this->children[index] != r; index++) {
        ;
    }
    return remove_render(index, del);
}

bool E_EffectList::remove_render(int index, int del) {
    for (auto it = this->children.begin(); it != this->children.end(); ++it) {
        if (del && this->children[index] == *it) {
            this->children.erase(it);
            return true;
        }
    }
    return false;
}

void E_EffectList::clear_renders() {
    this->children.clear();
    if (this->list_framebuffer) {
        free(this->list_framebuffer);
    }
    this->list_framebuffer = 0;
}

// index=-1 for add
int64_t E_EffectList::insert_render(Effect* effect, int index) {
    if (!effect || index < -1) {
        return INT64_MAX;
    }
    if (index >= 0 && (size_t)index < this->children.size()) {
        this->insert(effect, this->children[index], INSERT_BEFORE);
        return index;
    }
    this->insert(effect, this, INSERT_CHILD);
    return this->children.size() - 1;
}

void E_EffectList::smp_render_list(int min_threads,
                                   Effect* component,
                                   char visdata[2][2][576],
                                   int is_beat,
                                   int* framebuffer,
                                   int* fbout,
                                   int w,
                                   int h) {
    int x;
    E_EffectList::smp.nthreads = min_threads;
    if (!E_EffectList::smp.quit_signal) {
        E_EffectList::smp.quit_signal = signal_create_broadcast();
    }

    E_EffectList::smp.vis_data = visdata;
    E_EffectList::smp.is_beat = is_beat;
    E_EffectList::smp.framebuffer = framebuffer;
    E_EffectList::smp.fbout = fbout;
    E_EffectList::smp.w = w;
    E_EffectList::smp.h = h;
    E_EffectList::smp.render = component;
    for (x = 0; x < min_threads; x++) {
        if (x >= E_EffectList::smp.num_threads) {
            // unsigned long int id;
            E_EffectList::smp.thread_start[x] = signal_create_single();
            signal_set(E_EffectList::smp.thread_start[x]);
            E_EffectList::smp.thread_done[x] = signal_create_single();

            E_EffectList::smp.threads[x] =
                thread_create(E_EffectList::smp_thread_proc, (void*)(x));
            E_EffectList::smp.num_threads = x + 1;
        } else {
            signal_set(E_EffectList::smp.thread_start[x]);
        }
    }
    signal_wait_all(
        E_EffectList::smp.thread_done, E_EffectList::smp.nthreads, WAIT_INFINITE);
}

uint32_t E_EffectList::smp_thread_proc(void* parm) {
    int which = (int)parm;
    void* hdls[2] = {E_EffectList::smp.thread_start[which],
                     E_EffectList::smp.quit_signal};
    for (;;) {
        if (signal_wait_any(hdls, 2, WAIT_INFINITE) == E_EffectList::smp.quit_signal) {
            return 0;
        }

        E_EffectList::smp.render->smp_render(
            which,
            E_EffectList::smp.nthreads,
            *(char(*)[2][2][576])E_EffectList::smp.vis_data,
            E_EffectList::smp.is_beat,
            E_EffectList::smp.framebuffer,
            E_EffectList::smp.fbout,
            E_EffectList::smp.w,
            E_EffectList::smp.h);
        signal_set(E_EffectList::smp.thread_done[which]);
    }
}

E_EffectList::smp_param_t E_EffectList::smp;

// output_blend_modes:
// (note how the blendmodes are pairwise flipped with the input list)
//     Replace
//     Ignore
//     Maximum
//     Fifty Fifty
//     Sub Dest Src
//     Additive
//     Every Other Line
//     Sub Src Dest
//     Xor
//     Every Other Pixel
//     Multiply
//     Adjustable
//     Buffer

void E_EffectList::load_legacy(unsigned char* data, int len) {
    int pos = 0;
    // the "first mode byte" is present twice in succession, except for the MSB which is
    // 1 in the first copy for virtually all but the most ancient versions of AVS.
    uint8_t first_mode_byte = 0;
    if (pos < len) {
        first_mode_byte = data[pos++];
        this->config.clear_every_frame = first_mode_byte & 0b01;
        this->enabled = (first_mode_byte & 0b10) >> 1;
    }
    uint8_t mode[4] = {0, 0, 0, 0};
    if (first_mode_byte & 0x80) {  // true for 99.99% of known legacy preset files.
        first_mode_byte &= ~0x80;
        mode[0] = first_mode_byte | data[pos++];
        mode[1] = data[pos++];
        mode[2] = data[pos++];
        mode[3] = data[pos++];
    }
    this->config.input_blend_mode = mode[1] & 0b111111;
    // yes, the output blendmode flips the 1-bit for some reason.
    // (probably so that 0 maps to Replace instead of Ignore, which is a saner default
    // when loading ancient EffectLists which interprets the field as 0. the consequence
    // is that all following blendmode pairs in the list are also switched.)
    this->config.output_blend_mode = (mode[2] & 0b111111) ^ 1;
    int ext = (int)mode[3] + 5;
    if (ext > 5) {
        if (pos < ext) {
            this->config.input_blend_adjustable = GET_INT();
            pos += 4;
        }
        if (pos < ext) {
            this->config.output_blend_adjustable = GET_INT();
            pos += 4;
        }
        if (pos < ext) {
            this->config.input_blend_buffer = GET_INT();
            pos += 4;
        }
        if (pos < ext) {
            this->config.output_blend_buffer = GET_INT();
            pos += 4;
        }
        if (pos < ext) {
            this->config.input_blend_buffer_invert = GET_INT();
            pos += 4;
        }
        if (pos < ext) {
            this->config.output_blend_buffer_invert = GET_INT();
            pos += 4;
        }
        if (pos < ext - 4) {
            this->config.on_beat = GET_INT();
            pos += 4;
        }
        if (pos < ext - 4) {
            this->config.on_beat_frames = GET_INT();
            pos += 4;
        }
    }
    while (pos < len) {
        int legacy_effect_id = GET_INT();
        char legacy_effect_ape_id[LEGACY_APE_ID_LENGTH + 1];
        legacy_effect_ape_id[0] = '\0';
        pos += 4;
        if (legacy_effect_id >= DLLRENDERBASE) {
            if (pos + 32 > len) {
                break;
            }
            memcpy(legacy_effect_ape_id, data + pos, LEGACY_APE_ID_LENGTH);
            legacy_effect_ape_id[LEGACY_APE_ID_LENGTH] = '\0';
            pos += LEGACY_APE_ID_LENGTH;
        }
        if (pos + 4 > len) {
            break;
        }
        int l_len = GET_INT();
        pos += 4;
        if (pos + l_len > len || l_len < 0) {
            break;
        }

        // special case for v2.81+ codeable EffectList.
        // saved as an effect, but loaded into this EffectList's config.
        if (ext > 5 && legacy_effect_id >= DLLRENDERBASE
            && !strncmp(legacy_effect_ape_id,
                        EffectList_Info::legacy_v28_ape_id,
                        LEGACY_APE_ID_LENGTH)) {
            int effect_end = pos + l_len;
            // pos += 4;
            if (len - pos >= 4) {
                this->config.use_code = *(uint32_t*)(&data[pos]);
                pos += 4;
            }
            auto* str = (char*)data;
            pos += Effect::string_load_legacy(
                &str[pos], this->config.init, effect_end - pos);
            pos += Effect::string_load_legacy(
                &str[pos], this->config.frame, effect_end - pos);
        } else {
            auto effect =
                component_factory_legacy(legacy_effect_id, legacy_effect_ape_id);
            effect->load_legacy(data + pos, l_len);
            pos += l_len;
            this->insert(effect, this, INSERT_CHILD);
        }
    }
}

uint32_t E_EffectList::legacy_save_code_section_size() {
    // If the string is completely empty, save only the length.
    // Else save length+1 for the terminating null byte.
    uint32_t init_length = this->config.init.empty() ? 0 : this->config.init.size() + 1;
    uint32_t frame_length =
        this->config.frame.empty() ? 0 : this->config.frame.size() + 1;
    return sizeof(uint32_t)    // code-enabled
           + sizeof(uint32_t)  // init length
           + init_length       // init code
           + sizeof(uint32_t)  // frame length
           + frame_length;     // frame code
}

int E_EffectList::save_legacy(unsigned char* data) {
    uint32_t pos = 0;
    uint8_t first_mode_byte =
        (this->config.clear_every_frame & 0x01) | ((!this->enabled) & 0x02);
    data[pos++] = first_mode_byte | 0x80;
    data[pos++] = first_mode_byte;
    data[pos++] = this->config.input_blend_mode & 0xff;
    data[pos++] = (this->config.output_blend_mode & 0xff) ^ 1;
    data[pos++] = EFFECTLIST_AVS281D_EXT_SIZE;
    // extended_data
    PUT_INT(this->config.input_blend_adjustable);
    pos += 4;
    PUT_INT(this->config.output_blend_adjustable);
    pos += 4;
    PUT_INT(this->config.input_blend_buffer);
    pos += 4;
    PUT_INT(this->config.output_blend_buffer);
    pos += 4;
    PUT_INT(this->config.input_blend_buffer_invert);
    pos += 4;
    PUT_INT(this->config.output_blend_buffer_invert);
    pos += 4;
    PUT_INT(this->config.on_beat);
    pos += 4;
    PUT_INT(this->config.on_beat_frames);
    pos += 4;
    // end extended data

    // write APE extension
    PUT_INT(DLLRENDERBASE);
    pos += 4;
    char* str = (char*)data;
    memset(&str[pos], 0, LEGACY_APE_ID_LENGTH);
    strncpy(&str[pos], EffectList_Info::legacy_v28_ape_id, LEGACY_APE_ID_LENGTH);
    pos += LEGACY_APE_ID_LENGTH;
    uint32_t code_section_size = this->legacy_save_code_section_size();
    PUT_INT(code_section_size);
    pos += 4;
    PUT_INT(this->config.use_code);
    pos += 4;
    pos += Effect::string_save_legacy(
        this->config.init, &str[pos], MAX_CODE_LEN - 1 - pos, /*with_nt*/ true);
    pos += Effect::string_save_legacy(
        this->config.frame, &str[pos], MAX_CODE_LEN - 1 - pos, /*with_nt*/ true);

    for (auto& effect : this->children) {
        int32_t legacy_effect_id = effect->get_legacy_id();
        if (legacy_effect_id == Unknown_Info::legacy_id) {
            auto unknown = (E_Unknown*)effect;
            if (!unknown->legacy_ape_id[0]) {
                PUT_INT(unknown->legacy_id);
                pos += 4;
            } else {
                PUT_INT(DLLRENDERBASE);
                pos += 4;
                memcpy(data + pos, unknown->legacy_ape_id, LEGACY_APE_ID_LENGTH);
                pos += LEGACY_APE_ID_LENGTH;
            }
        } else {
            if (legacy_effect_id == -1) {
                PUT_INT(DLLRENDERBASE);
                pos += 4;
                char ape_id_buf[LEGACY_APE_ID_LENGTH];
                memset(ape_id_buf, 0, LEGACY_APE_ID_LENGTH);
                strncpy(ape_id_buf, effect->get_legacy_ape_id(), LEGACY_APE_ID_LENGTH);
                memcpy(data + pos, ape_id_buf, LEGACY_APE_ID_LENGTH);
                pos += LEGACY_APE_ID_LENGTH;
            } else {
                PUT_INT(legacy_effect_id);
                pos += 4;
            }
        }

        int t = effect->save_legacy(data + pos + 4);
        PUT_INT(t);
        pos += 4 + t;
    }

    return pos;
}

Effect_Info* create_EffectList_Info() { return new EffectList_Info(); }
Effect* create_EffectList() { return new E_EffectList(); }
void set_EffectList_desc(char* desc) { E_EffectList::set_desc(desc); }
