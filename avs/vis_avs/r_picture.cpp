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
#include "c_picture.h"

#include "r_defs.h"

#include "files.h"
#include "image.h"

#include <stdlib.h>

std::vector<char*> C_THISCLASS::file_list;
unsigned int C_THISCLASS::instance_count = 0;

C_THISCLASS::C_THISCLASS()
    : enabled(1),
      width(0),
      height(0),
      blend(0),
      blendavg(1),
      adapt(0),
      persist(6),
      persistCount(0),
      ratio(0),
      axis_ratio(0),
      image(""),
      image_data(NULL),
      image_lock(lock_init()) {
    this->instance_count++;
    if (this->file_list.empty()) {
        this->find_image_files();
    }
}

C_THISCLASS::~C_THISCLASS() {
    free(this->image_data);
    this->instance_count--;
    if (this->instance_count == 0) {
        this->clear_image_files();
    }
}

static void add_file_callback(const char* file, void* data) {
    C_THISCLASS* picture = (C_THISCLASS*)data;
    size_t filename_length = strnlen(file, MAX_PATH);
    char* filename = (char*)calloc(filename_length + 1, sizeof(char));
    strncpy(filename, file, filename_length + 1);
    picture->file_list.push_back(filename);
}

void C_THISCLASS::find_image_files() {
    this->clear_image_files();
    const int num_extensions = 5;
    char* extensions[num_extensions] = {".bmp", ".png", ".jpg", ".jpeg", ".gif"};
    find_files_by_extensions(g_path,
                             extensions,
                             num_extensions,
                             add_file_callback,
                             this,
                             sizeof(*this),
                             /*recursive*/ false,
                             /*background*/ false);
}

void C_THISCLASS::clear_image_files() {
    for (auto file : this->file_list) {
        free(file);
    }
    this->file_list.clear();
}

void C_THISCLASS::load_image() {
    char file_path[MAX_PATH];
    snprintf(file_path, MAX_PATH, "%s\\%s", g_path, this->image);
    AVS_image* tmp_image = image_load(file_path, AVS_PXL_FMT_RGB8);
    if (tmp_image->data == NULL || tmp_image->w == 0 || tmp_image->h == 0
        || tmp_image->error != NULL) {
        image_free(tmp_image);
        return;
    }

    int x_start = 0;
    int screen_width = this->width;
    int y_start = 0;
    int screen_height = this->height;
    if (ratio) {
        if (axis_ratio == 0) {
            // fit x
            screen_height = tmp_image->h * this->width / tmp_image->w;
            y_start = (this->height / 2) - (screen_height / 2);
        } else {
            // fit y
            screen_width = tmp_image->w * this->height / tmp_image->h;
            x_start = (this->width / 2) - (screen_width / 2);
        }
    }

    lock(this->image_lock);
    free(this->image_data);
    this->image_data =
        (pixel_rgb8*)malloc(this->width * this->height * sizeof(pixel_rgb8));
    for (int y = 0; y < this->height; y++) {
        for (int x = 0; x < this->width; x++) {
            int source_x = (x - x_start) * tmp_image->w / screen_width;
            int source_y = (y - y_start) * tmp_image->h / screen_height;
            if (source_x < 0 || source_x >= tmp_image->w || source_y < 0
                || source_y >= tmp_image->h) {
                this->image_data[x + y * this->width] = 0;
            } else {
                this->image_data[x + y * this->width] =
                    ((pixel_rgb8*)tmp_image->data)[source_x + source_y * tmp_image->w];
            }
        }
    }
    lock_unlock(this->image_lock);
    image_free(tmp_image);
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
        blend = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        blendavg = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        adapt = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        persist = GET_INT();
        pos += 4;
    }

    char* p = image;
    while (data[pos] && len - pos > 0) *p++ = data[pos++];
    *p = 0;
    pos++;

    if (len - pos >= 4) {
        ratio = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        axis_ratio = GET_INT();
        pos += 4;
    }

    if (*image) {
        this->load_image();
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
    PUT_INT(blend);
    pos += 4;
    PUT_INT(blendavg);
    pos += 4;
    PUT_INT(adapt);
    pos += 4;
    PUT_INT(persist);
    pos += 4;
    strcpy((char*)data + pos, image);
    pos += strlen(image) + 1;
    PUT_INT(ratio);
    pos += 4;
    PUT_INT(axis_ratio);
    pos += 4;
    return pos;
}

// render function
// render should return 0 if it only used framebuffer, or 1 if the new output data is in
// fbout. this is used when you want to do something that you'd otherwise need to make a
// copy of the framebuffer. w and h are the width and height of the screen, in pixels.
// isBeat is 1 if a beat has been detected.
// visdata is in the format of [spectrum:0,wave:1][channel][band].
int C_THISCLASS::render(char[2][2][576],
                        int isBeat,
                        int* framebuffer,
                        int*,
                        int w,
                        int h) {
    if (!enabled) {
        return 0;
    }

    if (this->image_data == NULL || this->width != w || this->height != h) {
        this->width = w;
        this->height = h;
        this->load_image();
    }
    if (this->image_data == NULL) {
        return 0;
    }
    if (isBeat & 0x80000000) {
        return 0;
    }

    if (isBeat) {
        persistCount = persist;
    } else if (persistCount > 0) {
        persistCount--;
    }

    lock(this->image_lock);
    if (blend || (adapt && (isBeat || persistCount))) {
        for (int i = 0; i < w * h; i++) {
            framebuffer[i] = BLEND(framebuffer[i], this->image_data[i]);
        }
    } else if (blendavg || adapt) {
        for (int i = 0; i < w * h; i++) {
            framebuffer[i] = BLEND_AVG(framebuffer[i], this->image_data[i]);
        }
    } else {
        memcpy(framebuffer, this->image_data, w * h * sizeof(pixel_rgb8));
    }
    lock_unlock(this->image_lock);

    return 0;
}

C_RBASE* R_Picture(char* desc)  // creates a new effect object if desc is NULL,
                                // otherwise fills in desc with description
{
    if (desc) {
        strcpy(desc, MOD_NAME);
        return NULL;
    }
    return (C_RBASE*)new C_THISCLASS();
}
