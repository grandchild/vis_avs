#pragma once

/**
 * == How to Create a New Effect ==
 *
 * An Effect has at least one "Config" struct, which consists of a set of "Parameters",
 * and an "Info" struct describing the config. To create the config and info, each
 * effect subclasses `Effect_Config` and `Effect_Info`, and creates its own class from
 * the `Configurable_Effect` template with those two classes as template parameters. If
 * you're unfamiliar with C++ templates this may be confusing, but you don't really need
 * to understand how templates work in order to implement a new Effect class.
 *
 * The incantations for a new Effect all follow the same pattern:
 *
 *   1. Declare the Config struct.
 *   2. Declare the Info struct, referencing the config. This is the hard part.
 *   3. Declare the Effect class, using the Config and Info struct names as template
 *      parameters.
 *
 * In more detail, if you wanted to create a new effect "Foo":
 *
 * 1.
 *
 * Declare a `Foo_Config` struct inheriting from `Effect_Config` (which can be empty if
 * your effect has no options). Set default values for every parameter (except lists).
 *
 *     struct Foo_Config : public Effect_Config {
 *         int64_t foo = 1;
 *         double bar = 0.5;
 *         // ...
 *     };
 *
 * Note that no "enabled" parameter is needed, every effect automatically gets one.
 *
 * All members of the config struct must use one of the following C++ types:
 *   - booleans:                 bool
 *   - integer values & selects: int64_t
 *   - floating point values:    double
 *   - color values:             uint64_t
 *   - strings:                  std::string
 *   Special cases:
 *   - parameter lists:          std::vector<T>, where T is the type of the child config
 *   - actions:                  Action parameters don't have a value & don't appear in
 *                               the config struct.
 *
 * You may declare a default constructor (but usually shouldn't need to) if special
 * things need to happen on init of your config (e.g. to fill a list with a default set
 * of entries). If you define a non-default constructor, you have to define the default
 * constructor explicitly too (write `Foo_Config() = default;`). Declaring any other
 * methods inside your struct invokes compiler-specific and possibly undefined behavior
 * because of the way `Effect_Info` is implemented! Use free functions instead, if you
 * must, or rather just add them to your effect class instead of the config.
 *
 * 2.
 *
 * Declare an accompanying `Foo_Info` struct inheriting from `Effect_Info`, which has
 * some required fields:
 *
 *     struct Foo_Info : public Effect_Info {
 *         static constexpr char* group = "Render";
 *         static constexpr char* name = "Foo";
 *
 *         static constexpr uint32_t num_parameters = 2;
 *         static constexpr Parameter parameters[num_parameters] = {
 *             P_INT(offsetof(Foo_Config, foo), "Foo", "Does a foo"),
 *             P_FRANGE(offsetof(Foo_Config, bar), "Bar", 0.0, 1.0, "Does bar"),
 *         };
 *
 *         EFFECT_INFO_GETTERS;
 *     };
 *
 * Use the `P_` functions to safely add entries to the `parameters` array. Alternatively
 * you can declare `Parameter` structs yourself, and set unneeded fields to 0 or NULL as
 * appropriate. The `P_` functions do all of this for you and are highly recommended.
 * For select- and list-type parameters, use `P_SELECT` and `P_LIST` respectively.
 *
 * There are quite a few requirements on the Info struct and inter-dependencies with the
 * Config struct:
 *
 *   - `group` can be any string. Groups are not part of the identity of the
 *     effect, so you can change the group of the effect and older presets will load
 *     just fine. It's _highly discouraged_ to make up your own category here. Instead
 *     see what other groups already exist, and find one that fits. If the existing
 *     groups won't, there's a "Misc" group for outliers.
 *
 *   - `name` can be any string (can include spaces, etc.). It should be unique among
 *     existing effects.
 *
 *   - `num_parameters` in Info must match the number of parameters defined in Config.
 *     (To be precise, it must not be larger. If it's smaller, then the remaining config
 *     parameters will be inaccessible for editing. Do with that information what you
 *     will.)
 *
 *   - There are a few different functions to choose from when declaring parameters:
 *
 *     - `PARAM` declares a generic parameter. You shouldn't generally use this one,
 *       rather use one of `P_BOOL`, `P_INT`, `P_FLOAT`, `P_COLOR` or `P_STRING`
 *       directly.
 *
 *     - `P_IRANGE` and `P_FRANGE` declare limited integer and floating-point
 *       parameters, respectively. Setting a value outside of the min/max range will set
 *       a clamped value. An editor UI may render a range-limited parameter as a slider.
 *
 *     - `P_SELECT` creates a parameter that can be selected from a list of options. The
 *       value type is actually an integer index (refer to avs_editor.h). To set the
 *       options pass the name of a function that returns a list of strings, and writes
 *       the length of the list to its input parameter. Have a look into existing
 *       effects for examples. A smart editor UI may turn a short list into a radio
 *       select, and a longer list into a dropdown select.
 *
 *     - `P_LIST` is an advanced feature not needed by most effects. It creates a
 *       list of sets of parameters. Declaring parameter lists will be explained in more
 *       detail further down (TODO).
 *
 *     - `P_ACTION` lets you register a function to be triggered through the API. An
 *       action parameter isn't represented by a config field and cannot be read or set.
 *
 *     - Most functions have an `_X`-suffixed version, which excludes them from being
 *       saved in a preset. So if you choose `P_*_X` instead of `P_*` the parameter will
 *       not be saved and not loaded back.
 *
 *     - The arguments to the `P_` function (which it shares with the others) are as
 *       follows:
 *
 *       - `offset` must be the offset, in bytes, of the parameter's field in its Config
 *         struct. Use `offsetof(Foo_Config, foo)` to let the compiler figure it out.
 *
 *       - `name` can be any string, and it must not be NULL. It must be unique among
 *         your effect's other parameters. It doesn't have to bear any resemblance to
 *         the config field (but it'd probably be wise). An editor UI may use this to
 *         label the parameter.
 *
 *       - `description` can be any string, or NULL. An editor UI may use this to
 *         further describe and explain the parameter if needed (e.g. in a tooltip).
 *
 *       - `on_value_change` (or `on_run` for action parameters) may be a function
 *         pointer or NULL. The function gets called whenever the value is changed
 *         (or the action triggered) through the editor API. The signature is:
 *
 *            void on_change(Effect* component, const Parameter* parameter,
 *                           std::vector<int64_t> parameter_path)
 *
 *         You could use `parameter->name` to identify a parameter if the function
 *         handles multiple parameter changes.
 *
 *         Both the `component` and `parameter` pointers are guaranteed to be non-NULL
 *         and valid. If the handler is for a parameter inside a list, `parameter_path`
 *         is guaranteed to be of the correct length to contain your list index, you
 *         don't need to test `parameter_path.size()` in your handlers. If your effect
 *         doesn't have list-type parameters, it's safe to just ignore `parameter_path`.
 *         For `P_LIST` use `on_add`, `on_move` & `on_remove` (see below).
 *
 *         Tip: Make the handler function a static member function of the info struct
 *         instead of a free function. This way it will not conflict with other effects'
 *         handler functions (which are often named similarly).
 *
 *     - `int_min`/`int_max` & `float_min`/`float_max` set the minimum and maximum value
 *       of a ranged parameter. Note that if `min == max`, then the parameter is
 *       unlimited!
 *
 *     - `get_options` is a function to retrieve the list of options. This is not a
 *       static list because some lists aren't known at compile time (e.g. image
 *       filenames). The signature is:
 *
 *         const char* const* get_options(int64_t* length_out)
 *
 *       The list length must be written to the location pointed to by `length_out`.
 *
 *     - If `length_min` and `length_max` in `P_LIST` are both 0 then the list length is
 *       unlimited. If they are both the same but non-zero the list is fixed at that
 *       size (elements can still be moved around).
 *
 *     - `on_add`, `on_move` & `on_remove` are three callback functions that get called
 *       whenever the respective action was successfully performed on the parameter
 *       list. The signature for all 3 functions is:
 *
 *         void on_list_change(Effect* component, const Parameter* parameter,
 *                             std::vector<int64_t> parameter_path,
 *                             int64_t index1, int64_t index2)
 *
 *       where:
 *
 *       - for `on_add` `index1` is the new index and `index2` is always zero.
 *       - for `on_move` `index1` is the previous index and `index2` is the new index.
 *       - for `on_remove` `index1` is the removed index and `index2` is always zero.
 *
 *       With the signatures all the same, you can pass the same function to all three.
 *
 *       Note that the `on_move` handler will not be called if the element is "moved" to
 *       the same index it's already at. It's only called for actual changes.
 *
 *   - Write `EFFECT_INFO_GETTERS;` at the end of the Info class declaration, it will
 *     fill in some methods needed for accessing the fields declared above. If your
 *     effect has no parameters, i.e. it can only be enabled and disabled, then use the
 *     macro `EFFECT_INFO_GETTERS_NO_PARAMETERS` instead, otherwise it will not compile
 *     on MSVC.
 *
 * 3.
 *
 * Now you're finally ready to declare your `E_Foo` class, specializing the
 * `Configurable_Effect` template, using the Info and Config struct names as template
 * parameters:
 *
 *     class E_Foo : public Configurable_Effect<Foo_Info, Foo_Config> {
 *        public:
 *         E_Foo() {
 *             // Access config through `this->config`.
 *             this->config.foo += 1;
 *         };
 *         // ...
 *     };
 *
 * This is the basic recipe for setting up a configurable effect.
 *
 * TODO: Explain variable length parameter lists in more detail. For now have a look at
 *       ColorMap for an effect with _all_ the bells and whistles.
 * TODO: Move this into an example effect file.
 *
 * Q: Why are Config and Info `struct`s, but Effect is a `class`?
 * A: In C++, for all intents and purposes `class` and `struct` are interchangeable,
 *    with one key difference: `struct` members are public by default, and `class`
 *    members private. So `struct` is just less to write if all members are public.
 *    Additionally, using `struct`s is sometimes used to signal a "simpler" object type
 *    containing predominantly data members (as opposed to methods). This is also the
 *    case here.
 */

#include "avs_editor.h"
#include "handles.h"

#include <algorithm>  // std::move() for list_move
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

/**
 * This file encapsulates most of the dirty parts around component- and parameter
 * introspection (with effect.h covering the rest).
 *
 * The goal is to abstract away the introspection (which is bound to be hacky, because
 * C++ is not tuned for this) and keep both the external API _and_ the internal code for
 * individual effects as clean and simple as possible. Beyond declaration of the
 * Effect_Info struct, effects should have simple direct access to a struct with basic
 * config parameters, without any further intermediaries like getters/setters. In the
 * other direction the API should be as simple as C allows, with plain numerical
 * handles and arrays.
 *
 * The result is that the Effect_Info structure declaration is the most verbose and
 * fragile part. But it does make sense to make the trade-off this way, because it's
 * only declared statically at compile time once per effect and after that never really
 * touched again by any outside (API) or inside (effect) code.
 */

/**
 * The base for all effects' config structs. Must be empty, because `offsetof` is not
 * strictly supported for structs/classes with inherited members.
 */
struct Effect_Config {};

class Effect;      // declared in effect.h
struct Parameter;  // declared below

/* Function types for various Parameter fields. */
typedef void (*value_change_handler)(Effect* component,
                                     const Parameter* parameter,
                                     std::vector<int64_t> parameter_path);
typedef const char* const* (*options_getter)(int64_t* length_out);
typedef size_t (*list_length_getter)(uint8_t* list_address);
typedef bool (*list_adder)(uint8_t* list_address, uint32_t length_max, int64_t* before);
typedef bool (*list_mover)(uint8_t* list_address, int64_t* from, int64_t* to);
typedef bool (*list_remover)(uint8_t* list_address,
                             uint32_t length_min,
                             int64_t* to_remove);
typedef void (*list_edit_handler)(Effect* component,
                                  const Parameter* parameter,
                                  std::vector<int64_t> parameter_path,
                                  int64_t index1,
                                  int64_t index2);

struct Parameter {
    AVS_Parameter_Handle handle = 0;
    size_t offset = 0;
    AVS_Parameter_Type type = AVS_PARAM_INVALID;
    const char* name = NULL;
    const char* description = NULL;
    bool is_saved = true;
    bool is_global = false;
    value_change_handler on_value_change = NULL;

    options_getter get_options = NULL;

    int64_t int_min = 0;
    int64_t int_max = 0;
    double float_min = 0.0;
    double float_max = 0.0;

    uint32_t num_child_parameters = 0;
    uint32_t num_child_parameters_min = 0;
    uint32_t num_child_parameters_max = 0;
    const Parameter* child_parameters = NULL;
    size_t sizeof_child = 1;

    list_length_getter list_length = NULL;
    list_adder list_add = NULL;
    list_mover list_move = NULL;
    list_remover list_remove = NULL;
    list_edit_handler on_list_add = NULL;
    list_edit_handler on_list_move = NULL;
    list_edit_handler on_list_remove = NULL;
};

template <typename T>
size_t list_length(uint8_t* list_address) {
    auto list = (std::vector<T>*)list_address;
    return list->size();
}
template <typename T>
bool list_add(uint8_t* list_address, uint32_t length_max, int64_t* before) {
    auto list = (std::vector<T>*)list_address;
    if (list->size() >= length_max) {
        return false;
    }
    if (*before < 0 || (size_t)(*before < 0 ? 0 : *before) >= list->size()) {
        list->emplace_back();
        *before = list->size() - 1;
    } else {
        list->emplace(list->begin() + *before);
    }
    return true;
}
template <typename T>
bool list_move(uint8_t* list_address, int64_t* from, int64_t* to) {
    auto list = (std::vector<T>*)list_address;
    if (*from < 0) {
        *from = list->size() - 1;
    }
    if (*to < 0) {
        *to = list->size() - 1;
    }
    if (*from == *to) {
        // Technically correct, but don't run the on_move handler.
        return false;
    }
    if ((size_t)*from >= list->size() || (size_t)*to >= list->size()) {
        return false;
    }
    std::vector<T> new_list;
    bool found_from = false;
    bool found_to = false;
    for (auto e = list->begin(); e != list->end(); e++) {
        if (e == list->begin() + *to + found_from) {
            found_to = true;
            new_list.push_back((*list)[*from]);
        }
        if (e != list->begin() + *from) {
            new_list.push_back(*e);
        } else {
            found_from = true;
        }
    }
    if (!found_to) {
        new_list.push_back((*list)[*from]);
    }
    list->swap(new_list);
    return true;
}
template <typename T>
bool list_remove(uint8_t* list_address, uint32_t length_min, int64_t* to_remove) {
    auto list = (std::vector<T>*)list_address;
    if (list->size() <= length_min) {
        return false;
    }
    if (*to_remove < 0) {
        *to_remove = list->size() + *to_remove;
    }
    if (*to_remove < 0 || (size_t)(*to_remove) >= list->size()) {
        return false;
    }
    list->erase(std::next(list->begin(), *to_remove));
    return true;
}

constexpr Parameter PARAM(size_t offset,
                          AVS_Parameter_Type type,
                          const char* name,
                          const char* description = NULL,
                          value_change_handler on_value_change = NULL,
                          bool is_saved = true) {
    Parameter _param = Parameter();
    _param.handle = Handles::comptime_get(name);
    _param.offset = offset;
    _param.type = type;
    _param.name = name;
    _param.description = description;
    _param.is_saved = is_saved;
    _param.on_value_change = on_value_change;
    return _param;
}
#define PARAM_SIMPLE(TYPE)                                                           \
    constexpr Parameter P_##TYPE(size_t offset,                                      \
                                 const char* name,                                   \
                                 const char* description = NULL,                     \
                                 value_change_handler on_value_change = NULL,        \
                                 bool is_saved = true) {                             \
        return PARAM(                                                                \
            offset, AVS_PARAM_##TYPE, name, description, on_value_change, is_saved); \
    }                                                                                \
    constexpr Parameter P_##TYPE##_X(size_t offset,                                  \
                                     const char* name,                               \
                                     const char* description = NULL,                 \
                                     value_change_handler on_value_change = NULL) {  \
        return PARAM(                                                                \
            offset, AVS_PARAM_##TYPE, name, description, on_value_change, false);    \
    }                                                                                \
    constexpr Parameter P_##TYPE##_G(size_t offset,                                  \
                                     const char* name,                               \
                                     const char* description = NULL,                 \
                                     value_change_handler on_value_change = NULL,    \
                                     bool is_saved = true) {                         \
        Parameter _param =                                                           \
            P_##TYPE(offset, name, description, on_value_change, is_saved);          \
        _param.is_global = true;                                                     \
        return _param;                                                               \
    }                                                                                \
    constexpr Parameter P_##TYPE##_GX(size_t offset,                                 \
                                      const char* name,                              \
                                      const char* description = NULL,                \
                                      value_change_handler on_value_change = NULL) { \
        Parameter _param = P_##TYPE##_X(offset, name, description, on_value_change); \
        _param.is_global = true;                                                     \
        return _param;                                                               \
    }
PARAM_SIMPLE(BOOL)
PARAM_SIMPLE(INT)
PARAM_SIMPLE(FLOAT)
PARAM_SIMPLE(COLOR)
PARAM_SIMPLE(STRING)
PARAM_SIMPLE(INT_ARRAY)
PARAM_SIMPLE(FLOAT_ARRAY)
PARAM_SIMPLE(COLOR_ARRAY)

constexpr Parameter P_IRANGE(size_t offset,
                             const char* name,
                             int64_t int_min = 0,
                             int64_t int_max = 0,
                             const char* description = NULL,
                             value_change_handler on_value_change = NULL,
                             bool is_saved = false) {
    Parameter _param =
        PARAM(offset, AVS_PARAM_INT, name, description, on_value_change, is_saved);
    _param.int_min = int_min;
    _param.int_max = int_max;
    return _param;
}
constexpr Parameter P_IRANGE_G(size_t offset,
                               const char* name,
                               int64_t int_min = 0,
                               int64_t int_max = 0,
                               const char* description = NULL,
                               value_change_handler on_value_change = NULL,
                               bool is_saved = false) {
    Parameter _param = P_IRANGE(
        offset, name, int_min, int_max, description, on_value_change, is_saved);
    _param.is_global = true;
    return _param;
}

constexpr Parameter P_FRANGE(size_t offset,
                             const char* name,
                             double float_min = 0,
                             double float_max = 0,
                             const char* description = NULL,
                             value_change_handler on_value_change = NULL,
                             bool is_saved = true) {
    Parameter _param =
        PARAM(offset, AVS_PARAM_FLOAT, name, description, on_value_change, is_saved);
    _param.float_min = float_min;
    _param.float_max = float_max;
    return _param;
}
constexpr Parameter P_FRANGE_G(size_t offset,
                               const char* name,
                               double float_min = 0,
                               double float_max = 0,
                               const char* description = NULL,
                               value_change_handler on_value_change = NULL,
                               bool is_saved = true) {
    Parameter _param = P_FRANGE(
        offset, name, float_min, float_max, description, on_value_change, is_saved);
    _param.is_global = true;
    return _param;
}

constexpr Parameter P_SELECT(size_t offset,
                             const char* name,
                             options_getter get_options,
                             const char* description = NULL,
                             value_change_handler on_value_change = NULL,
                             bool is_saved = true) {
    Parameter _param =
        PARAM(offset, AVS_PARAM_SELECT, name, description, on_value_change, is_saved);
    _param.get_options = get_options;
    return _param;
}
constexpr Parameter P_SELECT_X(size_t offset,
                               const char* name,
                               options_getter get_options,
                               const char* description = NULL,
                               value_change_handler on_value_change = NULL) {
    Parameter _param =
        PARAM(offset, AVS_PARAM_SELECT, name, description, on_value_change, false);
    _param.get_options = get_options;
    return _param;
}
constexpr Parameter P_SELECT_G(size_t offset,
                               const char* name,
                               options_getter get_options,
                               const char* description = NULL,
                               value_change_handler on_value_change = NULL,
                               bool is_saved = true) {
    Parameter _param =
        P_SELECT(offset, name, get_options, description, on_value_change, is_saved);
    _param.is_global = true;
    return _param;
}
constexpr Parameter P_SELECT_GX(size_t offset,
                                const char* name,
                                options_getter get_options,
                                const char* description = NULL,
                                value_change_handler on_value_change = NULL) {
    Parameter _param =
        P_SELECT_X(offset, name, get_options, description, on_value_change);
    _param.is_global = true;
    return _param;
}

template <typename Subtype>
constexpr Parameter P_LIST(size_t offset,
                           const char* name,
                           const Parameter* list,
                           uint32_t length,
                           uint32_t length_min = 0,
                           uint32_t length_max = 0,
                           const char* description = NULL,
                           list_edit_handler on_add = NULL,
                           list_edit_handler on_move = NULL,
                           list_edit_handler on_remove = NULL,
                           bool is_saved = true) {
    Parameter _param = PARAM(offset, AVS_PARAM_LIST, name, description, NULL, is_saved);
    _param.num_child_parameters = length;
    _param.num_child_parameters_min = length_min;
    _param.num_child_parameters_max =
        (length_max == length_min && length_max == 0) ? UINT32_MAX : length_max;
    _param.child_parameters = list;
    _param.sizeof_child = sizeof(Subtype);
    _param.list_length = list_length<Subtype>;
    _param.list_add = list_add<Subtype>;
    _param.list_move = list_move<Subtype>;
    _param.list_remove = list_remove<Subtype>;
    _param.on_list_add = on_add;
    _param.on_list_move = on_move;
    _param.on_list_remove = on_remove;
    return _param;
}
template <typename Subtype>
constexpr Parameter P_LIST_G(size_t offset,
                             const char* name,
                             const Parameter* list,
                             uint32_t length,
                             uint32_t length_min = 0,
                             uint32_t length_max = 0,
                             const char* description = NULL,
                             list_edit_handler on_add = NULL,
                             list_edit_handler on_move = NULL,
                             list_edit_handler on_remove = NULL,
                             bool is_saved = true) {
    Parameter _param = P_LIST<Subtype>(offset,
                                       name,
                                       list,
                                       length,
                                       length_min,
                                       length_max,
                                       description,
                                       on_add,
                                       on_move,
                                       on_remove,
                                       is_saved);
    _param.is_global = true;
    return _param;
}

constexpr Parameter P_ACTION(const char* name,
                             value_change_handler on_run,
                             const char* description = NULL) {
    return PARAM(0, AVS_PARAM_ACTION, name, description, on_run, false);
}

struct Effect_Info {
    virtual const char* get_group() const = 0;
    virtual const char* get_name() const = 0;
    virtual const char* get_help() const = 0;
    static constexpr uint32_t num_parameters = 0;
    static constexpr Parameter* parameters = NULL;
    virtual uint32_t get_num_parameters() const { return 0; };
    virtual const Parameter* get_parameters() const { return NULL; };
    virtual const AVS_Parameter_Handle* get_parameters_for_api() const { return NULL; };
    virtual bool can_have_child_components() const { return false; };
    const Parameter* get_parameter_from_handle(AVS_Parameter_Handle to_find) {
        return this->_get_parameter_from_handle(
            this->get_num_parameters(), this->get_parameters(), to_find);
    }
    static const Parameter* _get_parameter_from_handle(uint32_t num_parameters,
                                                       const Parameter* parameters,
                                                       AVS_Parameter_Handle to_find) {
        for (uint32_t i = 0; i < num_parameters; i++) {
            if (parameters[i].handle == to_find) {
                return &parameters[i];
            }
            if (parameters[i].type == AVS_PARAM_LIST) {
                const Parameter* subtree_result =
                    Effect_Info::_get_parameter_from_handle(
                        parameters[i].num_child_parameters,
                        parameters[i].child_parameters,
                        to_find);
                if (subtree_result != NULL) {
                    return subtree_result;
                }
            }
        }
        return NULL;
    }
    /**
     * This is the heart of ugliness around parameter introspection. It makes use of
     * `sizeof` and `offsetof`, both recorded in each field's Parameter struct, to find
     * and return the data address for a specific parameter in the config struct.
     */
    static uint8_t* get_config_address(Effect_Config* config,
                                       const Parameter* parameter,
                                       uint32_t num_params,
                                       const Parameter* param_list,
                                       const std::vector<int64_t>& parameter_path,
                                       uint32_t cur_depth) {
        for (uint32_t i = 0; i < num_params; i++) {
            uint8_t* config_data = (uint8_t*)config;
            uint8_t* parameter_address = &config_data[param_list[i].offset];
            if (param_list[i].type == AVS_PARAM_LIST
                && cur_depth < parameter_path.size()) {
                auto list = (std::vector<Effect_Config>*)parameter_address;
                size_t list_size =
                    list->size() * sizeof(Effect_Config) / param_list[i].sizeof_child;
                int64_t clamped_size =
#if SIZE_MAX > INT64_MAX
                    (list_size > INT64_MAX ? INT64_MAX : (int64_t)list_size);
#else
                    (int64_t)list_size;
#endif
                int64_t list_index = parameter_path[cur_depth];
                if (list_index < 0) {
                    // wrap -1, -2, etc. around to offset-from-back
                    list_index += list_size;
                }
                if (list_index < 0 || list_index >= clamped_size) {
                    continue;
                }
                auto child_config =
                    (list->data() + list_index * param_list[i].sizeof_child);
                auto subtree_result =
                    Effect_Info::get_config_address(child_config,
                                                    parameter,
                                                    param_list[i].num_child_parameters,
                                                    param_list[i].child_parameters,
                                                    parameter_path,
                                                    cur_depth + 1);
                if (subtree_result != NULL) {
                    return subtree_result;
                }
            }
            if (parameter->handle == param_list[i].handle) {
                return parameter_address;
            }
        }
        return NULL;
    };

    /**
     * In the case of nested parameters, the child handles have to be returned as a
     * plain array in the API. But the handles are inside the Parameter's children
     * structs. So, upon first request, cache all child handles in a vector in the
     * global 'h_parameter_children' map, keyed by their parent parameter's handle.
     * Finally, return the plain C array from the vector.
     */
    const AVS_Parameter_Handle* get_parameter_children_for_api(
        const Parameter* parameter,
        bool reset = false) const {
        if (parameter->type != AVS_PARAM_LIST) {
            return NULL;
        }
        if (h_parameter_children.find(parameter->handle) == h_parameter_children.end()
            || reset) {
            h_parameter_children[parameter->handle].clear();
            for (uint32_t i = 0; i < parameter->num_child_parameters; i++) {
                h_parameter_children[parameter->handle].push_back(
                    parameter->child_parameters[i].handle);
            }
        }
        return h_parameter_children[parameter->handle].data();
    };
};

/**
 * Every struct inheriting from Effect_Info should add this macro to the bottom of the
 * effect's info-struct declaration. This macro defines all the accessor methods to the
 * static members an Effect_Info subclass should define.
 * We need these constructs because static members in the child class cannot be accessed
 * by automatically inherited virtual functions. So we need to explicitly override each
 * one in each child class.
 */
// Use this first one only for effects that have no parameters, i.e. only "enable".
// Use EFFECT_INFO_GETTERS for all other effects.
#define EFFECT_INFO_GETTERS_NO_PARAMETERS                          \
    virtual const char* get_group() const { return this->group; }; \
    virtual const char* get_name() const { return this->name; };   \
    virtual const char* get_help() const { return this->help; };

#define EFFECT_INFO_GETTERS                                                       \
    EFFECT_INFO_GETTERS_NO_PARAMETERS                                             \
    virtual uint32_t get_num_parameters() const { return this->num_parameters; }; \
    virtual const Parameter* get_parameters() const { return this->parameters; }; \
    virtual const AVS_Parameter_Handle* get_parameters_for_api() const {          \
        static AVS_Parameter_Handle parameters_for_api[this->num_parameters];     \
        for (int64_t i = 0; i < this->num_parameters; i++) {                      \
            parameters_for_api[i] = this->parameters[i].handle;                   \
        }                                                                         \
        return parameters_for_api;                                                \
    };

/**
 * This one is only useful for Root and Effect List. All other effects automatically
 * inherit the default `can_have_child_components()` method (which always returns
 * `false`) from the `Effect_Info` base.
 */
#define EFFECT_INFO_GETTERS_WITH_CHILDREN \
    EFFECT_INFO_GETTERS;                  \
    virtual bool can_have_child_components() const { return true; }

/**
 * This is needed by the `Configurable_Effect` template class, but it's here since it
 * fits the context better, and effect.h shouldn't be _too_ messy.
 *
 * Every parameter type needs a `get()`, `set()` and `zero()` method. By explicitly
 * specializing a template struct for this collection of methods, but only for each of
 * the allowed scalar parameter types, the correct value and return types for these
 * methods can be checked in `Configurable_Effect`'s `get()` and `set()` at compile
 * time.
 */
template <typename T>
struct parameter_dispatch;

template <>
struct parameter_dispatch<bool> {
    static bool get(uint8_t* config_data) { return *(bool*)config_data; };
    static void set(const Parameter*, uint8_t* addr, bool value) {
        *(bool*)addr = value;
    }
    static bool zero() { return false; };
};

template <>
struct parameter_dispatch<int64_t> {
    static int64_t get(uint8_t* config_data) { return *((int64_t*)config_data); };
    static void set(const Parameter* parameter, uint8_t* addr, int64_t value) {
        auto _min = parameter->int_min;
        auto _max = parameter->int_max;
        if (_min != _max) {
            value = value < _min ? _min : value > _max ? _max : value;
        }
        *(int64_t*)addr = value;
    }
    static std::vector<int64_t>& get_array(uint8_t* config_data) {
        return *(std::vector<int64_t>*)config_data;
    };
    static int64_t zero() { return 0; };
};

template <>
struct parameter_dispatch<double> {
    static double get(uint8_t* config_data) { return *(double*)config_data; };
    static void set(const Parameter* parameter, uint8_t* addr, double value) {
        auto _min = parameter->float_min;
        auto _max = parameter->float_max;
        if (_min != _max) {
            value = value < _min ? _min : value > _max ? _max : value;
        }
        *(double*)addr = value;
    }
    static std::vector<double>& get_array(uint8_t* config_data) {
        return *(std::vector<double>*)config_data;
    };
    static double zero() { return 0.0; };
};

template <>
struct parameter_dispatch<uint64_t> {
    static uint64_t get(uint8_t* config_data) { return *((uint64_t*)config_data); };
    static void set(const Parameter*, uint8_t* addr, uint64_t value) {
        *(uint64_t*)addr = value;
    }
    static std::vector<uint64_t>& get_array(uint8_t* config_data) {
        return *(std::vector<uint64_t>*)config_data;
    };
    static uint64_t zero() { return 0; };
};

template <>
struct parameter_dispatch<const char*> {
    static const char* get(uint8_t* config_data) {
        return ((std::string*)config_data)->c_str();
    };
    static void set(const Parameter*, uint8_t* addr, const char* value) {
        *(std::string*)addr = value;
    };

    static const char* zero() { return ""; };
};
