#include "e_root.h"

#include "effect_library.h"

#include <string>
#include <vector>

typedef Effect* (*effect_component_factory)(AVS_Handle);

std::unordered_map<AVS_Effect_Handle, Effect_Info*> g_effect_lib;
std::vector<AVS_Effect_Handle> g_effect_lib_handles_for_api;
AVS_Effect_Handle g_root_handle;
std::unordered_map<Effect_Info*, effect_component_factory> g_component_factories;

static std::vector<std::string> find_duplicate_effect_names(
    const std::unordered_map<AVS_Effect_Handle, Effect_Info*>& lib) {
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
    auto duplicate_names = find_duplicate_effect_names(g_effect_lib);
    if (!duplicate_names.empty()) {
        g_effect_lib.clear();
        return false;
    }
    return true;
}

Effect* component_factory(const Effect_Info* effect, AVS_Handle avs) {
    for (auto const& factory : g_component_factories) {
        if (factory.first->get_handle() == effect->get_handle()) {
            return factory.second(avs);
        }
    }
    return NULL;
}
