#include "vis_avs/avs.h"
#include "vis_avs/avs_editor.h"

#ifdef _WIN32
#include <windows.h>
#define sleep(x) Sleep(x * 1000)
#elif __linux__
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char const* argv[]) {
#ifdef _WIN32
    DWORD flags;
    if (GetProcessDEPPolicy(GetCurrentProcess(), &flags, NULL) && flags) {
        printf(
            "DEP is on. AVS will not be able to execute code. Compile this program"
            " with --disable-nxcompat\n");
        return 1;
    }
#endif
    AVS_Handle avs = avs_init(".", AVS_AUDIO_INTERNAL, AVS_BEAT_EXTERNAL);
    if (argc <= 1) {
        size_t num_effects = 0;
        const AVS_Effect_Handle* effects = avs_effect_library(avs, &num_effects);
        AVS_Effect_Handle super_scope = 0;
        AVS_Effect_Handle blur = 0;
        AVS_Effect_Handle effect_list = 0;
        AVS_Effect_Handle texer2 = 0;
        AVS_Effect_Handle movement = 0;
        AVS_Effect_Info info;
        for (size_t i = 0; i < num_effects; i++) {
            if (!avs_effect_info(avs, effects[i], &info)) {
                printf("Error getting effect info %d\n", effects[i]);
                continue;
            }
            if (strcmp(info.name, "SuperScope") == 0) {
                super_scope = effects[i];
                continue;
            }
            if (strcmp(info.name, "Blur") == 0) {
                blur = effects[i];
                continue;
            }
            if (strcmp(info.name, "Effect List") == 0) {
                effect_list = effects[i];
            }
            if (strcmp(info.name, "Texer II") == 0) {
                texer2 = effects[i];
            }
            if (strcmp(info.name, "Movement") == 0) {
                movement = effects[i];
            }
        }
        if (effect_list) {
            AVS_Component_Handle tx =
                avs_component_create(avs, texer2, 0, AVS_COMPONENT_POSITION_DONTCARE);
            AVS_Component_Handle el = avs_component_create(
                avs, effect_list, 0, AVS_COMPONENT_POSITION_DONTCARE);
            if (super_scope) {
                avs_component_create(
                    avs, super_scope, el, AVS_COMPONENT_POSITION_CHILD);
            }
            AVS_Effect_Info effect_list_info;
            avs_effect_info(avs, effect_list, &effect_list_info);
            AVS_Parameter_Info param;
            AVS_Parameter_Handle output_blend = 0;
            for (size_t i = 0; i < effect_list_info.parameters_length; ++i) {
                avs_parameter_info(
                    avs, effect_list, effect_list_info.parameters[i], &param);
                if (strcmp(param.name, "Output Blend Mode") == 0) {
                    output_blend = effect_list_info.parameters[i];
                    for (size_t k = 0; k < param.options_length; k++) {
                        if (strcmp(param.options[k], "Every Other Pixel") == 0) {
                            avs_parameter_set_int(avs, el, output_blend, k, 0, NULL);
                        }
                    }
                }
            }
        }
        AVS_Component_Handle root_component = avs_component_root(avs);
        if (movement) {
            AVS_Component_Handle mv = avs_component_create(
                avs, movement, root_component, AVS_COMPONENT_POSITION_CHILD);
            AVS_Effect_Info movement_info;
            avs_effect_info(avs, movement, &movement_info);
            AVS_Parameter_Info param;
            AVS_Parameter_Handle code = 0;
            AVS_Parameter_Handle coords = 0;
            for (size_t i = 0; i < movement_info.parameters_length; ++i) {
                avs_parameter_info(avs, movement, movement_info.parameters[i], &param);
                if (strcmp(param.name, "Code") == 0) {
                    code = movement_info.parameters[i];
                    avs_parameter_set_string(avs, mv, code, "x = 0;", 0, NULL);
                }
                if (strcmp(param.name, "Coordinates") == 0) {
                    coords = movement_info.parameters[i];
                    avs_parameter_set_int(avs, mv, coords, 1 /*cartesian*/, 0, NULL);
                }
            }
            // AVS_Component_Handle mv2 = avs_component_duplicate(avs, mv);
        }
        AVS_Effect_Handle root = avs_component_effect(avs, root_component);
        AVS_Effect_Info root_info;
        avs_effect_info(avs, root, &root_info);
        avs_parameter_set_bool(
            avs, root_component, root_info.parameters[0], true, 0, NULL);
    } else {
        if (avs_preset_load(avs, argv[1])) {
            printf("loaded %s\n", argv[1]);
        } else {
            printf("failed loading: %s\n", avs_error_str(avs));
        }
        const char* preset = avs_preset_get(avs, true);
        printf("%s\n", preset);
    }
    // avs_preset_save(avs, argv[1], true);
    float audio_left[1024];
    float audio_right[1024];
    float dir = 0.03;
    float val = 0.0;
    for (size_t i = 0; i < 1024; i++) {
        // audio_left[i] = (double)(random()) / RAND_MAX * 20 - 10;
        // audio_right[i] = (double)(random()) / RAND_MAX * 20 - 10;
        audio_left[i] = val;
        audio_right[i] = val;
        val += dir;
        if (val > 1.0 || val < -1.0) {
            dir = -dir;
        }
    }
    // avs_audio_set(avs, audio_left, audio_right, 1024, 44100, 2000);
    // avs_audio_set(
    //     avs, audio_left, audio_right, 1024, 44100, 2000 + 1024 * 1000 / 44100);
    // avs_audio_set(
    //     avs, audio_left, audio_right, 1024, 44100, 2000 + 2048 * 1000 / 44100);
    size_t w = 100;
    size_t h = 80;
    void* framebuffer = calloc(w * h, sizeof(uint32_t));
    for (int i = 0; i < 2; i++) {
        avs_render_frame(avs, framebuffer, w, h, 0, false, AVS_PIXEL_RGB0_8);
        avs_render_frame(avs, framebuffer, w, h, 0, true, AVS_PIXEL_RGB0_8);
        sleep(1);
    }
    avs_render_frame(avs, framebuffer, w, h, 0, false, AVS_PIXEL_RGB0_8);
    avs_render_frame(avs, framebuffer, w, h, 0, false, AVS_PIXEL_RGB0_8);

    uint32_t* first_line = (uint32_t*)calloc(w, sizeof(uint32_t));
    for (size_t y = 0; y < h; y += 2) {
        for (size_t x = 0; x < w; x++) {
            first_line[x] = ((uint32_t*)framebuffer)[y * w + x];
        }
        for (size_t x = 0; x < w; x++) {
            uint8_t r1 = (first_line[x] & 0xff0000) >> 16;
            uint8_t g1 = (first_line[x] & 0xff00) >> 8;
            uint8_t b1 = first_line[x] & 0xff;
            uint32_t col = ((uint32_t*)framebuffer)[(y + 1) * w + x];
            uint8_t r2 = (col & 0xff0000) >> 16;
            uint8_t g2 = (col & 0xff00) >> 8;
            uint8_t b2 = col & 0xff;
            // By using the "▀" (half upper block) character, we can render two lines of
            // pixels in one line of characters, with the effect of making the pixels
            // more square (a character cell in a terminal has ~1:2 aspect ratio).
            // The VT100 foreground color is used for the top half of the block, and set
            // to the pixel color from the first of the two lines, and the background
            // color is used for the bottom half of the block, and set to the pixel
            // color from the second of the two lines.
            printf("\x1b[38;2;%d;%d;%dm\x1b[48;2;%d;%d;%dm▀", r1, g1, b1, r2, g2, b2);
            /*
             */
        }
        printf("\x1b[0m\n");
    }
    avs_free(avs);

    return 0;
}
