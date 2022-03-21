#include "c_picture2.h"

#include "r_defs.h"

#include "files.h"
#include "image.h"
#include "pixel_format.h"

#include "../platform.h"
#include "../util.h"  // ssizeof32()

std::vector<char*> C_Picture2::file_list;
unsigned int C_Picture2::instance_count = 0;

C_Picture2::C_Picture2()
    : image(NULL),
      iw(0),
      ih(0),
      work_image(NULL),
      work_image_bilinear(NULL),
      wiw(-1),
      wih(-1),
      need_image_refresh(true) {
    memset(&this->config, 0, sizeof(picture2_config));
    this->config.bilinear = 1;
    this->config.output = OUT_REPLACE;
    this->config.adjustable = 128;
    this->config.bilinearbeat = 1;
    this->config.outputbeat = OUT_REPLACE;
    this->config.adjustablebeat = 128;

    this->instance_count++;
    if (this->file_list.empty()) {
        this->find_image_files();
    }
    this->image_lock = lock_init();
}

C_Picture2::~C_Picture2() {
    delete[] this->image;
    delete[] this->work_image;
    delete[] this->work_image_bilinear;
    this->instance_count--;
    if (this->instance_count <= 0) {
        this->clear_image_files();
    }
    lock_destroy(this->image_lock);
}

static void add_file_callback(const char* file, void* data) {
    C_Picture2* picture2 = (C_Picture2*)data;
    size_t filename_length = strnlen(file, MAX_PATH);
    char* filename = (char*)calloc(filename_length + 1, sizeof(char));
    strncpy(filename, file, filename_length + 1);
    picture2->file_list.push_back(filename);
}

void C_Picture2::find_image_files() {
    this->clear_image_files();
    const int num_extensions = 5;
    char* extensions[num_extensions] = {".bmp", ".png", ".jpg", ".jpeg", ".gif"};
    find_files_by_extensions(g_path,
                             extensions,
                             num_extensions,
                             add_file_callback,
                             this,
                             sizeof(*this),
                             /*recursive*/ true,
                             /*background*/ false);
}

void C_Picture2::clear_image_files() {
    for (auto file : this->file_list) {
        free(file);
    }
    this->file_list.clear();
}

bool C_Picture2::load_image() {
    if (this->config.image[0] == 0) {
        return true;
    }
    lock_lock(this->image_lock);
    char filename[MAX_PATH];
    snprintf(filename, MAX_PATH, "%s\\%s", g_path, this->config.image);
    AVS_Image* tmp_image = image_load(filename);
    if (tmp_image->data == NULL) {
        this->image = NULL;
        lock_unlock(this->image_lock);
        return false;
    }

    // Duplicate first and last row and column to optimize the bilinear algorithm, so it
    // blends with pixels "outside" the image.
    this->iw = tmp_image->w + 2;
    this->ih = tmp_image->h + 2;
    delete[] this->image;
    this->image = (int*)new unsigned char[this->iw * this->ih * sizeof(pixel_rgb0_8)];
    for (int y = 0; y < this->ih; ++y) {
        for (int x = 0; x < this->iw; ++x) {
            int tmp_index = max(0, min(this->iw - 3, x - 1))
                            + max(0, min(this->ih - 3, y - 1)) * (this->iw - 2);
            this->image[x + y * this->iw] = ((pixel_rgb0_8*)tmp_image->data)[tmp_index];
        }
    }
    image_free(tmp_image);

    this->need_image_refresh = true;
    lock_unlock(this->image_lock);
    return true;
}

void C_Picture2::refresh_image(int w, int h) {
    int* wit;
    int y;
    lock_lock(this->image_lock);
    delete[] this->work_image;
    delete[] this->work_image_bilinear;
    this->wiw = w;
    this->wih = h;
    this->work_image = new int[w * h];
    this->work_image_bilinear = new int[w * h];

    wit = this->work_image_bilinear;
    for (y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            double px = .5 + (double)(x) * (double)(this->iw - 2) / (double)(w);
            double py = .5 + (double)(y) * (double)(this->ih - 2) / (double)(h);
            int pxi = (int)(px);
            int pyi = (int)(py);
            double fx = px - (double)pxi;
            double fy = py - (double)pyi;
            unsigned char fxi = (unsigned char)(fx * 256);
            unsigned char fyi = (unsigned char)(fy * 256);

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
        for (int x = 0; x < w; ++x) {
            *(wit++) = this->image[1 + x * (this->iw - 2) / w
                                   + (1 + y * (this->ih - 2) / h) * this->iw];
        }
    }
    this->need_image_refresh = false;
    lock_unlock(this->image_lock);
}

int C_Picture2::render(char[2][2][576],
                       int is_beat,
                       int* framebuffer,
                       int*,
                       int w,
                       int h) {
    if (this->image == NULL) {
        return 0;
    }
    if (this->need_image_refresh || this->wiw != w || this->wih != h
        || this->work_image == NULL) {
        this->refresh_image(w, h);
    }
    int* wit =
        is_beat
            ? (this->config.bilinearbeat ? this->work_image_bilinear : this->work_image)
            : (this->config.bilinear ? this->work_image_bilinear : this->work_image);
    int wh = w * h;
    // TODO [speed]: optimize with 4px at once
    switch (is_beat ? this->config.outputbeat : this->config.output) {
        case OUT_REPLACE: {
            memcpy(framebuffer, wit, wh * 4);
            break;
        }

        case OUT_ADDITIVE: {
            for (int i = 0; i < wh; i++) {
                framebuffer[i] = BLEND(wit[i], framebuffer[i]);
            }
            break;
        }

        case OUT_MAXIMUM: {
            for (int i = 0; i < wh; i++) {
                framebuffer[i] = BLEND_MAX(wit[i], framebuffer[i]);
            }
            break;
        }

        case OUT_5050: {
            for (int i = 0; i < wh; i++) {
                framebuffer[i] = BLEND_AVG(framebuffer[i], wit[i]);
            }
            break;
        }

        case OUT_SUB1: {
            for (int i = 0; i < wh; i++) {
                framebuffer[i] = BLEND_SUB(wit[i], framebuffer[i]);
            }
            break;
        }

        case OUT_SUB2: {
            for (int i = 0; i < wh; i++) {
                framebuffer[i] = BLEND_SUB(framebuffer[i], wit[i]);
            }
            break;
        }

        case OUT_MULTIPLY: {
            for (int i = 0; i < wh; i++) {
                framebuffer[i] = BLEND_MUL(framebuffer[i], wit[i]);
            }
            break;
        }

        case OUT_XOR: {
            for (int i = 0; i < wh; i++) {
                framebuffer[i] = framebuffer[i] ^ wit[i];
            }
            break;
        }

        case OUT_ADJUSTABLE: {
            int t = is_beat ? this->config.adjustablebeat : this->config.adjustable;
            for (int i = 0; i < wh; i++) {
                framebuffer[i] = BLEND_ADJ(framebuffer[i], wit[i], t);
            }
            break;
        }

        case OUT_MINIMUM: {
            for (int i = 0; i < wh; i++) {
                framebuffer[i] = BLEND_MIN(framebuffer[i], wit[i]);
            }
            break;
        }
    }

    return 0;
}

char* C_Picture2::get_desc(void) { return MOD_NAME; }

void C_Picture2::load_config(unsigned char* data, int len) {
    if (len >= ssizeof32(picture2_config))
        memcpy(&this->config, data, ssizeof32(picture2_config));
    else
        memset(&this->config, 0, sizeof(picture2_config));

    load_image();
}

int C_Picture2::save_config(unsigned char* data) {
    memcpy(data, &this->config, sizeof(picture2_config));

    return sizeof(picture2_config);
}

C_RBASE* R_Picture2(char* desc) {
    if (desc) {
        strcpy(desc, MOD_NAME);
        return NULL;
    }
    return (C_RBASE*)new C_Picture2();
}
