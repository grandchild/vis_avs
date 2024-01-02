#pragma once

#include "pixel_format.h"

#include <algorithm>  // std::swap
#include <array>
#include <cstdlib>
#include <cstring>

#define AUDIO_BUFFER_LEN 576

template <typename T>
struct AudioChannels {
    T left[AUDIO_BUFFER_LEN];
    T right[AUDIO_BUFFER_LEN];
    T center[AUDIO_BUFFER_LEN];
    static T avg(T l, T r);
    void average_center() {
        for (int i = 0; i < AUDIO_BUFFER_LEN; ++i) {
            this->center[i] = avg(this->left[i], this->right[i]);
        }
    }
};

struct AudioData {
    AudioChannels<int16_t> osc{};
    AudioChannels<uint16_t> spec{};
    bool is_beat = false;
    void to_legacy_visdata(char visdata[2][2][AUDIO_BUFFER_LEN]) {
        for (int i = 0; i < AUDIO_BUFFER_LEN; ++i) {
            //                         .-- high 8bits of sample --.
            visdata[1 /*osc*/][0][i] = ((int8_t*)(&osc.left[i]))[1];
            visdata[1 /*osc*/][1][i] = ((int8_t*)(&osc.right[i]))[1];
        }
        for (int i = 0; i < AUDIO_BUFFER_LEN; ++i) {
            visdata[0 /*spec*/][0][i] = ((int8_t*)(&spec.left[i]))[1];
            visdata[0 /*spec*/][1][i] = ((int8_t*)(&spec.right[i]))[1];
        }
    }
};

struct Buffer {
    size_t w = 0;
    size_t h = 0;
    AVS_Pixel_Format pixel_format = AVS_PIXEL_RGB0_8;
    void* data = nullptr;

    Buffer(size_t w,
           size_t h,
           AVS_Pixel_Format pixel_format,
           void* external_buffer = nullptr)
        : w(w), h(h), pixel_format(pixel_format), data(external_buffer) {
        if (this->data == nullptr) {
            this->data = malloc(w * h * pixel_size(this->pixel_format));
        } else {
            this->owns_data = false;
        }
    }
    ~Buffer() {
        if (this->owns_data) {
            free(this->data);
        }
    }
    Buffer(const Buffer& other)
        : w(other.w),
          h(other.h),
          pixel_format(other.pixel_format),
          data(malloc(other.w * other.h * pixel_size(other.pixel_format))) {
        memcpy(
            this->data, other.data, other.w * other.h * pixel_size(this->pixel_format));
    }
    Buffer& operator=(const Buffer& other) {
        if (this == &other) {
            return *this;
        }
        if (this->owns_data) {
            free(this->data);
            this->data = malloc(other.w * other.h * pixel_size(other.pixel_format));
        }
        // TODO [bug][feature]: Else, resize incoming framebuffer to target if of
        //                      different size/format and this framebuffer is not owned.
        memcpy(
            this->data, other.data, other.w * other.h * pixel_size(this->pixel_format));
        this->w = other.w;
        this->h = other.h;
        this->pixel_format = other.pixel_format;
        return *this;
    }
    Buffer(Buffer&& other) noexcept : data(nullptr) { this->swap(other); }
    Buffer& operator=(Buffer&& other) noexcept {
        this->swap(other);
        return *this;
    }

   private:
    // A C++ idiom for copy-c'tor- & copy-assignment deduplication:
    // Define the copy-assignment operator in terms of a temporary copy and a swap of
    // the members. The advantage is that swap() is simple, and reusable for move-c'tor
    // and move-assignment as well.
    // https://stackoverflow.com/a/3279550
    void swap(Buffer& other) {
        std::swap(this->w, other.w);
        std::swap(this->h, other.h);
        std::swap(this->pixel_format, other.pixel_format);
        std::swap(this->data, other.data);
        std::swap(this->owns_data, other.owns_data);
    }
    /**
     * Does this buffer manage its own memory or does it refer to memory from the
     * outside.
     */
    bool owns_data = true;
};

struct RenderContext {
    AVS_Pixel_Format pixel_format = AVS_PIXEL_RGB0_8;
    size_t w = 0;
    size_t h = 0;
    Buffer framebuffers[2];
    std::array<Buffer, 8>& global_buffers;
    AudioData audio;
    bool is_preinit = false;
    bool needs_final_fb_copy = false;

    RenderContext(AVS_Pixel_Format pixel_format,
                  size_t w,
                  size_t h,
                  std::array<Buffer, 8>& global_buffers,
                  void* external_buffer = nullptr)
        : pixel_format(pixel_format),
          w(w),
          h(h),
          framebuffers{Buffer(w, h, pixel_format, external_buffer),
                       Buffer(w, h, pixel_format)},
          global_buffers(global_buffers) {}

    void swap_framebuffers() {
        std::swap(this->framebuffers[0], this->framebuffers[1]);
        this->needs_final_fb_copy = !this->needs_final_fb_copy;
    }

    void copy_secondary_to_output_framebuffer_if_needed() {
        if (this->needs_final_fb_copy) {
            // In the swapped state the final effect rendered to the secondary buffer,
            // so the output pointer is in framebuffers[1], the other one.
            // Copy the contents of the last-used buffer to the one with the output
            // pointer.
            this->framebuffers[1] = this->framebuffers[0];
            this->needs_final_fb_copy = false;
        }
    }
};
