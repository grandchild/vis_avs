#pragma once

#include "avs_editor.h"
#include "effect_info.h"

#include <algorithm>  // std::remove
#include <memory>     // std::shared_ptr, std::weak_ptr
#include <set>
#include <stdint.h>
#include <string.h>
#include <string>
#include <type_traits>  // std::is_same<T1, T2>
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
    Effect(AVS_Handle = 0)
        : handle(h_components.get()), enabled(true), child_handles_for_api(NULL){};
    virtual ~Effect();
    Effect* insert(Effect* to_insert, Effect* relative_to, Insert_Direction direction);
    Effect* lift(Effect* to_lift);
    Effect* move(Effect* to_move, Effect* relative_to, Insert_Direction direction);
    void remove(Effect* to_remove);
    Effect* duplicate(Effect* to_duplicate);
    void set_enabled(bool enabled);
    virtual void on_enable(bool enabled) { (void)enabled; };
    Effect* find_by_handle(AVS_Component_Handle handle);
    const AVS_Component_Handle* get_child_handles_for_api();

    virtual Effect* clone() = 0;
    virtual bool can_have_child_components() = 0;
    virtual Effect_Info* get_info() = 0;
    virtual size_t parameter_list_length(
        AVS_Parameter_Handle parameter,
        const std::vector<int64_t>& parameter_path) = 0;
    virtual bool parameter_list_entry_add(
        AVS_Parameter_Handle parameter,
        int64_t before,
        std::vector<AVS_Parameter_Value>,
        const std::vector<int64_t>& parameter_path) = 0;
    virtual bool parameter_list_entry_move(
        AVS_Parameter_Handle parameter,
        int64_t from,
        int64_t to,
        const std::vector<int64_t>& parameter_path) = 0;
    virtual bool parameter_list_entry_remove(
        AVS_Parameter_Handle parameter,
        int64_t to_remove,
        const std::vector<int64_t>& parameter_path) = 0;
    virtual bool run_action(AVS_Parameter_Handle parameter,
                            const std::vector<int64_t>& parameter_path = {}) = 0;

#define GET_SET_PARAMETER_ABSTRACT(AVS_TYPE, TYPE)                                    \
    virtual TYPE get_##AVS_TYPE(AVS_Parameter_Handle parameter,                       \
                                const std::vector<int64_t>& parameter_path = {}) = 0; \
    virtual bool set_##AVS_TYPE(AVS_Parameter_Handle parameter,                       \
                                TYPE value,                                           \
                                const std::vector<int64_t>& parameter_path = {}) = 0
    GET_SET_PARAMETER_ABSTRACT(bool, bool);
    GET_SET_PARAMETER_ABSTRACT(int, int64_t);
    GET_SET_PARAMETER_ABSTRACT(float, double);
    GET_SET_PARAMETER_ABSTRACT(color, uint64_t);
    GET_SET_PARAMETER_ABSTRACT(string, const char*);

#define GET_ARRAY_PARAMETER_ABSTRACT(AVS_TYPE, TYPE)  \
    virtual std::vector<TYPE> get_##AVS_TYPE##_array( \
        AVS_Parameter_Handle parameter,               \
        const std::vector<int64_t>& parameter_path = {}) = 0
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
    uint32_t string_load_legacy(const char* src,
                                std::string& dest,
                                uint32_t max_length);
    uint32_t string_save_legacy(std::string& src,
                                char* dest,
                                uint32_t max_length = 0x7fff,
                                bool with_nt = false);
    uint32_t string_nt_load_legacy(const char* src,
                                   std::string& dest,
                                   uint32_t max_length);
    uint32_t string_nt_save_legacy(std::string& src, char* dest, uint32_t max_length);

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

template <class Info_T, class Config_T, class Global_Config_T = Effect_Config>
class Configurable_Effect : public Effect {
   public:
    // TODO [clean]: make info & config protected.
    static Info_T info;
    Config_T config;

    Configurable_Effect(AVS_Handle avs = 0) {
        this->prune_empty_globals();
        this->init_global_config(avs);
    };
    ~Configurable_Effect() { this->remove_from_global_instances(); };
    Effect* clone() {
        auto cloned = new Configurable_Effect(*this);
        cloned->handle = h_components.get();
        return cloned;
    };
    bool can_have_child_components() { return this->info.can_have_child_components(); };
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
    T get(AVS_Parameter_Handle parameter,
          const std::vector<int64_t>& parameter_path = {}) {
        if (parameter == 0) {
            return parameter_dispatch<T>::zero();
        }
        auto param = this->info.get_parameter_from_handle(parameter);
        if (param == NULL) {
            return parameter_dispatch<T>::zero();
        }
        auto addr = this->get_config_address(param, parameter_path);
        return parameter_dispatch<T>::get(addr);
    };

    template <typename T>
    bool set(AVS_Parameter_Handle parameter,
             T value,
             const std::vector<int64_t>& parameter_path = {}) {
        if (parameter == 0) {
            return false;
        }
        auto param = this->info.get_parameter_from_handle(parameter);
        if (param == NULL) {
            return false;
        }
        auto addr = this->get_config_address(param, parameter_path);
        parameter_dispatch<T>::set(param, addr, value);
        if (param->on_value_change != NULL) {
            param->on_value_change(this, param, parameter_path);
        }
        return true;
    };

    template <typename T>
    std::vector<T>& get_array(AVS_Parameter_Handle parameter,
                              const std::vector<int64_t>& parameter_path = {}) {
        static std::vector<T> empty;
        if (parameter == 0) {
            return empty;
        }
        auto param = this->info.get_parameter_from_handle(parameter);
        if (param == NULL) {
            return empty;
        }
        auto addr = this->get_config_address(param, parameter_path);
        return parameter_dispatch<T>::get_array(addr);
    };

    size_t parameter_list_length(AVS_Parameter_Handle parameter,
                                 const std::vector<int64_t>& parameter_path = {}) {
        if (parameter == 0) {
            return 0;
        }
        auto param = this->info.get_parameter_from_handle(parameter);
        if (param == NULL) {
            return 0;
        }
        auto addr = this->get_config_address(param, parameter_path);
        return param->list_length(addr);
    };
    bool parameter_list_entry_add(
        AVS_Parameter_Handle parameter,
        int64_t before,
        std::vector<AVS_Parameter_Value> parameter_values = {},
        const std::vector<int64_t>& parameter_path = {}) {
        if (parameter == 0) {
            return false;
        }
        auto param = this->info.get_parameter_from_handle(parameter);
        if (param == NULL) {
            return false;
        }
        auto addr = this->get_config_address(param, parameter_path);
        bool success = param->list_add(addr, param->num_child_parameters_max, &before);
        if (success) {
            std::vector<int64_t> new_parameter_path = parameter_path;
            new_parameter_path.push_back(before);
            for (auto const& pv : parameter_values) {
                auto pv_param = this->info.get_parameter_from_handle(pv.parameter);
                auto pv_addr = this->get_config_address(pv_param, new_parameter_path);
                if (pv_param == NULL) {
                    continue;
                }
                switch (pv_param->type) {
                    case AVS_PARAM_BOOL:
                        parameter_dispatch<bool>::set(pv_param, pv_addr, pv.value.b);
                        break;
                    case AVS_PARAM_SELECT:
                    case AVS_PARAM_INT:
                        parameter_dispatch<int64_t>::set(pv_param, pv_addr, pv.value.i);
                        break;
                    case AVS_PARAM_FLOAT:
                        parameter_dispatch<double>::set(pv_param, pv_addr, pv.value.f);
                        break;
                    case AVS_PARAM_COLOR:
                        parameter_dispatch<uint64_t>::set(
                            pv_param, pv_addr, pv.value.c);
                        break;
                    case AVS_PARAM_STRING:
                        parameter_dispatch<const char*>::set(
                            pv_param, pv_addr, pv.value.s);
                        break;
                    case AVS_PARAM_LIST:
                    case AVS_PARAM_ACTION:
                    case AVS_PARAM_INT_ARRAY:
                    case AVS_PARAM_FLOAT_ARRAY:
                    case AVS_PARAM_COLOR_ARRAY:
                    case AVS_PARAM_INVALID:
                    default: break;
                }
            }
            if (param->on_list_add != NULL) {
                param->on_list_add(this, param, parameter_path, before, 0);
            }
        }
        return success;
    };
    bool parameter_list_entry_move(AVS_Parameter_Handle parameter,
                                   int64_t from,
                                   int64_t to,
                                   const std::vector<int64_t>& parameter_path = {}) {
        if (parameter == 0) {
            return 0;
        }
        auto param = this->info.get_parameter_from_handle(parameter);
        if (param == NULL) {
            return false;
        }
        auto addr = this->get_config_address(param, parameter_path);
        bool success = param->list_move(addr, &from, &to);
        if (success && param->on_list_move != NULL) {
            param->on_list_move(this, param, parameter_path, from, to);
        }
        return success;
    };
    bool parameter_list_entry_remove(AVS_Parameter_Handle parameter,
                                     int64_t to_remove,
                                     const std::vector<int64_t>& parameter_path = {}) {
        if (parameter == 0) {
            return 0;
        }
        auto param = this->info.get_parameter_from_handle(parameter);
        if (param == NULL) {
            return false;
        }
        auto addr = this->get_config_address(param, parameter_path);
        bool success =
            param->list_remove(addr, param->num_child_parameters_min, &to_remove);
        if (success && param->on_list_remove != NULL) {
            param->on_list_remove(this, param, parameter_path, to_remove, 0);
        }
        return success;
    };

    bool run_action(AVS_Parameter_Handle parameter,
                    const std::vector<int64_t>& parameter_path = {}) {
        if (parameter == 0) {
            return false;
        }
        auto param = this->info.get_parameter_from_handle(parameter);
        if (param == NULL || param->on_value_change == NULL) {
            return false;
        }
        param->on_value_change(this, param, parameter_path);
        return true;
    };

#define GET_SET_PARAMETER(AVS_TYPE, TYPE)                                          \
    virtual TYPE get_##AVS_TYPE(AVS_Parameter_Handle parameter,                    \
                                const std::vector<int64_t>& parameter_path = {}) { \
        return get<TYPE>(parameter, parameter_path);                               \
    };                                                                             \
    virtual bool set_##AVS_TYPE(AVS_Parameter_Handle parameter,                    \
                                TYPE value,                                        \
                                const std::vector<int64_t>& parameter_path = {}) { \
        return set<TYPE>(parameter, value, parameter_path);                        \
    }
    GET_SET_PARAMETER(bool, bool);
    GET_SET_PARAMETER(int, int64_t);
    GET_SET_PARAMETER(float, double);
    GET_SET_PARAMETER(color, uint64_t);
    GET_SET_PARAMETER(string, const char*);

#define GET_ARRAY_PARAMETER(AVS_TYPE, TYPE)                \
    virtual std::vector<TYPE> get_##AVS_TYPE##_array(      \
        AVS_Parameter_Handle parameter,                    \
        const std::vector<int64_t>& parameter_path = {}) { \
        return get_array<TYPE>(parameter, parameter_path); \
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

    struct Global {
        AVS_Handle avs;
        Global_Config_T config;
        std::set<Configurable_Effect*> instances;
        Global(AVS_Handle avs) : avs(avs){};
    };
    std::shared_ptr<Global> global = nullptr;

   protected:  // TODO [clean]: Make this private as soon as NSEEL & EELTrans are fixed
    static std::vector<std::weak_ptr<Global>> globals;

   private:
    static void prune_empty_globals() {
        for (auto it = Configurable_Effect::globals.begin();
             it != Configurable_Effect::globals.end();) {
            if (it->expired()) {
                it = Configurable_Effect::globals.erase(it);
            } else {
                it++;
            }
        }
    }
    void init_global_config(AVS_Handle avs) {
        if (std::is_same<Global_Config_T, Effect_Config>::value) {
            return;
        }
        {
            std::shared_ptr<Global> tmp;
            for (auto& g : Configurable_Effect::globals) {
                if (g.expired()) {
                    continue;
                }
                tmp = g.lock();
                if (tmp->avs == avs) {
                    this->global = g.lock();
                    this->global->instances.insert(this);
                    return;
                }
            }
            tmp = std::make_shared<Global>(avs);
            this->globals.emplace_back(tmp);
            this->global = tmp;
            this->global->instances.insert(this);
        }
    };
    void remove_from_global_instances() {
        if (this->global.get()) {
            this->global->instances.erase(this);
        }
    };

    uint8_t* get_config_address(const Parameter* parameter,
                                const std::vector<int64_t>& parameter_path) {
        Effect_Config* _config = parameter->is_global
                                     ? (Effect_Config*)&this->global->config
                                     : (Effect_Config*)&this->config;
        return this->info.get_config_address(_config,
                                             parameter,
                                             this->info.num_parameters,
                                             this->info.parameters,
                                             parameter_path,
                                             0);
    };
};

template <class Info_T, class Config_T, class Global_Config_T>
Info_T Configurable_Effect<Info_T, Config_T, Global_Config_T>::info;
template <class Info_T, class Config_T, class Global_Config_T>
std::string Configurable_Effect<Info_T, Config_T, Global_Config_T>::desc;
template <class Info_T, class Config_T, class Global_Config_T>
std::vector<std::weak_ptr<
    typename Configurable_Effect<Info_T, Config_T, Global_Config_T>::Global>>
    Configurable_Effect<Info_T, Config_T, Global_Config_T>::globals;
