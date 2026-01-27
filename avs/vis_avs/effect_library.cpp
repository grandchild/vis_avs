#include "e_unknown.h"

#include "effect_library.h"

#include <string>
#include <vector>

typedef Effect* (*effect_component_factory)(AVS_Instance*);

std::unordered_map<AVS_Effect_Handle, Effect_Info*> g_effect_lib;
std::unordered_map<int32_t, AVS_Effect_Handle> g_legacy_builtin_effect_lib;
std::unordered_map<std::string, AVS_Effect_Handle> g_legacy_ape_effect_lib;
std::vector<AVS_Effect_Handle> g_effect_lib_handles_for_api;
std::unordered_map<Effect_Info*, effect_component_factory> g_component_factories;

static std::unordered_map<std::string, uint32_t> find_duplicate_effect_names(
    const std::unordered_map<AVS_Effect_Handle, Effect_Info*>& lib) {
    std::unordered_map<std::string, uint32_t> duplicates;
    for (auto const& entry : lib) {
        std::string name = entry.second->get_name();
        duplicates[name]++;
    }
    for (auto entry = duplicates.begin(); entry != duplicates.end();) {
        if (entry->second <= 1) {
            entry = duplicates.erase(entry);
        } else {
            entry++;
        }
    }
    return duplicates;
}

/**
 * Parameter Handles
 *
 * Parameter handles are created by hashing its "Effect/Path/To/Parameter" string, which
 * for the overwhelming majority is simply "Effect/Parameter".
 *
 * We also check for hash collisions globally. A collision is highly unlikely, with an
 * output space of 2^32-1 and around 400 parameters across all effects.
 */

static bool check_handle_collision(AVS_Parameter_Handle handle,
                                   const std::string& path) {
    auto other = g_param_map.find(handle);
    if (other != g_param_map.end()) {
        // TODO[feature]: Find out effect/path for other parameter, so that the error
        //                message becomes more actionable.
        log_err(
            "Effect lib: Parameter handle collision %s with %s: %d\n"
            "This is a rare collision in the FNV1a hash function, and the only way to"
            " resolve this is to rename one or more of the parameters.",
            path.c_str(),
            other->second->name,
            handle);
        return true;
    }
    return false;
}

static void register_nested_parameters(const Parameter* parent,
                                       const std::string& parent_path) {
    g_child_parameters_for_api[parent].clear();
    auto params = parent->child_parameters;
    for (uint32_t i = 0; i < parent->num_child_parameters; i++) {
        std::string path = parent_path + params[i].name;
        auto handle = Handles::comptime_get(path.c_str());
        if (check_handle_collision(handle, path)) {
            continue;
        }
        g_param_map[handle] = &params[i];
        g_child_parameters_for_api[parent].push_back(handle);
        if (params[i].type == AVS_PARAM_LIST) {
            register_nested_parameters(&params[i], path + "/");
        }
    }
}

static void register_parameters(const Effect_Info* effect) {
    auto root_path = std::string(effect->get_name()) + "/";
    g_effect_parameters_for_api[effect].clear();
    auto params = effect->get_parameters();
    for (uint32_t i = 0; i < effect->get_num_parameters(); i++) {
        std::string path = root_path + params[i].name;
        auto handle = Handles::comptime_get(path.c_str());
        if (check_handle_collision(handle, path)) {
            continue;
        }
        g_param_map[handle] = &params[i];
        g_effect_parameters_for_api[effect].push_back(handle);
        if (params[i].type == AVS_PARAM_LIST) {
            register_nested_parameters(&params[i], path + "/");
        }
    }
}

#define MAKE_EFFECT_LIB_ENTRY(NAME)                                                \
    extern Effect_Info* create_##NAME##_Info(void);                                \
    extern Effect* create_##NAME(AVS_Instance*);                                   \
    auto NAME##_info = create_##NAME##_Info();                                     \
    auto NAME##_handle = NAME##_info->get_handle();                                \
    g_effect_lib[NAME##_handle] = NAME##_info;                                     \
    if (NAME##_info->get_legacy_id() != -1) {                                      \
        g_legacy_builtin_effect_lib[NAME##_info->get_legacy_id()] = NAME##_handle; \
    }                                                                              \
    if (NAME##_info->get_legacy_ape_id()) {                                        \
        g_legacy_ape_effect_lib[NAME##_info->get_legacy_ape_id()] = NAME##_handle; \
    }                                                                              \
    g_component_factories[NAME##_info] = create_##NAME;

bool make_effect_lib() {
    if (!g_effect_lib.empty()) {
        return true;
    }
    /* Builtins */
    MAKE_EFFECT_LIB_ENTRY(Root);
    MAKE_EFFECT_LIB_ENTRY(EffectList);
    MAKE_EFFECT_LIB_ENTRY(Unknown);
    MAKE_EFFECT_LIB_ENTRY(Simple);
    MAKE_EFFECT_LIB_ENTRY(DotPlane);
    MAKE_EFFECT_LIB_ENTRY(OscilloscopeStar);
    MAKE_EFFECT_LIB_ENTRY(Fadeout);
    MAKE_EFFECT_LIB_ENTRY(BlitterFeedback);
    MAKE_EFFECT_LIB_ENTRY(OnBeatClear);
    MAKE_EFFECT_LIB_ENTRY(Blur);
    MAKE_EFFECT_LIB_ENTRY(BassSpin);
    MAKE_EFFECT_LIB_ENTRY(MovingParticle);
    MAKE_EFFECT_LIB_ENTRY(RotoBlitter);
    MAKE_EFFECT_LIB_ENTRY(SVP);
    MAKE_EFFECT_LIB_ENTRY(Colorfade);
    MAKE_EFFECT_LIB_ENTRY(ColorClip);
    MAKE_EFFECT_LIB_ENTRY(RotStar);
    MAKE_EFFECT_LIB_ENTRY(Ring);
    MAKE_EFFECT_LIB_ENTRY(Movement);
    MAKE_EFFECT_LIB_ENTRY(Scatter);
    MAKE_EFFECT_LIB_ENTRY(DotGrid);
    MAKE_EFFECT_LIB_ENTRY(BufferSave);
    MAKE_EFFECT_LIB_ENTRY(DotFountain);
    MAKE_EFFECT_LIB_ENTRY(Water);
    MAKE_EFFECT_LIB_ENTRY(Comment);
    MAKE_EFFECT_LIB_ENTRY(Brightness);
    MAKE_EFFECT_LIB_ENTRY(Interleave);
    MAKE_EFFECT_LIB_ENTRY(Grain);
    MAKE_EFFECT_LIB_ENTRY(ClearScreen);
    MAKE_EFFECT_LIB_ENTRY(Mirror);
    MAKE_EFFECT_LIB_ENTRY(Starfield);
    MAKE_EFFECT_LIB_ENTRY(Text);
    MAKE_EFFECT_LIB_ENTRY(Bump);
    MAKE_EFFECT_LIB_ENTRY(Mosaic);
    MAKE_EFFECT_LIB_ENTRY(WaterBump);
    MAKE_EFFECT_LIB_ENTRY(Video);
    MAKE_EFFECT_LIB_ENTRY(CustomBPM);
    MAKE_EFFECT_LIB_ENTRY(Picture);
    MAKE_EFFECT_LIB_ENTRY(DynamicDistanceModifier);
    MAKE_EFFECT_LIB_ENTRY(SuperScope);
    MAKE_EFFECT_LIB_ENTRY(Invert);
    MAKE_EFFECT_LIB_ENTRY(UniqueTone);
    MAKE_EFFECT_LIB_ENTRY(Timescope);
    MAKE_EFFECT_LIB_ENTRY(SetRenderMode);
    MAKE_EFFECT_LIB_ENTRY(Interferences);
    MAKE_EFFECT_LIB_ENTRY(DynamicShift);
    MAKE_EFFECT_LIB_ENTRY(DynamicMovement);
    MAKE_EFFECT_LIB_ENTRY(FastBright);
    MAKE_EFFECT_LIB_ENTRY(ColorModifier);
    /* APEs */
    MAKE_EFFECT_LIB_ENTRY(ChannelShift);
    MAKE_EFFECT_LIB_ENTRY(ColorReduction);
    MAKE_EFFECT_LIB_ENTRY(Multiplier);
    MAKE_EFFECT_LIB_ENTRY(VideoDelay);
    MAKE_EFFECT_LIB_ENTRY(MultiDelay);
    MAKE_EFFECT_LIB_ENTRY(Convolution);
#ifndef __amd64__
    MAKE_EFFECT_LIB_ENTRY(Texer2);
#endif
    MAKE_EFFECT_LIB_ENTRY(Normalise);
    MAKE_EFFECT_LIB_ENTRY(ColorMap);
    MAKE_EFFECT_LIB_ENTRY(AddBorders);
    MAKE_EFFECT_LIB_ENTRY(Triangle);
    MAKE_EFFECT_LIB_ENTRY(EelTrans);
    MAKE_EFFECT_LIB_ENTRY(GlobalVariables);
    MAKE_EFFECT_LIB_ENTRY(MultiFilter);
    MAKE_EFFECT_LIB_ENTRY(Picture2);
    MAKE_EFFECT_LIB_ENTRY(FramerateLimiter);
    MAKE_EFFECT_LIB_ENTRY(Texer);
    /* Example */
    // MAKE_EFFECT_LIB_ENTRY(Example);

    g_effect_lib_handles_for_api.clear();
    g_effect_lib_handles_for_api.reserve(g_effect_lib.size());
    for (auto const& entry : g_effect_lib) {
        g_effect_lib_handles_for_api.push_back(entry.first);
    }
    auto duplicate_names = find_duplicate_effect_names(g_effect_lib);
    if (!duplicate_names.empty()) {
        for (auto const& entry : duplicate_names) {
            log_err("Effect lib: %d duplicate effects named: \"%s\"",
                    entry.second,
                    entry.first.c_str());
        }
        g_effect_lib.clear();
        return false;
    }
    for (auto const& effect : g_effect_lib) {
        if (effect.second->get_parameters() == nullptr
            || effect.second->get_num_parameters() == 0) {
            continue;
        }
        register_parameters(effect.second);
    }
    return true;
}

extern Effect* create_Unknown(AVS_Instance*);

Effect* component_factory(const Effect_Info* effect, AVS_Instance* avs) {
    for (auto const& factory : g_component_factories) {
        if (factory.first->get_handle() == effect->get_handle()) {
            if (!effect->is_createable_by_user()) {
                log_warn("%s is not user-creatable", effect->get_name());
            } else {
                return factory.second(avs);
            }
        }
    }
    return create_Unknown(avs);
}

Effect* component_factory_by_name(const std::string& name, AVS_Instance* avs) {
    for (auto const& entry : g_component_factories) {
        if (entry.first->get_name() == name) {
            if (!entry.first->is_createable_by_user()) {
                log_warn("%s is not user-creatable", name.c_str());
            } else {
                return entry.second(avs);
            }
        }
    }
    return create_Unknown(avs);
}

Effect* component_factory_legacy(int legacy_effect_id,
                                 char* legacy_effect_ape_id,
                                 AVS_Instance* avs) {
    Effect* new_effect = nullptr;
    auto builtin_it = g_legacy_builtin_effect_lib.find(legacy_effect_id);
    if (builtin_it != g_legacy_builtin_effect_lib.end()) {
        new_effect = component_factory(g_effect_lib[builtin_it->second], avs);
    } else if (legacy_effect_ape_id != nullptr) {
        auto ape_it = g_legacy_ape_effect_lib.find(legacy_effect_ape_id);
        if (ape_it != g_legacy_ape_effect_lib.end()) {
            new_effect = component_factory(g_effect_lib[ape_it->second], avs);
        }
    }
    if (!new_effect) {
        log_warn("component_factory: unknown effect %d/%s",
                 legacy_effect_id,
                 legacy_effect_ape_id);
        extern Effect*(create_Unknown)(AVS_Instance*);
        new_effect = create_Unknown(avs);
        ((E_Unknown*)new_effect)->set_id(legacy_effect_id, legacy_effect_ape_id);
    }
    return new_effect;
}
