#pragma once

#include "e_root.h"

#include "avs.h"
#include "avs_editor.h"
#include "effect.h"
#include "effect_info.h"

class AVS_Instance {
   public:
    AVS_Instance(AVS_Audio_Source audio_source, AVS_Beat_Source beat_source);
    ~AVS_Instance();

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
    bool preset_load_file(const char* file_path);
    bool preset_set(char* preset);
    bool preset_set_legacy(uint8_t* preset, size_t preset_length);
    void preset_save(const char* file_path, bool indent);
    void preset_save_legacy(const char* file_path);
    const char* preset_get();
    void preset_clear();
    const char* error_str();

    Effect_Info* get_effect_from_handle(AVS_Effect_Handle effect);
    Effect* get_component_from_handle(AVS_Component_Handle component);

    AVS_Audio_Source audio_source;
    AVS_Beat_Source beat_source;
    char* error;
    char* audio_devices[1] = {""};

    E_Root root;
    std::vector<Effect*> scrap;
};
