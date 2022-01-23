#pragma once

#include "avs_editor.h"
#include "avs_internal.h"
#include "effect.h"
#include "effect_info.h"

#include <unordered_map>
#include <vector>

extern std::unordered_map<AVS_Effect_Handle, Effect_Info*> g_effect_lib;
extern std::vector<AVS_Effect_Handle> g_effect_lib_handles_for_api;
extern AVS_Effect_Handle g_root_handle;

bool make_effect_lib();
AVS_Effect_Handle get_handle_from_effect_info(Effect_Info* effect);
Effect* component_factory(const Effect_Info* effect);
