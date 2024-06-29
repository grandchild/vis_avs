#pragma once

#include "video.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#ifdef _WIN32
#define LIBAV_SUFFIX_AVCODEC  "-61"
#define LIBAV_SUFFIX_AVFORMAT "-61"
#define LIBAV_SUFFIX_AVUTIL   "-59"
#define LIBAV_SUFFIX_SWSCALE  "-8"
#elif __linux__
#define LIBAV_SUFFIX_AVCODEC  ".so"
#define LIBAV_SUFFIX_AVFORMAT ".so"
#define LIBAV_SUFFIX_AVUTIL   ".so"
#define LIBAV_SUFFIX_SWSCALE  ".so"
#endif

// Hide libav types in here so that video.h doesn't have to include any libav headers.
struct AVS_Video::LibAV {
    AVFormatContext* demuxer = NULL;
    AVCodecContext* decoder = NULL;
    const AVCodec* codec = NULL;
    AVCodecParameters* codec_params = NULL;
    AVFrame* frame = NULL;
    AVPacket* packet = NULL;
    SwsContext* scaler = NULL;
    bool loaded = false;

    LibAV() : loaded(load_libav_dlibs()){};
    // TODO [feature][bug]: avs_register_exit_cleanup(this->unload_libav_dlibs);

    static const char* load_error;
    static dlib_t* libavutil;
    static dlib_t* libavformat;
    static dlib_t* libavcodec;
    static dlib_t* libswscale;
    static decltype(&avformat_alloc_context) alloc_context;
    static decltype(&avformat_open_input) open_input;
    static decltype(&avformat_close_input) close_input;
    static decltype(&av_find_best_stream) find_best_stream;
    static decltype(&av_packet_alloc) packet_alloc;
    static decltype(&av_frame_alloc) frame_alloc;
    static decltype(&av_frame_free) frame_free;
    static decltype(&av_packet_free) packet_free;
    static decltype(&av_packet_unref) packet_unref;
    static decltype(&av_read_frame) read_frame;
    static decltype(&av_seek_frame) seek_frame;
    static decltype(&av_frame_unref) frame_unref;
    static decltype(&avcodec_find_decoder) find_decoder;
    static decltype(&avcodec_alloc_context3) alloc_context3;
    static decltype(&avcodec_parameters_to_context) parameters_to_context;
    static decltype(&avcodec_open2) open2;
    static decltype(&avcodec_free_context) free_context;
    static decltype(&avcodec_send_packet) send_packet;
    static decltype(&avcodec_receive_frame) receive_frame;
    static decltype(&sws_scale) scale;
    static decltype(&sws_freeContext) freeContext;
    static decltype(&sws_getContext) getContext;

    static bool load_libav_dlibs();
};
