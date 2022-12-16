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
#include "e_avi.h"

#include "r_defs.h"

#include "files.h"

#include "../util.h"

#include <stdlib.h>

constexpr Parameter AVI_Info::parameters[];
std::vector<std::string> E_AVI::filenames;
const char** E_AVI::c_filenames;

void AVI_Info::on_file_change(Effect* component,
                              const Parameter*,
                              const std::vector<int64_t>&) {
    E_AVI* avi = (E_AVI*)component;
    avi->load_file();
}

const char* const* AVI_Info::video_files(int64_t* length_out) {
    *length_out = E_AVI::filenames.size();
    auto new_list = (const char**)malloc(E_AVI::filenames.size() * sizeof(char*));
    for (size_t i = 0; i < E_AVI::filenames.size(); i++) {
        new_list[i] = E_AVI::filenames[i].c_str();
    }
    auto old_list = E_AVI::c_filenames;
    E_AVI::c_filenames = new_list;
    free(old_list);
    return E_AVI::c_filenames;
}

E_AVI::E_AVI()
    : loaded(false), frame_index(0), last_time(0), video_file_lock(lock_init()) {
    this->find_video_files();
}

E_AVI::~E_AVI() { close_file(); }

static void add_file_callback(const char* file, void* data) {
    ((E_AVI*)data)->filenames.push_back(file);
}

void E_AVI::find_video_files() {
    this->clear_video_files();
    const int num_extensions = 10;
    const char* extensions[num_extensions] = {
        ".avi",
        ".mp4",
        ".mkv",
        ".mpeg4",
        ".mov",
        ".webm",
        ".mpg",
        ".divx",
        ".m2ts",
        ".wmv",
    };
    find_files_by_extensions(g_path,
                             extensions,
                             num_extensions,
                             add_file_callback,
                             this,
                             sizeof(*this),
                             /*recursive*/ false,
                             /*background*/ false);
}

void E_AVI::clear_video_files() { this->filenames.clear(); }

void E_AVI::load_file() {
    char pathfile[MAX_PATH];
    lock_lock(this->video_file_lock);

    if (this->loaded) {
        close_file();
    }

    sprintf(pathfile, "%s\\%s", g_path, this->config.filename.c_str());

    this->video = new AVS_Video(pathfile, 32, 0);
    if (this->video != NULL && this->video->error == NULL) {
        this->loaded = true;
    }
    lock_unlock(this->video_file_lock);
}

void E_AVI::close_file() {
    if (this->loaded) {
        lock_lock(this->video_file_lock);
        this->loaded = false;
        delete this->video;
        lock_unlock(this->video_file_lock);
    }
}

int E_AVI::render(char[2][2][576],
                  int is_beat,
                  int* framebuffer,
                  int* fbout,
                  int w,
                  int h) {
    int *p, *d;
    int i, j;
    int persist_count = 0;

    if (!this->loaded || !this->enabled || this->video->length <= 0
        || is_beat & 0x80000000) {
        return 0;
    }

    if ((this->last_time + this->config.playback_speed) <= (int64_t)timer_ms()) {
        this->frame_index += 1;
        if (this->frame_index >= this->video->length) {
            this->frame_index = 0;
        }
        this->last_time = timer_ms();
    }
    lock_lock(this->video_file_lock);
    auto frame = this->video->get_frame(this->frame_index,
                                        w,
                                        h,
                                        AVS_PIXEL_RGB0_8,
                                        (AVS_Video::Scale)this->config.resampling_mode);
    if (frame == NULL || frame->data == NULL) {
        return 0;
    }
    auto copy_success = this->video->copy(frame, fbout, AVS_PIXEL_RGB0_8);
    lock_unlock(this->video_file_lock);
    if (!copy_success) {
        return 0;
    }

    if (is_beat) {
        persist_count = this->config.on_beat_persist;
    } else if (persist_count > 0) {
        persist_count--;
    }

    p = fbout;
    d = framebuffer + w * (h - 1);
    if (this->config.blend_mode == AVI_BLEND_ADD
        || (this->config.blend_mode == AVI_BLEND_FIFTY_FIFTY_ONBEAT_ADD
            && (is_beat || persist_count > 0))) {
        for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
                *d = BLEND(*p, *d);
                d++;
                p++;
            }
            d -= w * 2;
        }
    } else if (this->config.blend_mode == AVI_BLEND_FIFTY_FIFTY
               || this->config.blend_mode == AVI_BLEND_FIFTY_FIFTY_ONBEAT_ADD) {
        for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
                *d = BLEND_AVG(*p, *d);
                d++;
                p++;
            }
            d -= w * 2;
        }
    } else {  // AVI_BLEND_REPLACE
        for (i = 0; i < h; i++) {
            memcpy(d, p, w * 4);
            p += w;
            d -= w;
        }
    }
    return 0;
}

#define GET_INT() \
    (data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24))
void E_AVI::load_legacy(unsigned char* data, int len) {
    int32_t blend_add = 0;
    int32_t blend_5050 = 0;
    int32_t blend_5050_onbeat_add = 0;
    int pos = 0;
    if (len - pos >= 4) {
        this->enabled = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        blend_add = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        blend_5050 = GET_INT();
        pos += 4;
    }
    char* str_data = (char*)data;
    pos += string_nt_load_legacy(&str_data[pos], this->config.filename, MAX_PATH);
    if (len - pos >= 4) {
        blend_5050_onbeat_add = GET_INT();
        pos += 4;
    }
    if (blend_add > 0) {
        this->config.blend_mode = AVI_BLEND_ADD;
    } else if (blend_5050_onbeat_add > 0) {
        this->config.blend_mode = AVI_BLEND_FIFTY_FIFTY_ONBEAT_ADD;
    } else if (blend_5050 > 0) {
        this->config.blend_mode = AVI_BLEND_FIFTY_FIFTY;
    } else {
        this->config.blend_mode = AVI_BLEND_REPLACE;
    }
    if (len - pos >= 4) {
        this->config.on_beat_persist = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        this->config.playback_speed = GET_INT();
        pos += 4;
    }

    if (!this->config.filename.empty()) {
        this->load_file();
    }
}

#define PUT_INT(y)                   \
    data[pos] = (y)&255;             \
    data[pos + 1] = (y >> 8) & 255;  \
    data[pos + 2] = (y >> 16) & 255; \
    data[pos + 3] = (y >> 24) & 255
int E_AVI::save_legacy(unsigned char* data) {
    int pos = 0;
    PUT_INT(this->enabled);
    pos += 4;
    PUT_INT(this->config.blend_mode == AVI_BLEND_ADD ? 1 : 0);
    pos += 4;
    PUT_INT(this->config.blend_mode == AVI_BLEND_FIFTY_FIFTY ? 1 : 0);
    pos += 4;
    char* str_data = (char*)data;
    pos += this->string_nt_save_legacy(this->config.filename, &str_data[pos], MAX_PATH);
    PUT_INT(this->config.blend_mode == AVI_BLEND_FIFTY_FIFTY_ONBEAT_ADD ? 1 : 0);
    pos += 4;
    PUT_INT(this->config.on_beat_persist);
    pos += 4;
    PUT_INT(this->config.playback_speed);
    pos += 4;
    return pos;
}

Effect_Info* create_AVI_Info() { return new AVI_Info(); }
Effect* create_AVI() { return new E_AVI(); }
void set_AVI_desc(char* desc) { E_AVI::set_desc(desc); }
