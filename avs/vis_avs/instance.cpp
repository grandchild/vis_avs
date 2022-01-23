#include "instance.h"

#include <stdio.h>

AVS_Instance::AVS_Instance(AVS_Audio_Source audio_source, AVS_Beat_Source beat_source)
    : audio_source(audio_source), beat_source(beat_source), error("") {}

AVS_Instance::~AVS_Instance(){};

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
void AVS_Instance::audio_device_set(int32_t device) { return; }
bool AVS_Instance::preset_load_file(const char* file_path) { return true; }
bool AVS_Instance::preset_set(char* preset) { return true; }
bool AVS_Instance::preset_set_legacy(uint8_t* preset, size_t preset_length) {
    return true;
}
void AVS_Instance::preset_save(const char* file_path, bool indent) { return; }
void AVS_Instance::preset_save_legacy(const char* file_path) { return; }
const char* AVS_Instance::preset_get() { return ""; }
void AVS_Instance::preset_clear() { this->root = E_Root(); }
const char* AVS_Instance::error_str() { return this->error; }

Effect* AVS_Instance::get_component_from_handle(AVS_Component_Handle component) {
    return this->root.find_by_handle(component);
}
