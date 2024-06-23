#include "audio_in.h"

#include "../platform.h"

#include <math.h>
#include <signal.h>
#include <stdio.h>

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>

struct UserCallbackData {
    avs_audio_input_handler callback;
    void* data;
};

struct LoopData {
    pw_thread_loop* loop;
    pw_stream* stream;
    spa_audio_info format;
    UserCallbackData user_callback_data;
};

static void on_process(void* data) {
    auto loop_data = (LoopData*)data;
    struct pw_buffer* b;
    struct spa_buffer* buf;
    float *samples, max;
    uint32_t c, n, n_channels, n_samples, peak;

    if (loop_data->stream == nullptr) {
        return;
    }

    if ((b = pw_stream_dequeue_buffer(loop_data->stream)) == NULL) {
        return;
    }

    buf = b->buffer;
    if ((samples = (float*)buf->datas[0].data) == NULL) {
        return;
    }

    n_channels = loop_data->format.info.raw.channels;
    n_samples = buf->datas[0].chunk->size / sizeof(float);

    auto user_callback = loop_data->user_callback_data.callback;
    auto user_data = loop_data->user_callback_data.data;
    user_callback(
        user_data, (AudioFrame*)samples, loop_data->format.info.raw.rate, n_samples);

    // for (c = 0; c < loop_data->format.info.raw.channels; c++) {
    //     max = 0.0f;
    //     for (n = c; n < n_samples; n += n_channels) {
    //         max = fmaxf(max, fabsf(samples[n]));
    //     }
    // }

    pw_stream_queue_buffer(loop_data->stream, b);
}

static void on_stream_param_changed(void* data,
                                    uint32_t id,
                                    const struct spa_pod* param) {
    auto loop_data = (LoopData*)data;

    if (param == nullptr || id != SPA_PARAM_Format) {
        return;
    }

    if (spa_format_parse(
            param, &loop_data->format.media_type, &loop_data->format.media_subtype)
        < 0) {
        return;
    }

    if (loop_data->format.media_type != SPA_MEDIA_TYPE_audio
        || loop_data->format.media_subtype != SPA_MEDIA_SUBTYPE_raw) {
        return;
    }

    spa_format_audio_raw_parse(param, &loop_data->format.info.raw);
}

static const struct pw_stream_events stream_events = {
    PW_VERSION_STREAM_EVENTS,
    .param_changed = on_stream_param_changed,
    .process = on_process,
};

AVS_Audio_Input* audio_in_start(avs_audio_input_handler callback, void* data) {
    auto loop_data = (LoopData*)calloc(1, sizeof(LoopData));
    loop_data->user_callback_data.callback = callback;
    loop_data->user_callback_data.data = data;
    const struct spa_pod* params[1];
    uint8_t buffer[1024];
    struct pw_properties* props;
    struct spa_pod_builder builder = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

    pw_init(0, nullptr);
    loop_data->loop = pw_thread_loop_new("my thread", NULL);

    /* If you plan to autoconnect your stream, you need to provide at least
     * media, category and role properties.
     * Pass your events and a user_data pointer as the last arguments. This
     * will inform you about the stream state. The most important event
     * you need to listen to is the process event where you need to produce
     * the data.
     */
    props = pw_properties_new(PW_KEY_MEDIA_TYPE,
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
    pw_properties_set(props, PW_KEY_STREAM_CAPTURE_SINK, "true");
    loop_data->stream = pw_stream_new_simple(pw_thread_loop_get_loop(loop_data->loop),
                                             "Visuals Audio",
                                             props,
                                             &stream_events,
                                             loop_data);

    if (loop_data->stream == nullptr) {
        log_err("failed to create stream");
        return nullptr;
    }

    /* Make one parameter with the supported formats. The SPA_PARAM_EnumFormat
     * id means that this is a format enumeration (of 1 value).
     * We leave the channels and rate empty to accept the native graph
     * rate and channels. */
    auto info = SPA_AUDIO_INFO_RAW_INIT(.format = SPA_AUDIO_FORMAT_F32);
    params[0] = spa_format_audio_raw_build(&builder, SPA_PARAM_EnumFormat, &info);

    pw_stream_connect(
        loop_data->stream,
        PW_DIRECTION_INPUT,
        PW_ID_ANY,
        (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS
                          | PW_STREAM_FLAG_RT_PROCESS),
        params,
        1);

    pw_thread_loop_start(loop_data->loop);
    return loop_data;
}

void audio_in_stop(AVS_Audio_Input* audio_in) {
    auto loop_data = (LoopData*)audio_in;
    // pw_thread_loop_unlock(loop_data->loop);  // we don't lock the thread (yet?)
    pw_thread_loop_stop(loop_data->loop);
    pw_stream_destroy(loop_data->stream);
    pw_thread_loop_destroy(loop_data->loop);
    pw_deinit();
    free(audio_in);
}
