/*
Reconstructed by decompiling the original colormap.ape v1.3 binary.
Credits:
  Steven Wittens (a.k.a. "Unconed" -> https://acko.net), for the original Colormap,
  & the Ghidra authors.

Most of the code deals with handling the UI interactions (all the Win32 UI API calls
were thankfully plainly visible in the decompiled binary), and the actual mapping
code is fairly straightforward.

The original implementation used MMX asm, which has been updated to using SSE2/SSSE3
intrinsics. The code could further be sped up by leveraging the "gather" instructions
available with Intel's AVX2 extension (ca. 2014 and later CPU models) to load colors by
index from the baked map.
*/
#include "c_colormap.h"
#include <cstdio>
#include <time.h>
#include "r_defs.h"


// Integer range-mapping macros. Avoid truncation by multiplying first.
// Map point A from one value range to another.
// Get distance from A to B (in B-range) by mapping A to B.
#define GET_INT() (data[pos] | (data[pos+1] << 8) | (data[pos+2] << 16) | (data[pos+3] << 24))
#define PUT_INT(y) data[pos] = (y) & 0xff;         \
                   data[pos+1] = ((y) >> 8) & 0xff;  \
                   data[pos+2] = ((y) >> 16) & 0xff; \
                   data[pos+3] = ((y) >> 24) & 0xff

C_ColorMap::C_ColorMap() :
    config{
        COLORMAP_COLOR_KEY_RED,
        COLORMAP_BLENDMODE_REPLACE,
        COLORMAP_MAP_CYCLE_NONE,
        0,   // adjustable alpha
        0,   // [unused]
        0,   // don't-skip-fast-beats is unchecked
        11,  // default map cycle speed
    },
    current_map{0},
    next_map{0},
    change_animation_step{COLORMAP_MAP_CYCLE_ANIMATION_STEPS},
    disable_map_change{0}
    {
    for(unsigned int i = 0; i < COLORMAP_NUM_MAPS; i++) {
        this->maps[i].enabled = i == 0;
        this->maps[i].length = 2;
        this->maps[i].map_id = this->get_new_id();
        this->maps[i].colors = NULL;
        memset(this->maps[i].filename, 0, COLORMAP_MAP_FILENAME_MAXLEN);
        this->make_default_map(i);
    }
}

C_ColorMap::~C_ColorMap() {
    for(unsigned int i = 0; i < COLORMAP_NUM_MAPS; i++) {
        delete[] this->maps[i].colors;
    }
}

int C_ColorMap::get_new_id() {
    return this->next_id++;
}

void C_ColorMap::make_default_map(int map_index) {
    delete[] this->maps[map_index].colors;
    this->maps[map_index].colors = new map_color[2];
    this->maps[map_index].colors[0].position = 0;
    this->maps[map_index].colors[0].color = 0x000000;
    this->maps[map_index].colors[0].color_id = this->get_new_id();
    this->maps[map_index].colors[1].position = 255;
    this->maps[map_index].colors[1].color = 0xffffff;
    this->maps[map_index].colors[1].color_id = this->get_new_id();
    this->bake_full_map(map_index);
}

void C_ColorMap::bake_full_map(int map_index) {
    unsigned int cache_index = 0;
    unsigned int color_index = 0;
    if(this->maps[map_index].length < 1) {
        return;
    }
    this->sort_colors(map_index);
    map_color first = this->maps[map_index].colors[0];
    for(; cache_index < first.position; cache_index++) {
        this->baked_maps[map_index].colors[cache_index] = first.color;
    }
    for(; color_index < this->maps[map_index].length - 1; color_index++) {
        map_color from = this->maps[map_index].colors[color_index];
        map_color to = this->maps[map_index].colors[color_index + 1];
        for(; cache_index < to.position; cache_index++) {
            int rel_i = cache_index - from.position;
            int w = to.position - from.position;
            int lerp = NUM_COLOR_VALUES * (float)rel_i / (float)w;
            this->baked_maps[map_index].colors[cache_index] = BLEND_ADJ_NOMMX(to.color, from.color, lerp);
        }
    }
    map_color last = this->maps[map_index].colors[color_index];
    for(; cache_index < NUM_COLOR_VALUES; cache_index++) {
        this->baked_maps[map_index].colors[cache_index] = last.color;
    }
}

void C_ColorMap::add_map_color(int map_index, unsigned int position, int color) {
    if(this->maps[map_index].length >= COLORMAP_MAX_COLORS) {
        return;
    }
    map_color* new_map_colors = new map_color[this->maps[map_index].length + 1];
    int added = 0;
    if(position < this->maps[map_index].colors[0].position) {
        new_map_colors[0].position = position;
        new_map_colors[0].color = color;
        new_map_colors[0].color_id = this->get_new_id();
        added = 1;
    }
    for(unsigned int i = 0; i < this->maps[map_index].length; i++) {
        if(added == 0 && position >= this->maps[map_index].colors[i].position) {
            new_map_colors[i].position = position;
            new_map_colors[i].color = color;
            new_map_colors[i].color_id = this->get_new_id();
            added = 1;
        }
        new_map_colors[i + added] = this->maps[map_index].colors[i];
    }
    map_color* old_map_colors = this->maps[map_index].colors;
    this->maps[map_index].colors = new_map_colors;
    this->maps[map_index].length += 1;
    this->sort_colors(map_index);
    delete[] old_map_colors;
}

void C_ColorMap::remove_map_color(int map_index, int remove_id) {
    if(this->maps[map_index].length <= 1) {
        return;
    }
    map_color* new_map_colors = new map_color[this->maps[map_index].length];
    for(unsigned int i = 0, a = 0; i < this->maps[map_index].length - 1; i++) {
        if(this->maps[map_index].colors[i].color_id == remove_id) {
            a = 1;
        }
        new_map_colors[i] = this->maps[map_index].colors[i + a];
    }
    map_color* old_map_colors = this->maps[map_index].colors;
    this->maps[map_index].length -= 1;
    this->maps[map_index].colors = new_map_colors;
    this->sort_colors(map_index);
    delete[] old_map_colors;
}

int compare_colors(const void* color1, const void* color2) {
    int diff = ((map_color*)color1)->position - ((map_color*)color2)->position;
    if (diff == 0) {
        // TODO [bugfix,feature]: If color positions are the same, the brighter color
        // gets sorted higher. This is somewhat arbitrary. We should try and sort by
        // previous position, so that when dragging a color, it does not flip until its
        // position actually becomes less than the other.
        diff = ((map_color*)color1)->color - ((map_color*)color2)->color;
    }
    return diff;
}

void C_ColorMap::sort_colors(int map_index) {
    qsort(this->maps[map_index].colors, this->maps[map_index].length, sizeof(map_color), compare_colors);
}

bool C_ColorMap::any_maps_enabled() {
    bool any_maps_enabled = false;
    for(unsigned int i = 0; i < COLORMAP_NUM_MAPS; i++) {
        if(this->maps[i].enabled) {
            any_maps_enabled = true;
            break;
        }
    }
    return any_maps_enabled;
}

baked_map* C_ColorMap::animate_map_frame(int is_beat) {
    this->next_map %= COLORMAP_NUM_MAPS;
    if(this->config.map_cycle_mode == COLORMAP_MAP_CYCLE_NONE || this->disable_map_change) {
        this->change_animation_step = 0;
        return &this->baked_maps[this->current_map];
    } else {
        this->change_animation_step += this->config.map_cycle_speed;
        this->change_animation_step = min(this->change_animation_step, COLORMAP_MAP_CYCLE_ANIMATION_STEPS);
        if(is_beat && (!this->config.dont_skip_fast_beats || this->change_animation_step == COLORMAP_MAP_CYCLE_ANIMATION_STEPS)) {
            if(this->any_maps_enabled()) {
                do {
                    if(this->config.map_cycle_mode == COLORMAP_MAP_CYCLE_BEAT_RANDOM) {
                        this->next_map = rand() % COLORMAP_NUM_MAPS;
                    } else{
                        this->next_map = (this->next_map + 1) % COLORMAP_NUM_MAPS;
                    }
                } while(!this->maps[this->next_map].enabled);
            }
            this->change_animation_step = 0;
        }
        if(this->change_animation_step == 0) {
            this->reset_tween_map();
        } else if(this->change_animation_step == COLORMAP_MAP_CYCLE_ANIMATION_STEPS) {
            this->current_map = this->next_map;
            this->reset_tween_map();
        } else {
            if(this->current_map != this->next_map) {
                this->animation_step();
            } else {
                this->reset_tween_map();
            }
        }
        return &this->tween_map;
    }
}

inline void C_ColorMap::animation_step() {
    for(unsigned int i = 0; i < NUM_COLOR_VALUES; i++) {
        this->tween_map.colors[i] = BLEND_ADJ_NOMMX(
            this->baked_maps[this->next_map].colors[i],
            this->baked_maps[this->current_map].colors[i],
            this->change_animation_step
        );
    }
    return;
}

inline void C_ColorMap::animation_step_sse2() {
    for(unsigned int i = 0; i < NUM_COLOR_VALUES; i += 4) {
        // __m128i four_current_values = _mm_loadu_si128((__m128i*)&(this->baked_maps[this->current_map].colors[i]));
        // __m128i four_next_values = _mm_loadu_si128((__m128i*)&(this->baked_maps[this->next_map].colors[i]));
        // __m128i blend_lerp = _mm_set1_epi8((unsigned char)this->change_animation_step);
        /* TODO */
    }
    return;
}

inline void C_ColorMap::reset_tween_map() {
    memcpy(&this->tween_map, &this->baked_maps[this->current_map], sizeof(baked_map));
}

inline int C_ColorMap::get_key(int color) {
    int r, g, b;
    switch(this->config.color_key) {
        case COLORMAP_COLOR_KEY_RED:
            return color >> 16 & 0xff;
        case COLORMAP_COLOR_KEY_GREEN:
            return color >> 8 & 0xff;
        case COLORMAP_COLOR_KEY_BLUE:
            return color & 0xff;
        case COLORMAP_COLOR_KEY_RGB_SUM_HALF:
            return min(((color >> 16 & 0xff) + (color >> 8 & 0xff) + (color & 0xff)) / 2, NUM_COLOR_VALUES - 1);
        case COLORMAP_COLOR_KEY_MAX:
            r = color >> 16 & 0xff;
            g = color >> 8 & 0xff;
            b = color & 0xff;
            r = max(r, g);
            return max(r, b);
        default:
        case COLORMAP_COLOR_KEY_RGB_AVERAGE:
            return ((color >> 16 & 0xff) + (color >> 8 & 0xff) + (color & 0xff)) / 3;
    }
}

void C_ColorMap::blend(baked_map* blend_map, int *framebuffer, int w, int h) {
    int four_px_colors[4];
    int four_keys[4];
    switch(this->config.blendmode) {
        case COLORMAP_BLENDMODE_REPLACE:
            for(int i = 0; i < w * h; i += 4) {
                for(int k = 0; k < 4; k++) {
                    four_keys[k] = this->get_key(framebuffer[i + k]);
                    framebuffer[i + k] = blend_map->colors[four_keys[k]];
                }
            }
            break;
        case COLORMAP_BLENDMODE_ADDITIVE:
            for(int i = 0; i < w * h; i += 4) {
                for(int k = 0; k < 4; k++) {
                    four_keys[k] = this->get_key(framebuffer[i + k]);
                    four_px_colors[k] = blend_map->colors[four_keys[k]];
                    framebuffer[i + k] = BLEND(framebuffer[i + k], four_px_colors[k]);
                }
            }
            break;
        case COLORMAP_BLENDMODE_MAXIMUM:
            for(int i = 0; i < w * h; i += 4) {
                for(int k = 0; k < 4; k++) {
                    four_keys[k] = this->get_key(framebuffer[i + k]);
                    four_px_colors[k] = blend_map->colors[four_keys[k]];
                    framebuffer[i + k] = BLEND_MAX(framebuffer[i + k], four_px_colors[k]);
                }
            }
            break;
        case COLORMAP_BLENDMODE_MINIMUM:
            for(int i = 0; i < w * h; i += 4) {
                for(int k = 0; k < 4; k++) {
                    four_keys[k] = this->get_key(framebuffer[i + k]);
                    four_px_colors[k] = blend_map->colors[four_keys[k]];
                    framebuffer[i + k] = BLEND_MIN(framebuffer[i + k], four_px_colors[k]);
                }
            }
            break;
        case COLORMAP_BLENDMODE_5050:
            for(int i = 0; i < w * h; i += 4) {
                for(int k = 0; k < 4; k++) {
                    four_keys[k] = this->get_key(framebuffer[i + k]);
                    four_px_colors[k] = blend_map->colors[four_keys[k]];
                    framebuffer[i + k] = BLEND_AVG(framebuffer[i + k], four_px_colors[k]);
                }
            }
            break;
        case COLORMAP_BLENDMODE_SUB1:
            for(int i = 0; i < w * h; i += 4) {
                for(int k = 0; k < 4; k++) {
                    four_keys[k] = this->get_key(framebuffer[i + k]);
                    four_px_colors[k] = blend_map->colors[four_keys[k]];
                    framebuffer[i + k] = BLEND_SUB(framebuffer[i + k], four_px_colors[k]);
                }
            }
            break;
        case COLORMAP_BLENDMODE_SUB2:
            for(int i = 0; i < w * h; i += 4) {
                for(int k = 0; k < 4; k++) {
                    four_keys[k] = this->get_key(framebuffer[i + k]);
                    four_px_colors[k] = blend_map->colors[four_keys[k]];
                    framebuffer[i + k] = BLEND_SUB(four_px_colors[k], framebuffer[i + k]);
                }
            }
            break;
        case COLORMAP_BLENDMODE_MULTIPLY:
            for(int i = 0; i < w * h; i += 4) {
                for(int k = 0; k < 4; k++) {
                    four_keys[k] = this->get_key(framebuffer[i + k]);
                    four_px_colors[k] = blend_map->colors[four_keys[k]];
                    framebuffer[i + k] = BLEND_MUL(framebuffer[i + k], four_px_colors[k]);
                }
            }
            break;
        case COLORMAP_BLENDMODE_XOR:
            for(int i = 0; i < w * h; i += 4) {
                for(int k = 0; k < 4; k++) {
                    four_keys[k] = this->get_key(framebuffer[i + k]);
                    four_px_colors[k] = blend_map->colors[four_keys[k]];
                    framebuffer[i + k] ^= four_px_colors[k];
                }
            }
            break;
        case COLORMAP_BLENDMODE_ADJUSTABLE:
            for(int i = 0; i < w * h; i += 4) {
                for(int k = 0; k < 4; k++) {
                    four_keys[k] = this->get_key(framebuffer[i + k]);
                    four_px_colors[k] = blend_map->colors[four_keys[k]];
                    framebuffer[i + k] = BLEND_ADJ_NOMMX(four_px_colors[k], framebuffer[i + k], this->config.adjustable_alpha);
                }
            }
            break;
    }
}

inline __m128i C_ColorMap::get_key_ssse3(__m128i color4) {
    // Gather uint8s from certain source locations. (0xff => dest will be zero.) Collect
    // the respective channels into the pixel's lower 8bits.
    __m128i gather_red =    _mm_set_epi32(0xffffff0e, 0xffffff0a, 0xffffff06, 0xffffff02);
    __m128i gather_green =   _mm_set_epi32(0xffffff0d, 0xffffff09, 0xffffff05, 0xffffff01);
    __m128i gather_blue =     _mm_set_epi32(0xffffff0c, 0xffffff08, 0xffffff04, 0xffffff00);
    __m128i max_channel_value = _mm_set1_epi32(NUM_COLOR_VALUES - 1);
    __m128i r, g;
    __m128 color4f;
    switch(this->config.color_key) {
        case COLORMAP_COLOR_KEY_RED:
            return _mm_shuffle_epi8(color4, gather_red);
        case COLORMAP_COLOR_KEY_GREEN:
            return _mm_shuffle_epi8(color4, gather_green);
        case COLORMAP_COLOR_KEY_BLUE:
            return _mm_shuffle_epi8(color4, gather_blue);
        case COLORMAP_COLOR_KEY_RGB_SUM_HALF:
            r = _mm_shuffle_epi8(color4, gather_red);
            g = _mm_shuffle_epi8(color4, gather_green);
            color4 = _mm_shuffle_epi8(color4, gather_blue);
            color4 = _mm_add_epi32(color4, r);
            // Correct average, round up on odd sum, i.e.: (a+b+c + 1) >> 1:
            //color4 = _mm_avg_epu16(color4, g);
            // Original Colormap behavior, round down on .5, i.e. (a+b+c) >> 1:
            color4 = _mm_add_epi32(color4, g);
            color4 = _mm_srli_epi32(color4, 1);
            return _mm_min_epi16(color4, max_channel_value);
        case COLORMAP_COLOR_KEY_MAX:
            r = _mm_shuffle_epi8(color4, gather_red);
            g = _mm_shuffle_epi8(color4, gather_green);
            color4 = _mm_shuffle_epi8(color4, gather_blue);
            color4 = _mm_max_epu8(color4, r);
            return _mm_max_epu8(color4, g);
        default:
        case COLORMAP_COLOR_KEY_RGB_AVERAGE:
            r = _mm_shuffle_epi8(color4, gather_red);
            g = _mm_shuffle_epi8(color4, gather_green);
            color4 = _mm_shuffle_epi8(color4, gather_blue);
            color4 = _mm_add_epi16(color4, r);
            color4 = _mm_add_epi16(color4, g);
            // For inputs up to 255 * 3, float32 division returns the same results as
            // integer division
            color4f = _mm_cvtepi32_ps(color4);
            color4f = _mm_div_ps(color4f, _mm_set1_ps(3.0f));
            return _mm_cvtps_epi32(color4f);
    }
}

void C_ColorMap::blend_ssse3(baked_map* blend_map, int *framebuffer, int w, int h) {
    __m128i framebuffer_4px;
    int four_keys[4];
    __m128i colors_4px;
    __m128i extend_lo_p8_to_p16 = _mm_set_epi32(0xff07ff06, 0xff05ff04, 0xff03ff02, 0xff01ff00);
    __m128i extend_hi_p8_to_p16 = _mm_set_epi32(0xff0fff0e, 0xff0dff0c, 0xff0bff0a, 0xff09ff08);
    __m128i framebuffer_2_px[2];
    __m128i colors_2_px[2];
    switch(this->config.blendmode) {
        case COLORMAP_BLENDMODE_REPLACE:
            for(int i = 0; i < w * h; i += 4) {
                framebuffer_4px = _mm_loadu_si128((__m128i*)&framebuffer[i]);
                _mm_store_si128((__m128i*)four_keys, this->get_key_ssse3(framebuffer_4px));
                for(int k = 0; k < 4; k++) {
                    framebuffer[i + k] = blend_map->colors[four_keys[k]];
                }
            }
            break;
        case COLORMAP_BLENDMODE_ADDITIVE:
            for(int i = 0; i < w * h; i += 4) {
                framebuffer_4px = _mm_loadu_si128((__m128i*)&framebuffer[i]);
                _mm_store_si128((__m128i*)four_keys, this->get_key_ssse3(framebuffer_4px));
                colors_4px = _mm_set_epi32(blend_map->colors[four_keys[3]],
                                           blend_map->colors[four_keys[2]],
                                           blend_map->colors[four_keys[1]],
                                           blend_map->colors[four_keys[0]]);
                framebuffer_4px = _mm_adds_epu8(framebuffer_4px, colors_4px);
                _mm_store_si128((__m128i*)&framebuffer[i], framebuffer_4px);
            }
            break;
        case COLORMAP_BLENDMODE_MAXIMUM:
            for(int i = 0; i < w * h; i += 4) {
                framebuffer_4px = _mm_loadu_si128((__m128i*)&framebuffer[i]);
                _mm_store_si128((__m128i*)four_keys, this->get_key_ssse3(framebuffer_4px));
                colors_4px = _mm_set_epi32(blend_map->colors[four_keys[3]],
                                           blend_map->colors[four_keys[2]],
                                           blend_map->colors[four_keys[1]],
                                           blend_map->colors[four_keys[0]]);
                framebuffer_4px = _mm_max_epu8(framebuffer_4px, colors_4px);
                _mm_store_si128((__m128i*)&framebuffer[i], framebuffer_4px);
            }
            break;
        case COLORMAP_BLENDMODE_MINIMUM:
            for(int i = 0; i < w * h; i += 4) {
                framebuffer_4px = _mm_loadu_si128((__m128i*)&framebuffer[i]);
                _mm_store_si128((__m128i*)four_keys, this->get_key_ssse3(framebuffer_4px));
                colors_4px = _mm_set_epi32(blend_map->colors[four_keys[3]],
                                           blend_map->colors[four_keys[2]],
                                           blend_map->colors[four_keys[1]],
                                           blend_map->colors[four_keys[0]]);
                framebuffer_4px = _mm_min_epu8(framebuffer_4px, colors_4px);
                _mm_store_si128((__m128i*)&framebuffer[i], framebuffer_4px);
            }
            break;
        case COLORMAP_BLENDMODE_5050:
            for(int i = 0; i < w * h; i += 4) {
                framebuffer_4px = _mm_loadu_si128((__m128i*)&framebuffer[i]);
                _mm_store_si128((__m128i*)four_keys, this->get_key_ssse3(framebuffer_4px));
                colors_4px = _mm_set_epi32(blend_map->colors[four_keys[3]],
                                           blend_map->colors[four_keys[2]],
                                           blend_map->colors[four_keys[1]],
                                           blend_map->colors[four_keys[0]]);
                framebuffer_4px = _mm_avg_epu8(framebuffer_4px, colors_4px);
                _mm_store_si128((__m128i*)&framebuffer[i], framebuffer_4px);
            }
            break;
        case COLORMAP_BLENDMODE_SUB1:
            for(int i = 0; i < w * h; i += 4) {
                framebuffer_4px = _mm_loadu_si128((__m128i*)&framebuffer[i]);
                _mm_store_si128((__m128i*)four_keys, this->get_key_ssse3(framebuffer_4px));
                colors_4px = _mm_set_epi32(blend_map->colors[four_keys[3]],
                                           blend_map->colors[four_keys[2]],
                                           blend_map->colors[four_keys[1]],
                                           blend_map->colors[four_keys[0]]);
                framebuffer_4px = _mm_subs_epu8(framebuffer_4px, colors_4px);
                _mm_store_si128((__m128i*)&framebuffer[i], framebuffer_4px);
            }
            break;
        case COLORMAP_BLENDMODE_SUB2:
            for(int i = 0; i < w * h; i += 4) {
                framebuffer_4px = _mm_loadu_si128((__m128i*)&framebuffer[i]);
                _mm_store_si128((__m128i*)four_keys, this->get_key_ssse3(framebuffer_4px));
                colors_4px = _mm_set_epi32(blend_map->colors[four_keys[3]],
                                           blend_map->colors[four_keys[2]],
                                           blend_map->colors[four_keys[1]],
                                           blend_map->colors[four_keys[0]]);
                framebuffer_4px = _mm_subs_epu8(colors_4px, framebuffer_4px);
                _mm_store_si128((__m128i*)&framebuffer[i], framebuffer_4px);
            }
            break;
        case COLORMAP_BLENDMODE_MULTIPLY:
            for(int i = 0; i < w * h; i += 4) {
                framebuffer_4px = _mm_loadu_si128((__m128i*)&framebuffer[i]);
                _mm_store_si128((__m128i*)four_keys, this->get_key_ssse3(framebuffer_4px));
                colors_4px = _mm_set_epi32(blend_map->colors[four_keys[3]],
                                           blend_map->colors[four_keys[2]],
                                           blend_map->colors[four_keys[1]],
                                           blend_map->colors[four_keys[0]]);
                // Unfortunately intel CPUs do not have a packed unsigned 8bit multiply.
                // So the calculation becomes a matter of extending both sides of the
                // multiplication to 16 bits, which doubles the size, resulting in 2
                // packed 128bit values.
                framebuffer_2_px[0] = _mm_shuffle_epi8(framebuffer_4px, extend_lo_p8_to_p16);
                framebuffer_2_px[1] = _mm_shuffle_epi8(framebuffer_4px, extend_hi_p8_to_p16);
                colors_2_px[0] = _mm_shuffle_epi8(colors_4px, extend_lo_p8_to_p16);
                colors_2_px[1] = _mm_shuffle_epi8(colors_4px, extend_hi_p8_to_p16);
                // We can then packed-multiply the half-filled 16bit (only the lower 8
                // bits are non-zero) values. Thus we are interested only in the lower
                // 16 bits of the 32bit result.
                framebuffer_2_px[0] = _mm_mullo_epi16(framebuffer_2_px[0], colors_2_px[0]);
                framebuffer_2_px[1] = _mm_mullo_epi16(framebuffer_2_px[1], colors_2_px[1]);
                // Divide by 256 again, to normalize. This loses accuracy, because
                // 0xff * 0xff => 0xfe, but it's the way Multiply works throughout AVS.
                framebuffer_2_px[0] = _mm_srli_epi16(framebuffer_2_px[0], 8);
                framebuffer_2_px[1] = _mm_srli_epi16(framebuffer_2_px[1], 8);
                // Pack the expanded 16bit values back into 8bit values.
                framebuffer_4px = _mm_packus_epi16(framebuffer_2_px[0], framebuffer_2_px[1]);
                _mm_store_si128((__m128i*)&framebuffer[i], framebuffer_4px);
            }
            break;
        case COLORMAP_BLENDMODE_XOR:
            for(int i = 0; i < w * h; i += 4) {
                framebuffer_4px = _mm_loadu_si128((__m128i*)&framebuffer[i]);
                _mm_store_si128((__m128i*)four_keys, this->get_key_ssse3(framebuffer_4px));
                colors_4px = _mm_set_epi32(blend_map->colors[four_keys[3]],
                                           blend_map->colors[four_keys[2]],
                                           blend_map->colors[four_keys[1]],
                                           blend_map->colors[four_keys[0]]);
                framebuffer_4px = _mm_xor_si128(framebuffer_4px, colors_4px);
                _mm_store_si128((__m128i*)&framebuffer[i], framebuffer_4px);
            }
            break;
        case COLORMAP_BLENDMODE_ADJUSTABLE:
            __m128i v = _mm_set1_epi16((unsigned char)this->config.adjustable_alpha);
            __m128i i_v = _mm_set1_epi16(COLORMAP_ADJUSTABLE_BLEND_MAX - this->config.adjustable_alpha);
            for(int i = 0; i < w * h; i += 4) {
                framebuffer_4px = _mm_loadu_si128((__m128i*)&framebuffer[i]);
                _mm_store_si128((__m128i*)four_keys, this->get_key_ssse3(framebuffer_4px));
                colors_4px = _mm_set_epi32(blend_map->colors[four_keys[3]],
                                           blend_map->colors[four_keys[2]],
                                           blend_map->colors[four_keys[1]],
                                           blend_map->colors[four_keys[0]]);
                // See Multiply blend case for details. This is basically the same
                // thing, except that each side gets multiplied with v and 1-v
                // respectively and then added together.
                framebuffer_2_px[0] = _mm_shuffle_epi8(framebuffer_4px, extend_lo_p8_to_p16);
                framebuffer_2_px[1] = _mm_shuffle_epi8(framebuffer_4px, extend_hi_p8_to_p16);
                colors_2_px[0] = _mm_shuffle_epi8(colors_4px, extend_lo_p8_to_p16);
                colors_2_px[1] = _mm_shuffle_epi8(colors_4px, extend_hi_p8_to_p16);
                framebuffer_2_px[0] = _mm_mullo_epi16(framebuffer_2_px[0], i_v);
                framebuffer_2_px[1] = _mm_mullo_epi16(framebuffer_2_px[1], i_v);
                colors_2_px[0] = _mm_mullo_epi16(colors_2_px[0], v);
                colors_2_px[1] = _mm_mullo_epi16(colors_2_px[1], v);
                framebuffer_2_px[0] = _mm_adds_epu16(framebuffer_2_px[0], colors_2_px[0]);
                framebuffer_2_px[1] = _mm_adds_epu16(framebuffer_2_px[1], colors_2_px[1]);
                framebuffer_2_px[0] = _mm_srli_epi16(framebuffer_2_px[0], 8);
                framebuffer_2_px[1] = _mm_srli_epi16(framebuffer_2_px[1], 8);
                framebuffer_4px = _mm_packus_epi16(framebuffer_2_px[0], framebuffer_2_px[1]);
                _mm_store_si128((__m128i*)&framebuffer[i], framebuffer_4px);
            }
            break;
    }
}

int C_ColorMap::render(char visdata[2][2][576], int is_beat, int *framebuffer, int *fbout, int w, int h) {
    (void)visdata;
    (void)fbout;
    baked_map* blend_map = this->animate_map_frame(is_beat);
    this->blend_ssse3(blend_map, framebuffer, w, h);
    return 0;
}

char *C_ColorMap::get_desc(void) {
    return MOD_NAME;
}

bool C_ColorMap::load_map_header(unsigned char *data, int len, int map_index, int pos) {
    if(len - pos < (int)COLORMAP_SAVE_MAP_HEADER_SIZE) {
        return false;
    }
    this->maps[map_index].enabled = GET_INT();
    pos += 4;
    this->maps[map_index].length = GET_INT();
    pos += 4;
    this->maps[map_index].map_id = this->get_new_id();
    pos += 4;
    strncpy(this->maps[map_index].filename, (char*)&data[pos], COLORMAP_MAP_FILENAME_MAXLEN - 1);
    this->maps[map_index].filename[COLORMAP_MAP_FILENAME_MAXLEN - 1] = '\0';
    return true;
}

bool C_ColorMap::load_map_colors(unsigned char *data, int len, int map_index, int pos) {
    delete[] this->maps[map_index].colors;
    this->maps[map_index].colors = new map_color[this->maps[map_index].length];
    unsigned int i;
    for(i = 0; i < this->maps[map_index].length; i++) {
        if(len - pos < (int)sizeof(map_color)) {
            return false;
        }
        this->maps[map_index].colors[i].position = GET_INT();
        pos += 4;
        this->maps[map_index].colors[i].color = GET_INT();
        pos += 4;
        this->maps[map_index].colors[i].color_id = this->get_new_id();
        pos += 4;
    }
    return true;
}

void C_ColorMap::load_config(unsigned char *data, int len) {
    if (len >= (int)sizeof(colormap_apeconfig))
        memcpy(&this->config, data, sizeof(colormap_apeconfig));

    bool success = true;
    unsigned int pos = sizeof(colormap_apeconfig);
    for(int map_index = 0; map_index < COLORMAP_NUM_MAPS; map_index++) {
        success = this->load_map_header(data, len, map_index, pos);
        pos += COLORMAP_SAVE_MAP_HEADER_SIZE;
    }
    for(int map_index = 0; map_index < COLORMAP_NUM_MAPS; map_index++) {
        success = this->load_map_colors(data, len, map_index, pos);
        if(!success) {
            break;
        }
        this->bake_full_map(map_index);
        pos += this->maps[map_index].length * sizeof(map_color);
    }
}

int C_ColorMap::save_config(unsigned char *data) {
    memcpy(data, &this->config, sizeof(colormap_apeconfig));
    int pos = sizeof(colormap_apeconfig);

    for(int map_index = 0; map_index < COLORMAP_NUM_MAPS; map_index++) {
        PUT_INT(this->maps[map_index].enabled);
        pos += 4;
        PUT_INT(this->maps[map_index].length);
        pos += 4;
        PUT_INT(this->maps[map_index].map_id);
        pos += 4;
        memset((char*)&data[pos], 0, COLORMAP_MAP_FILENAME_MAXLEN);
        strncpy((char*)&data[pos], this->maps[map_index].filename, COLORMAP_MAP_FILENAME_MAXLEN - 1);
        pos += COLORMAP_MAP_FILENAME_MAXLEN;
    }
    for(int map_index = 0; map_index < COLORMAP_NUM_MAPS; map_index++) {
        for(unsigned int i = 0; i < this->maps[map_index].length; i++) {
            PUT_INT(this->maps[map_index].colors[i].position);
            pos += 4;
            PUT_INT(this->maps[map_index].colors[i].color);
            pos += 4;
            PUT_INT(this->maps[map_index].colors[i].color_id);
            pos += 4;
        }
    }
    return pos;
}

C_RBASE *R_ColorMap(char *desc) {
    srand(time(NULL));
    if (desc) {
        strcpy(desc, MOD_NAME);
        return NULL;
    }
    return (C_RBASE *) new C_ColorMap();
}
