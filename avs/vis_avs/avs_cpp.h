#pragma once

/**
 * C++ wrapper around both the AVS basic API and the editor API.
 *
 * Basic usage:
 *
 *   int main() {
 *       AVS avs;
 *       avs.load("path/to/my/preset.avs");
 *       void* framebuffer = malloc(sizeof(uint32_t) * 800 * 600);
 *       while (true) {
 *           if (!avs.render(framebuffer, 800, 600)) {
 *               std::cout << "Error: " << avs.error() << "\n";
 *               return 1;
 *           }
 *           // Display the framebuffer.
 *       }
 *       return 0;
 *   }
 *
 *
 * Editor usage:
 *
 *   int main() {
 *       AVS avs;
 *
 *       // Get a list of effects available
 *       for(auto effect : avs.effects_library) {
 *           std::cout << "> " << effect.group() << " / " << effect.name(); << "\n";
 *       }
 *
 *       // Create a component
 *       auto ssc = avs.create("Super Scope");
 *       if (ssc.invalid()) {
 *           std::cout << "Create error: " << avs.error() << "\n";
 *           return 1;
 *       }
 *
 *       // You can also
 *       auto clear_screen = avs.create("Clear Screen");
 *       if (clear_screen.invalid()) {
 *           std::cout << "Error: " << avs.error() << "\n";
 *           return 1;
 *       }
 *
 *       // Oops, the clear screen should be _before_ the SSC!
 *       clear_screen.move(ssc, AVS_COMPONENT_POSITION_BEFORE);
 *
 *       // To avoid this, you can place effects directly when creating them:
 *       auto movement = avs.create("Movement", ssc, AVS_COMPONENT_POSITION_AFTER);
 *       if (movement.invalid()) {
 *           std::cout << "Error: " << avs.error() << "\n";
 *           return 1;
 *       }
 *
 *       // Really, this whole "Clear Screen" thing was quite embarrassing:
 *       clear_screen.remove();
 *       // Using `clear_screen` from now on will result in errors, because the
 *       // underlying handle was invalidated!
 *
 *       // Edit values through the component's `config` member:
 *       ssc.config["Code Init"] = "n = 100;";
 *       ssc.config["Code Point"] = "x = i * 2 - 1; y = v;";
 *       // Some values are inside mutable lists.
 *       ssc.config["Colors"][0]["Color"] = 0xff0000;
 *       ssc.config["Colors"].add_child();
 *       ssc.config["Colors"][1]["Color"] = 0x00ff00;
 *
 *       // To read values, you have to know their type:
 *       std::cout << "Current SSC init code: "
 *                 << ssc.config["Code Init"].as_string()
 *                 << "\n";
 *       // You can query the type through the parameter's `type()` method.
 *       // See `AVS_Parameter_Type` in `avs_editor.h` for the list of types to expect.
 *
 *       // Rendering code omitted...
 *
 *       return 0;
 *   }
 *
 *
 * Note: Every API object has a `handle` member that you can use to fall back onto the C
 * API if necessary.
 */

#include "avs.h"
#include "avs_editor.h"

#include <map>
#include <stdio.h>
#include <string>
#include <vector>

class AVS {
   private:
    class Effect;
    class Parameter {
        friend class Component_Parameter;

        AVS* avs;
        Effect* effect;
        AVS_Parameter_Info info;

       public:
        AVS_Parameter_Handle handle;

        Parameter() = delete;
        Parameter(AVS* avs, Effect* effect, AVS_Parameter_Handle parameter)
            : avs(avs), effect(effect), handle(parameter) {
            memset(&this->info, 0, sizeof(this->info));
            this->info.type = AVS_PARAM_INVALID;
            this->info.name = "";
            this->info.description = "";
            if (this->avs && this->effect && this->handle) {
                avs_parameter_info(
                    this->avs->handle, this->effect->handle, this->handle, &this->info);
            }
        };
        AVS_Parameter_Type type() const { return this->info.type; };
        const std::string name() const { return this->info.name; };
        const std::string description() const { return this->info.description; };
        int64_t int_min() const { return this->info.int_min; };
        int64_t int_max() const { return this->info.int_max; };
        double float_min() const { return this->info.float_min; };
        double float_max() const { return this->info.float_max; };
        std::vector<std::string> options() {
            avs_parameter_info(
                this->avs->handle, this->effect->handle, this->handle, &this->info);
            return std::vector<std::string>(
                this->info.options, this->info.options + this->info.options_length);
        };
        std::vector<Parameter> children() {
            std::vector<Parameter> out;
            for (uint32_t i = 0; i < this->info.children_length; i++) {
                out.emplace_back(this->avs, this->effect, this->info.children[i]);
            }
            return out;
        };
    };
    class Effect {
        AVS* avs;
        AVS_Effect_Info info;

       public:
        AVS_Effect_Handle handle;
        Effect() = delete;  // Use AVS::effects_library or AVS::effects_by_name.
        Effect(AVS* avs, const AVS_Effect_Handle effect) : avs(avs), handle(effect) {
            this->info.group = "";
            this->info.name = "";
            this->info.help = "";
            avs_effect_info(this->avs->handle, this->handle, &this->info);
        };

        std::string group() { return this->info.group; };
        std::string name() { return this->info.name; };
        std::string help() { return this->info.help; };
        std::vector<Parameter> children() {
            std::vector<Parameter> out;
            if (avs_effect_info(this->avs->handle, this->handle, &this->info)) {
                for (uint32_t i = 0; i < this->info.parameters_length; i++) {
                    out.emplace_back(this->avs, this, this->info.parameters[i]);
                }
            }
            return out;
        };
    };

   public:
    AVS_Handle handle;
    std::vector<Effect> effects_library;
    std::map<std::string, Effect*> effects_by_name;

    AVS(AVS_Audio_Source audio = AVS_AUDIO_INTERNAL,
        AVS_Beat_Source beat = AVS_BEAT_INTERNAL)
        : handle(avs_init(audio, beat)) {
        if (this->handle == 0) {
            this->_error = this->global_error_str();
            return;
        }
        uint32_t effects_length;
        auto effects = avs_effect_library(this->handle, &effects_length);
        for (uint32_t i = 0; i < effects_length; i++) {
            this->effects_library.emplace_back(this, effects[i]);
        }
        for (auto effect : this->effects_library) {
            this->effects_by_name[effect.name()] = &effect;
        }
    };
    ~AVS() { avs_free(this->handle); }
    AVS(AVS&) = delete;  // Always call by reference, e.g.: void foo(AVS& avs) {...}

    bool render_frame(void* framebuffer,
                      size_t width,
                      size_t height,
                      uint64_t time_ms = 0,
                      bool is_beat = false,
                      AVS_Pixel_Format pixel_format = AVS_PIXEL_RGB08) {
        return avs_render_frame(
            this->handle, framebuffer, width, height, time_ms, is_beat, pixel_format);
    }
    int32_t audio_set(int16_t* left,
                      int16_t* right,
                      size_t audio_length,
                      size_t samples_per_second) {
        return avs_audio_set(
            this->handle, left, right, audio_length, samples_per_second);
    }
    bool load(const char* file_path) {
        return avs_preset_load(this->handle, file_path);
    }
    bool set(const char* preset) { return avs_preset_set(this->handle, preset); }
    void save(const char* file_path, bool indent) {
        avs_preset_save(this->handle, file_path, indent);
    }
    std::string get() { return avs_preset_get(this->handle); }
    bool set_legacy(uint8_t* preset, size_t preset_length) {
        return avs_preset_set_legacy(this->handle, preset, preset_length);
    }
    std::vector<uint8_t> get_legacy() {
        size_t preset_length;
        auto preset = avs_preset_get_legacy(this->handle, &preset_length);
        return std::vector<uint8_t>(preset, preset + preset_length);
    }
    void save_legacy(const char* file_path) {
        avs_preset_save_legacy(this->handle, file_path);
    }
    std::string error() {
        if (this->_error == "") {
            return avs_error_str(this->handle);
        } else {
            return this->_error;
        }
    }
    void set_error(std::string error_str) { this->_error = error_str; };
    void clear_error() { this->_error = ""; };
    static std::string global_error_str() { return avs_error_str(0); };

   private:
    std::string _error;

    class Has_AVS_Ref {
       protected:
        AVS* avs;
        Has_AVS_Ref() = delete;
        Has_AVS_Ref(AVS* avs) : avs(avs){};
        AVS_Handle avs_handle() { return this->avs != NULL ? this->avs->handle : 0; };
        void clear_error() {
            if (this->avs != NULL) {
                this->avs->clear_error();
            }
        };
        void set_error(std::string error_msg) {
            if (this->avs != NULL) {
                this->avs->set_error(error_msg);
            }
        };
    };

    /**
     * The `Component_Parameter` class provides the interaction with config values for
     * components. Assign to a components parameters by name from it's `config` member:
     *
     *     component.config["Foo"] = 3;
     *
     * To retrieve values you have to know their type:
     *
     *     auto foo_value = component.config["Foo"].as_int();
     *
     * If you don't know the type at compile time, you can dispatch with `type()`:
     *
     *     switch(component.config["Foo"].type()) {
     *         case AVS_PARAM_INT: {
     *             auto foo_value = component.config["Foo"].as_int();
     *             // do things with foo_value
     *             break;
     *         }
     *         // ... other AVS_Parameter_Type cases
     *     }
     *
     * In all of this, `Component_Parameter` doesn't throw. If a parameter name doesn't
     * exist, then an invalid Parameter will be returned that only contains the zero
     * value for each type. Setting such a parameter's value will have no effect.
     *
     *                                       ---
     *
     * But the `Component_Parameter` class does double duty. It can be either a set of
     * parameters or a specific parameter. For regular effects, with just one top level
     * of parameters this is barely noticeable.
     *
     * But for the few effects with nested lists of parameter sets, this becomes
     * more relevant -- and potentially confusing. Consider the following _effect_
     * parameter layout:
     *
     *     Foo: int
     *     Bar:
     *       Baz: float
     *       Fleep: string
     *
     * Now, a _component_ created from this effect may have the following values:
     *
     *     Foo: 5
     *     Bar:
     *       - Baz: 0.1
     *         Fleep: hey
     *       - Baz: 0.2
     *         Fleep: ho
     *
     * Notice how the "Bar" list in the _effect_ expanded from just containing it's
     * constituent fields to a list of sets of these fields in the _component_.
     * This `Component_Parameter` class is dealing strictly with the components' view of
     * things.
     *
     * To change `0.2` to `0.3` at the end of the component's config we go down the
     * tree 4 times:
     *
     *     component.config["Bar"][1]["Baz"] = 0.3;
     *                 ^      ^    ^    ^
     *                 1      2    3    4
     *
     * All return values along the path, from `.config` onward are `Component_Parameter`
     * instances. But they are, always, in alternating modes:
     *     1 returns a set, the config root.
     *     2 returns a scalar parameter (which happens to be a list), "Bar".
     *     3 returns a set of parameters again, the second "Bar" entry.
     *     4 returns a scalar parameter again, which can be assigned to.
     *
     * The important thing to understand here is that sets are different from lists.
     * While in a list every entry has the same shape (consisting of the same
     * parameter sets), every entry in a set is a different parameter.
     *
     * Lists can be nested. The example only had a single list, but it should be easy to
     * imagine that e.g. "Fleep" in the example above would be another list-type
     * parameter, like "Bar", instead of a string.
     */
    class Component_Parameter : public Has_AVS_Ref {
        Effect* effect;
        AVS_Component_Handle component;
        Parameter parameter;
        std::vector<int64_t> parameter_path;
        bool is_set;  // This `Component_Parameter` instance is a fixed set of
                      // parameters, and not a scalar type or a list. `is_set`
                      // alternates every time one of the []-operators is invoked.
        bool is_config_root;  // This set of parameters is the topmost level of
                              // parameters for this component.

        std::vector<Component_Parameter> child_parameters() {
            std::vector<Component_Parameter> out;
            std::vector<Parameter> children;
            if (this->is_config_root) {
                children = this->effect->children();
            } else {
                children = this->parameter.children();
            }
            for (auto parameter : children) {
                out.emplace_back(this, parameter, this->parameter_path, !this->is_set);
            }
            return out;
        };

       public:
        Component_Parameter(AVS* avs, Effect* effect, AVS_Component_Handle component)
            : Has_AVS_Ref(avs),
              effect(effect),
              component(component),
              parameter(NULL, NULL, 0),
              is_set(true),
              is_config_root(true){};
        Component_Parameter(Component_Parameter* parent,
                            Parameter parameter,
                            std::vector<int64_t> parameter_path = {},
                            bool is_set = true)
            : Has_AVS_Ref(parent->avs),
              effect(parent->effect),
              component(parent->component),
              parameter(parameter),
              parameter_path(parameter_path),
              is_set(is_set),
              is_config_root(false){};

        AVS_Parameter_Type type() const { return this->parameter.type(); };
        const std::string name() const { return this->parameter.name(); };
        const std::string description() const { return this->parameter.description(); };
        const std::vector<std::string> options() { return this->parameter.options(); };

        bool add_child(int64_t before = -1) {
            this->clear_error();
            if (this->is_set) {
                this->set_error("Cannot use add_child() on a parameter set");
                return false;
            }
            return avs_parameter_list_element_add(this->avs->handle,
                                                  this->component,
                                                  this->parameter.handle,
                                                  before,
                                                  this->parameter_path.size(),
                                                  this->parameter_path.data());
        };
        bool move_child(int64_t from, int64_t to) {
            this->clear_error();
            if (this->is_set) {
                this->set_error("Cannot use move_child() on a parameter set");
                return false;
            }
            return avs_parameter_list_element_move(this->avs->handle,
                                                   this->component,
                                                   this->parameter.handle,
                                                   from,
                                                   to,
                                                   this->parameter_path.size(),
                                                   this->parameter_path.data());
        };
        bool remove_child(int64_t remove) {
            this->clear_error();
            if (this->is_set) {
                this->set_error("Cannot use remove_child() on a parameter set");
                return false;
            }
            return avs_parameter_list_element_remove(this->avs->handle,
                                                     this->component,
                                                     this->parameter.handle,
                                                     remove,
                                                     this->parameter_path.size(),
                                                     this->parameter_path.data());
        };

        bool run() {
            this->clear_error();
            if (!this->is_set && this->type() == AVS_PARAM_ACTION) {
                return avs_parameter_run_action(this->avs->handle,
                                                this->component,
                                                this->parameter.handle,
                                                this->parameter_path.size(),
                                                this->parameter_path.data());
            } else {
                this->set_error("Cannot use run() on a parameter set");
                return false;
            }
        };

        const std::vector<Component_Parameter> children() {
            this->clear_error();
            if (this->is_set) {
                return this->child_parameters();
            } else {
                std::vector<Component_Parameter> out;
                auto list_length =
                    avs_parameter_list_length(this->avs->handle,
                                              this->component,
                                              this->parameter.handle,
                                              this->parameter_path.size(),
                                              this->parameter_path.data());
                for (uint32_t i = 0; i < list_length; ++i) {
                    std::vector<int64_t> new_parameter_path(this->parameter_path);
                    new_parameter_path.push_back(i);
                    out.emplace_back(
                        this, this->parameter, new_parameter_path, !this->is_set);
                }
                return out;
            }
        };

        Component_Parameter operator[](std::string parameter_name) {
            this->clear_error();
            for (auto parameter : this->child_parameters()) {
                if (parameter_name == parameter.name()) {
                    return parameter;
                }
            }
            return Component_Parameter(this->avs, this->effect, this->component);
        };
        Component_Parameter operator[](int index) {
            this->clear_error();
            std::vector<int64_t> new_parameter_path(this->parameter_path);
            new_parameter_path.push_back(index);
            return Component_Parameter(
                this, this->parameter, new_parameter_path, !this->is_set);
        };

        // See SPECIALIZE_PARAMETER_ASSIGN macro below.
        template <typename T>
        Component_Parameter& operator=(T value);

#define PARAMETER_GETTER(AVS_TYPE, TYPE)                                  \
    TYPE as_##AVS_TYPE() {                                                \
        this->clear_error();                                              \
        return avs_parameter_get_##AVS_TYPE(this->avs->handle,            \
                                            this->component,              \
                                            this->parameter.handle,       \
                                            this->parameter_path.size(),  \
                                            this->parameter_path.data()); \
    }
        // Create: as_bool(), as_int(), as_float(), as_color(), as_string()
        PARAMETER_GETTER(bool, bool);
        PARAMETER_GETTER(int, int64_t);
        PARAMETER_GETTER(float, double);
        PARAMETER_GETTER(color, uint64_t);
        PARAMETER_GETTER(string, const char*);
    };

// This macro is invoked for all types outside of the AVS class.
// (Because template specializations have to be in a namespace scope.)
#define SPECIALIZE_PARAMETER_ASSIGN(AVS_TYPE, TYPE)                             \
    template <>                                                                 \
    AVS::Component_Parameter& AVS::Component_Parameter::operator=(TYPE value) { \
        this->clear_error();                                                    \
        avs_parameter_set_##AVS_TYPE(this->avs->handle,                         \
                                     this->component,                           \
                                     this->parameter.handle,                    \
                                     value,                                     \
                                     this->parameter_path.size(),               \
                                     this->parameter_path.data());              \
        return *this;                                                           \
    }

    class Component : public Has_AVS_Ref {
        Effect* effect;

       public:
        AVS_Component_Handle handle;
        Component_Parameter config;

        Component() = delete;  // Construct components through `AVS::create()`
        Component(AVS* avs, Effect* effect, AVS_Component_Handle component)
            : Has_AVS_Ref(avs),
              effect(effect),
              handle(component),
              config(this->avs, this->effect, component){};

        bool invalid() { return !(this->avs && this->effect && this->handle); };

        bool move(Component& relative_to, AVS_Component_Position direction) {
            this->clear_error();
            if (!avs_component_move(
                    this->avs->handle, this->handle, relative_to.handle, direction)) {
                return false;
            }
            return true;
        }

        Component duplicate() {
            this->clear_error();
            return Component(this->avs,
                             this->effect,
                             avs_component_duplicate(this->avs->handle, this->handle));
        };

        bool remove() { return avs_component_delete(this->avs->handle, this->handle); };

        std::vector<Component> children() {
            this->clear_error();
            std::vector<Component> out;
            uint32_t num_children;
            auto children =
                avs_component_children(this->avs->handle, this->handle, &num_children);
            for (uint32_t i = 0; i < num_children; i++) {
                out.emplace_back(this->avs, this->effect, children[i]);
            }
            return out;
        };
        Component& operator[](int64_t index) {
            this->clear_error();
            auto children = this->children();
            if (index < 0) {
                return children[children.size() + index];
            } else {
                if (index > UINT32_MAX) {
                    index = UINT32_MAX;
                }
                return children[index];
            }
        }
    };

   public:
    Component root() {
        this->clear_error();
        return Component(
            this, this->effects_by_name["Root"], avs_component_root(this->handle));
    };

    /**
     * Create AVS component by effect name.
     */
    Component create(
        std::string effect_name,
        Component relative_to = Component(NULL, NULL, 0),
        AVS_Component_Position direction = AVS_COMPONENT_POSITION_DONTCARE) {
        this->clear_error();
        if (this->effects_by_name.find(effect_name) == this->effects_by_name.end()) {
            this->_error = "Can't find effect with this name";
            return Component(this, NULL, 0);
        } else {
            this->_error = "";
        }
        auto effect = this->effects_by_name[effect_name];
        return this->create(effect, relative_to, direction);
    };

    /**
     * Create AVS component from an effect.
     */
    Component create(
        Effect* effect,
        Component relative_to = Component(NULL, NULL, 0),
        AVS_Component_Position direction = AVS_COMPONENT_POSITION_DONTCARE) {
        this->clear_error();
        return Component(
            this,
            effect,
            avs_component_create(
                this->handle, effect->handle, relative_to.handle, direction));
    };
};

SPECIALIZE_PARAMETER_ASSIGN(bool, bool);
SPECIALIZE_PARAMETER_ASSIGN(int, int64_t);
SPECIALIZE_PARAMETER_ASSIGN(int, int);
SPECIALIZE_PARAMETER_ASSIGN(float, double);
SPECIALIZE_PARAMETER_ASSIGN(color, uint64_t);
SPECIALIZE_PARAMETER_ASSIGN(color, unsigned int);
SPECIALIZE_PARAMETER_ASSIGN(string, const char*);
