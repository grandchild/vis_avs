/**
 * This is the (much more involved) editor API for AVS. For the basic player API see
 * "avs.h".
 *
 * This file provides data types and functions to edit the current preset of an AVS
 * instance at runtime.
 *
 * But first, some terminology:
 *
 *   "Effect":
 *     A type of visual effect that can be used in a preset. This is kind of like a type
 *     in programming. A "Component" is of an "Effect" type. AVS has a list of available
 *     Effects, that you can query with `avs_effects_get()`. Effects are described by
 *     their name, group and parameters.
 *
 *   "Component":
 *     An instance of an "Effect". Every Component has a pointer to its Effect struct.
 *     You can query it for information about the names and types of the available
 *     parameters, and use the `avs_parameter_get_*` and `-set_*` methods to read and
 *     write the Component's parameter values.
 *
 *   "Preset":
 *     A tree of "Components".
 */
#pragma once

#include "avs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t AVS_Component_Handle;
typedef uint32_t AVS_Effect_Handle;
typedef uint32_t AVS_Parameter_Handle;

enum AVS_Parameter_Type {
    AVS_PARAM_INVALID = -1,

    AVS_PARAM_LIST = 0,    // A list of several sets of child parameters.
                           // The size/index of the list is a signed(!) 64-bit integer.
    AVS_PARAM_ACTION = 1,  // An effect-defined function to run. Has no value and cannot
                           // be read from. Use `avs_parameter_run_action()` to trigger
                           // the action.

    AVS_PARAM_BOOL = 2,    // =0 false, ≠0 true.
    AVS_PARAM_INT = 3,     // Signed 64-bit integer.
    AVS_PARAM_FLOAT = 4,   // 64-bit float (a.k.a. "double").
    AVS_PARAM_COLOR = 5,   // 64-bit RGBA16 unsigned integer.
    AVS_PARAM_STRING = 6,  // Const zero-terminated C string.
    AVS_PARAM_SELECT = 7,  // List of zero-terminated C-strings.
                           // The value type of a SELECT parameter is a signed 64-bit
                           // integer index into the `options` list. Expect and handle
                           // out-of-range indices, esp. -1 for "no selection".
                           // When getting/setting a parameter of this type use the
                           // `_int()` getter/setter flavors.
    AVS_PARAM_INT_ARRAY = 8,
    AVS_PARAM_FLOAT_ARRAY = 9,
    AVS_PARAM_COLOR_ARRAY = 10,
};

enum AVS_Component_Position {
    AVS_COMPONENT_POSITION_BEFORE = 0,
    AVS_COMPONENT_POSITION_AFTER = 1,
    AVS_COMPONENT_POSITION_CHILD = 2,
    AVS_COMPONENT_POSITION_DONTCARE = AVS_COMPONENT_POSITION_AFTER
};

/**
 * Metadata for an effect.
 *
 * Populate this struct by calling `avs_effect_info()`.
 *
 * Name is never empty.
 *
 * Group is one of "Render", "Trans", "Misc", rarely something else. No group, i.e. ""
 * is possible.
 *
 * Help text can be empty, short or several paragraphs long.
 *
 * The list of parameters may be empty, in which case `parameters` will be NULL.
 */
typedef struct {
    const char* group;
    const char* name;
    const char* help;
    uint32_t parameters_length;
    const AVS_Parameter_Handle* parameters;
} AVS_Effect_Info;

/**
 * Metadata for one of an effect's parameters.
 *
 * Populate this struct by calling `avs_parameter_info()`.
 *
 * Name is never empty.
 *
 * Description may be empty.
 *
 * If `is_global` is `true`, then this parameter is shared across all components of this
 * effect type within a preset. Changing the value of this parameter will change it for
 * all components.
 *
 * The rest of the fields may be unused for a given parameter `type`, and will contain
 * the appropriate null value for other types:
 * - `int_min`, `int_max`, `float_min`, `float_max are only meaningful for their
 *   respective numerical type.
 * - `options_length` and `options` are only used for `AVS_PARAM_SELECT` type
 *   parameters.
 * - all `children*` parameters are only used for `AVS_PARAM_LIST` type parameters.
 */
typedef struct {
    AVS_Parameter_Type type;
    const char* name;
    const char* description;
    bool is_global;
    int64_t int_min;
    int64_t int_max;
    double float_min;
    double float_max;
    int64_t options_length;
    const char* const* options;
    uint32_t children_length;
    uint32_t children_length_min;
    uint32_t children_length_max;
    const AVS_Parameter_Handle* children;
} AVS_Parameter_Info;

/**
 * Query available effects in AVS.
 *
 * Note that effects' names are globally unique, not just within their group. Groups
 * exist purely as categories for better overview.
 */
const AVS_Effect_Handle* avs_effect_library(AVS_Handle avs, uint32_t* length_out);

/**
 * Query information about a specific effect.
 *
 * Returns `false` if any of the handles cannot be found or another error occurred.
 */
bool avs_effect_info(AVS_Handle avs,
                     AVS_Effect_Handle effect,
                     AVS_Effect_Info* info_out);

/**
 * Retrieve the Effect information handle for a specific Component.
 *
 * Returns 0 on error.
 */
AVS_Effect_Handle avs_component_effect(AVS_Handle avs, AVS_Component_Handle component);

/**
 * Get the root of the current preset's component tree.
 *
 * Returns 0 on error.
 */
AVS_Component_Handle avs_component_root(AVS_Handle avs);

/**
 * To traverse the current preset's component tree, use the following methods to
 * retrieve a component's list of children.
 *
 * Each time a component is added to, moved in or removed from the parent `component`,
 * any previously returned array address for that specific `component` is invalidated,
 * and only regenerated when calling this method. The simple rule to remember is: Always
 * call this function before iterating over children of a component.
 *
 * `avs_component_can_have_child_components()` helps if you quickly want to distinguish
 * between "leaf"- and "tree"-type components. Note: The overwhelming majority of
 * effects/components cannot have child components.
 *
 * `avs_component_children()` will return 0 both on error and if the component has no
 * children.
 */
bool avs_component_can_have_child_components(AVS_Handle avs,
                                             AVS_Component_Handle component);
const AVS_Component_Handle* avs_component_children(AVS_Handle avs,
                                                   AVS_Component_Handle component,
                                                   uint32_t* length_out);

/**
 * Add a new component of the type given by `effect`. The component is added near an
 * existing `relative_to` component in the given `direction`. Note that you cannot
 * create a new component with the "root" effect type.
 *
 * The `direction` may be either `AVS_COMPONENT_POSITION_BEFORE`,
 * `AVS_COMPONENT_POSITION_AFTER` or `AVS_COMPONENT_POSITION_CHILD`. The child
 * direction is only valid if the `relative_to` component's effect type supports
 * children. It's intended use is as disambiguation when creating the first child of a
 * component, where `AFTER` could mean both actually after or inside. If `relative_to`
 * already has children, you should instead call this function with an existing child
 * within that component as `relative_to`. But in case you don't, the new component will
 * be created as the new last child.
 *
 * `relative_to` may be 0, in which case the component is created as the last child
 * of the preset root. The `direction` is ignored in this case, but you may explicitly
 * use `AVS_COMPONENT_POSITION_DONTCARE` to highlight this fact. Note that it is the
 * same as `AVS_COMPONENT_POSITION_AFTER`.
 *
 * Returns a pointer to the new component. Returns 0 if `effect` is invalid,
 * `relative_to` is not 0 but doesn't exist, or `direction` is invalid or another error
 * occurred.
 */
AVS_Component_Handle avs_component_create(AVS_Handle avs,
                                          AVS_Effect_Handle effect,
                                          AVS_Component_Handle relative_to,
                                          AVS_Component_Position direction);

/**
 * Move an existing component from one place in the component tree to another.
 *
 * Returns `false` if the `component` or the `relative_to` cannot be found, the
 * `direction` is invalid (e.g. before/after the root element) or another error
 * occurred.
 */
bool avs_component_move(AVS_Handle avs,
                        AVS_Component_Handle component,
                        AVS_Component_Handle relative_to,
                        AVS_Component_Position direction);

/**
 * Duplicate the given `component` and return the new duplicate. The new component will
 * be placed after the original.
 *
 * Returns 0 if the operation fails for some reason.
 */
AVS_Component_Handle avs_component_duplicate(AVS_Handle avs,
                                             AVS_Component_Handle component);

/**
 * Remove the given component. If the root component is removed the preset will be
 * cleared and a new default root component (with a new handle!) will be added
 * automatically. There's always a root component.
 *
 * Returns `false` if any of the handles cannot be found or another error occurred.
 */
bool avs_component_delete(AVS_Handle avs, AVS_Component_Handle component);

/**
 * Each component has an `enabled` flag that determines whether it should take part in
 * the rendering or be skipped. Use the methods below to query and set the `enabled`
 * flag.
 *
 * The return value of `avs_component_is_enabled()` indicates the state of the
 * component, but will also be `false` on error (e.g. the `component` wasn't found).
 * You have to check `avs_error_str()` to know whether an error occurred.
 *
 * The return value of `avs_component_set_enabled()` is `true` if the state was set
 * successfully, and `false` on error.
 */
bool avs_component_is_enabled(AVS_Handle avs, AVS_Component_Handle component);
bool avs_component_set_enabled(AVS_Handle avs,
                               AVS_Component_Handle component,
                               bool enabled);

/**
 * Query information about an effect's parameters.
 *
 * Returns `false` if any of the handles cannot be found or another error occurred.
 * The fields of `info_out` will be filled with meaningful zero values. Lists will be
 * NULL pointers, but strings will be the empty string, not NULL.
 */
bool avs_parameter_info(AVS_Handle avs,
                        AVS_Effect_Handle effect,
                        AVS_Parameter_Handle parameter,
                        AVS_Parameter_Info* info_out);

/**
 * Use these methods to retrieve parameter values from components.
 *
 * If parameter lookup fails, a default zero value will be returned by all `get`
 * methods. It's `0` for all numerical types, `false` for booleans, and `""` for
 * strings.
 *
 * If the zero value was returned you should check `avs_error_str()`. It will return the
 * empty string `""` if the last call to a `get` method succeeded, or an error message
 * otherwise.
 */
#define NOT_IN_A_LIST 0
bool avs_parameter_get_bool(AVS_Handle avs,
                            AVS_Component_Handle component,
                            AVS_Parameter_Handle parameter,
                            uint32_t list_depth,
                            int64_t* list_indices);
int64_t avs_parameter_get_int(AVS_Handle avs,
                              AVS_Component_Handle component,
                              AVS_Parameter_Handle parameter,
                              uint32_t list_depth,
                              int64_t* list_indices);
double avs_parameter_get_float(AVS_Handle avs,
                               AVS_Component_Handle component,
                               AVS_Parameter_Handle parameter,
                               uint32_t list_depth,
                               int64_t* list_indices);
uint64_t avs_parameter_get_color(AVS_Handle avs,
                                 AVS_Component_Handle component,
                                 AVS_Parameter_Handle parameter,
                                 uint32_t list_depth,
                                 int64_t* list_indices);
const char* avs_parameter_get_string(AVS_Handle avs,
                                     AVS_Component_Handle component,
                                     AVS_Parameter_Handle parameter,
                                     uint32_t list_depth,
                                     int64_t* list_indices);

/**
 * Use these methods to set new parameter values for components.
 *
 * The `set` methods all return `true` on success or `false` otherwise.
 *
 * Note: Parameters may internally have on-change handler functions registered for them.
 * Setting a value will invoke that function if present. Only a minority of parameters
 * make use of this. But consider that some parameters' setters might not always return
 * quite as instantaneously as most do.
 */
bool avs_parameter_set_bool(AVS_Handle avs,
                            AVS_Component_Handle component,
                            AVS_Parameter_Handle parameter,
                            bool value,
                            uint32_t list_depth,
                            int64_t* list_indices);
bool avs_parameter_set_int(AVS_Handle avs,
                           AVS_Component_Handle component,
                           AVS_Parameter_Handle parameter,
                           int64_t value,
                           uint32_t list_depth,
                           int64_t* list_indices);
bool avs_parameter_set_float(AVS_Handle avs,
                             AVS_Component_Handle component,
                             AVS_Parameter_Handle parameter,
                             double value,
                             uint32_t list_depth,
                             int64_t* list_indices);
bool avs_parameter_set_color(AVS_Handle avs,
                             AVS_Component_Handle component,
                             AVS_Parameter_Handle parameter,
                             uint64_t value,
                             uint32_t list_depth,
                             int64_t* list_indices);
bool avs_parameter_set_string(AVS_Handle avs,
                              AVS_Component_Handle component,
                              AVS_Parameter_Handle parameter,
                              const char* value,
                              uint32_t list_depth,
                              int64_t* list_indices);

/**
 * Array-type parameters are read-only. The length of the array is written to the
 * `length_out` argument.
 */
const int64_t* avs_parameter_get_int_array(AVS_Handle avs,
                                           AVS_Component_Handle component,
                                           AVS_Parameter_Handle parameter,
                                           uint64_t* length_out,
                                           uint32_t list_depth,
                                           int64_t* list_indices);
const double* avs_parameter_get_float_array(AVS_Handle avs,
                                            AVS_Component_Handle component,
                                            AVS_Parameter_Handle parameter,
                                            uint64_t* length_out,
                                            uint32_t list_depth,
                                            int64_t* list_indices);
const uint64_t* avs_parameter_get_color_array(AVS_Handle avs,
                                              AVS_Component_Handle component,
                                              AVS_Parameter_Handle parameter,
                                              uint64_t* length_out,
                                              uint32_t list_depth,
                                              int64_t* list_indices);

/**
 * Run the action associated with an AVS_PARAM_ACTION parameter.
 *
 * Returns `false` if the arguments are invalid. Does _not_ check for the success of the
 * action function.
 */
bool avs_parameter_run_action(AVS_Handle avs,
                              AVS_Component_Handle component,
                              AVS_Parameter_Handle parameter,
                              uint32_t list_depth,
                              int64_t* list_indices);

/**
 * Use these methods to manage parameter lists. Parameter lists are rare, but they allow
 * effects to have a variable-length list of items with a set of multiple parameters.
 * The Effect_Info- and Parameter API around this is a bit complex.
 *
 * Consider an AVS effect "Foo" with two parameters, one regular, "awesome", the other a
 * list of the same set of sub-parameters, "things". In JSON the component may look a
 * bit like this:
 *
 *     {
 *         "type" : "Foo",
 *         "config": {
 *             "awesome": true,
 *             "things": [
 *                 {
 *                     "bar": true,
 *                     "baz": 3.14
 *                 },
 *                 {
 *                     "bar": false,
 *                     "baz": -1.0
 *                 }
 *             ]
 *         }
 *     }
 *
 * Remember these are the specific values for this component _instance_. How do you know
 * that "awesome" has a bool type, and "things" is a list, and "baz" inside "things"
 * should be a floating-point number? This is where the Effect_Info data comes in.
 *
 * You know the Effect_Info for a specific effect from the list of available effects
 * given to you by `avs_effects_get()`. In it you will find one Effect_Info struct that
 * has its "name" property set to "Foo". It's parameters and parameter types are
 * recorded within. So a (shortened) JSON representation of the Effect_Info struct may
 * look like below. Note that this is the _general_ parameter description for _all_
 * components that have type "Foo":
 *
 *     {
 *         "group": "Render",
 *         "name": "Foo",
 *         "parameters" [
 *             {
 *                 "name": "awesome",
 *                 "type": "bool"
 *             },
 *             {
 *                 "name": "things",
 *                 "type": "list",
 *                 "parameters": [
 *                     {
 *                         "name": "bar",
 *                         "type": "bool"
 *                     },
 *                     {
 *                         "name": "baz",
 *                         "type": "float"
 *                     }
 *                 ]
 *             }
 *         ]
 *     }
 *
 * Take a second to compare this JSON representation with the `AVS_Effect_Info` and
 * `AVS_Parameter_Info` structs at the beginning of this file, to see how you may get at
 * the desired `AVS_Parameter_Info` reference which you will need to set the component's
 * parameter.
 *
 * Armed with this knowledge you can now get to any parameter within any component. To
 * set the second "thing"'s "baz" value, of a component `my_foo` to, say, 5.0, you have
 * to (recursively) scan the `AVS_Effect_Info` and `AVS_Parameter_Info` structs and find
 * that "baz"'s Parameter_Info tells you it's of type "float". Together with your
 * AVS_Handle `my_avs` and your `AVS_Component_Handle` `my_foo`, and your freshly
 * discovered `AVS_Parameter_Info` `things_baz`, you now call the setter for the correct
 * type:
 *
 *     uint32_t list_depth = 1;
 *     int64_t list_index[1] = {1};
 *     double new_value = 5.0;
 *     if (!avs_parameter_set_float(
 *             my_avs, my_foo, things_baz, new_value, list_depth, list_index)) {
 *         printf("Error setting new value: %s\n", avs_error_str(my_avs));
 *     }
 *
 * Validate that this actually worked:
 *
 *     double check = avs_parameter_get_float(my_avs, my_foo, things_baz, list_index);
 *     assert(check == new_value);
 */
typedef union {
    bool b;
    int64_t i;
    double f;
    uint64_t c;
    const char* s;
} AVS_Value;

typedef struct {
    AVS_Parameter_Handle parameter;
    AVS_Value value;
} AVS_Parameter_Value;

int64_t avs_parameter_list_length(AVS_Handle avs,
                                  AVS_Component_Handle component,
                                  AVS_Parameter_Handle parameter,
                                  uint32_t list_depth,
                                  int64_t* list_indices);
bool avs_parameter_list_element_add(AVS_Handle avs,
                                    AVS_Component_Handle component,
                                    AVS_Parameter_Handle parameter,
                                    int64_t before,
                                    uint32_t values_length,
                                    const AVS_Parameter_Value* values,
                                    uint32_t list_depth,
                                    int64_t* list_indices);
bool avs_parameter_list_element_move(AVS_Handle avs,
                                     AVS_Component_Handle component,
                                     AVS_Parameter_Handle parameter,
                                     int64_t from_index,
                                     int64_t to_index,
                                     uint32_t list_depth,
                                     int64_t* list_indices);
bool avs_parameter_list_element_remove(AVS_Handle avs,
                                       AVS_Component_Handle component,
                                       AVS_Parameter_Handle parameter,
                                       int64_t remove_index,
                                       uint32_t list_depth,
                                       int64_t* list_indices);

#ifdef __cplusplus
}
#endif
