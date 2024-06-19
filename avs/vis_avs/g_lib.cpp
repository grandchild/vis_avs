#include "e_effectlist.h"
#include "e_root.h"
#include "e_unknown.h"

#include "g__lib.h"

#include "resource.h"

#include <string>

#define ADD_COMPONENT(ID_NUM, ID_STRING, RES_SUFFIX, HANDLER_SUFFIX)          \
    this->components[i].id = ID_NUM;                                          \
    this->components[i].dialog_resource_id = IDD_CFG_##RES_SUFFIX;            \
    this->components[i].ui_handler = (DLGPROC)win32_dlgproc_##HANDLER_SUFFIX; \
    i++

#define ADD_STANDARD_COMPONENT(ID_NUM, RES_SUFFIX, HANDLER_SUFFIX) \
    this->components[i].idstring[0] = '\0';                        \
    ADD_COMPONENT(ID_NUM, nullptr, RES_SUFFIX, HANDLER_SUFFIX)

#define ADD_APE_COMPONENT(ID_STRING, RES_SUFFIX, HANDLER_SUFFIX)              \
    strncpy(this->components[i].idstring, ID_STRING, COMPONENT_IDSTRING_LEN); \
    this->components[i].idstring[COMPONENT_IDSTRING_LEN - 1] = '\0';          \
    ADD_COMPONENT(-1, ID_STRING, RES_SUFFIX, HANDLER_SUFFIX)

#define ADD_APE_COMPONENT_PREP(ID_STRING, RES_SUFFIX, HANDLER_AND_PREP_SUFFIX) \
    strncpy(this->components[i].idstring, ID_STRING, COMPONENT_IDSTRING_LEN);  \
    this->components[i].idstring[COMPONENT_IDSTRING_LEN - 1] = '\0';           \
    this->components[i].uiprep = win32_uiprep_##HANDLER_AND_PREP_SUFFIX;       \
    ADD_COMPONENT(-1, ID_STRING, RES_SUFFIX, HANDLER_AND_PREP_SUFFIX)

C_GLibrary::C_GLibrary() {
    unsigned int i = 0;
    // clang-format off
    ADD_STANDARD_COMPONENT(0,  SIMPLE,      simple);
    ADD_STANDARD_COMPONENT(1,  DOTPLANE,    dotplane);
    ADD_STANDARD_COMPONENT(2,  OSCSTAR,     oscstar);
    ADD_STANDARD_COMPONENT(3,  FADE,        fade);
    ADD_STANDARD_COMPONENT(4,  BLT,         blitterfeedback);
    ADD_STANDARD_COMPONENT(5,  NFC,         onbeatclear);
    ADD_STANDARD_COMPONENT(6,  BLUR,        blur);
    ADD_STANDARD_COMPONENT(7,  BSPIN,       bassspin);
    ADD_STANDARD_COMPONENT(8,  PARTS,       movingparticle);
    ADD_STANDARD_COMPONENT(9,  ROTBLT,      rotoblitter);
    ADD_STANDARD_COMPONENT(10, SVP,         svp);
    ADD_STANDARD_COMPONENT(11, COLORFADE,   colorfade);
    ADD_STANDARD_COMPONENT(12, COLORCLIP,   colorclip);
    ADD_STANDARD_COMPONENT(13, ROTSTAR,     rotstar);
    ADD_STANDARD_COMPONENT(14, RING,        ring);
    ADD_STANDARD_COMPONENT(15, TRANS,       movement);
    ADD_STANDARD_COMPONENT(16, SCAT,        scatter);
    ADD_STANDARD_COMPONENT(17, DOTGRID,     dotgrid);
    ADD_STANDARD_COMPONENT(18, STACK,       buffersave);
    ADD_STANDARD_COMPONENT(19, DOTFOUNTAIN, dotfountain);
    ADD_STANDARD_COMPONENT(20, WATER,       water);
    ADD_STANDARD_COMPONENT(21, COMMENT,     comment);
    ADD_STANDARD_COMPONENT(22, BRIGHTNESS,  brightness);
    ADD_STANDARD_COMPONENT(23, INTERLEAVE,  interleave);
    ADD_STANDARD_COMPONENT(24, GRAIN,       grain);
    ADD_STANDARD_COMPONENT(25, CLEAR,       clearscreen);
    ADD_STANDARD_COMPONENT(26, MIRROR,      mirror);
    ADD_STANDARD_COMPONENT(27, STARFIELD,   starfield);
    ADD_STANDARD_COMPONENT(28, TEXT,        text);
    ADD_STANDARD_COMPONENT(29, BUMP,        bump);
    ADD_STANDARD_COMPONENT(30, MOSAIC,      mosaic);
    ADD_STANDARD_COMPONENT(31, WATERBUMP,   waterbump);
    ADD_STANDARD_COMPONENT(32, VIDEO,       video);
    ADD_STANDARD_COMPONENT(33, BPM,         custombpm);
    ADD_STANDARD_COMPONENT(34, PICTURE,     picture);
    ADD_STANDARD_COMPONENT(35, DDM,         dynamicdistancemodifier);
    ADD_STANDARD_COMPONENT(36, SSCOPE,      superscope);
    ADD_STANDARD_COMPONENT(37, INVERT,      invert);
    ADD_STANDARD_COMPONENT(38, ONETONE,     uniquetone);
    ADD_STANDARD_COMPONENT(39, TIMESCOPE,   timescope);
    ADD_STANDARD_COMPONENT(40, LINEMODE,    setrendermode);
    ADD_STANDARD_COMPONENT(41, INTERF,      interferences);
    ADD_STANDARD_COMPONENT(42, SHIFT,       dynamicshift);
    ADD_STANDARD_COMPONENT(43, DMOVE,       dynamicmovement);
    ADD_STANDARD_COMPONENT(44, FASTBRIGHT,  fastbrightness);
    ADD_STANDARD_COMPONENT(45, COLORMOD,    colormodifier);
    ADD_APE_COMPONENT("Channel Shift",                CHANSHIFT,      chanshift);
    ADD_APE_COMPONENT("Color Reduction",              COLORREDUCTION, colorreduction);
    ADD_APE_COMPONENT("Multiplier",                   MULT,           multiplier);
    ADD_APE_COMPONENT("Holden05: Multi Delay",        MULTIDELAY,     multidelay);
    ADD_APE_COMPONENT("Holden04: Video Delay",        VIDEODELAY,     videodelay);
    ADD_APE_COMPONENT("Holden03: Convolution Filter", CONVOLUTION,    convolution);
    ADD_APE_COMPONENT("Acko.net: Texer II",           TEXERII,        texer2);
    ADD_APE_COMPONENT("Trans: Normalise",             NORMALISE,      normalise);
    ADD_APE_COMPONENT_PREP("Color Map",               COLORMAP,       colormap);
    ADD_APE_COMPONENT("Virtual Effect: Addborders",   ADDBORDERS,     addborders);
    ADD_APE_COMPONENT("Render: Triangle",             TRIANGLE,       triangle);
    ADD_APE_COMPONENT("Misc: AVSTrans Automation",    EELTRANS,       eeltrans);
    ADD_APE_COMPONENT("Jheriko: Global",              GLOBALVARS,     globalvariables);
    ADD_APE_COMPONENT("Jheriko : MULTIFILTER",        MULTIFILTER,    multifilter);
    ADD_APE_COMPONENT("Picture II",                   PICTUREII,      picture2);
    ADD_APE_COMPONENT("VFX FRAMERATE LIMITER",        FPSLIMITER,     frameratelimiter);
    ADD_APE_COMPONENT("Texer",                        TEXER,          texer);
    // clang-format on
    this->size = i;

    this->unknown.id = Unknown_Info::legacy_id;
    this->unknown.idstring[0] = '\0';
    this->unknown.dialog_resource_id = IDD_CFG_UNKN;
    this->unknown.ui_handler = (DLGPROC)win32_dlgproc_unknown;

    this->effectlist.id = EffectList_Info::legacy_id;
    this->effectlist.idstring[0] = '\0';
    this->effectlist.dialog_resource_id = IDD_CFG_LIST;
    this->effectlist.ui_handler = (DLGPROC)win32_dlgproc_effectlist;

    this->root.id = Root_Info::legacy_id;
    this->root.idstring[0] = '\0';
    this->root.dialog_resource_id = IDD_CFG_LISTROOT;
    this->root.ui_handler = (DLGPROC)win32_dlgproc_root;
}

C_Win32GuiComponent* C_GLibrary::get(int id_or_idstring) {
    if (id_or_idstring == -1) {
        return &this->unknown;
    }
    if (id_or_idstring > APE_ID_BASE) {
        return this->get_by_idstring((char*)id_or_idstring);
    }
    for (unsigned int i = 0; i < this->size; i++) {
        if (this->components[i].id == id_or_idstring) {
            return &this->components[i];
        }
    }
    if (id_or_idstring == EffectList_Info::legacy_id) {
        return &this->effectlist;
    }
    if (id_or_idstring == Root_Info::legacy_id) {
        return &this->root;
    }
    return nullptr;
}

C_Win32GuiComponent* C_GLibrary::get_by_idstring(char* idstring) {
    for (unsigned int i = 0; i < this->size; i++) {
        if (strncmp(this->components[i].idstring, idstring, COMPONENT_IDSTRING_LEN)
            == 0) {
            return &this->components[i];
        }
    }
    return nullptr;
}

std::string string_from_dlgitem(HWND hwnd, int dlgItem) {
    int l = SendDlgItemMessage(hwnd, dlgItem, WM_GETTEXTLENGTH, 0, 0);
    char* buf = (char*)calloc(l + 1, sizeof(char));
    GetDlgItemText(hwnd, dlgItem, buf, l + 1);
    std::string str(buf);
    free(buf);
    return str;
}

void* g_current_render;
