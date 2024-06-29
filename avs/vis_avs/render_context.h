#pragma once

#include "pixel_format.h"

#include <array>

#define AUDIO_BUFFER_LEN 576

struct AudioChannels {
    float left[AUDIO_BUFFER_LEN];
    float right[AUDIO_BUFFER_LEN];
    float center[AUDIO_BUFFER_LEN];
    void average_center() {
        for (int i = 0; i < AUDIO_BUFFER_LEN; ++i) {
            this->center[i] = (this->left[i] + this->right[i]) / 2.0f;
        }
    }
};

struct AudioData {
    AudioChannels osc{};
    AudioChannels spec{};
    bool is_beat = false;
    void to_legacy_visdata(char visdata[2][2][AUDIO_BUFFER_LEN]);
};

struct Buffer {
    size_t w = 0;
    size_t h = 0;
    AVS_Pixel_Format pixel_format = AVS_PIXEL_RGB0_8;
    void* data = nullptr;

    Buffer(size_t w,
           size_t h,
           AVS_Pixel_Format pixel_format,
           void* external_buffer = nullptr);
    ~Buffer();
    Buffer(const Buffer& other);
    Buffer& operator=(const Buffer& other);
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

   private:
    void swap(Buffer& other);
    /**
     * Does this buffer manage its own memory or does it refer to memory from the
     * outside.
     */
    bool owns_data = true;
};

struct RenderContext {
    Buffer framebuffers[2];
    size_t w = 0;
    size_t h = 0;
    AVS_Pixel_Format pixel_format = AVS_PIXEL_RGB0_8;
    std::array<Buffer, 8>& global_buffers;
    AudioData audio;
    bool is_preinit = false;
    bool needs_final_fb_copy = false;

    RenderContext(size_t w,
                  size_t h,
                  AVS_Pixel_Format pixel_format,
                  std::array<Buffer, 8>& global_buffers,
                  void* external_buffer = nullptr);
    void swap_framebuffers();
    void copy_secondary_to_output_framebuffer_if_needed();
};
