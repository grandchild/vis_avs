#ifndef NO_FFMPEG

#include "video_libav.h"

#include "../optional_lib_helper.h"

bool AVS_Video::LibAV::load_libav_dlibs() {
    if (LibAV::libavutil != NULL && LibAV::libavformat != NULL
        && LibAV::libavcodec != NULL && LibAV::libswscale != NULL) {
        return true;
    }
    LOAD_LIB(AVS_Video::LibAV, avutil, LIBAV_SUFFIX_AVUTIL);
    LOAD_LIB(AVS_Video::LibAV, avformat, LIBAV_SUFFIX_AVFORMAT);
    LOAD_LIB(AVS_Video::LibAV, avcodec, LIBAV_SUFFIX_AVCODEC);
    LOAD_LIB(AVS_Video::LibAV, swscale, LIBAV_SUFFIX_SWSCALE);
    LOAD_FUNC(AVS_Video::LibAV, libavformat, avformat, alloc_context);
    LOAD_FUNC(AVS_Video::LibAV, libavformat, avformat, open_input);
    LOAD_FUNC(AVS_Video::LibAV, libavformat, avformat, close_input);
    LOAD_FUNC(AVS_Video::LibAV, libavformat, av, find_best_stream);
    LOAD_FUNC(AVS_Video::LibAV, libavcodec, av, packet_alloc);
    LOAD_FUNC(AVS_Video::LibAV, libavutil, av, frame_alloc);
    LOAD_FUNC(AVS_Video::LibAV, libavutil, av, frame_free);
    LOAD_FUNC(AVS_Video::LibAV, libavcodec, av, packet_free);
    LOAD_FUNC(AVS_Video::LibAV, libavcodec, av, packet_unref);
    LOAD_FUNC(AVS_Video::LibAV, libavformat, av, read_frame);
    LOAD_FUNC(AVS_Video::LibAV, libavformat, av, seek_frame);
    LOAD_FUNC(AVS_Video::LibAV, libavutil, av, frame_unref);
    LOAD_FUNC(AVS_Video::LibAV, libavcodec, avcodec, find_decoder);
    LOAD_FUNC(AVS_Video::LibAV, libavcodec, avcodec, alloc_context3);
    LOAD_FUNC(AVS_Video::LibAV, libavcodec, avcodec, parameters_to_context);
    LOAD_FUNC(AVS_Video::LibAV, libavcodec, avcodec, open2);
    LOAD_FUNC(AVS_Video::LibAV, libavcodec, avcodec, free_context);
    LOAD_FUNC(AVS_Video::LibAV, libavcodec, avcodec, send_packet);
    LOAD_FUNC(AVS_Video::LibAV, libavcodec, avcodec, receive_frame);
    LOAD_FUNC(AVS_Video::LibAV, libswscale, sws, scale);
    LOAD_FUNC(AVS_Video::LibAV, libswscale, sws, freeContext);
    LOAD_FUNC(AVS_Video::LibAV, libswscale, sws, getContext);
    return true;
}

std::string AVS_Video::LibAV::load_error;
dlib_t* AVS_Video::LibAV::libavutil = NULL;
dlib_t* AVS_Video::LibAV::libavformat = NULL;
dlib_t* AVS_Video::LibAV::libavcodec = NULL;
dlib_t* AVS_Video::LibAV::libswscale = NULL;
DEFINE_FUNC(AVS_Video::LibAV, avformat, alloc_context);
DEFINE_FUNC(AVS_Video::LibAV, avformat, open_input);
DEFINE_FUNC(AVS_Video::LibAV, avformat, close_input);
DEFINE_FUNC(AVS_Video::LibAV, av, find_best_stream);
DEFINE_FUNC(AVS_Video::LibAV, av, packet_alloc);
DEFINE_FUNC(AVS_Video::LibAV, av, frame_alloc);
DEFINE_FUNC(AVS_Video::LibAV, av, frame_free);
DEFINE_FUNC(AVS_Video::LibAV, av, packet_free);
DEFINE_FUNC(AVS_Video::LibAV, av, packet_unref);
DEFINE_FUNC(AVS_Video::LibAV, av, read_frame);
DEFINE_FUNC(AVS_Video::LibAV, av, seek_frame);
DEFINE_FUNC(AVS_Video::LibAV, av, frame_unref);
DEFINE_FUNC(AVS_Video::LibAV, avcodec, find_decoder);
DEFINE_FUNC(AVS_Video::LibAV, avcodec, alloc_context3);
DEFINE_FUNC(AVS_Video::LibAV, avcodec, parameters_to_context);
DEFINE_FUNC(AVS_Video::LibAV, avcodec, open2);
DEFINE_FUNC(AVS_Video::LibAV, avcodec, free_context);
DEFINE_FUNC(AVS_Video::LibAV, avcodec, send_packet);
DEFINE_FUNC(AVS_Video::LibAV, avcodec, receive_frame);
DEFINE_FUNC(AVS_Video::LibAV, sws, scale);
DEFINE_FUNC(AVS_Video::LibAV, sws, freeContext);
DEFINE_FUNC(AVS_Video::LibAV, sws, getContext);

#endif
