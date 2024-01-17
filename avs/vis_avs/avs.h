/**
 * AVS' API has two parts. This file is the basic API which is all you need if you only
 * want to "play" AVS presets, i.e. render consecutive frames from audio input. It has
 * the following sections:
 *
 *   Init/Free:    avs_init() & avs_free()
 *   Rendering:    avs_render_frame()
 *   Audio:        avs_audio_set(), avs_audio_device_count(),
 *                 avs_audio_device_names() & avs_audio_device_set()
 *   Preset files: avs_preset_load(), avs_preset_set(),
 *                 avs_preset_save(), avs_preset_get(),
 *                 avs_preset_set_legacy(), avs_preset_get_legacy() &
 *                 avs_preset_save_legacy()
 *   Errors:       avs_error_str()
 *
 * The basic API should be fairly straightforward and all the individual function
 * details are described below.
 *
 * For editing AVS presets, you will need the second part, "avs_editor.h". See there for
 * more information.
 *
 * Here's a minimal usage example of the basic API:
 *
 *     #include "avs.h"
 *     #include <stdio.h>   // printf()
 *     #include <stdint.h>  // uint32_t etc.
 *
 *     int main() {
 *         AVS_Handle avs = avs_init(AVS_AUDIO_INTERNAL, AVS_BEAT_INTERNAL);
 *         if(!avs) {
 *             printf("Error initializing AVS: %s\n", avs_error_str(avs));
 *             return 1;
 *         }
 *         if(!avs_preset_load(avs, "my-preset.avs")) {
 *             printf("Error loading preset: %s\n", avs_error_str(avs));
 *             avs_free(avs);
 *             return 1;
 *         }
 *         size_t width = 800;
 *         size_t height = 500;
 *         uint32_t* framebuffer = (uint32_t*)malloc(width * height * sizeof(uint32_t));
 *         int stop = 0;
 *         while(!stop) {  // Exit with Ctrl-C or set `stop`.
 *             if(avs_render_frame(
 *                    avs,
 *                    framebuffer,
 *                    width,
 *                    height,
 *                    0,                // zero for realtime rendering
 *                    false,            // ignored, since AVS_BEAT_INTERNAL
 *                    AVS_PIXEL_RGB0_8  // the only valid pixel format
 *             )) {
 *                 // -------------------------------------
 *                 // Do something with `framebuffer` here,
 *                 // like displaying it in a window.
 *                 // -------------------------------------
 *             } else {
 *                 printf("Error rendering: %s\n", avs_error_str(avs));
 *             }
 *         }
 *         avs_free(avs);
 *         return 0;
 *     }
 *
 */
#pragma once

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
extern "C" {
#else
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#endif

typedef uint32_t AVS_Handle;

typedef enum { AVS_PIXEL_RGB0_8 = 0 } AVS_Pixel_Format;  // To be expanded.

typedef enum { AVS_AUDIO_INTERNAL = 0, AVS_AUDIO_EXTERNAL = 1 } AVS_Audio_Source;
typedef enum { AVS_BEAT_INTERNAL = 0, AVS_BEAT_EXTERNAL = 1 } AVS_Beat_Source;

/**
 * Initialize an AVS instance. If initialization fails for some reason, the returned
 * AVS_Handle will be 0. You may call `avs_error_str()` at that point, to get a short
 * message explaining what went wrong.
 *
 * An empty preset is loaded at startup, so you can start calling `avs_render_frame()`
 * right away, if you want to.
 *
 *   `audio_source`
 *       If `AVS_AUDIO_EXTERNAL`, AVS will depend on calls to `avs_audio_set()` to fill
 *       its audio buffer. If `avs_audio_set()` is not called, AVS will assume silence.
 *       If `AVS_AUDIO_INTERNAL`, AVS will open a recording device itself and get audio
 *       input from there.
 *
 *   `beat_source`
 *       If `AVS_BEAT_EXTERNAL`, AVS will depend on `avs_render_frame()`'s `is_beat`
 *       parameter to know when a beat has occurred. If `AVS_BEAT_INTERNAL`, AVS will do
 *       its own beat/onset detection on the audio available (either external or via the
 *       internal input device), and the `is_beat` parameter to `avs_render_frame()` is
 *       ignored.
 *
 * Note: `audio_source` and `beat_source` are independent of one another, all 4 settings
 * combinations are valid. (Having internal audio and external beat detection makes
 * little sense though, although you could disable beat detection this way.)
 */
AVS_Handle avs_init(const char* base_path,
                    AVS_Audio_Source audio_source,
                    AVS_Beat_Source beat_source);

/**
 * Render a single frame of the loaded preset. If rendering fails due to invalid
 * parameters or something else, the function will return false. You may call
 * `avs_error_str()` at that point to get a short message explaining what went wrong.
 *
 *   `framebuffer`
 *       A pointer to a memory section that's at least as large as
 *
 *           `width * height * sizeof(pixel)`
 *
 *       for AVS to write the frame data into.
 *       A pixel is currently always a 32-bit RGB8 integer (the high-order byte is 0),
 *       but might change in the future with more supported pixel formats.
 *
 *   `width`
 *       Width of the framebuffer. Must be > 0 and a multiple of 4.
 *       (Note: This might change to a multiple of 8 in the future.)
 *
 *   `height`
 *       Height of the framebuffer. Must be > 0 and a multiple of 2.
 *
 *   `time_ms`
 *       May be between 0 and 1000 for realtime rendering or a value > 1000 for non-
 *       realtime rendering. In non-realtime mode, `time_ms` represents a monotonic time
 *       in milliseconds.
 *       If `time_ms` ≤ 1000, AVS will currently ignore the specific value and simply
 *       use the current monotonic system time. The values between 0 and 1000 are
 *       reserved for future use, so if you want non-realtime rendering, make sure your
 *       `time_ms` values are greater than 1000.
 *
 *       In non-realtime mode, from the point of view of a preset, the time is
 *       guaranteed to always advance between frames, so if the `time_ms` parameter does
 *       not change or goes backwards between frames, it will be ignored and the
 *       internal time will increase by two(!) milliseconds each frame (i.e. 500 fps).
 *
 *       There's really no need to worry about overflow, since a millisecond-resolution
 *       64-bit counter will last for more than 580 million years.
 *
 *   `is_beat`
 *       If AVS was initialized with `beat_source=AVS_BEAT_EXTERNAL`, and `is_beat` is
 *       `true`, the beat flag will be set for this frame and preset effects can respond
 *       to it.
 *       If AVS was initialized with `beat_source=AVS_BEAT_INTERNAL` this parameter is
 *       ignored.
 *
 *   `pixel_format`
 *       Must be `AVS_PIXEL_RGB0_8`. Any other value is invalid at the moment.
 *       Note: Has no impact on what pixel format the preset can use internally. Only
 *       defines the pixel format of the final rendered frame.
 */
bool avs_render_frame(AVS_Handle avs,
                      void* framebuffer,
                      size_t width,
                      size_t height,
                      uint64_t time_ms,
                      bool is_beat,
                      AVS_Pixel_Format pixel_format);

/**
 * Fill AVS' audio wave data ring buffer with new data. Audio data should be in 32-bit
 * float format, in stereo. AVS will calculate the FFT of the audio for frequencies
 * internally.
 *
 * If the audio buffer overflows (too much total data for the ring buffer to hold), the
 * audio will be set just the same. There might be a jump in the audio which might lead
 * to a spurious beat being detected. The return value will be INT32_MAX.
 *
 * If the audio buffer underflows (too little data for too much time), AVS will fill in
 * silence. The return value will be negative. Make sure that your `audio_length`,
 * `samples_per_second` and `avs_render_frame()`'s `time_ms` parameters match up. They
 * should roughly satisfy the following relation:
 *
 *   audio_length / samples_per_second = (time_ms - last_time_ms) * 1000
 *
 * Returns the number of milliseconds of audio available in the buffer right _before_
 * insertion. Note that you can inspect the state of the buffer by setting
 * `audio_length` to 0. `audio_left`, `audio_right` and `samples_per_second` are ignored
 * in that case.
 *
 * Returns INT32_MAX on overflow, and negative values ≤ -2 on underflow or -1 if `avs`
 * was initialized with `audio_source=AVS_AUDIO_INTERNAL`.
 */
int32_t avs_audio_set(AVS_Handle avs,
                      const float* audio_left,
                      const float* audio_right,
                      size_t audio_length,
                      size_t samples_per_second,
                      int64_t start_time_in_samples);

/**
 * Returns the number of audio input devices AVS has detected.
 *
 * Returns -1 if `avs` was initialized with `audio_source=AVS_AUDIO_EXTERNAL`, or some
 * other error occurred.
 */
int32_t avs_audio_device_count(AVS_Handle avs);

/**
 * Returns a list of the names of audio input devices AVS has detected. The number of
 * devices is returned by `avs_audio_device_count()`.
 *
 * The returned list memory will be valid until you either call
 * `avs_audio_device_names()` again or `avs_free()` is called for this instance.
 *
 * Returns NULL if no devices found or `avs` was initialized with
 * `audio_source=AVS_AUDIO_EXTERNAL`.
 */
const char* const* avs_audio_device_names(AVS_Handle avs);

/**
 * Set the device to use for audio input. The device number is the index of the list of
 * device names.
 *
 * Has no effect if `avs` was initialized with `audio_source=AVS_AUDIO_EXTERNAL`.
 *
 * Returns `true` if device was successfully set (also when unchanged!) or `false` if
 * the device handle is invalid or some other error occurred.
 */
bool avs_audio_device_set(AVS_Handle avs, int32_t device);

/**
 * Load a preset file from the given location and return `true` on success. You may call
 * `avs_error_str()` if loading failed to get an error message.
 *
 * This convenience wrapper for `avs_preset_set()` & `avs_preset_set_legacy()` relieves
 * you of the task of checking whether the preset file is JSON or a legacy binary file.
 *
 * Returns `true` on success or `false` if loading fails for some reason.
 */
bool avs_preset_load(AVS_Handle avs, const char* file_path);

/**
 * Load a preset from a JSON string.
 *
 * Returns `true` on success or `false` if loading fails for some reason.
 */
bool avs_preset_set(AVS_Handle avs, const char* preset);

/**
 * Save the currently loaded preset to the given file path. If `indent` is `true`, the
 * JSON will be indented with 4 spaces in the file. Otherwise it will be a single line.
 *
 * If `file_path` does not end in ".avs" it will be appended. Setting `file_path` to a
 * directory is an error.
 *
 * Returns `true` on success or `false` if saving fails for some reason.
 */
bool avs_preset_save(AVS_Handle avs, const char* file_path, bool indent);

/**
 * Return the currently loaded preset's JSON string.
 *
 * This is also the method to convert legacy presets into JSON, by loading it with
 * `avs_preset_load()` or `avs_preset_set_legacy()` and returning JSON with this
 * method, or saving it to disk with `avs_preset_save()`.
 *
 * The returned string memory will be valid until you either call `avs_preset_get()`
 * again or `avs_free()` is called for this instance.
 *
 * Returns `NULL` on error.
 */
const char* avs_preset_get(AVS_Handle avs);

/**
 * Load a preset from a legacy binary buffer.
 *
 * Returns `true` on success or `false` if loading fails for some reason.
 */
bool avs_preset_set_legacy(AVS_Handle avs, const uint8_t* preset, size_t preset_length);

/**
 * Get the currently loaded preset in legacy binary format. Note that this method will
 * fail if the current preset uses features that aren't available in the legacy AVS save
 * format.
 *
 * Using this method is discouraged. Use the JSON format through `avs_preset_get()`
 * instead.
 *
 * The returned memory will be valid until you either call `avs_preset_get_legacy()`
 * again or `avs_free()` is called for this instance.
 *
 * Returns `NULL` if `preset_length_out` is NULL or serialization fails for some reason.
 */
const uint8_t* avs_preset_get_legacy(AVS_Handle avs, size_t* preset_length_out);

/**
 * Save the currently loaded preset in legacy binary format to the given file path. Note
 * that this method will fail if the current preset uses features that aren't available
 * in the legacy AVS save format.
 *
 * If `file_path` does not end in ".avs" it will be appended. Setting `file_path` to a
 * directory is an error.
 *
 * Using this method is discouraged. Use the JSON format through `avs_preset_save()`
 * instead.
 *
 * Returns `true` on success or `false` if saving fails for some reason.
 */
bool avs_preset_save_legacy(AVS_Handle avs, const char* file_path);

/**
 * When called, will return a string with the most recent error for the given AVS
 * instance, if any. If no error occurred the return value is the empty string, `""`.
 *
 * If `avs_error_str()` is called with a `0` or otherwise invalid `AVS_Handle`, return
 * the global error string, which is useful for checking `avs_init()` calls or if the
 * `AVS_Handle` for any other API call was invalid.
 *
 * Both the instance- and the global error string will be reset after a successful API
 * call, so make sure to check for errors before making any other API calls.
 */
const char* avs_error_str(AVS_Handle avs);

/**
 * Destroy an AVS_Handle returned by `avs_init()`. It's safe to pass a `0` handle, which
 * is a no-op.
 */
void avs_free(AVS_Handle avs);

/**
 * The version of AVS. `major`, `minor` and `patch` adhere to "Semantic Versioning":
 * https://semver.org
 */
typedef struct {
    /** AVS major version: API changes */
    uint32_t major;
    /** AVS minor version: New features */
    uint32_t minor;
    /** AVS patch version: Bug fixes */
    uint32_t patch;
    /** Release candidate. Is 0 if not an RC, i.e. most of the time. */
    uint32_t rc;
    /** The 20-byte SHA1 commit hash of the git revision from which AVS was built. */
    const uint8_t commit[20];
    /** A hash of any uncommitted changes on top of `commit`. The same hash should point
        to the same set of changes, although it might not be possible to guarantee that
        completely. It's all zero when the repository was "clean" at build time. */
    const uint8_t changes[20];
} AVS_Version;

AVS_Version avs_version();

#ifdef __cplusplus
}
#endif
