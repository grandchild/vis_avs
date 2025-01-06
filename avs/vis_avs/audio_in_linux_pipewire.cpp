#include "audio_in.h"
#include "audio_libpipewire.h"

#include "../platform.h"

#include <math.h>
#include <signal.h>
#include <stdio.h>

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>

static void on_process(void* data) {
    auto pw = (LibPipewire*)data;
    struct pw_buffer* b;
    struct spa_buffer* buf;
    float *samples, max;
    uint32_t c, n, n_channels, n_samples, peak;

    if (pw->stream == nullptr) {
        return;
    }

    if ((b = pw->stream_dequeue_buffer(pw->stream)) == NULL) {
        return;
    }

    buf = b->buffer;
    if ((samples = (float*)buf->datas[0].data) == NULL) {
        return;
    }

    n_channels = pw->format.info.raw.channels;
    n_samples = buf->datas[0].chunk->size / sizeof(float);

    pw->user_callback(
        pw->user_data, (AudioFrame*)samples, pw->format.info.raw.rate, n_samples);

    // for (c = 0; c < pw->format.info.raw.channels; c++) {
    //     max = 0.0f;
    //     for (n = c; n < n_samples; n += n_channels) {
    //         max = fmaxf(max, fabsf(samples[n]));
    //     }
    // }

    pw->stream_queue_buffer(pw->stream, b);
}

static void on_stream_param_changed(void* data,
                                    uint32_t id,
                                    const struct spa_pod* param) {
    auto pw = (LibPipewire*)data;

    if (param == nullptr || id != SPA_PARAM_Format) {
        return;
    }

    if (spa_format_parse(param, &pw->format.media_type, &pw->format.media_subtype)
        < 0) {
        return;
    }

    if (pw->format.media_type != SPA_MEDIA_TYPE_audio
        || pw->format.media_subtype != SPA_MEDIA_SUBTYPE_raw) {
        return;
    }

    spa_format_audio_raw_parse(param, &pw->format.info.raw);
}

static const struct pw_stream_events stream_events = {
    PW_VERSION_STREAM_EVENTS,
    .param_changed = on_stream_param_changed,
    .process = on_process,
};

AVS_Audio_Input* audio_in_start(avs_audio_input_handler callback, void* data) {
    auto pw = new LibPipewire(callback, data);
    if (!pw->loaded) {
        log_err("failed to load pipewire: %s", pw->load_error.c_str());
        free(pw);
        return nullptr;
    }
    const struct spa_pod* params[1];
    uint8_t buffer[1024];
    struct pw_properties* props;
    struct spa_pod_builder builder = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

    pw->init(0, nullptr);
    pw->loop = pw->thread_loop_new("my thread", NULL);

    /* If you plan to autoconnect your stream, you need to provide at least
     * media, category and role properties.
     * Pass your events and a user_data pointer as the last arguments. This
     * will inform you about the stream state. The most important event
     * you need to listen to is the process event where you need to produce
     * the data.
     */
    props = pw->properties_new(PW_KEY_MEDIA_TYPE,
                               "Audio",
                               PW_KEY_CONFIG_NAME,
                               "client-rt.conf",
                               PW_KEY_MEDIA_CATEGORY,
                               "Capture",
                               PW_KEY_MEDIA_ROLE,
                               "Music",
                               nullptr);
    // uncomment if you want to capture from the sink monitor ports
    // TODO: make input device selectable both from the outside and from the inside
    // pw->properties_set(props, PW_KEY_STREAM_CAPTURE_SINK, "true");
    pw->stream = pw->stream_new_simple(
        pw->thread_loop_get_loop(pw->loop), "Visuals Audio", props, &stream_events, pw);

    if (pw->stream == nullptr) {
        log_err("failed to create stream");
        return nullptr;
    }

    /* Make one parameter with the supported formats. The SPA_PARAM_EnumFormat
     * id means that this is a format enumeration (of 1 value).
     * We leave the channels and rate empty to accept the native graph
     * rate and channels. */
    auto info = SPA_AUDIO_INFO_RAW_INIT(.format = SPA_AUDIO_FORMAT_F32);
    params[0] = spa_format_audio_raw_build(&builder, SPA_PARAM_EnumFormat, &info);

    pw->stream_connect(
        pw->stream,
        PW_DIRECTION_INPUT,
        PW_ID_ANY,
        (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS
                          | PW_STREAM_FLAG_RT_PROCESS),
        params,
        1);

    pw->thread_loop_start(pw->loop);
    return pw;
}

void audio_in_stop(AVS_Audio_Input* audio_in) {
    auto pw = (LibPipewire*)audio_in;
    // pw->thread_loop_unlock(pw->loop);  // we don't lock the thread (yet?)
    pw->thread_loop_stop(pw->loop);
    pw->stream_destroy(pw->stream);
    pw->thread_loop_destroy(pw->loop);
    pw->deinit();
    free(audio_in);
}
