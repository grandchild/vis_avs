#pragma once

#include "audio_in.h"

#include "../platform.h"

#include <string>

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>

struct LibPipewire {
    pw_thread_loop* loop = nullptr;
    pw_stream* stream = nullptr;
    pw_buffer* buffer = nullptr;
    pw_properties* properties = nullptr;
    spa_audio_info format;
    static struct pw_stream_events stream_events;
    bool loaded = false;

    avs_audio_input_handler user_callback;
    void* user_data;

    static decltype(&pw_deinit) deinit;
    static decltype(&pw_init) init;
    static decltype(&pw_properties_new) properties_new;
    static decltype(&pw_properties_set) properties_set;
    static decltype(&pw_stream_connect) stream_connect;
    static decltype(&pw_stream_dequeue_buffer) stream_dequeue_buffer;
    static decltype(&pw_stream_destroy) stream_destroy;
    static decltype(&pw_stream_new_simple) stream_new_simple;
    static decltype(&pw_stream_queue_buffer) stream_queue_buffer;
    static decltype(&pw_thread_loop_destroy) thread_loop_destroy;
    static decltype(&pw_thread_loop_get_loop) thread_loop_get_loop;
    static decltype(&pw_thread_loop_new) thread_loop_new;
    static decltype(&pw_thread_loop_start) thread_loop_start;
    static decltype(&pw_thread_loop_stop) thread_loop_stop;
    static decltype(&pw_thread_loop_unlock) thread_loop_unlock;

    LibPipewire(avs_audio_input_handler callback, void* data)
        : loaded(load_pipewire_dlib()), user_callback(callback), user_data(data) {}
    static bool load_pipewire_dlib();
    static std::string load_error;
    static dlib_t* libpipewire;
};
