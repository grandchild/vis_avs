#include "e_root.h"

#include "effect_library.h"
#include "handles.h"
#include "instance.h"
#include "pixel_format.h"

#include <cstdio>

AVS_Instance::AVS_Instance(const char* base_path,
                           AVS_Audio_Source audio_source,
                           AVS_Beat_Source beat_source)
    : handle(h_instances.get()),
      base_path(base_path),
      audio_source(audio_source),
      beat_source(beat_source),
      error(""),
      root(this),
      root_secondary(this),
      render_lock(lock_init()) {
    for (auto& pixel_format : this->buffers_pixel_format) {
        pixel_format = AVS_PIXEL_RGB0_8;
    }
    make_effect_lib();
}

bool AVS_Instance::render_frame(void* framebuffer,
                                uint64_t time_ms,
                                bool is_beat,
                                size_t width,
                                size_t height,
                                AVS_Pixel_Format pixel_format) {
    return true;
}

bool AVS_Instance::audio_set(int16_t* audio_left,
                             int16_t* audio_right,
                             size_t audio_length,
                             size_t samples_per_second) {
    return this->audio_source == AVS_AUDIO_EXTERNAL;
}

int32_t AVS_Instance::audio_device_count() { return 1; }

const char* const* AVS_Instance::audio_device_names() { return this->audio_devices; }

void AVS_Instance::audio_device_set(int32_t device) {}

bool AVS_Instance::preset_load_file(const char* file_path, bool with_transition) {
    bool success = false;
    FILE* fp = fopen(file_path, "rb");
    if (fp != nullptr) {
        fseek(fp, 0, SEEK_END);
        size_t len = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        if (len > MAX_LEGACY_PRESET_FILESIZE_BYTES) {
            len = MAX_LEGACY_PRESET_FILESIZE_BYTES;
        }
        auto data = (uint8_t*)calloc(len, 1);
        if (!fread(data, 1, len, fp)) {
            len = 0;
        }
        fclose(fp);
        auto file_magic_length = strlen(AVS_Instance::legacy_file_magic);
        if (len > file_magic_length + 2
            && !memcmp(data, AVS_Instance::legacy_file_magic, file_magic_length - 2)
            && data[file_magic_length - 2] >= '1' && data[file_magic_length - 2] <= '2'
            && data[file_magic_length - 1] == '\x1a') {
            success = this->preset_load_legacy(
                data + file_magic_length, len - file_magic_length, with_transition);
        } else {
            auto preset_string = std::string((char*)data, len);
            success = this->preset_load(preset_string, with_transition);
        }
        free(data);
    }
    return success;
}

bool AVS_Instance::preset_load(std::string& preset, bool with_transition) {
    return true;
}

bool AVS_Instance::preset_load_legacy(uint8_t* preset,
                                      size_t preset_length,
                                      bool with_transition) {
    E_Root& load_root = with_transition ? this->root_secondary : this->root;
    lock_lock(this->render_lock);
    load_root = E_Root(this);
    load_root.load_legacy(preset, (int)(preset_length));
    lock_unlock(this->render_lock);
    return true;
}

void AVS_Instance::preset_save_file(const char* file_path, bool indent) {}

int AVS_Instance::preset_save_file_legacy(const char* file_path) {
    int result = -1;
    size_t size = 0;
    auto data = this->preset_save_legacy(&size);
    if (size < MAX_LEGACY_PRESET_FILESIZE_BYTES) {
        FILE* fp = fopen(file_path, "wb");
        if (fp != nullptr) {
            result = 0;
            fwrite(data, 1, size, fp);
            fclose(fp);
        } else {
            result = 2;
        }
    } else {
        result = 1;
    }
    return result;
}

const char* AVS_Instance::preset_save() { return ""; }

const uint8_t* AVS_Instance::preset_save_legacy(size_t* preset_length_out,
                                                bool secondary) {
    if (this->preset_legacy_save_buffer == nullptr) {
        this->preset_legacy_save_buffer =
            (uint8_t*)calloc(MAX_LEGACY_PRESET_FILESIZE_BYTES, sizeof(uint8_t));
    }
    size_t pos = 0;
    if (this->preset_legacy_save_buffer != nullptr && preset_length_out != nullptr) {
        auto file_magic_length = strlen(AVS_Instance::legacy_file_magic);
        memcpy(this->preset_legacy_save_buffer,
               AVS_Instance::legacy_file_magic,
               file_magic_length);
        auto legacy_preset_content =
            this->preset_legacy_save_buffer + file_magic_length;
        lock_lock(this->render_lock);
        if (secondary) {
            pos += this->root_secondary.save_legacy(legacy_preset_content);
        } else {
            pos += this->root.save_legacy(legacy_preset_content);
        }
        lock_unlock(this->render_lock);
    }
    *preset_length_out = pos;
    return this->preset_legacy_save_buffer;
}

void AVS_Instance::clear() { this->root = E_Root(this); }
void AVS_Instance::clear_secondary() { this->root_secondary = E_Root(this); }

const char* AVS_Instance::error_str() { return this->error; }

bool AVS_Instance::undo() { return false; }
bool AVS_Instance::redo() { return false; }

Effect* AVS_Instance::get_component_from_handle(AVS_Component_Handle component) {
    return this->root.find_by_handle(component);
}

void* AVS_Instance::get_buffer(size_t w,
                               size_t h,
                               int32_t buffer_num,
                               bool allocate_if_needed) {
    if (buffer_num < 0 || buffer_num >= NBUF) {
        return nullptr;
    }
    if (w == 0 || h == 0) {
        return nullptr;
    }
    void*& buffer = this->buffers[buffer_num];
    size_t& buffer_w = this->buffers_w[buffer_num];
    size_t& buffer_h = this->buffers_h[buffer_num];
    if (buffer == nullptr || buffer_w < w || buffer_h < h) {
        if (buffer != nullptr) {
            free(buffer);
        }
        if (allocate_if_needed) {
            buffer = calloc(w * h, pixel_size(this->buffers_pixel_format[buffer_num]));
            buffer_w = w;
            buffer_h = h;
        } else {
            buffer = nullptr;
            buffer_w = 0;
            buffer_h = 0;
        }
        return buffer;
    }
    return buffer;
}
