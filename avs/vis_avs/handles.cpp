#include "handles.h"

Handles h_instances;
Handles h_components;
std::unordered_map<AVS_Parameter_Handle, const Parameter*> g_param_map;
std::unordered_map<const Effect_Info*, std::vector<AVS_Parameter_Handle>>
    g_effect_parameters_for_api;
std::unordered_map<const Parameter*, std::vector<AVS_Parameter_Handle>>
    g_child_parameters_for_api;
