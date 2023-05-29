#pragma once

#include "effect.h"
#include "effect_info.h"

#include <stdio.h>
#include <stdlib.h>

struct Root_Config : public Effect_Config {
    bool clear = false;
};

struct Root_Info : public Effect_Info {
    static constexpr char* group = "";
    static constexpr char* name = "Root";
    static constexpr char* help =
        "The main component of the preset\n"
        "Define basic settings for the preset here\n";

    static constexpr uint32_t num_parameters = 1;
    static constexpr Parameter parameters[num_parameters] = {
        P_BOOL(offsetof(Root_Config, clear),
               "Clear",
               "Clear the screen for every new frame"),
    };

    bool can_have_child_components() const { return true; };
    EFFECT_INFO_GETTERS;
};

class E_Root : public Configurable_Effect<Root_Info, Root_Config> {};
