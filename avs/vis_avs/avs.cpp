#include "avs.h"

#include "avs_editor.h"
#include "avs_internal.h"
#include "effect_library.h"
#include "handles.h"
#include "instance.h"

#include <stdio.h>
#include <stdlib.h>
#include <unordered_map>
#include <vector>

#ifdef WIN32
#define AVS_API __declspec(dllexport)
#elif defined(__linux__)
#define AVS_API __attribute__((visibility("default")))
#endif

std::unordered_map<AVS_Handle, AVS_Instance*> g_instances;
char* g_error = "";
#define NUM_EFFECTS 2

AVS_Instance* get_instance_from_handle(AVS_Handle avs) {
    if (avs > 0) {
        auto search = g_instances.find(avs);
        if (search != g_instances.end()) {
            return search->second;
        }
    }
    g_error = "Invalid AVS handle";
    return NULL;
}

AVS_API
AVS_Handle avs_init(AVS_Audio_Source audio_source, AVS_Beat_Source beat_source) {
    make_effect_lib();
    AVS_Handle new_handle = h_instances.get();
    if (new_handle == 0) {
        g_error = "AVS handles exhausted (try unloading the library and reloading it)";
        return 0;
    }
    g_instances[new_handle] = new AVS_Instance(audio_source, beat_source);
    return new_handle;
}

AVS_API
bool avs_render_frame(AVS_Handle avs,
                      void* framebuffer,
                      size_t width,
                      size_t height,
                      uint64_t time_ms,
                      bool is_beat,
                      AVS_Pixel_Format pixel_format) {
    return true;
}

AVS_API
int32_t avs_audio_set(AVS_Handle avs,
                      const int16_t* left,
                      const int16_t* right,
                      size_t audio_length,
                      size_t samples_per_second) {
    return -1;
}

AVS_API
int32_t avs_audio_device_count(AVS_Handle avs) { return 0; }
AVS_API
const char* const* avs_audio_device_names(AVS_Handle avs) { return NULL; }
AVS_API
bool avs_audio_device_set(AVS_Handle avs, int32_t device) { return false; }

AVS_API
bool avs_preset_load(AVS_Handle avs, const char* file_path) { return false; }
AVS_API
bool avs_preset_set(AVS_Handle avs, const char* preset) { return false; }
AVS_API
bool avs_preset_save(AVS_Handle avs, const char* file_path, bool indent) {
    return false;
}
AVS_API
const char* avs_preset_get(AVS_Handle avs) { return NULL; }
AVS_API
bool avs_preset_set_legacy(AVS_Handle avs,
                           const uint8_t* preset,
                           size_t preset_length) {
    return false;
}
AVS_API
uint8_t* avs_preset_get_legacy(AVS_Handle avs, size_t preset_length_out) {
    return NULL;
}
AVS_API
bool avs_preset_save_legacy(AVS_Handle avs, const char* file_path) { return false; }

AVS_API
const char* avs_error_str(AVS_Handle avs) {
    AVS_Instance* instance = get_instance_from_handle(avs);
    if (instance == NULL) {
        return g_error;
    } else {
        return instance->error_str();
    }
}

AVS_API
void avs_free(AVS_Handle avs) {
    AVS_Instance* instance = get_instance_from_handle(avs);
    if (instance != NULL) {
        g_instances.erase(avs);
        delete instance;
    }
}
