#pragma once

#include "avs_editor.h"
#include "avs_internal.h"
#include "effect.h"
#include "effect_info.h"

#include <unordered_map>
#include <vector>

extern std::unordered_map<AVS_Effect_Handle, Effect_Info*> g_effect_lib;
extern std::vector<AVS_Effect_Handle> g_effect_lib_handles_for_api;

bool make_effect_lib();
Effect* component_factory(const Effect_Info* effect, AVS_Handle avs);
