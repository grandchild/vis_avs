#include "e_root.h"
#include "e_unknown.h"

#include "effect_library.h"
#include "pixel_format.h"
#include "rlib.h"

#include <iostream>

#define PUT_INT(y)                   \
    data[pos] = (y)&255;             \
    data[pos + 1] = (y >> 8) & 255;  \
    data[pos + 2] = (y >> 16) & 255; \
    data[pos + 3] = (y >> 24) & 255
#define GET_INT() \
    (data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24))

constexpr Parameter Root_Info::parameters[];

E_Root::E_Root() : buffers_saved(false) {}
E_Root::~E_Root() {
    for (int i = 0; i < NBUF; i++) {
        free(this->buffers[i]);
        this->buffers[i] = nullptr;
        this->buffers_w[i] = 0;
        this->buffers_h[i] = 0;
    }
}

extern int g_buffers_w[NBUF], g_buffers_h[NBUF];
extern void* g_buffers[NBUF];

void E_Root::start_buffer_context() {
    if (this->buffers_saved) {
        return;
    }
    this->buffers_saved = true;
    memcpy(this->buffers_temp_w, g_buffers_w, sizeof(this->buffers_temp_w));
    memcpy(this->buffers_temp_h, g_buffers_h, sizeof(this->buffers_temp_h));
    memcpy(this->buffers_temp, g_buffers, sizeof(this->buffers_temp));

    memcpy(g_buffers_w, this->buffers_w, sizeof(this->buffers_w));
    memcpy(g_buffers_h, this->buffers_h, sizeof(this->buffers_h));
    memcpy(g_buffers, this->buffers, sizeof(this->buffers));
}

void E_Root::end_buffer_context() {
    if (!this->buffers_saved) {
        return;
    }
    this->buffers_saved = false;

    memcpy(this->buffers_w, g_buffers_w, sizeof(this->buffers_w));
    memcpy(this->buffers_h, g_buffers_h, sizeof(this->buffers_h));
    memcpy(this->buffers, g_buffers, sizeof(this->buffers));

    memcpy(g_buffers_w, this->buffers_temp_w, sizeof(this->buffers_temp_w));
    memcpy(g_buffers_h, this->buffers_temp_h, sizeof(this->buffers_temp_h));
    memcpy(g_buffers, this->buffers_temp, sizeof(this->buffers_temp));
}

int E_Root::render(char visdata[2][2][576],
                   int is_beat,
                   int* framebuffer,
                   int* fbout,
                   int w,
                   int h) {
    if (is_beat & 0x800000000) {
        if (this->config.clear) {
            memset(framebuffer, 0, w * h * sizeof(pixel_rgb0_8));
        }
    }
    this->start_buffer_context();
    for (auto& effect : this->children) {
        if (effect) {
            effect->render(visdata, is_beat, framebuffer, fbout, w, h);
        } else {
            log_warn("NULL effect child in Root (%d children). This shouldn't happen.",
                     this->children.size());
        }
    }
    this->end_buffer_context();
    return 0;
}

void E_Root::load_legacy(unsigned char* data, int len) {
    int32_t pos = 0;
    if (len >= 1) {
        this->config.clear = data[pos++];
    }
    while (pos < len) {
        int legacy_effect_id = GET_INT();
        char legacy_effect_ape_id[LEGACY_APE_ID_LENGTH + 1];
        legacy_effect_ape_id[0] = '\0';
        pos += 4;
        if (legacy_effect_id >= DLLRENDERBASE) {
            if (pos + LEGACY_APE_ID_LENGTH > len) {
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

        Effect* new_effect =
            component_factory_legacy(legacy_effect_id, legacy_effect_ape_id);
        new_effect->load_legacy(data + pos, l_len);
        pos += l_len;
        this->insert(new_effect, this, INSERT_CHILD);
    }
}

int E_Root::save_legacy(unsigned char* data) {
    size_t pos = 0;
    data[pos++] = this->config.clear;

    for (auto& effect : this->children) {
        if (effect == nullptr) {
            log_warn("NULL effect child in Root (%d children). This shouldn't happen.",
                     this->children.size());
            continue;
        }
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
    return (int)pos;
}

Effect_Info* create_Root_Info() { return new Root_Info(); }
Effect* create_Root() { return new E_Root(); }
void set_Root_desc(char* desc) { E_Root::set_desc(desc); }
