#include "effect_info.h"

#include "pixel_format.h"

#include "../platform.h"

#include <iostream>

/**
 * 0x0000000000ffffff -> "rgb0_8(255, 255, 255)"
 *
 * Other formats will be added in the future.
 */
static std::string serialize_color(uint64_t color,
                                   AVS_Pixel_Format pixel_format = AVS_PIXEL_RGB0_8) {
    std::stringstream color_str;
    switch (pixel_format) {
        case AVS_PIXEL_RGB0_8: {
            uint16_t r = (color & 0x00ff0000) >> 16;
            uint16_t g = (color & 0x0000ff00) >> 8;
            uint16_t b = color & 0x000000ff;
            color_str << "rgb0_8(" << r << ", " << g << ", " << b << ")";
            break;
        }
        default: break;
    }
    return color_str.str();
}

static std::string& replace_crlf_newlines_with_lf(std::string& str) {
    for (auto i = str.size(); i >= 2; i--) {
        if (str[i - 2] == '\r') {
            if (str[i - 1] == '\n') {
                str.erase(i - 2, 1);
            } else {
                // unlikely, but if a lone '\r' appears, simply replace by '\n'.
                str[i - 2] = '\n';
            }
        }
    }
    return str;
}

static std::string& replace_lf_newlines_with_crlf_for_winamp_ui(std::string& str) {
    for (int64_t i = 0; i < str.size(); i++) {
        if (str[i] == '\n') {
            str.insert(i, "\r");
        }
    }
    return str;
}

json Parameter::to_json(const Effect_Config* config) const {
    uint8_t* addr = &((uint8_t*)config)[this->offset];
    switch (this->type) {
        case AVS_PARAM_BOOL: return parameter_dispatch<bool>::get(addr);
        case AVS_PARAM_INT: return parameter_dispatch<int64_t>::get(addr);
        case AVS_PARAM_FLOAT: return parameter_dispatch<double>::get(addr);
        case AVS_PARAM_STRING: {
            std::string str = parameter_dispatch<const char*>::get(addr);
            return replace_crlf_newlines_with_lf(str);
        }
        case AVS_PARAM_RESOURCE: return parameter_dispatch<const char*>::get(addr);
        case AVS_PARAM_COLOR: {
            return serialize_color(parameter_dispatch<uint64_t>::get(addr));
        }
        case AVS_PARAM_SELECT: {
            auto value = parameter_dispatch<int64_t>::get(addr);
            int64_t num_options = 0;
            const char* const* options = this->get_options(&num_options);
            const char* selection = nullptr;
            if (value >= 0 && value < num_options) {
                selection = options[value];
            }
            if (selection == nullptr) {
                selection = "";
            }
            return selection;
        }
        case AVS_PARAM_LIST: {
            auto config_list = (std::vector<Effect_Config>*)addr;
            auto list = json::array();
            size_t list_size =
                config_list->size() * sizeof(Effect_Config) / this->sizeof_child;
            for (size_t i = 0; i < list_size; i++) {
                json json_child_config;
                for (size_t j = 0; j < this->num_child_parameters; j++) {
                    auto child_param = this->child_parameters[j];
                    if (child_param.is_saved) {
                        auto child_config =
                            config_list->data() + i * this->sizeof_child;
                        json_child_config[child_param.name] =
                            child_param.to_json(child_config);
                    }
                }
                list += json_child_config;
            }
            return list;
        }
        case AVS_PARAM_INT_ARRAY: return parameter_dispatch<int64_t>::get_array(addr);
        case AVS_PARAM_FLOAT_ARRAY: return parameter_dispatch<double>::get_array(addr);
        case AVS_PARAM_COLOR_ARRAY: {
            auto values = parameter_dispatch<uint64_t>::get_array(addr);
            auto array = json::array();
            for (auto& value : values) {
                array += serialize_color(value);
            }
            return array;
        }
        case AVS_PARAM_ACTION: [[fallthrough]];
        case AVS_PARAM_INVALID:
        default: return {};
    }
}

json Effect_Info::save_config(const Effect_Config* config,
                              const Effect_Config* global_config) const {
    json config_save;
    for (size_t i = 0; i < this->get_num_parameters(); i++) {
        auto parameter = this->get_parameters()[i];
        if (parameter.type == AVS_PARAM_INVALID) {
            log_warn(
                "Invalid parameter in %s:"
                "Check its Info struct's `parameters` list length vs `num_parameters`.",
                this->get_name());
            continue;
        }
        if (parameter.is_saved) {
            auto local_or_global_config = parameter.is_global ? global_config : config;
            try {
                auto value = parameter.to_json(local_or_global_config);
                if (!value.empty()) {
                    config_save[parameter.name] = value;
                }
            } catch (std::exception& e) {
                log_err("Exception while saving config for %s/%s: %s\n",
                        this->get_name(),
                        parameter.name,
                        e.what());
            }
        }
    }
    return config_save;
}

bool operator==(const Effect_Info& a, const Effect_Info& b) {
    return a.get_handle() == b.get_handle();
}
bool operator==(const Effect_Info* a, const Effect_Info& b) {
    return a == nullptr ? false : *a == b;
}
bool operator==(const Effect_Info& a, const Effect_Info* b) {
    return b == nullptr ? false : *b == a;
}
bool operator!=(const Effect_Info& a, const Effect_Info& b) { return !(a == b); }
bool operator!=(const Effect_Info* a, const Effect_Info& b) { return !(a == b); }
bool operator!=(const Effect_Info& a, const Effect_Info* b) { return !(b == a); }
