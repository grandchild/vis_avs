/**
 * == How to Create a New Effect ==
 *
 * An Effect has at least one "Config" struct, which consists of a set of "Parameters",
 * and an "Info" struct describing the config. You can think of the config as the
 * internal data structure and the info as the metadata. The info is used to convey all
 * necessary information about the effect and its parameters to any libAVS consumer.
 *
 * To create the config and info, each effect subclasses `Effect_Config`, `Effect_Info`
 * and then creates its own class from the `Configurable_Effect` template with the two
 * former classes as template parameters. If you're unfamiliar with C++ templates this
 * might sound or look confusing, but you don't really need to understand how templates
 * work in order to implement a new Effect class. Read on!
 *
 * The incantations for a new Effect all follow the same pattern:
 *
 *   1. Declare the Config struct.
 *   2. Declare the Info struct, referencing the config. This is the hard part.
 *        Note: The reasoning for spending the effort here, is that we get:
 *          - a simple-to-use config struct for clean code inside your effect,
 *          - a generic API that can handle all effects the same way,
 *          - a way for a UI to render unknown effects with generic interface widgets,
 *          - a way to save effects in a way that can be transparently loaded and saved
 *            with an AVS version that doesn't know about them.
 *        To achieve this we have to spend some rather ugly-looking lines of code
 *        describing the properties of the effect and its parameters.
 *   3. Declare the Effect class, using the Config and Info struct names as template
 *      parameters.
 */

#include "effect.h"       // Configurable_Effect<...>
#include "effect_info.h"  // Effect_Info, Parameter, P_*

/* Let's look at it through an example effect called, well, "Example":
 *
 * 1. Config
 *
 * Declare an `Example_Config` struct inheriting from `Effect_Config` (which can be
 * empty if your effect has no options). Set default values for every parameter (except
 * lists).
 */
struct Example_Config : public Effect_Config {
    int64_t dots = 4;
    uint64_t color = 0x00ffffff;
    /* Note that no "enabled" parameter is needed, every effect automatically gets one.
     *
     * All members of the config struct _must_ use the appropriate C++ type:
     *   - booleans:               bool
     *   - integers & selects:     int64_t
     *   - floating point values:  double
     *   - color values:           uint64_t
     *   - strings:                std::string
     *   Special cases:
     *   - parameter lists:        std::vector<T>, T being the type of the child config
     *   - actions:                Action parameters don't have a value & don't appear
     *                             in the config struct.
     *
     * You may declare a default constructor (but usually shouldn't need to) if special
     * things need to happen on init of your config (e.g. to fill a list with a default
     * set of entries). If you define a non-default constructor, you have to define the
     * default constructor explicitly too (if you don't know how, write
     * `Example_Config() = default;`).
     *
     * Declaring any other methods inside your struct invokes compiler-specific and
     * possibly undefined behavior because of the way `Effect_Info` is implemented! Use
     * free functions instead, if you must, or rather just add them to your effect class
     * instead of the config.
     */
};

/* 2. Info
 *
 * Declare an accompanying `Example_Info` struct inheriting from `Effect_Info`.
 */
struct Example_Info : public Effect_Info {
    /* The Info struct has some required fields.
     *
     * There are quite a few requirements on the Info struct and inter-dependencies with
     * the Config struct:
     *
     *   - `group` can be any string. Groups are not part of the identity of the effect,
     *     so you can change the group of the effect and older presets will load just
     *     fine. It's _highly discouraged_ to make up your own category here. Instead
     *     see what other groups already exist, and find one that fits. If the existing
     *     groups won't, there's a "Misc" group for outliers.
     *
     *   - `name` can be any string (can include spaces, etc.). It should be unique
     *     among existing effects.
     */
    static constexpr const char* group = "Render";
    static constexpr const char* name = "Example";
    static constexpr const char* help =
        "A simple render effect to showcase how to write effect classes";
    static constexpr int32_t legacy_id = -1;
    static constexpr char* legacy_ape_id = nullptr;

    /* `num_parameters` in Info must match the number of parameters defined in Config.
     * (To be precise, it must not be larger. If it's smaller, then the remaining config
     * parameters will be inaccessible for editing. Do with that information what you
     * will.)
     */
    static constexpr uint32_t num_parameters = 2;
    static constexpr Parameter parameters[num_parameters] = {
        /* Use the `P_` functions to safely add entries to the `parameters` array.
         * Alternatively you can declare `Parameter` structs yourself, and set unneeded
         * fields to 0 or NULL as appropriate. The `P_*()` functions do all of this for
         * you and are highly recommended.
         *
         * There are a few different functions to choose from when declaring parameters:
         *
         * - `PARAM()` declares a generic parameter. You shouldn't generally use this
         *   one, rather use one of `P_BOOL()`, `P_INT()`, `P_FLOAT()`, `P_COLOR()` or
         *   `P_STRING()` directly.
         *
         * - `P_IRANGE()` and `P_FRANGE()` declare limited integer and floating-point
         *   parameters, respectively. Setting a value outside of the min/max range
         *   will set a clamped value. An editor UI may render a range-limited
         *   parameter as a slider.
         *
         * - `P_SELECT()` creates a parameter that can be selected from a list of
         *   options. The value type is actually an integer index (refer to
         *   avs_editor.h). To set the options pass the name of a function that returns
         *   a list of strings, and writes the length of the list to its input
         *   parameter. Have a look into existing effects for examples. A smart editor
         *   UI may turn a short list into a radio select, and a longer list into a
         *   dropdown select.
         *
         * - `P_RESOURCE()` is like `P_SELECT()` (a list of options to choose from). The
         *   difference is that the value is the string of the option itself, not its
         *   index. This is needed for dynamic lists that change often, such as
         *   available files on disk. If the resource is missing, its name is still
         *   available, and can help the user in finding it. The rest works the same as
         *   `P_SELECT()`, namely setting the available options.
         *
         * - `P_LIST()` is an advanced feature not needed by most effects. It creates a
         *   list of sets of parameters. Declaring parameter lists will be explained in
         *   more detail further down (TODO).
         *
         * - `P_ACTION()` lets you register a function to be triggered through the API.
         *   An action parameter isn't represented by a config field and cannot be read
         *   or set.
         *
         * - Most functions have an `_X`-suffixed version, which excludes them from
         *   being saved in a preset. So if you choose `P_*_X()` instead of `P_*()` the
         *   parameter will not be saved and not loaded back.
         *
         * - Some functions have a `_G`-suffixed version, which declares them as part of
         *   an effect's global config, shared by all instances of this effect in one
         *   AVS instance. Global configurations and parameters will be explained in
         *   more detail further down (TODO).
         *
         * - The common arguments to all `P_*()` functions are as follows:
         *
         *   - `offset` must be the offset, in bytes, of the parameter's field in its
         *     Config struct. Use `offsetof(Example_Config, foo)` to let the compiler
         *     figure it out.
         *
         *   - `name` can be any string, and it must not be NULL. It must be unique
         *     among your effect's other parameters. It doesn't have to bear any
         *     resemblance to the config field (but it'd probably be wise). An editor
         *     UI may use this to label the parameter.
         *
         *   - `description` can be any string, or NULL. An editor UI may use this to
         *     further describe and explain the parameter if needed (e.g. in a
         *     tooltip).
         *
         *   - `on_value_change` (or `on_run` for action parameters) may be a function
         *     pointer or NULL. The function gets called whenever the value is changed
         *     (or the action triggered) through the editor API. The signature is:
         *
         *         void on_change(Effect* component, const Parameter* parameter,
         *                        const std::vector<int64_t>& parameter_path)
         *
         *     You could use `parameter->name` to identify a parameter if the function
         *     handles multiple parameter changes.
         *
         *     Both the `component` and `parameter` pointers are guaranteed to be
         *     non-NULL and valid. If the handler is for a parameter inside a list,
         *     `parameter_path` is guaranteed to be of the correct length to contain
         *     your list index, you don't need to test `parameter_path.size()` in your
         *     handlers. If your effect doesn't have list-type parameters, it's safe to
         *     just ignore `parameter_path`. For `P_LIST()` use `on_add`, `on_move` and
         *     `on_remove` (see below).
         *
         *     Tip: Make the handler function a static member function of the info
         *     struct instead of a free function. This way it will not conflict with
         *     other effects' handler functions (which are often named similarly).
         *
         * - `int_min`/`int_max` & `float_min`/`float_max` set the minimum and maximum
         *   value of a ranged parameter. Note that if you set `min == max`, then the
         *   parameter is unlimited!
         *
         * - `get_options` is a function to retrieve the list of options. This is not a
         *   static list because some lists aren't known at compile time(e.g. image
         *   filenames). The signature is:
         *
         *         const char* const* get_options(int64_t* length_out)
         *
         *   The list length must be written to the location pointed to by `length_out`.
         *
         * - If `length_min` and `length_max` in `P_LIST()` are both 0 then the list
         *   length is unlimited. If they are both the same but non-zero the list is
         *   fixed at that size (elements can still be moved around).
         *
         * - `on_add`, `on_move` & `on_remove` are three callback functions that get
         *   called whenever the respective action was successfully performed on the
         *   parameter list. The signature for all 3 functions is:
         *
         *       void on_list_change(Effect* component, const Parameter* parameter,
         *                           const std::vector<int64_t>& parameter_path,
         *                           int64_t index1, int64_t index2)
         *
         *   where:
         *
         *   - for `on_add` `index1` is the new index and `index2` is unused.
         *   - for `on_move` `index1` is the previous and `index2` the new index.
         *   - for `on_remove` `index1` is the removed index and `index2` is unused.
         *
         *   You can pass the same function to all three, since the signature is the
         *   same.
         *   The `on_move` handler will not be called if the element is "moved" to the
         *   same index it's already at. It's only called for actual changes.
         */
        P_IRANGE(offsetof(Example_Config, dots),
                 "Dots",
                 /*min*/ 0,
                 /*max*/ 100,
                 "The number of dots"),
        P_COLOR(offsetof(Example_Config, color), "Color", "The color of the dots"),
    };

    /* Write `EFFECT_INFO_GETTERS;` at the end of the Info class declaration, it will
     * fill in some methods needed for accessing the fields declared above.
     * If, and only if, your effect has no parameters, i.e. it can only be enabled and
     * disabled, then use the macro `EFFECT_INFO_GETTERS_NO_PARAMETERS` instead,
     * otherwise it will not compile on MSVC.
     */
    EFFECT_INFO_GETTERS;
};

/* 3. Effect
 *
 * Now you're finally ready to declare your `E_Foo` class, specializing the
 * `Configurable_Effect` template, using the Info and Config struct names as template
 * parameters:
 */
class E_Example : public Configurable_Effect<Example_Info, Example_Config> {
   public:
    explicit E_Example(AVS_Instance* avs);

    virtual int render(char visdata[2][2][576],
                       int is_beat,
                       int* framebuffer,
                       int* fbout,
                       int w,
                       int h);
    /* You need to explicitly override clone() so that it returns the correct type. */
    virtual E_Example* clone() { return new E_Example(*this); }

    /* Define any other member values or methods you need. */
    // ...
};

// ===================================================================================
// For simplicity this is all in one .cpp file. Actual effects should declare the above
// in a .h header file e_example.h and the parts below in a .cpp file e_example.cpp.
// ===================================================================================

E_Example::E_Example(AVS_Instance* avs) : Configurable_Effect(avs) {}

/* This is the render function. It is called once per frame.
 *
 *   - `visdata` is the audio data. It is an array of 2 types (0=waveform, 1=spectrum),
 *     each with 2 channels, each again with 576 values.
 *   - `is_beat` is a boolean indicating whether the current frame is a beat frame.
 *   - `framebuffer` is the current framebuffer.
 *   - `fbout` is the secondary framebuffer (write to `fbout` if you need double-
 *     buffering, because you're reading pixels that were overwritten earlier).
 *   - `w` and `h` are the width and height of the framebuffer.
 *
 * Return 1 if you're writing to `fbout`, or 0 if you're writing to `framebuffer`
 * directly.
 */
int E_Example::render(char visdata[2][2][576],
                      int is_beat,
                      int* framebuffer,
                      int* fbout,
                      int w,
                      int h) {
    // Draw a horizontal line of dots in the color specified by the config.
    int* fb_at_y = &framebuffer[h / 2 * w];
    for (int i = 1; i <= this->config.dots; i++) {
        int64_t x = (i * w) / (this->config.dots + 1);
        fb_at_y[x] = (int)this->config.color;
    }
    return 0;
}

/* You need to put the storage of the parameter list array into the .cpp file. */
constexpr Parameter Example_Info::parameters[];

/* You also need to define two functions that will be called by the effect library.
 * You don't need to put these in the header, they will be declared in the effect
 * library via the `extern` keyword, and the linker will find them.
 *
 * The names must match the patterns `create_<EFFECT>_Info` and `create_<EFFECT>`,
 * where <EFFECT> is the name of your effect, that you set in `effect_libray.cpp`.
 */
Effect_Info* create_Example_Info() { return new Example_Info(); }
Effect* create_Example(AVS_Instance* avs) { return new E_Example(avs); }

/* Now edit `effect_library.cpp` to enable the line
 *     // MAKE_EFFECT_LIB_ENTRY(Example);
 * If you changed the name, use that name here. The name string is the one from the
 * `create_*()` methods.
 */

/* ... alright! This is the basic recipe for setting up a configurable effect.
 *
 * TODO: Explain variable length parameter lists in more detail. For now have a look at
 *       ColorMap for an effect with _all_ the bells and whistles.
 * TODO: Explain global parameters & config. For now have a look at MultiDelay for an
 *       effect that makes use of a set of global config fields.
 *
 * Q: Why are Config and Info `struct`s, but Effect is a `class`?
 * A: In C++, for all intents and purposes `class` and `struct` are interchangeable,
 *    with one key difference: `struct` members are public by default, and `class`
 *    members private. So `struct` is just less to write if all members are public.
 *    Additionally, using `struct`s is sometimes used to signal a "simpler" object type
 *    containing predominantly data members (as opposed to methods). This is also the
 *    case here.
 */
