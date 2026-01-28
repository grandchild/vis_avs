#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "3rdparty/stb_image_write.h"
#include "vis_avs/avs.h"
#include "vis_avs/avs_editor.h"

#include <math.h>    // sin
#include <stdlib.h>  // random(), malloc(), free()

#define AUDIO_SRATE 44100
#define FRAMERATE   30

int main(int argc, char const* argv[]) {
    if (argc < 4) {
        printf("Usage: %s <input.avs-preset> <size> <output.png> [<warmup>]\n",
               argv[0]);
        return 1;
    }
    const char* preset = argv[1];
    int size = atoi(argv[2]);
    if (size <= 0) {
        printf("Invalid size: %d\n", size);
        return 2;
    }
    const char* output_png = argv[3];
    int warmup = -1;
    if (argc >= 5) {
        warmup = atoi(argv[4]);
        if (warmup < 0) {
            warmup = 0;
        }
    }
    AVS_Handle avs = avs_init(NULL, AVS_AUDIO_EXTERNAL, AVS_BEAT_EXTERNAL, NULL);
    if (!avs) {
        printf("Error initializing AVS: %s\n", avs_error_str(avs));
        return 3;
    }
    if (!avs_preset_load(avs, preset)) {
        printf("Error loading preset: %s\n", avs_error_str(avs));
        avs_free(avs);
        return 4;
    }
    if (warmup < 0) {
        AVS_Component_Handle root = avs_component_root(avs);
        if (!root) {
            printf("Error finding root component: %s\n", avs_error_str(avs));
            avs_free(avs);
            return 5;
        }
        AVS_Effect_Handle root_effect = avs_component_effect(avs, root);
        if (!root_effect) {
            printf("Error getting root effect: %s\n", avs_error_str(avs));
            avs_free(avs);
            return 6;
        }
        AVS_Effect_Info root_info;
        if (!avs_effect_info(avs, root_effect, &root_info)) {
            printf("Error getting root info: %s\n", avs_error_str(avs));
            avs_free(avs);
            return 7;
        }
        for (uint32_t i = 0; i < root_info.parameters_length; i++) {
            AVS_Parameter_Info param;
            if (!avs_parameter_info(
                    avs, root_effect, root_info.parameters[i], &param)) {
                printf("Error getting root param at index %d: %s\n",
                       i,
                       avs_error_str(avs));
                avs_free(avs);
                return 8;
            }
            if (!strncmp(param.name, "Warmup Frames", sizeof("Warmup Frames"))) {
                warmup =
                    avs_parameter_get_int(avs, root, root_info.parameters[i], 0, NULL);
            }
        }
    }
    size_t width = size;
    size_t height = size;
    uint32_t* framebuffer = (uint32_t*)malloc(width * height * sizeof(uint32_t));
    if (!framebuffer) {
        printf("Error allocating framebuffer\n");
        avs_free(avs);
        return 9;
    }
    int64_t time_in_ms = 0;
    float audio[2][AUDIO_SRATE];
    srandom(0x12345678);
    for (int i = 0; i < warmup; i++) {
        if (i % FRAMERATE == 0) {
            for (size_t s = 0; s < AUDIO_SRATE; s++) {
                float ran_left = ((float)(random() % 2000) / 1000.0f) - 1.0f;
                float ran_right = ((float)(random() % 2000) / 1000.0f) - 1.0f;
                // audio[0][s] = ((float)(s % 20) / 10.f) - 1.0f;
                // audio[1][s] = ((float)(s % 20) / 10.f) - 1.0f;
                audio[0][s] = sinf((float)s * 0.1f * (i + 10) * .02) + ran_left * .1;
                audio[1][s] = sinf((float)s * 0.1f * (i + 10) * .02) + ran_right * .1;
            }
            avs_audio_set(avs,
                          audio[0],
                          audio[1],
                          AUDIO_SRATE,
                          AUDIO_SRATE,
                          (i + 1) * AUDIO_SRATE);
        }
        if (!avs_render_frame(
                avs, framebuffer, width, height, time_in_ms, false, AVS_PIXEL_RGB0_8)) {
            printf("Error during warmup: %s\n", avs_error_str(avs));
            free(framebuffer);
            avs_free(avs);
            return 10;
        }
        time_in_ms += 1000 / FRAMERATE;
    }

    if (avs_render_frame(
            avs, framebuffer, width, height, time_in_ms, false, AVS_PIXEL_RGB0_8)) {
        unsigned char* img_data = (unsigned char*)malloc(width * height * 4);
        if (!img_data) {
            printf("Error allocating output image data\n");
            free(framebuffer);
            avs_free(avs);
            return 11;
        }
        for (size_t i = 0; i < width * height; i++) {
            img_data[i * 4 + 0] = (framebuffer[i] >> 16) & 0xFF;
            img_data[i * 4 + 1] = (framebuffer[i] >> 8) & 0xFF;
            img_data[i * 4 + 2] = (framebuffer[i] >> 0) & 0xFF;
            img_data[i * 4 + 3] = 255;
        }
        stbi_write_png(output_png, width, height, 4, img_data, width * 4);
        free(img_data);
    } else {
        printf("Error rendering: %s\n", avs_error_str(avs));
        free(framebuffer);
        avs_free(avs);
        return 12;
    }
    free(framebuffer);
    avs_free(avs);
    return 0;
}
