#include "e_picture2.h"

#include "r_defs.h"

#include "files.h"
#include "image.h"
#include "pixel_format.h"

#include "../platform.h"
#include "../util.h"  // ssizeof32

#include <sys/types.h>

constexpr Parameter Picture2_Info::parameters[];

std::vector<const char*> E_Picture2::file_list;

void Picture2_Info::on_file_change(Effect* component,
                                   const Parameter*,
                                   const std::vector<int64_t>&) {
    auto picture2 = (E_Picture2*)component;
    picture2->load_image();
}

const char* const* Picture2_Info::image_files(int64_t* length_out) {
    *length_out = E_Picture2::file_list.size();
    return E_Picture2::file_list.data();
}

E_Picture2::E_Picture2(AVS_Instance* avs)
    : Configurable_Effect(avs),
      image_lock(lock_init()),
      image(nullptr),
      iw(0),
      ih(0),
      work_image(nullptr),
      work_image_bilinear(nullptr),
      wiw(0),
      wih(0),
      need_image_refresh(true) {
    if (E_Picture2::file_list.empty()) {
        E_Picture2::find_image_files();
    }
}

E_Picture2::~E_Picture2() {
    delete[] this->image;
    delete[] this->work_image;
    delete[] this->work_image_bilinear;
    if (E_Picture2::instance_count() <= 1) {
        E_Picture2::clear_image_files();
    }
    lock_destroy(this->image_lock);
}

static void add_file_callback(const char* file, void*) {
    size_t filename_length = strnlen(file, MAX_PATH);
    char* filename = (char*)calloc(filename_length + 1, sizeof(char));
    strncpy(filename, file, filename_length + 1);
    E_Picture2::file_list.push_back(filename);
}

void E_Picture2::find_image_files() {
    E_Picture2::clear_image_files();
    const int num_extensions = 5;
    const char* extensions[num_extensions] = {".bmp", ".png", ".jpg", ".jpeg", ".gif"};
    find_files_by_extensions(g_path,
                             extensions,
                             num_extensions,
                             add_file_callback,
                             nullptr,
                             0,
                             /*recursive*/ false,
                             /*background*/ false);
}

void E_Picture2::clear_image_files() {
    for (auto file : E_Picture2::file_list) {
        free((/*mutable*/ char*)file);
    }
    E_Picture2::file_list.clear();
}

bool E_Picture2::load_image() {
    if (this->config.image.empty()) {
        return true;
    }
    lock_lock(this->image_lock);
    char filename[MAX_PATH];
    snprintf(filename, MAX_PATH, "%s\\%s", g_path, this->config.image.c_str());
    AVS_Image* tmp_image = image_load(filename);
    if (tmp_image->data == nullptr) {
        this->image = nullptr;
        this->config.error_msg = tmp_image->error;
        lock_unlock(this->image_lock);
        return false;
    }

    // Duplicate first and last row and column to optimize the bilinear algorithm, so it
    // blends with pixels "outside" the image.
    this->iw = tmp_image->w + 2;
    this->ih = tmp_image->h + 2;
    delete[] this->image;
    this->image =
        (pixel_rgb0_8*)new unsigned char[this->iw * this->ih * sizeof(pixel_rgb0_8)];
    for (uint32_t y = 0; y < this->ih; ++y) {
        for (uint32_t x = 0; x < this->iw; ++x) {
            int tmp_index = max(0, min(this->iw - 3, x - 1))
                            + max(0, min(this->ih - 3, y - 1)) * (this->iw - 2);
            this->image[x + y * this->iw] = ((pixel_rgb0_8*)tmp_image->data)[tmp_index];
        }
    }
    image_free(tmp_image);

    this->need_image_refresh = true;
    this->config.error_msg = "";
    lock_unlock(this->image_lock);
    return true;
}

void E_Picture2::refresh_image(uint32_t w, uint32_t h) {
    pixel_rgb0_8* wit;
    uint32_t y;
    lock_lock(this->image_lock);
    delete[] this->work_image;
    delete[] this->work_image_bilinear;
    this->wiw = w;
    this->wih = h;
    this->work_image = new pixel_rgb0_8[w * h];
    this->work_image_bilinear = new pixel_rgb0_8[w * h];

    wit = this->work_image_bilinear;
    for (y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            double px = .5 + (double)(x) * (double)(this->iw - 2) / (double)(w);
            double py = .5 + (double)(y) * (double)(this->ih - 2) / (double)(h);
            int pxi = (int)px;
            int pyi = (int)py;
            double fx = px - (double)pxi;
            double fy = py - (double)pyi;
            auto fxi = (uint8_t)(fx * 256);
            auto fyi = (uint8_t)(fy * 256);

            unsigned int pixel[4];
            pixel[0] = this->image[pxi + pyi * this->iw];
            pixel[1] = this->image[pxi + pyi * this->iw + 1];
            pixel[2] = this->image[pxi + (pyi + 1) * this->iw];
            pixel[3] = this->image[pxi + (pyi + 1) * this->iw + 1];
            *(wit++) = BLEND_ADJ_NOMMX(BLEND_ADJ_NOMMX(pixel[3], pixel[2], fxi),
                                       BLEND_ADJ_NOMMX(pixel[1], pixel[0], fxi),
                                       fyi);
        }
    }

    wit = this->work_image;
    for (y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            *(wit++) = this->image[1 + x * (this->iw - 2) / w
                                   + (1 + y * (this->ih - 2) / h) * this->iw];
        }
    }
    this->need_image_refresh = false;
    lock_unlock(this->image_lock);
}

int E_Picture2::render(char[2][2][576],
                       int is_beat,
                       int* framebuffer,
                       int*,
                       int w,
                       int h) {
    if (this->image == nullptr) {
        return 0;
    }
    uint32_t uw = (uint32_t)w;
    uint32_t uh = (uint32_t)h;
    if (this->need_image_refresh || this->wiw != uw || this->wih != uh
        || this->work_image == nullptr) {
        this->refresh_image(uw, uh);
    }
    pixel_rgb0_8* wit =
        is_beat
            ? (this->config.on_beat_bilinear ? this->work_image_bilinear
                                             : this->work_image)
            : (this->config.bilinear ? this->work_image_bilinear : this->work_image);
    uint32_t wh = uw * uh;
    // TODO [performance]: optimize with 4px at once
    switch (is_beat ? this->config.on_beat_blend_mode : this->config.blend_mode) {
        case P2_BLEND_REPLACE: {
            memcpy(framebuffer, wit, wh * 4);
            break;
        }

        case P2_BLEND_ADDITIVE: {
            for (uint32_t i = 0; i < wh; i++) {
                framebuffer[i] = BLEND(wit[i], framebuffer[i]);
            }
            break;
        }

        case P2_BLEND_MAXIMUM: {
            for (uint32_t i = 0; i < wh; i++) {
                framebuffer[i] = BLEND_MAX(wit[i], framebuffer[i]);
            }
            break;
        }

        case P2_BLEND_5050: {
            for (uint32_t i = 0; i < wh; i++) {
                framebuffer[i] = BLEND_AVG(framebuffer[i], wit[i]);
            }
            break;
        }

        case P2_BLEND_SUB1: {
            for (uint32_t i = 0; i < wh; i++) {
                framebuffer[i] = BLEND_SUB(framebuffer[i], wit[i]);
            }
            break;
        }

        case P2_BLEND_SUB2: {
            for (uint32_t i = 0; i < wh; i++) {
                framebuffer[i] = BLEND_SUB(wit[i], framebuffer[i]);
            }
            break;
        }

        case P2_BLEND_MULTIPLY: {
            for (uint32_t i = 0; i < wh; i++) {
                framebuffer[i] = BLEND_MUL(framebuffer[i], wit[i]);
            }
            break;
        }

        case P2_BLEND_XOR: {
            for (uint32_t i = 0; i < wh; i++) {
                framebuffer[i] = framebuffer[i] ^ wit[i];
            }
            break;
        }

        case P2_BLEND_ADJUSTABLE: {
            int t = is_beat ? (int32_t)this->config.on_beat_adjust_blend
                            : (int32_t)this->config.adjust_blend;
            for (uint32_t i = 0; i < wh; i++) {
                framebuffer[i] = BLEND_ADJ(wit[i], framebuffer[i], t);
            }
#ifdef _MSC_VER  // MSVC asm
            __asm emms;
#else  // _MSC_VER, GCC asm
            __asm__ __volatile__("emms");
#endif
            break;
        }

        case P2_BLEND_MINIMUM: {
            for (uint32_t i = 0; i < wh; i++) {
                framebuffer[i] = BLEND_MIN(framebuffer[i], wit[i]);
            }
            break;
        }
    }

    return 0;
}

// PictureII's legacy save format has a slightly different order of blend modes.
// Translate here and only here for legacy load/save so we can use the common format
// everywhere else.
static uint32_t common_to_legacy_blend_translate[P2_NUM_BLEND_MODES] = {
    0,   // Replace
    1,   // Additive
    2,   // Maximum
    4,   // 50/50
    5,   // Sub1
    6,   // Sub2
    7,   // Multiply
    9,   // Adjustable
    8,   // XOR
    3,   // Minimum
    10,  // Ignore
};

struct picture2_config {
    char image[LEGACY_SAVE_PATH_LEN];
    int32_t output;
    int32_t outputbeat;
    int32_t bilinear;
    int32_t bilinearbeat;
    int32_t adjustable;
    int32_t adjustablebeat;
};

void E_Picture2::load_legacy(unsigned char* data, int len) {
    char* str = (char*)data;
    uint32_t pos = 0;
    E_Picture2::string_nt_load_legacy(
        &str[pos], this->config.image, LEGACY_SAVE_PATH_LEN);
    pos += LEGACY_SAVE_PATH_LEN;
    if (len - pos >= 4) {
        auto legacy_blend_mode = (*(uint32_t*)&data[pos]);
        this->config.blend_mode = P2_BLEND_REPLACE;
        for (int i = 0; i < P2_NUM_BLEND_MODES; ++i) {
            if (common_to_legacy_blend_translate[i] == legacy_blend_mode) {
                this->config.blend_mode = i;
                break;
            }
        }
        pos += 4;
    }
    if (len - pos >= 4) {
        auto legacy_blend_mode = (*(uint32_t*)&data[pos]);
        this->config.on_beat_blend_mode = P2_BLEND_REPLACE;
        for (int i = 0; i < P2_NUM_BLEND_MODES; ++i) {
            if (common_to_legacy_blend_translate[i] == legacy_blend_mode) {
                this->config.on_beat_blend_mode = i;
                break;
            }
        }
        pos += 4;
    }
    if (len - pos >= 4) {
        this->config.bilinear = (*(int32_t*)&data[pos]) != 0;
        pos += 4;
    }
    if (len - pos >= 4) {
        this->config.on_beat_bilinear = (*(int32_t*)&data[pos]) != 0;
        pos += 4;
    }
    if (len - pos >= 4) {
        this->config.adjust_blend = (*(int32_t*)&data[pos]);
        pos += 4;
    }
    if (len - pos >= 4) {
        this->config.on_beat_adjust_blend = (*(int32_t*)&data[pos]);
        pos += 4;
    }
    this->load_image();
}

int E_Picture2::save_legacy(unsigned char* data) {
    char* str = (char*)data;
    int pos = 0;
    uint32_t str_len = 0;
    if (!this->config.image.empty()) {
        str_len = string_nt_save_legacy(this->config.image, str, LEGACY_SAVE_PATH_LEN);
    }
    memset(&data[pos + str_len], '\0', LEGACY_SAVE_PATH_LEN - str_len);
    pos += LEGACY_SAVE_PATH_LEN;
    *(uint32_t*)&data[pos] = common_to_legacy_blend_translate[this->config.blend_mode];
    pos += sizeof(uint32_t);
    *(uint32_t*)&data[pos] =
        common_to_legacy_blend_translate[this->config.on_beat_blend_mode];
    pos += sizeof(uint32_t);
    *(uint32_t*)&data[pos] = this->config.bilinear ? 1 : 0;
    pos += sizeof(uint32_t);
    *(uint32_t*)&data[pos] = this->config.on_beat_bilinear ? 1 : 0;
    pos += sizeof(uint32_t);
    *(uint32_t*)&data[pos] = this->config.adjust_blend;
    pos += sizeof(uint32_t);
    *(uint32_t*)&data[pos] = this->config.on_beat_adjust_blend;
    pos += sizeof(uint32_t);
    return pos;
}

Effect_Info* create_Picture2_Info() { return new Picture2_Info(); }
Effect* create_Picture2(AVS_Instance* avs) { return new E_Picture2(avs); }
void set_Picture2_desc(char* desc) { E_Picture2::set_desc(desc); }
