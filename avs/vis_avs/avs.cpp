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

#ifdef __linux__
#define AVS_API __attribute__((visibility("default")))
#else
#define AVS_API
#endif

std::unordered_map<AVS_Handle, AVS_Instance*> g_instances;
const char* g_error = "";

AVS_Instance* get_instance_from_handle(AVS_Handle avs) {
    if (avs > 0) {
        auto search = g_instances.find(avs);
        if (search != g_instances.end()) {
            g_error = NULL;
            return search->second;
        }
    }
    g_error = "Invalid AVS handle";
    return NULL;
}

AVS_API
AVS_Handle avs_init(const char* base_path,
                    AVS_Audio_Source audio_source,
                    AVS_Beat_Source beat_source) {
    make_effect_lib();
    auto new_instance = new AVS_Instance(base_path, audio_source, beat_source);
    if (new_instance->handle == 0) {
        g_error = "AVS handles exhausted (try unloading the library and reloading it)";
        return 0;
    }
    g_instances[new_instance->handle] = new_instance;
    g_error = NULL;
    return new_instance->handle;
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
const uint8_t* avs_preset_get_legacy(AVS_Handle avs, size_t* preset_length_out) {
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
    if (avs == 0) {
        return;
    }
    AVS_Instance* instance = get_instance_from_handle(avs);
    if (instance != NULL) {
        g_instances.erase(avs);
        delete instance;
    }
}

// TODO [build]: These fields should be set by the build system. For now set them here.
#define AVS_VERSION_MAJOR   2
#define AVS_VERSION_MINOR   81
#define AVS_VERSION_PATCH   4  // It's actually "d", but SemVer can't have letters.
#define AVS_VERSION_RC      0
#define AVS_VERSION_COMMIT  "..................."
#define AVS_VERSION_CHANGES "..................."

AVS_API
AVS_Version avs_version() {
    return AVS_Version{AVS_VERSION_MAJOR,
                       AVS_VERSION_MINOR,
                       AVS_VERSION_PATCH,
                       AVS_VERSION_RC,
                       AVS_VERSION_COMMIT,
                       AVS_VERSION_CHANGES};
}
