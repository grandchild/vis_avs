#include "e_root.h"

#include "avs.h"
#include "effect_library.h"
#include "handles.h"
#include "instance.h"
#include "pixel_format.h"
#include "render_context.h"

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
      transition(this),
      render_lock(lock_init()) {
    make_effect_lib();
}

bool AVS_Instance::render_frame(void* framebuffer,
                                uint64_t time_ms,
                                bool is_beat,
                                size_t width,
                                size_t height,
                                AVS_Pixel_Format pixel_format) {
    this->init_global_buffers_if_needed(width, height, pixel_format);

    auto render_context =
        RenderContext(pixel_format, width, height, *this->global_buffers, framebuffer);
    this->root.render_with_context(render_context);

    // char visdata[2][2][AUDIO_BUFFER_LEN];
    // this->root.render(
    //     visdata, is_beat, (int*)framebuffer, (int*)framebuffer, width, height);

    return true;
}

void AVS_Instance::init_global_buffers_if_needed(size_t width,
                                                 size_t height,
                                                 AVS_Pixel_Format pixel_format) {
    if (this->global_buffers != nullptr
        && ((*this->global_buffers)[0].w != width
            || (*this->global_buffers)[0].h != height
            || (*this->global_buffers)[0].pixel_format != pixel_format)) {
        delete this->global_buffers;
        this->global_buffers = nullptr;
    }
    if (this->global_buffers == nullptr) {
        this->global_buffers = new std::array<Buffer, AVS_Instance::num_global_buffers>{
            Buffer(width, height, pixel_format),
            Buffer(width, height, pixel_format),
            Buffer(width, height, pixel_format),
            Buffer(width, height, pixel_format),
            Buffer(width, height, pixel_format),
            Buffer(width, height, pixel_format),
            Buffer(width, height, pixel_format),
            Buffer(width, height, pixel_format)};
    }
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
        success = this->preset_load_legacy(data, len, with_transition);
        if (!success) {
            std::string preset_string((char*)data, len);
            success = this->preset_load(preset_string, with_transition);
        }
        free(data);
    }
    return success;
}

bool AVS_Instance::preset_load(const std::string& preset, bool with_transition) {
    try {
        auto preset_root = json::parse(preset);
        try {
            this->root_secondary.load(preset_root);
        } catch (const std::exception& e) {
            log_err("error loading preset: %s", e.what());
        }
        lock_lock(this->render_lock);
        std::swap(this->root, this->root_secondary);
        lock_unlock(this->render_lock);
        this->root.print_tree();
    } catch (const std::exception& e) {
        log_err("error loading: %s", e.what());
        lock_unlock(this->render_lock);
        return false;
    }
    return true;
}

bool AVS_Instance::preset_load_legacy(const uint8_t* preset,
                                      size_t preset_length,
                                      bool with_transition) {
    auto file_magic_length = strlen(AVS_Instance::legacy_file_magic);
    if (preset_length > file_magic_length + 2
        && !memcmp(preset, AVS_Instance::legacy_file_magic, file_magic_length - 2)
        && preset[file_magic_length - 2] >= '1' && preset[file_magic_length - 2] <= '2'
        && preset[file_magic_length - 1] == '\x1a') {
        //                               trustmebro! (i.e. TODO: make data param const)
        this->root_secondary.load_legacy((unsigned char*)preset + file_magic_length,
                                         (int)(preset_length - file_magic_length));
        if (with_transition) {
            // unimplemented!
            // this->transition.do_transition();
        }
        lock_lock(this->render_lock);
        std::swap(this->root, this->root_secondary);
        lock_unlock(this->render_lock);
        this->root.print_tree();
        return true;
    }
    return false;
}

bool AVS_Instance::preset_save_file(const char* file_path, bool indent) {
    auto preset_str = this->preset_save(indent);
    FILE* fp = fopen(file_path, "wb");
    if (fp != nullptr) {
        fwrite(preset_str, 1, strlen(preset_str), fp);
        fclose(fp);
        return true;
    }
    return false;
}

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

const char* AVS_Instance::preset_save(bool indent) {
    json j;
    j["preset format version"] = "0";
    j["avs version"] = "2.81.4";
    j.update(this->root.save());
    j.erase("effect");
    this->preset_save_buffer = j.dump(4) + "\n";
    return this->preset_save_buffer.c_str();
}

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
        pos += file_magic_length;
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

void* AVS_Instance::get_buffer(size_t, size_t, int32_t buffer_num, bool) {
    if (buffer_num >= 0 && (uint32_t)buffer_num <= AVS_Instance::num_global_buffers) {
        return (*this->global_buffers)[buffer_num].data;
    }
    return nullptr;
}
