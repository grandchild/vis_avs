#pragma once

#include "c_transition.h"
#include "e_root.h"

#include "audio.h"
#include "avs.h"
#include "avs_editor.h"
#include "effect.h"
#include "effect_info.h"
#include "render_context.h"

#include "../platform.h"

#include <string>

class AVS_Instance {
   public:
    AVS_Instance(const char* base_path,
                 AVS_Audio_Source audio_source,
                 AVS_Beat_Source beat_source);
    ~AVS_Instance();

    bool render_frame(void* framebuffer,
                      int64_t time_in_ms,
                      bool is_beat,
                      size_t width,
                      size_t height,
                      AVS_Pixel_Format pixel_format);
    int32_t audio_set(const float* audio_left,
                      const float* audio_right,
                      size_t audio_length,
                      size_t samples_per_second,
                      int64_t end_time_ms);
    int32_t audio_device_count();
    const char* const* audio_device_names();
    void audio_device_set(int32_t device);
    bool preset_load_file(const char* file_path, bool with_transition = false);
    bool preset_load(const std::string& preset, bool with_transition = false);
    bool preset_load_legacy(const uint8_t* preset,
                            size_t preset_length,
                            bool with_transition = false);
    bool preset_save_file(const char* file_path,
                          bool as_remix = false,
                          bool indent = false);
    int preset_save_file_legacy(const char* file_path);
    /** Allocate 1 MiB of memory at `data`. Return value is the actual size. */
    const uint8_t* preset_save_legacy(size_t* preset_length_out,
                                      bool secondary = false);
    const char* preset_save(bool as_remix = false, bool indent = false);
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
    void init_global_buffers_if_needed(size_t width,
                                       size_t height,
                                       AVS_Pixel_Format pixel_format);

    void update_time(int64_t time_in_ms);

    AVS_Handle handle;

    std::string base_path;
    AVS_Audio_Source audio_source;
    AVS_Beat_Source beat_source;
    Audio audio;
    std::string error;
    const char* audio_devices[1] = {""};

    E_Root root;
    /** Used for transitioning between presets. */
    E_Root root_secondary;
    std::vector<Effect*> scrap;
    Transition transition;

    lock_t* render_lock;

   private:
    static constexpr char const* legacy_file_magic = "Nullsoft AVS Preset 0.2\x1a";
    static constexpr size_t num_global_buffers = 8;
    std::array<Buffer, num_global_buffers>* global_buffers = nullptr;
    std::string preset_save_buffer;
    uint8_t* preset_legacy_save_buffer = nullptr;

    enum AVS_TimeMode {
        AVS_TIME_MODE_UNKNOWN = -1,
        AVS_TIME_MODE_REALTIME = 0,
        AVS_TIME_MODE_VIDEO = 1,
    };
    int64_t current_time_in_ms = -1;
    int last_time_mode = AVS_TIME_MODE_UNKNOWN;
    int64_t time_mode_switch_offset = 0;
};
