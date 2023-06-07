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

char* C_RenderListClass::get_desc() {
    if (isroot) {
        return "Main";
    }
    return "Effect list";
}

int g_config_seh = 1;
extern int g_config_smp_mt, g_config_smp;

static char extsigstr[] = "AVS 2.8+ Effect List Config";

void C_RenderListClass::load_config_code(unsigned char* data, int len) {
    int pos = 0;
    if (len - pos >= 4) {
        use_code = GET_INT();
        pos += 4;
        load_string(effect_exp[0], data, pos, len);
        load_string(effect_exp[1], data, pos, len);
    }
}

void C_RenderListClass::load_config(unsigned char* data, int len) {
    int pos = 0, ext;
    if (pos < len) {
        mode = data[pos++];
    }
    if (mode & 0x80) {
        mode &= ~0x80;
        mode = mode | GET_INT();
        pos += 4;
    }
    ext = get_extended_datasize() + 5;
    if (ext > 5) {
        if (pos < ext) {
            inblendval = GET_INT();
            pos += 4;
        }
        if (pos < ext) {
            outblendval = GET_INT();
            pos += 4;
        }
        if (pos < ext) {
            bufferin = GET_INT();
            pos += 4;
        }
        if (pos < ext) {
            bufferout = GET_INT();
            pos += 4;
        }
        if (pos < ext) {
            ininvert = GET_INT();
            pos += 4;
        }
        if (pos < ext) {
            outinvert = GET_INT();
            pos += 4;
        }
        if (pos < ext - 4) {
            beat_render = GET_INT();
            pos += 4;
        }  // BU
        if (pos < ext - 4) {
            beat_render_frames = GET_INT();
            pos += 4;
        }
    }
    use_code = 0;
    effect_exp[0].assign("");
    effect_exp[1].assign("");
    while (pos < len) {
        char s[33];
        T_RenderListType t;
        int l_len;
        int effect_index = GET_INT();
        pos += 4;
        if (effect_index >= DLLRENDERBASE) {
            if (pos + 32 > len) {
                break;
            }
            memcpy(s, data + pos, 32);
            s[32] = 0;
            effect_index = (int)s;
            pos += 32;
        }
        if (pos + 4 > len) {
            break;
        }
        l_len = GET_INT();
        pos += 4;
        if (pos + l_len > len || l_len < 0) {
            break;
        }

        // special case for new 2.81+ codable effect list. saved as an effect, but
        // loaded into this very EL right here.
        if (ext > 5 && effect_index >= DLLRENDERBASE
            && !memcmp(s, extsigstr, strlen(extsigstr) + 1)) {
            load_config_code(data + pos, l_len);
        } else {
            t.effect = g_render_library->CreateRenderer(&effect_index);
            t.effect_index = effect_index;
            if (t.effect.valid()) {
                t.effect.load_config(data + pos, l_len);
                insertRender(&t, -1);
            }
        }
        pos += l_len;
    }
}

int C_RenderListClass::save_config_code(unsigned char* data) {
    int pos = 0;
    PUT_INT(use_code);
    pos += 4;
    save_string(data, pos, effect_exp[0]);
    save_string(data, pos, effect_exp[1]);
    return pos;
}

int C_RenderListClass::save_config(unsigned char* data) {
    return save_config_ex(data, 0);
}
int C_RenderListClass::save_config_ex(unsigned char* data, int rootsave) {
    int pos = 0;
    int x;
    if (!rootsave) {
        set_extended_datasize(36);  // size of extended data + 4 cause we fucked up
        data[pos++] = (mode & 0xff) | 0x80;
        PUT_INT(mode);
        pos += 4;
        // extended_data
        PUT_INT(inblendval);
        pos += 4;
        PUT_INT(outblendval);
        pos += 4;
        PUT_INT(bufferin);
        pos += 4;
        PUT_INT(bufferout);
        pos += 4;
        PUT_INT(ininvert);
        pos += 4;
        PUT_INT(outinvert);
        pos += 4;
        PUT_INT(beat_render);
        pos += 4;
        PUT_INT(beat_render_frames);
        pos += 4;
        // end extended data
    } else {
        data[pos++] = mode;
    }

    if (!rootsave) {
        // write in our ext field
        PUT_INT(DLLRENDERBASE);
        pos += 4;
        char s[33];
        strncpy(s, extsigstr, 32);
        memcpy(data + pos, s, 32);
        pos += 32;
        int t = save_config_code(data + pos + 4);
        PUT_INT(t);
        pos += 4 + t;
    }

    for (x = 0; x < num_renders; x++) {
        int t;
        int idx = renders[x].effect_index;
        if (idx == E_Unknown::info.legacy_id) {
            auto r = (E_Unknown*)renders[x].effect.effect;
            if (!r->legacy_ape_id[0]) {
                PUT_INT(r->legacy_id);
                pos += 4;
            } else {
                PUT_INT(r->legacy_id);
                pos += 4;
                memcpy(data + pos, r->legacy_ape_id, 32);
                pos += 32;
            }
        } else {
            PUT_INT(idx);
            pos += 4;
            if (idx >= DLLRENDERBASE) {
                char s[33];
                strncpy(s, (char*)idx, 32);
                memcpy(data + pos, s, 32);
                pos += 32;
            }
        }

        t = renders[x].effect.save_config(data + pos + 4);
        PUT_INT(t);
        pos += 4 + t;
    }

    return pos;
}

C_RenderListClass::C_RenderListClass(int iroot) {
    AVS_EEL_INITINST();
    isstart = 0;
    nsaved = 0;
    memset(nbw_save, 0, sizeof(nbw_save));
    memset(nbw_save2, 0, sizeof(nbw_save2));
    memset(nbh_save, 0, sizeof(nbh_save));
    memset(nbh_save2, 0, sizeof(nbh_save2));
    memset(nb_save, 0, sizeof(nb_save));
    memset(nb_save2, 0, sizeof(nb_save2));
    inblendval = 128;
    outblendval = 128;
    ininvert = 0;

    this->code_lock = lock_init();
    use_code = 0;
    inited = 0;
    need_recompile = 1;
    memset(codehandle, 0, sizeof(codehandle));
    var_beat = 0;

    effect_exp[0].assign("");
    effect_exp[1].assign("");
    outinvert = 0;
    bufferin = 0;
    bufferout = 0;
    isroot = iroot;
    num_renders = 0;
    num_renders_alloc = 0;
    renders = NULL;
    thisfb = NULL;
    l_w = l_h = 0;
    mode = 0;
    beat_render = 0;
    beat_render_frames = 1;
    fake_enabled = 0;
}

extern int g_n_buffers_w[NBUF], g_n_buffers_h[NBUF];
extern void* g_n_buffers[NBUF];

void C_RenderListClass::set_n_Context() {
    if (!isroot) {
        return;
    }
    if (nsaved) {
        return;
    }
    nsaved = 1;
    memcpy(nbw_save2, g_n_buffers_w, sizeof(nbw_save2));
    memcpy(nbh_save2, g_n_buffers_h, sizeof(nbh_save2));
    memcpy(nb_save2, g_n_buffers, sizeof(nb_save2));

    memcpy(g_n_buffers_w, nbw_save, sizeof(nbw_save));
    memcpy(g_n_buffers_h, nbh_save, sizeof(nbh_save));
    memcpy(g_n_buffers, nb_save, sizeof(nb_save));
}

void C_RenderListClass::unset_n_Context() {
    if (!isroot) {
        return;
    }
    if (!nsaved) {
        return;
    }
    nsaved = 0;

    memcpy(nbw_save, g_n_buffers_w, sizeof(nbw_save));
    memcpy(nbh_save, g_n_buffers_h, sizeof(nbh_save));
    memcpy(nb_save, g_n_buffers, sizeof(nb_save));

    memcpy(g_n_buffers_w, nbw_save2, sizeof(nbw_save2));
    memcpy(g_n_buffers_h, nbh_save2, sizeof(nbh_save2));
    memcpy(g_n_buffers, nb_save2, sizeof(nb_save2));
}

void C_RenderListClass::smp_cleanupthreads() {
    if (smp_parms.threadTop > 0) {
        if (smp_parms.hQuitHandle) {
            signal_set(smp_parms.hQuitHandle);
        }

        thread_join_all(smp_parms.hThreads, smp_parms.threadTop, WAIT_INFINITE);
        int x;
        for (x = 0; x < smp_parms.threadTop; x++) {
            thread_destroy(smp_parms.hThreads[x]);
            signal_destroy(smp_parms.hThreadSignalsDone[x]);
            signal_destroy(smp_parms.hThreadSignalsStart[x]);
        }
    }

    if (smp_parms.hQuitHandle) {
        signal_destroy(smp_parms.hQuitHandle);
    }

    memset(&smp_parms, 0, sizeof(smp_parms));
}

void C_RenderListClass::freeBuffers() {
    if (isroot) {
        int x;
        for (x = 0; x < NBUF; x++) {
            if (nb_save[x]) {
                free(nb_save[x]);
            }
            nb_save[x] = NULL;
            nbw_save[x] = nbh_save[x] = 0;
        }
    }
}

C_RenderListClass::~C_RenderListClass() {
    clearRenders();

    // free nb_save
    freeBuffers();

    int x;
    for (x = 0; x < 2; x++) {
        freeCode(codehandle[x]);
        codehandle[x] = 0;
    }
    AVS_EEL_QUITINST();
    lock_destroy(this->code_lock);
}

static int __inline depthof(int c, int i) {
    int r = max(max((c & 0xFF), ((c & 0xFF00) >> 8)), (c & 0xFF0000) >> 16);
    return i ? 255 - r : r;
}

int C_RenderListClass::render(char visdata[2][2][576],
                              int is_beat,
                              int* framebuffer,
                              int* fbout,
                              int w,
                              int h) {
    int is_preinit = (is_beat & 0x80000000);

    if (is_beat && beat_render) {
        fake_enabled = beat_render_frames;
    }

    int use_enabled = enabled();
    int use_inblendval = inblendval;
    int use_outblendval = outblendval;
    int use_clear = clearfb();

    if (!isroot && use_code) {
        if (need_recompile) {
            lock_lock(this->code_lock);

            if (!var_beat || g_reset_vars_on_recompile) {
                clearVars();
                var_beat = registerVar("beat");
                var_alphain = registerVar("alphain");
                var_alphaout = registerVar("alphaout");
                var_enabled = registerVar("enabled");
                var_clear = registerVar("clear");
                var_w = registerVar("w");
                var_h = registerVar("h");
                inited = 0;
            }

            need_recompile = 0;
            int x;
            for (x = 0; x < 2; x++) {
                freeCode(codehandle[x]);
                codehandle[x] = compileCode((char*)effect_exp[x].c_str());
            }

            lock_unlock(this->code_lock);
        }

        *var_beat = ((is_beat & 1) && !is_preinit) ? 1.0 : 0.0;
        *var_enabled = use_enabled ? 1.0 : 0.0;
        *var_w = (double)w;
        *var_h = (double)h;
        *var_clear = use_clear ? 1.0 : 0.0;
        *var_alphain = use_inblendval / 255.0;
        *var_alphaout = use_outblendval / 255.0;
        if (codehandle[0] && !inited) {
            executeCode(codehandle[0], visdata);
            inited = 1;
        }
        executeCode(codehandle[1], visdata);

        if (!is_preinit) {
            is_beat = *var_beat > 0.1 || *var_beat < -0.1;
        }
        use_inblendval = (int)(*var_alphain * 255.0);
        if (use_inblendval < 0) {
            use_inblendval = 0;
        } else if (use_inblendval > 255) {
            use_inblendval = 255;
        }
        use_outblendval = (int)(*var_alphaout * 255.0);
        if (use_outblendval < 0) {
            use_outblendval = 0;
        } else if (use_outblendval > 255) {
            use_outblendval = 255;
        }

        use_enabled = *var_enabled > 0.1 || *var_enabled < -0.1;
        use_clear = *var_clear > 0.1 || *var_clear < -0.1;

        // code execute
    }

    // root/replaceinout special cases
    if (isroot || (use_enabled && blendin() == 1 && blendout() == 1)) {
        int s = 0, x;
        int line_blend_mode_save = g_line_blend_mode;
        if (thisfb) {
            free(thisfb);
        }
        thisfb = NULL;
        if (use_clear && (isroot || blendin() != 1)) {
            memset(framebuffer, 0, w * h * sizeof(int));
        }
        if (!is_preinit) {
            g_line_blend_mode = 0;
            set_n_Context();
        }
        for (x = 0; x < num_renders; x++) {
            int t = 0;
            int smp_max_threads;
            Legacy_Effect_Proxy render = renders[x].effect;

            if (g_config_smp && render.can_multithread() && g_config_smp_mt > 1) {
                smp_max_threads = g_config_smp;
                render = renders[x].effect;
                if (smp_max_threads > MAX_SMP_THREADS) {
                    smp_max_threads = MAX_SMP_THREADS;
                }

                int nt = smp_max_threads;
                nt = render.smp_begin(nt,
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
                    smp_Render(nt,
                               &render,
                               visdata,
                               is_beat,
                               s ? fbout : framebuffer,
                               s ? framebuffer : fbout,
                               w,
                               h);

                    t = render.smp_finish(visdata,
                                          is_beat,
                                          s ? fbout : framebuffer,
                                          s ? framebuffer : fbout,
                                          w,
                                          h);
                }

            } else {
                if (g_config_seh && renders[x].effect_index != LIST_ID) {
                    try {
                        t = render.render(visdata,
                                          is_beat,
                                          s ? fbout : framebuffer,
                                          s ? framebuffer : fbout,
                                          w,
                                          h);
                    } catch (...) {
                        t = 0;
                    }
                } else {
                    t = render.render(visdata,
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
            unset_n_Context();
        }
        fake_enabled--;
        return s;
    }

    // check to see if we're enabled
    if (!use_enabled) {
        if (thisfb) {
            free(thisfb);
        }
        thisfb = NULL;
        return 0;
    }

    fake_enabled--;

    // handle resize
    if (l_w != w || l_h != h || !thisfb) {
        extern int config_reuseonresize;
        int do_resize = config_reuseonresize && !!thisfb && l_w && l_h && !use_clear;

        int* newfb;
        if (!do_resize) {
            newfb = (int*)calloc(w * h, sizeof(int));
        } else {
            newfb = (int*)malloc(w * h * sizeof(int));
            if (newfb) {
                int x, y;
                int dxpos = (l_w << 16) / w;
                int ypos = 0;
                int dypos = (l_h << 16) / h;
                int* out = newfb;
                for (y = 0; y < h; y++) {
                    int* p = thisfb + l_w * (ypos >> 16);
                    int xpos = 0;
                    for (x = 0; x < w; x++) {
                        *out++ = p[xpos >> 16];
                        xpos += dxpos;
                    }
                    ypos += dypos;
                }
            }
        }
        // TODO [bug]: What happens here if newfb alloc failed?
        l_w = w;
        l_h = h;
        if (thisfb) {
            free(thisfb);
        }
        thisfb = newfb;
    }
    // handle clear mode
    if (use_clear) {
        memset(thisfb, 0, w * h * sizeof(int));
    }

    // blend parent framebuffer into current, if necessary

    if (!is_preinit) {
        int x = w * h;
        int* tfb = framebuffer;
        int* o = thisfb;
        set_n_Context();
        int use_blendin = blendin();
        if (use_blendin == 10 && use_inblendval >= 255) {
            use_blendin = 1;
        }

        switch (use_blendin) {
            case 1: memcpy(o, tfb, w * h * sizeof(int)); break;
            case 2: mmx_avgblend_block(o, tfb, x); break;
            case 3:
                while (x--) {
                    *o = BLEND_MAX(*o, *tfb++);
                    o++;
                }
                break;
            case 4: mmx_addblend_block(o, tfb, x); break;
            case 5:
                while (x--) {
                    *o = BLEND_SUB(*o, *tfb++);
                    o++;
                }
                break;
            case 6:
                while (x--) {
                    *o = BLEND_SUB(*tfb++, *o);
                    o++;
                }
                break;
            case 7: {
                int y = h / 2;
                while (y-- > 0) {
                    memcpy(o, tfb, w * sizeof(int));
                    tfb += w * 2;
                    o += w * 2;
                }
            } break;
            case 8: {
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
            case 9:
                while (x--) {
                    *o = *o ^ *tfb++;
                    o++;
                }
                break;
            case 10: mmx_adjblend_block(o, tfb, o, x, use_inblendval); break;
            case 11: mmx_mulblend_block(o, tfb, x); break;
            case 13:
                while (x--) {
                    *o = BLEND_MIN(*o, *tfb++);
                    o++;
                }
                break;
            case 12: {
                int* buf = (int*)getGlobalBuffer(w, h, bufferin, 0);
                if (!buf) {
                    break;
                }
                while (x--) {
                    *o = BLEND_ADJ(*tfb++, *o, depthof(*buf, ininvert));
                    o++;
                    buf++;
                }
            }
#ifndef NO_MMX
#ifdef _MSC_VER  // MSVC asm
                __asm emms;
#else   // _MSC_VER, GCC asm
                __asm__ __volatile__("emms");
#endif  // _MSC_VER
#endif
                break;
            default: break;
        }
        unset_n_Context();
    }

    int s = 0;
    int x;
    int line_blend_mode_save = g_line_blend_mode;
    if (!is_preinit) {
        g_line_blend_mode = 0;
    }

    for (x = 0; x < num_renders; x++) {
        int t = 0;

        int smp_max_threads;
        Legacy_Effect_Proxy render = renders[x].effect;

        if (g_config_smp && render.can_multithread() && g_config_smp_mt > 1) {
            smp_max_threads = g_config_smp_mt;
            if (smp_max_threads > MAX_SMP_THREADS) {
                smp_max_threads = MAX_SMP_THREADS;
            }

            int nt = smp_max_threads;
            nt = render.smp_begin(nt,
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
                smp_Render(nt,
                           &render,
                           visdata,
                           is_beat,
                           s ? fbout : thisfb,
                           s ? thisfb : fbout,
                           w,
                           h);

                t = render.smp_finish(visdata,
                                      is_beat,
                                      s ? fbout : framebuffer,
                                      s ? framebuffer : fbout,
                                      w,
                                      h);
            }

        } else if (g_config_seh && renders[x].effect_index != LIST_ID) {
            try {
                t = renders[x].effect.render(
                    visdata, is_beat, s ? fbout : thisfb, s ? thisfb : fbout, w, h);
            } catch (...) {
                t = 0;
            }
        } else {
            t = renders[x].effect.render(
                visdata, is_beat, s ? fbout : thisfb, s ? thisfb : fbout, w, h);
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
            memcpy(thisfb, fbout, w * h * sizeof(int));
        }

        int* tfb = s ? fbout : thisfb;
        int* o = framebuffer;
        x = w * h;
        set_n_Context();

        int use_blendout = blendout();
        if (use_blendout == 10 && use_outblendval >= 255) {
            use_blendout = 1;
        }
        switch (use_blendout) {
            case 1:
                if (s) {
                    unset_n_Context();
                    return 1;
                }
                memcpy(o, tfb, x * sizeof(int));
                break;
            case 2: mmx_avgblend_block(o, tfb, x); break;
            case 3:
                while (x--) {
                    *o = BLEND_MAX(*o, *tfb++);
                    o++;
                }
                break;
            case 4: mmx_addblend_block(o, tfb, x); break;
            case 5:
                while (x--) {
                    *o = BLEND_SUB(*o, *tfb++);
                    o++;
                }
                break;
            case 6:
                while (x--) {
                    *o = BLEND_SUB(*tfb++, *o);
                    o++;
                }
                break;
            case 7: {
                int y = h / 2;
                while (y-- > 0) {
                    memcpy(o, tfb, w * sizeof(int));
                    tfb += w * 2;
                    o += w * 2;
                }
            } break;
            case 8: {
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
            case 9:
                while (x--) {
                    *o = *o ^ *tfb++;
                    o++;
                }
                break;
            case 10: mmx_adjblend_block(o, tfb, o, x, use_outblendval); break;
            case 11: mmx_mulblend_block(o, tfb, x); break;
            case 13:
                while (x--) {
                    *o = BLEND_MIN(*o, *tfb++);
                    o++;
                }
                break;
            case 12: {
                int* buf = (int*)getGlobalBuffer(w, h, bufferout, 0);
                if (!buf) {
                    break;
                }
                while (x--) {
                    *o = BLEND_ADJ(*tfb++, *o, depthof(*buf, outinvert));
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
            } break;
            default: break;
        }
        unset_n_Context();
    }
    return 0;
}

int C_RenderListClass::getNumRenders(void) { return num_renders; }

C_RenderListClass::T_RenderListType* C_RenderListClass::getRender(int index) {
    if (index >= 0 && index < num_renders) {
        return &renders[index];
    }
    return NULL;
}

int C_RenderListClass::findRender(T_RenderListType* r) {
    int idx;
    if (!r) {
        return -1;
    }
    for (idx = 0; idx < num_renders && renders[idx].effect != r->effect; idx++) {
        ;
    }
    if (idx < num_renders) {
        return idx;
    }
    return -1;
}

int C_RenderListClass::removeRenderFrom(T_RenderListType* r, int del) {
    int idx;
    if (!r) {
        return 1;
    }
    for (idx = 0; idx < num_renders && renders[idx].effect != r->effect; idx++) {
        ;
    }
    return removeRender(idx, del);
}

int C_RenderListClass::removeRender(int index, int del) {
    if (index >= 0 && index < num_renders) {
        if (del && renders[index].effect.valid()) {
            delete renders[index].effect.effect;
            delete renders[index].effect.legacy_effect;
        }
        num_renders--;
        while (index < num_renders) {
            renders[index] = renders[index + 1];
            index++;
        }
        if (!num_renders) {
            num_renders_alloc = 0;
            if (renders) {
                free(renders);
            }
            renders = NULL;
        }
        return 0;
    }
    return 1;
}
void C_RenderListClass::clearRenders(void) {
    int x;
    if (renders) {
        for (x = 0; x < num_renders; x++) {
            delete renders[x].effect.effect;
            delete renders[x].effect.legacy_effect;
        }
        free(renders);
    }
    num_renders = 0;
    num_renders_alloc = 0;
    renders = NULL;
    if (thisfb) {
        free(thisfb);
    }
    thisfb = 0;
}

int C_RenderListClass::insertRenderBefore(T_RenderListType* r,
                                          T_RenderListType* before) {
    int idx;
    if (!before) {
        idx = num_renders;
    } else {
        for (idx = 0; idx < num_renders && renders[idx].effect != before->effect;
             idx++) {
            ;
        }
    }
    return insertRender(r, idx);
}

int C_RenderListClass::insertRender(T_RenderListType* r, int index)  // index=-1 for add
{
    if (num_renders + 1 >= num_renders_alloc || !renders) {
        num_renders_alloc = num_renders + 16;
        T_RenderListType* newr =
            (T_RenderListType*)calloc(num_renders_alloc, sizeof(T_RenderListType));
        if (!newr) {
            return -1;
        }
        if (num_renders && renders) {
            memcpy(newr, renders, num_renders * sizeof(T_RenderListType));
        }
        if (renders) {
            free(renders);
        }
        renders = newr;
    }

    if (index < 0 || index >= num_renders) {
        renders[num_renders] = *r;
        return num_renders++;
    }
    int x;
    for (x = num_renders++; x > index; x--) {
        renders[x] = renders[x - 1];
    }
    renders[x] = *r;

    return x;
}

char C_RenderListClass::sig_str[] = "Nullsoft AVS Preset 0.2\x1a";

int C_RenderListClass::__SavePreset(char* filename) {
    lock_lock(g_render_cs);
    unsigned char* data = (unsigned char*)calloc(1024 * 1024, 1);
    int success = -1;
    if (data) {
        int pos = 0;
        memcpy(data + pos, sig_str, strlen(sig_str));
        pos += strlen(sig_str);
        pos += save_config_ex(data + pos, 1);
        if (pos < 1024 * 1024) {
            FILE* fp = fopen(filename, "wb");
            if (fp != NULL) {
                success = 0;
                fwrite(data, 1, pos, fp);
                fclose(fp);
            } else {
                success = 2;
            }
        } else {
            success = 1;
        }
        free(data);
    }
    lock_unlock(g_render_cs);
    return success;
}

int C_RenderListClass::__LoadPreset(char* filename, int clear) {
    lock_lock(g_render_cs);
    unsigned char* data = (unsigned char*)calloc(1024 * 1024, 1);
    int success = 1;
    if (clear) {
        clearRenders();
    }
    if (data) {
        FILE* fp = fopen(filename, "rb");
        if (fp != NULL) {
            fseek(fp, 0, SEEK_END);
            unsigned long int len = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            if (!fread(data, 1, min(len, 1024 * 1024), fp)) {
                len = 0;
            }
            fclose(fp);
            if (len > strlen(sig_str) + 2 && !memcmp(data, sig_str, strlen(sig_str) - 2)
                && data[strlen(sig_str) - 2] >= '1' && data[strlen(sig_str) - 2] <= '2'
                && data[strlen(sig_str) - 1] == '\x1a') {
                load_config(data + strlen(sig_str), len - strlen(sig_str));
                success = 0;
            }
        }
        free(data);
    }
    lock_unlock(g_render_cs);
    return success;
}

int C_RenderListClass::__SavePresetToUndo(C_UndoItem& item) {
    lock_lock(g_render_cs);
    unsigned char* data = (unsigned char*)calloc(1024 * 1024, 1);
    int success = -1;
    if (data) {
        // Do whatever the file saving stuff did
        int pos = 0;
        memcpy(data + pos, sig_str, strlen(sig_str));
        pos += strlen(sig_str);
        pos += save_config_ex(data + pos, 1);

        // And then set the data into the undo object.
        if (pos < 1024 * 1024) {
            item.set(data, pos, true);  // all undo items start dirty.
        }

        else {
            success = 1;
        }
        free(data);
    }
    lock_unlock(g_render_cs);
    return success;
}

int C_RenderListClass::__LoadPresetFromUndo(C_UndoItem& item, int clear) {
    lock_lock(g_render_cs);
    unsigned char* data = (unsigned char*)calloc(1024 * 1024, 1);
    int success = 1;
    if (clear) {
        clearRenders();
    }
    if (data) {
        if (item.size() < 1024 * 1024) {
            // Get the data from the undo object.
            unsigned int len = item.size();
            if (len == 0xffffffff) {
                len = 0;
            }
            memcpy(data, item.get(), item.size());

            // And then do whatever the file loading stuff did.
            if (!memcmp(data, sig_str, strlen(sig_str) - 2)
                && data[strlen(sig_str) - 2] >= '1' && data[strlen(sig_str) - 2] <= '2'
                && data[strlen(sig_str) - 1] == '\x1a') {
                load_config(data + strlen(sig_str), len - strlen(sig_str));
                success = 0;
            }
        }
        free(data);
    }
    lock_unlock(g_render_cs);
    return success;
}

/// smp fun

void C_RenderListClass::smp_Render(int minthreads,
                                   Legacy_Effect_Proxy* render,
                                   char visdata[2][2][576],
                                   int is_beat,
                                   int* framebuffer,
                                   int* fbout,
                                   int w,
                                   int h) {
    int x;
    smp_parms.nthreads = minthreads;
    if (!smp_parms.hQuitHandle) {
        smp_parms.hQuitHandle = signal_create_broadcast();
    }

    smp_parms.vis_data_ptr = visdata;
    smp_parms.is_beat = is_beat;
    smp_parms.framebuffer = framebuffer;
    smp_parms.fbout = fbout;
    smp_parms.w = w;
    smp_parms.h = h;
    smp_parms.render = render;
    for (x = 0; x < minthreads; x++) {
        if (x >= smp_parms.threadTop) {
            // unsigned long int id;
            smp_parms.hThreadSignalsStart[x] = signal_create_single();
            signal_set(smp_parms.hThreadSignalsStart[x]);
            smp_parms.hThreadSignalsDone[x] = signal_create_single();

            smp_parms.hThreads[x] = thread_create(smp_threadProc, (void*)(x));
            smp_parms.threadTop = x + 1;
        } else {
            signal_set(smp_parms.hThreadSignalsStart[x]);
        }
    }
    signal_wait_all(smp_parms.hThreadSignalsDone, smp_parms.nthreads, WAIT_INFINITE);
}

uint32_t C_RenderListClass::smp_threadProc(void* parm) {
    int which = (int)parm;
    void* hdls[2] = {smp_parms.hThreadSignalsStart[which], smp_parms.hQuitHandle};
    for (;;) {
        if (signal_wait_any(hdls, 2, WAIT_INFINITE) == smp_parms.hQuitHandle) {
            return 0;
        }

        smp_parms.render->smp_render(which,
                                     smp_parms.nthreads,
                                     *(char(*)[2][2][576])smp_parms.vis_data_ptr,

                                     smp_parms.is_beat,
                                     smp_parms.framebuffer,
                                     smp_parms.fbout,
                                     smp_parms.w,
                                     smp_parms.h);
        signal_set(smp_parms.hThreadSignalsDone[which]);
    }
}

C_RenderListClass::_s_smp_parms C_RenderListClass::smp_parms;
