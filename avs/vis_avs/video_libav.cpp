#ifndef NO_FFMPEG

#include "video_libav.h"

#define LOAD_LIB(LIB, FILENAME_SUFFIX)                                    \
    library_unload(LibAV::lib##LIB);                                      \
    if ((LibAV::lib##LIB = library_load(#LIB FILENAME_SUFFIX)) == NULL) { \
        LibAV::load_error = "error loading " #LIB FILENAME_SUFFIX;        \
        return false;                                                     \
    }

#define LOAD_FUNC(LIB, FUNC)                                               \
    if ((LibAV::FUNC = (decltype(FUNC))library_get(LIB, #FUNC)) == NULL) { \
        LibAV::load_error = #FUNC " not in " #LIB;                         \
        return false;                                                      \
    }

#define DEFINE_FUNC(FUNC) decltype(&FUNC) AVS_Video::LibAV::FUNC

#define STR(x) #x

bool AVS_Video::LibAV::load_libav_dlibs() {
    if (LibAV::libavutil != NULL && LibAV::libavformat != NULL
        && LibAV::libavcodec != NULL && LibAV::libswscale != NULL) {
        return true;
    }
    LOAD_LIB(avutil, "-" STR(LIBAV_VERSION_AVUTIL));
    LOAD_LIB(avformat, "-" STR(LIBAV_VERSION_AVFORMAT));
    LOAD_LIB(avcodec, "-" STR(LIBAV_VERSION_AVCODEC));
    LOAD_LIB(swscale, "-" STR(LIBAV_VERSION_SWSCALE));
    LOAD_FUNC(libavformat, avformat_alloc_context);
    LOAD_FUNC(libavformat, avformat_open_input);
    LOAD_FUNC(libavformat, avformat_close_input);
    LOAD_FUNC(libavformat, av_find_best_stream);
    LOAD_FUNC(libavcodec, av_packet_alloc);
    LOAD_FUNC(libavutil, av_frame_alloc);
    LOAD_FUNC(libavutil, av_frame_free);
    LOAD_FUNC(libavcodec, av_packet_free);
    LOAD_FUNC(libavcodec, av_packet_unref);
    LOAD_FUNC(libavformat, av_read_frame);
    LOAD_FUNC(libavformat, av_seek_frame);
    LOAD_FUNC(libavutil, av_frame_unref);
    LOAD_FUNC(libavcodec, avcodec_find_decoder);
    LOAD_FUNC(libavcodec, avcodec_alloc_context3);
    LOAD_FUNC(libavcodec, avcodec_parameters_to_context);
    LOAD_FUNC(libavcodec, avcodec_open2);
    LOAD_FUNC(libavcodec, avcodec_free_context);
    LOAD_FUNC(libavcodec, avcodec_send_packet);
    LOAD_FUNC(libavcodec, avcodec_receive_frame);
    LOAD_FUNC(libswscale, sws_scale);
    LOAD_FUNC(libswscale, sws_freeContext);
    LOAD_FUNC(libswscale, sws_getContext);
    return true;
}

const char* AVS_Video::LibAV::load_error = NULL;
dlib_t* AVS_Video::LibAV::libavutil = NULL;
dlib_t* AVS_Video::LibAV::libavformat = NULL;
dlib_t* AVS_Video::LibAV::libavcodec = NULL;
dlib_t* AVS_Video::LibAV::libswscale = NULL;
DEFINE_FUNC(avformat_alloc_context);
DEFINE_FUNC(avformat_open_input);
DEFINE_FUNC(avformat_close_input);
DEFINE_FUNC(av_find_best_stream);
DEFINE_FUNC(av_packet_alloc);
DEFINE_FUNC(av_frame_alloc);
DEFINE_FUNC(av_frame_free);
DEFINE_FUNC(av_packet_free);
DEFINE_FUNC(av_packet_unref);
DEFINE_FUNC(av_read_frame);
DEFINE_FUNC(av_seek_frame);
DEFINE_FUNC(av_frame_unref);
DEFINE_FUNC(avcodec_find_decoder);
DEFINE_FUNC(avcodec_alloc_context3);
DEFINE_FUNC(avcodec_parameters_to_context);
DEFINE_FUNC(avcodec_open2);
DEFINE_FUNC(avcodec_free_context);
DEFINE_FUNC(avcodec_send_packet);
DEFINE_FUNC(avcodec_receive_frame);
DEFINE_FUNC(sws_scale);
DEFINE_FUNC(sws_freeContext);
DEFINE_FUNC(sws_getContext);

#endif
