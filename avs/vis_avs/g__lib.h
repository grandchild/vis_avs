#pragma once

#include <windows.h>
#include <string>

#define APE_ID_BASE                 16384
#define UI_COMPONENT_LIST_ALLOC_LEN 80
#define COMPONENT_IDSTRING_LEN      32

#define DECL_HANDLER(NAME) extern int win32_dlgproc_##NAME(HWND, UINT, WPARAM, LPARAM)
#define DECL_UIPREP(NAME)  extern void win32_uiprep_##NAME(HINSTANCE)

DECL_HANDLER(unknown);
DECL_HANDLER(root);
DECL_HANDLER(effectlist);
DECL_HANDLER(simple);
DECL_HANDLER(dotplane);
DECL_HANDLER(oscstar);
DECL_HANDLER(fade);
DECL_HANDLER(blitterfeedback);
DECL_HANDLER(onbeatclear);
DECL_HANDLER(blur);
DECL_HANDLER(bassspin);
DECL_HANDLER(movingparticle);
DECL_HANDLER(rotoblitter);
DECL_HANDLER(svp);
DECL_HANDLER(colorfade);
DECL_HANDLER(colorclip);
DECL_HANDLER(rotstar);
DECL_HANDLER(ring);
DECL_HANDLER(movement);
DECL_HANDLER(scatter);
DECL_HANDLER(dotgrid);
DECL_HANDLER(buffersave);
DECL_HANDLER(dotfountain);
DECL_HANDLER(water);
DECL_HANDLER(comment);
DECL_HANDLER(brightness);
DECL_HANDLER(interleave);
DECL_HANDLER(grain);
DECL_HANDLER(clearscreen);
DECL_HANDLER(mirror);
DECL_HANDLER(starfield);
DECL_HANDLER(text);
DECL_HANDLER(bump);
DECL_HANDLER(mosaic);
DECL_HANDLER(waterbump);
DECL_HANDLER(video);
DECL_HANDLER(custombpm);
DECL_HANDLER(picture);
DECL_HANDLER(dynamicdistancemodifier);
DECL_HANDLER(superscope);
DECL_HANDLER(invert);
DECL_HANDLER(uniquetone);
DECL_HANDLER(timescope);
DECL_HANDLER(setrendermode);
DECL_HANDLER(interferences);
DECL_HANDLER(dynamicshift);
DECL_HANDLER(dynamicmovement);
DECL_HANDLER(fastbrightness);
DECL_HANDLER(colormodifier);
DECL_HANDLER(chanshift);
DECL_HANDLER(colorreduction);
DECL_HANDLER(multiplier);
DECL_HANDLER(multidelay);
DECL_HANDLER(videodelay);
DECL_HANDLER(convolution);
DECL_HANDLER(texer2);
DECL_HANDLER(normalise);
DECL_HANDLER(colormap);
DECL_UIPREP(colormap);
DECL_HANDLER(addborders);
DECL_HANDLER(triangle);
DECL_HANDLER(eeltrans);
DECL_HANDLER(globalvariables);
DECL_HANDLER(multifilter);
DECL_HANDLER(picture2);
DECL_HANDLER(frameratelimiter);
DECL_HANDLER(texer);

struct C_Win32GuiComponent {
    // This should match the render component id number. If component is an APE, which
    // are identified by strings, this is -1. Note that -2 is the special code for an
    // 2.81+ extended EL again. All other ids are positive.
    int id = -1;
    // This should match the render component unique id string, if component is of APE
    // type (plugin or builtin). If it's a standard component, this is a NULL pointer.
    char idstring[COMPONENT_IDSTRING_LEN]{};
    // A dialog resource id for the component config UI.
    int dialog_resource_id = 0;
    // Handler function for UI events.
    DLGPROC ui_handler = nullptr;
    // Optional preparation function, called before creating the dialog. Useful to
    // register custom UI classes.
    void (*uiprep)(HINSTANCE) = nullptr;
};

class C_GLibrary {
   public:
    C_GLibrary();
    C_Win32GuiComponent* get(int id_or_idstring);

   protected:
    C_Win32GuiComponent* get_by_idstring(char* idstring);
    C_Win32GuiComponent components[UI_COMPONENT_LIST_ALLOC_LEN]{};
    C_Win32GuiComponent unknown;
    C_Win32GuiComponent effectlist;
    C_Win32GuiComponent root;
    unsigned int size;
};

std::string string_from_dlgitem(HWND hwnd, int dlgItem);

// A pointer to the currently open dialog UI. Used by each component's dialog handler.
// Will be cast to the respective component class. Set in cfgwin.cpp.
extern void* g_current_render;
