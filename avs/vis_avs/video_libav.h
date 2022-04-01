#pragma once

#include "video.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

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
    static decltype(&avformat_alloc_context) avformat_alloc_context;
    static decltype(&avformat_open_input) avformat_open_input;
    static decltype(&avformat_close_input) avformat_close_input;
    static decltype(&av_find_best_stream) av_find_best_stream;
    static decltype(&av_packet_alloc) av_packet_alloc;
    static decltype(&av_frame_alloc) av_frame_alloc;
    static decltype(&av_frame_free) av_frame_free;
    static decltype(&av_packet_free) av_packet_free;
    static decltype(&av_packet_unref) av_packet_unref;
    static decltype(&av_read_frame) av_read_frame;
    static decltype(&av_seek_frame) av_seek_frame;
    static decltype(&av_frame_unref) av_frame_unref;
    static decltype(&avcodec_find_decoder) avcodec_find_decoder;
    static decltype(&avcodec_alloc_context3) avcodec_alloc_context3;
    static decltype(&avcodec_parameters_to_context) avcodec_parameters_to_context;
    static decltype(&avcodec_open2) avcodec_open2;
    static decltype(&avcodec_free_context) avcodec_free_context;
    static decltype(&avcodec_send_packet) avcodec_send_packet;
    static decltype(&avcodec_receive_frame) avcodec_receive_frame;
    static decltype(&sws_scale) sws_scale;
    static decltype(&sws_freeContext) sws_freeContext;
    static decltype(&sws_getContext) sws_getContext;

    static bool load_libav_dlibs();
};
