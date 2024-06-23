#pragma once

#include "audio_in.h"
#include "render_context.h"

#include "../platform.h"

#include <cstdint>
#include <map>
#include <tuple>
#include <vector>

class FFT;  // in "../3rdparty/md_fft.h"

class Audio {
   private:
    struct RingIter;

   public:
    explicit Audio(size_t ring_buffer_length = 1024);
    ~Audio();
    int32_t set(const float* audio_left,
                const float* audio_right,
                size_t audio_length,
                size_t samples_per_second,
                int64_t end_time_samples);
    void get(AudioData& audio_data, int64_t until_time_samples = 0);
    static void capture_handler(void* data,
                                AudioFrame* audio,
                                size_t samples_per_second,
                                uint32_t num_samples);
    void audio_in_start();
    void audio_in_stop();

   private:
    int32_t samples_remaining(int64_t relative_to) const;

    lock_t* lock = nullptr;
    AVS_Audio_Input* audio_in = nullptr;
    /** audio data ring buffer */
    std::vector<AudioFrame> buffer;
    /** index of the latest audio sample */
    size_t write_head;
    int64_t latest_sample_time;
    size_t samples_per_second;
    FFT* fft;
};
