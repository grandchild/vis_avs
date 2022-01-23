#include "e_root.h"

#include "effect_library.h"

#include <string>
#include <vector>

typedef Effect* (*effect_component_factory)(void);

std::unordered_map<AVS_Effect_Handle, Effect_Info*> g_effect_lib;
std::vector<AVS_Effect_Handle> g_effect_lib_handles_for_api;
AVS_Effect_Handle g_root_handle;
std::unordered_map<Effect_Info*, effect_component_factory> g_component_factories;

static std::vector<std::string> duplicate_effect_names(
    std::unordered_map<AVS_Effect_Handle, Effect_Info*>& lib) {
    std::vector<std::string> seen_names;
    std::vector<std::string> duplicates;
    seen_names.reserve(lib.size());
    for (auto const& entry : lib) {
        std::string name = entry.second->get_name();
        bool is_duplicate = false;
        for (auto const& seen_name : seen_names) {
            if (seen_name == name) {
                duplicates.push_back(name);
                is_duplicate = true;
                break;
            }
        }
        if (!is_duplicate) {
            seen_names.push_back(name);
        }
    }
    return duplicates;
}

bool make_effect_lib() {
    if (!g_effect_lib.empty()) {
        return true;
    }
    g_root_handle = h_effects.get();
    g_effect_lib[g_root_handle] = new Root_Info();
    g_effect_lib_handles_for_api.clear();
    g_effect_lib_handles_for_api.reserve(g_effect_lib.size());
    for (auto const& entry : g_effect_lib) {
        g_effect_lib_handles_for_api.push_back(entry.first);
    }
    auto duplicate_names = duplicate_effect_names(g_effect_lib);
    if (!duplicate_names.empty()) {
        g_effect_lib.clear();
        return false;
    }
    return true;
}

AVS_Effect_Handle get_handle_from_effect_info(Effect_Info* effect) {
    for (auto const& entry : g_effect_lib) {
        if (entry.second == NULL) {
            continue;
        }
        if (std::string(entry.second->get_name()) == std::string(effect->get_name())) {
            return entry.first;
        }
    }
    return 0;
}

Effect* component_factory(const Effect_Info* effect) {
    for (auto const& factory : g_component_factories) {
        if (factory.first == effect) {
            return factory.second();
        }
    }
    return NULL;
}
