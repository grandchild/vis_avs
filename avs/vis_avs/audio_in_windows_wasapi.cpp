#include "audio_in.h"

AVS_Audio_Input* audio_in_start(avs_audio_input_handler callback, void* data) {
    (void)callback;
    (void)data;
    return nullptr;
}

void audio_in_stop(AVS_Audio_Input* audio_in) { (void)audio_in; }
