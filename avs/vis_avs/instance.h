#pragma once

#include "c_transition.h"
#include "e_root.h"

#include "avs.h"
#include "avs_editor.h"
#include "effect.h"
#include "effect_info.h"

#include "../platform.h"

#include <string>

#define MAX_LEGACY_PRESET_FILESIZE_BYTES (1024 * 1024)

class AVS_Instance {
   public:
    AVS_Instance(AVS_Audio_Source audio_source, AVS_Beat_Source beat_source);
    ~AVS_Instance() = default;

    bool render_frame(void* framebuffer,
                      uint64_t time_ms,
                      bool is_beat,
                      size_t width,
                      size_t height,
                      AVS_Pixel_Format pixel_format);
    bool audio_set(int16_t* audio_left,
                   int16_t* audio_right,
                   size_t audio_length,
                   size_t samples_per_second);
    int32_t audio_device_count();
    const char* const* audio_device_names();
    void audio_device_set(int32_t device);
    bool preset_load_file(const char* file_path, bool with_transition = false);
    bool preset_load(std::string& preset, bool with_transition = false);
    bool preset_load_legacy(uint8_t* preset,
                            size_t preset_length,
                            bool with_transition = false);
    void preset_save_file(const char* file_path, bool indent);
    int preset_save_file_legacy(const char* file_path);
    /** Allocate 1 MiB of memory at `data`. Return value is the actual size. */
    size_t preset_save_legacy(uint8_t* data, bool secondary = false);
    const char* preset_save();
    void clear();
    void clear_secondary();
    const char* error_str();
    bool undo();
    bool redo();

    Effect_Info* get_effect_from_handle(AVS_Effect_Handle effect);
    Effect* get_component_from_handle(AVS_Component_Handle component);

    void* get_buffer(size_t w,
                     size_t h,
                     int32_t buffer_num,
                     bool allocate_if_needed = false);

    AVS_Handle handle;

    AVS_Audio_Source audio_source;
    AVS_Beat_Source beat_source;
    const char* error;
    const char* audio_devices[1] = {""};

    E_Root root;
    /** Used for transitioning between presets. */
    E_Root root_secondary;
    std::vector<Effect*> scrap;

    lock_t* render_lock;

   private:
    static constexpr char const* legacy_file_magic = "Nullsoft AVS Preset 0.2\x1a";
    static constexpr size_t num_buffers = 8;
    void* buffers[num_buffers];
    size_t buffers_w[num_buffers];
    size_t buffers_h[num_buffers];
    AVS_Pixel_Format buffers_pixel_format[num_buffers];
};
