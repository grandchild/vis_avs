#include "audio_libpipewire.h"

#include "../optional_lib_helper.h"

bool LibPipewire::load_pipewire_dlib() {
    if (LibPipewire::libpipewire != nullptr) {
        return true;
    }
    LOAD_LIB(LibPipewire, pipewire, "-0.3.so");
    LOAD_FUNC(LibPipewire, libpipewire, pw, deinit);
    LOAD_FUNC(LibPipewire, libpipewire, pw, init);
    LOAD_FUNC(LibPipewire, libpipewire, pw, properties_new);
    LOAD_FUNC(LibPipewire, libpipewire, pw, properties_set);
    LOAD_FUNC(LibPipewire, libpipewire, pw, stream_connect);
    LOAD_FUNC(LibPipewire, libpipewire, pw, stream_dequeue_buffer);
    LOAD_FUNC(LibPipewire, libpipewire, pw, stream_destroy);
    LOAD_FUNC(LibPipewire, libpipewire, pw, stream_new_simple);
    LOAD_FUNC(LibPipewire, libpipewire, pw, stream_queue_buffer);
    LOAD_FUNC(LibPipewire, libpipewire, pw, thread_loop_destroy);
    LOAD_FUNC(LibPipewire, libpipewire, pw, thread_loop_get_loop);
    LOAD_FUNC(LibPipewire, libpipewire, pw, thread_loop_new);
    LOAD_FUNC(LibPipewire, libpipewire, pw, thread_loop_start);
    LOAD_FUNC(LibPipewire, libpipewire, pw, thread_loop_stop);
    LOAD_FUNC(LibPipewire, libpipewire, pw, thread_loop_unlock);
    return true;
}

std::string LibPipewire::load_error;
dlib_t* LibPipewire::libpipewire = nullptr;
DEFINE_FUNC(LibPipewire, pw, deinit);
DEFINE_FUNC(LibPipewire, pw, init);
DEFINE_FUNC(LibPipewire, pw, properties_new);
DEFINE_FUNC(LibPipewire, pw, properties_set);
DEFINE_FUNC(LibPipewire, pw, stream_connect);
DEFINE_FUNC(LibPipewire, pw, stream_dequeue_buffer);
DEFINE_FUNC(LibPipewire, pw, stream_destroy);
DEFINE_FUNC(LibPipewire, pw, stream_new_simple);
DEFINE_FUNC(LibPipewire, pw, stream_queue_buffer);
DEFINE_FUNC(LibPipewire, pw, thread_loop_destroy);
DEFINE_FUNC(LibPipewire, pw, thread_loop_get_loop);
DEFINE_FUNC(LibPipewire, pw, thread_loop_new);
DEFINE_FUNC(LibPipewire, pw, thread_loop_start);
DEFINE_FUNC(LibPipewire, pw, thread_loop_stop);
DEFINE_FUNC(LibPipewire, pw, thread_loop_unlock);
