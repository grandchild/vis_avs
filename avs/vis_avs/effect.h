#pragma once

#include "avs_editor.h"
#include "effect_info.h"

#include <stdint.h>
#include <string.h>
#include <string>
#include <vector>

class Effect {
   public:
    enum Insert_Direction {
        INSERT_BEFORE,
        INSERT_AFTER,
        INSERT_CHILD,
    };
    AVS_Component_Handle handle;
    bool enabled;
    std::vector<Effect*> children;
    AVS_Component_Handle* child_handles_for_api;
    Effect() : handle(h_components.get()), enabled(true), child_handles_for_api(NULL){};
    virtual ~Effect();
    Effect* insert(Effect* to_insert, Effect* relative_to, Insert_Direction direction);
    Effect* lift(Effect* to_lift);
    Effect* move(Effect* to_move, Effect* relative_to, Insert_Direction direction);
    void remove(Effect* to_remove);
    Effect* duplicate(Effect* to_duplicate);
    Effect* find_by_handle(AVS_Component_Handle handle);
    const AVS_Component_Handle* get_child_handles_for_api();

    virtual Effect* clone() = 0;
    virtual bool can_have_child_components() = 0;
    virtual Effect_Info* get_info() = 0;
    virtual size_t parameter_list_length(const Parameter* parameter,
                                         std::vector<int64_t> parameter_path) = 0;
    virtual bool parameter_list_entry_add(const Parameter* parameter,
                                          int64_t before,
                                          std::vector<AVS_Parameter_Value>,
                                          std::vector<int64_t> parameter_path) = 0;
    virtual bool parameter_list_entry_move(const Parameter* parameter,
                                           int64_t from,
                                           int64_t to,
                                           std::vector<int64_t> parameter_path) = 0;
    virtual bool parameter_list_entry_remove(const Parameter* parameter,
                                             int64_t to_remove,
                                             std::vector<int64_t> parameter_path) = 0;
    virtual bool run_action(AVS_Parameter_Handle parameter,
                            std::vector<int64_t> parameter_path = {}) = 0;

#define GET_SET_PARAMETER_ABSTRACT(AVS_TYPE, TYPE)                             \
    virtual TYPE get_##AVS_TYPE(AVS_Parameter_Handle parameter,                \
                                std::vector<int64_t> parameter_path = {}) = 0; \
    virtual bool set_##AVS_TYPE(AVS_Parameter_Handle parameter,                \
                                TYPE value,                                    \
                                std::vector<int64_t> parameter_path = {}) = 0
    GET_SET_PARAMETER_ABSTRACT(bool, bool);
    GET_SET_PARAMETER_ABSTRACT(int, int64_t);
    GET_SET_PARAMETER_ABSTRACT(float, double);
    GET_SET_PARAMETER_ABSTRACT(color, uint64_t);
    GET_SET_PARAMETER_ABSTRACT(string, const char*);

#define GET_ARRAY_PARAMETER_ABSTRACT(AVS_TYPE, TYPE)  \
    virtual std::vector<TYPE> get_##AVS_TYPE##_array( \
        AVS_Parameter_Handle parameter, std::vector<int64_t> parameter_path = {}) = 0
    GET_ARRAY_PARAMETER_ABSTRACT(int, int64_t);
    GET_ARRAY_PARAMETER_ABSTRACT(float, double);
    GET_ARRAY_PARAMETER_ABSTRACT(color, uint64_t);

    // render api
    virtual int render(char visdata[2][2][576],
                       int isBeat,
                       int* framebuffer,
                       int* fbout,
                       int w,
                       int h) {
        (void)visdata, (void)isBeat, (void)framebuffer, (void)fbout, (void)w, (void)h;
        return 0;  // returns 1 if fbout has dest
    };
    virtual char* get_desc() = 0;
    virtual void set_desc(char* desc) = 0;
    virtual void load_legacy(unsigned char* data, int len) { (void)data, (void)len; };
    virtual int save_legacy(unsigned char* data) {
        (void)data;
        return 0;
    }
    void load_string_legacy(std::string& s, unsigned char* data, int& pos, int len);
    void save_string_legacy(unsigned char* data, int& pos, std::string& text);

    // multithread render api
    virtual bool can_multithread() { return false; };
    virtual int smp_getflags() { return 0; }
    virtual int smp_begin(int max_threads,
                          char visdata[2][2][576],
                          int isBeat,
                          int* framebuffer,
                          int* fbout,
                          int w,
                          int h) {
        (void)max_threads, (void)visdata, (void)isBeat, (void)framebuffer, (void)fbout,
            (void)w, (void)h;
        return 0;
    }
    virtual void smp_render(int this_thread,
                            int max_threads,
                            char visdata[2][2][576],
                            int isBeat,
                            int* framebuffer,
                            int* fbout,
                            int w,
                            int h) {
        (void)this_thread, (void)max_threads, (void)visdata, (void)isBeat,
            (void)framebuffer, (void)fbout, (void)w, (void)h;
    };
    virtual int smp_finish(char visdata[2][2][576],
                           int isBeat,
                           int* framebuffer,
                           int* fbout,
                           int w,
                           int h) {
        (void)visdata, (void)isBeat, (void)framebuffer, (void)fbout, (void)w, (void)h;
        return 0;
    };

   protected:
    Effect* duplicate_with_children();
};

template <class Info_T, class Config_T>
class Configurable_Effect : public Effect {
   public:
    // TODO [clean]: make info & config protected.
    static Info_T info;
    Config_T config;

    Effect* clone() {
        auto cloned = new Configurable_Effect(*this);
        cloned->handle = h_components.get();
        return cloned;
    };
    bool can_have_child_components() { return this->info.can_have_child_components(); };
    static Configurable_Effect* create() { return new Configurable_Effect(); };
    Effect_Info* get_info() { return &this->info; };
    virtual void set_desc(char* desc) {
        Configurable_Effect::_set_desc();
        if (desc) {
            strcpy(desc, Configurable_Effect::desc.c_str());
        }
    };
    virtual char* get_desc() {
        Configurable_Effect::_set_desc();
        return (char*)Configurable_Effect::desc.c_str();
    };

    template <typename T>
    T get(AVS_Parameter_Handle parameter, std::vector<int64_t> parameter_path = {}) {
        if (parameter == 0) {
            return parameter_dispatch<T>::zero();
        }
        auto config_param = this->get_config_parameter(parameter, parameter_path);
        if (config_param.info == NULL) {
            return parameter_dispatch<T>::zero();
        }
        return parameter_dispatch<T>::get(config_param.value_addr);
    };

    template <typename T>
    bool set(AVS_Parameter_Handle parameter,
             T value,
             std::vector<int64_t> parameter_path = {}) {
        if (parameter == 0) {
            return false;
        }
        auto config_param = this->get_config_parameter(parameter, parameter_path);
        if (config_param.info == NULL) {
            return false;
        }
        parameter_dispatch<T>::set(config_param, value);
        if (config_param.info->on_value_change != NULL) {
            config_param.info->on_value_change(this, config_param.info, parameter_path);
        }
        return true;
    };

    template <typename T>
    std::vector<T>& get_array(AVS_Parameter_Handle parameter,
                              std::vector<int64_t> parameter_path = {}) {
        static std::vector<T> empty;
        if (parameter == 0) {
            return empty;
        }
        auto config_param = this->get_config_parameter(parameter, parameter_path);
        if (config_param.info == NULL) {
            return empty;
        }
        return parameter_dispatch<T>::get_array(config_param.value_addr);
    };

    size_t parameter_list_length(const Parameter* parameter,
                                 std::vector<int64_t> parameter_path = {}) {
        if (parameter == 0) {
            return 0;
        }
        auto config_param =
            this->get_config_parameter(parameter->handle, parameter_path);
        if (config_param.info == NULL) {
            return 0;
        }
        return parameter->list_length(config_param.value_addr);
    };
    bool parameter_list_entry_add(
        const Parameter* parameter,
        int64_t before,
        std::vector<AVS_Parameter_Value> parameter_values = {},
        std::vector<int64_t> parameter_path = {}) {
        if (parameter == 0) {
            return false;
        }
        auto config_param =
            this->get_config_parameter(parameter->handle, parameter_path);
        if (config_param.info == NULL) {
            return false;
        }
        bool success = parameter->list_add(
            config_param.value_addr, parameter->num_child_parameters_max, &before);
        if (success) {
            std::vector<int64_t> new_parameter_path = parameter_path;
            new_parameter_path.push_back(before);
            for (auto const& pv : parameter_values) {
                auto value_config_param =
                    this->get_config_parameter(pv.parameter, new_parameter_path);
                if (value_config_param.info == NULL) {
                    continue;
                }
                switch (value_config_param.info->type) {
                    case AVS_PARAM_BOOL:
                        parameter_dispatch<bool>::set(value_config_param, pv.value.b);
                        break;
                    case AVS_PARAM_SELECT:
                    case AVS_PARAM_INT:
                        parameter_dispatch<int64_t>::set(value_config_param,
                                                         pv.value.i);
                        break;
                    case AVS_PARAM_FLOAT:
                        parameter_dispatch<double>::set(value_config_param, pv.value.f);
                        break;
                    case AVS_PARAM_COLOR:
                        parameter_dispatch<uint64_t>::set(value_config_param,
                                                          pv.value.c);
                        break;
                    case AVS_PARAM_STRING:
                        parameter_dispatch<const char*>::set(value_config_param,
                                                             pv.value.s);
                        break;
                    case AVS_PARAM_LIST:
                    case AVS_PARAM_INVALID:
                    case AVS_PARAM_INT_ARRAY:
                    case AVS_PARAM_FLOAT_ARRAY:
                    case AVS_PARAM_COLOR_ARRAY:
                    default:
                        break;
                }
            }
            if (config_param.info->on_list_add != NULL) {
                config_param.info->on_list_add(
                    this, config_param.info, parameter_path, before, 0);
            }
        }
        return success;
    };
    bool parameter_list_entry_move(const Parameter* parameter,
                                   int64_t from,
                                   int64_t to,
                                   std::vector<int64_t> parameter_path = {}) {
        if (parameter == 0) {
            return 0;
        }
        auto config_param =
            this->get_config_parameter(parameter->handle, parameter_path);
        if (config_param.info == NULL) {
            return false;
        }
        bool success = parameter->list_move(config_param.value_addr, &from, &to);
        if (success && config_param.info->on_list_move != NULL) {
            config_param.info->on_list_move(
                this, config_param.info, parameter_path, from, to);
        }
        return success;
    };
    bool parameter_list_entry_remove(const Parameter* parameter,
                                     int64_t to_remove,
                                     std::vector<int64_t> parameter_path = {}) {
        if (parameter == 0) {
            return 0;
        }
        auto config_param =
            this->get_config_parameter(parameter->handle, parameter_path);
        if (config_param.info == NULL) {
            return false;
        }
        bool success = parameter->list_remove(
            config_param.value_addr, parameter->num_child_parameters_min, &to_remove);
        if (success && config_param.info->on_list_remove != NULL) {
            config_param.info->on_list_remove(
                this, config_param.info, parameter_path, to_remove, 0);
        }
        return success;
    };

    bool run_action(AVS_Parameter_Handle parameter,
                    std::vector<int64_t> parameter_path) {
        if (parameter == 0) {
            return false;
        }
        auto config_param = this->get_config_parameter(parameter, parameter_path);
        if (config_param.info == NULL || config_param.info->on_value_change == NULL) {
            return false;
        }
        config_param.info->on_value_change(this, config_param.info, parameter_path);
        return true;
    };

#define GET_SET_PARAMETER(AVS_TYPE, TYPE)                                   \
    virtual TYPE get_##AVS_TYPE(AVS_Parameter_Handle parameter,             \
                                std::vector<int64_t> parameter_path = {}) { \
        return get<TYPE>(parameter, parameter_path);                        \
    };                                                                      \
    virtual bool set_##AVS_TYPE(AVS_Parameter_Handle parameter,             \
                                TYPE value,                                 \
                                std::vector<int64_t> parameter_path = {}) { \
        return set<TYPE>(parameter, value, parameter_path);                 \
    }
    GET_SET_PARAMETER(bool, bool);
    GET_SET_PARAMETER(int, int64_t);
    GET_SET_PARAMETER(float, double);
    GET_SET_PARAMETER(color, uint64_t);
    GET_SET_PARAMETER(string, const char*);

#define GET_ARRAY_PARAMETER(AVS_TYPE, TYPE)                                         \
    virtual std::vector<TYPE> get_##AVS_TYPE##_array(                               \
        AVS_Parameter_Handle parameter, std::vector<int64_t> parameter_path = {}) { \
        return get_array<TYPE>(parameter, parameter_path);                          \
    }
    GET_ARRAY_PARAMETER(int, int64_t);
    GET_ARRAY_PARAMETER(float, double);
    GET_ARRAY_PARAMETER(color, uint64_t);

   protected:
    static std::string desc;
    static void _set_desc() {
        if (Configurable_Effect::desc == "") {
            Configurable_Effect::desc += Configurable_Effect::info.group;
            if (Configurable_Effect::desc != "") {
                Configurable_Effect::desc += " / ";
            }
            Configurable_Effect::desc += Configurable_Effect::info.name;
        }
    };

   private:
    Config_Parameter get_config_parameter(AVS_Parameter_Handle parameter,
                                          std::vector<int64_t> parameter_path) {
        return this->info.get_config_parameter(this->config,
                                               parameter,
                                               this->info.num_parameters,
                                               this->info.parameters,
                                               parameter_path,
                                               0);
    };
};

template <class Info_T, class Config_T>
Info_T Configurable_Effect<Info_T, Config_T>::info;
template <class Info_T, class Config_T>
std::string Configurable_Effect<Info_T, Config_T>::desc;
