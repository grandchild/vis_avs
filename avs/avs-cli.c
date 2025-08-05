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

void save_as_ppm(void* framebuffer, size_t w, size_t h, const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Could not open %s for writing\n", filename);
        return;
    }
    fprintf(file, "P6\n%zu %zu\n255\n", w, h);
    uint32_t* pixels = (uint32_t*)framebuffer;
    for (size_t i = 0; i < w * h; ++i) {
        uint8_t r = (pixels[i] & 0xff0000) >> 16;
        uint8_t g = (pixels[i] & 0xff00) >> 8;
        uint8_t b = pixels[i] & 0xff;
        fwrite(&r, sizeof(uint8_t), 1, file);
        fwrite(&g, sizeof(uint8_t), 1, file);
        fwrite(&b, sizeof(uint8_t), 1, file);
    }
    fclose(file);
}

void print_framebuffer(void* framebuffer, size_t w, size_t h) {
#ifdef _WIN32
    const char* e = "\\x1b";
#else
    const char* e = "\x1b";
#endif
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
            printf("%s[38;2;%d;%d;%dm%s[48;2;%d;%d;%dm▀", e, r1, g1, b1, e, r2, g2, b2);
        }
        printf("%s[0m\n", e);
    }
    free(first_line);
}

AVS_Component_Handle get_nth_child_component_by_name(AVS_Handle avs,
                                                     const char* name,
                                                     size_t n,
                                                     AVS_Component_Handle parent) {
    uint32_t length_out;
    const AVS_Component_Handle* components =
        avs_component_children(avs, parent, &length_out);
    for (size_t i = 0; i < length_out; ++i) {
        AVS_Effect_Handle effect = avs_component_effect(avs, components[i]);
        AVS_Effect_Info info;
        avs_effect_info(avs, effect, &info);
        if (strcmp(info.name, name) == 0 && n-- == 0) {
            return components[i];
        }
        if (avs_component_can_have_child_components(avs, components[i])) {
            AVS_Component_Handle child =
                get_nth_child_component_by_name(avs, name, n, components[i]);
            if (child) {
                return child;
            }
        }
    }
    printf("Cannot find component '%s' %ld\n", name, n);
    return 0;
}
AVS_Component_Handle get_nth_component_by_name(AVS_Handle avs,
                                               const char* name,
                                               size_t n) {
    AVS_Component_Handle root = avs_component_root(avs);
    return get_nth_child_component_by_name(avs, name, n, root);
}

AVS_Parameter_Handle _get_parameter_by_name(AVS_Handle avs,
                                            AVS_Effect_Handle effect,
                                            const AVS_Parameter_Handle* parameters,
                                            size_t parameters_length,
                                            const char** name) {
    const char* cur_name = *name;
    if (cur_name == NULL) {
        return 0;
    }
    for (size_t i = 0; i < parameters_length; ++i) {
        AVS_Parameter_Info param;
        avs_parameter_info(avs, effect, parameters[i], &param);
        if (strcmp(param.name, cur_name) == 0) {
            if (param.type == AVS_PARAM_LIST) {
                return _get_parameter_by_name(
                    avs, effect, param.children, param.children_length, &name[1]);
            }
            return parameters[i];
        }
    }
    return 0;
}
AVS_Parameter_Handle get_parameter_by_name(AVS_Handle avs,
                                           AVS_Component_Handle component,
                                           const char** name) {
    AVS_Effect_Handle effect = avs_component_effect(avs, component);
    if (!effect) {
        printf("Error getting effect: %s\n", avs_error_str(avs));
        return 0;
    }
    AVS_Effect_Info info;
    if (!avs_effect_info(avs, effect, &info)) {
        printf("Error getting effect info: %s\n", avs_error_str(avs));
        return 0;
    }
    AVS_Parameter_Handle handle = _get_parameter_by_name(
        avs, effect, info.parameters, info.parameters_length, name);
    if (!handle) {
        printf("Cannot find param ");
        printf("'%s'", *name++);
        for (const char** n = name; *n; n++) {
            printf("/'%s'", *n);
        }
        printf("\n");
    }
    return handle;
}

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
        if (!avs_preset_load(avs, argv[1])) {
            printf("failed loading: %s\n", avs_error_str(avs));
        }
        const char* preset = avs_preset_get(avs, false, true);
        // printf("%s\n", preset);
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
    size_t w = 24;
    size_t h = 24;
    void* framebuffer = calloc(w * h, sizeof(uint32_t));
    uint32_t frame_number = 0;
    avs_input_mouse_pos_set(avs, 0.5, 0.5);
    AVS_Component_Handle convo =
        get_nth_component_by_name(avs, "Convolution Filter", 0);
    const char* c_filter_name[2] = {"C Filter", NULL};
    AVS_Parameter_Handle c_filter = get_parameter_by_name(avs, convo, c_filter_name);
    AVS_Component_Properties convo_props;
    avs_component_properties_get(avs, convo, &convo_props);
    const char* kernel_value_name[3] = {"Kernel", "Value", NULL};
    AVS_Parameter_Handle kernel_value =
        get_parameter_by_name(avs, convo, kernel_value_name);
    int64_t kernel_value_index = 0;
    for (int ky = 0; ky < 7; ky++) {
        printf(".");
        for (int kx = 0; kx < 7; kx++) {
            // kernel_value_index = kx + ky * 7;
            kernel_value_index = (6 - kx) + (6 - ky) * 7;
            int64_t value =
                avs_parameter_get_int(avs, convo, kernel_value, 1, &kernel_value_index);
            if (value != 0) {
                printf("%+ 2ld ", value);
            } else {
                printf("   ");
            }
            const char* err = avs_error_str(avs);
        }
        printf("\n");
    }
    for (int k = 0; k < 2; k++) {
        convo_props.enabled = false;
        avs_component_properties_set(avs, convo, convo_props);
        char filename[256];
        for (int i = 0; i < 1; i++) {
            avs_render_frame(avs, framebuffer, w, h, 0, false, AVS_PIXEL_RGB0_8);
            printf("frame %d\n", ++frame_number);
            print_framebuffer(framebuffer, w, h);
            sprintf(filename, "/tmp/frame%02d.ppm", k * 2);
            save_as_ppm(framebuffer, w, h, filename);
            convo_props.enabled = true;
            avs_component_properties_set(avs, convo, convo_props);
            avs_render_frame(avs, framebuffer, w, h, 0, true, AVS_PIXEL_RGB0_8);
            printf("frame %d\n", ++frame_number);
            print_framebuffer(framebuffer, w, h);
            sprintf(filename, "/tmp/frame%02d.ppm", k * 2 + 1);
            save_as_ppm(framebuffer, w, h, filename);
            // sleep(1);
        }
        avs_parameter_set_bool(avs, convo, c_filter, false, 0, NULL);
    }
    // avs_render_frame(avs, framebuffer, w, h, 0, false, AVS_PIXEL_RGB0_8);
    // printf("frame %d\n", ++frame_number);
    // print_framebuffer(framebuffer, w, h);
    // avs_render_frame(avs, framebuffer, w, h, 0, false, AVS_PIXEL_RGB0_8);
    // printf("frame %d\n", ++frame_number);
    // print_framebuffer(framebuffer, w, h);

    avs_free(avs);

    return 0;
}
