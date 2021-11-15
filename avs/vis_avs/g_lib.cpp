#include <string>
#include "g__lib.h"
#include "c_list.h"
#include "c_unkn.h"
#include "resource.h"


#define ADD_COMPONENT(ID_NUM, ID_STRING, RES_SUFFIX, HANDLER_SUFFIX)            \
    this->components[i].id = ID_NUM;                                            \
    this->components[i].dialog_resource_id = IDD_CFG_ ## RES_SUFFIX;            \
    this->components[i].ui_handler = (DLGPROC)win32_dlgproc_ ## HANDLER_SUFFIX; \
    i++

#define ADD_STANDARD_COMPONENT(ID_NUM, RES_SUFFIX, HANDLER_SUFFIX) \
    this->components[i].idstring[0] = '\0';                        \
    ADD_COMPONENT(ID_NUM, NULL, RES_SUFFIX, HANDLER_SUFFIX)

#define ADD_APE_COMPONENT(ID_STRING, RES_SUFFIX, HANDLER_SUFFIX)              \
    strncpy(this->components[i].idstring, ID_STRING, COMPONENT_IDSTRING_LEN); \
    this->components[i].idstring[COMPONENT_IDSTRING_LEN - 1] = '\0';          \
    ADD_COMPONENT(-1, ID_STRING, RES_SUFFIX, HANDLER_SUFFIX)

#define ADD_APE_COMPONENT_PREP(ID_STRING, RES_SUFFIX, HANDLER_AND_PREP_SUFFIX) \
    strncpy(this->components[i].idstring, ID_STRING, COMPONENT_IDSTRING_LEN);  \
    this->components[i].idstring[COMPONENT_IDSTRING_LEN - 1] = '\0';           \
    this->components[i].uiprep = win32_uiprep_ ## HANDLER_AND_PREP_SUFFIX;     \
    ADD_COMPONENT(-1, ID_STRING, RES_SUFFIX, HANDLER_AND_PREP_SUFFIX)


C_GLibrary::C_GLibrary() {
    this->components = (C_Win32GuiComponent*)calloc(
        UI_COMPONENT_LIST_ALLOC_LEN, sizeof(C_Win32GuiComponent));

    unsigned int i = 0;
    // clang-format off
    ADD_STANDARD_COMPONENT(0,  SIMPLE,     simple);
    ADD_STANDARD_COMPONENT(1,  DOTPLANE,   dotplane);
    ADD_STANDARD_COMPONENT(2,  OSCSTAR,    oscstar);
    ADD_STANDARD_COMPONENT(3,  FADE,       fade);
    ADD_STANDARD_COMPONENT(4,  BLT,        blitterfeedback);
    ADD_STANDARD_COMPONENT(5,  NFC,        onbeatclear);
    ADD_STANDARD_COMPONENT(6,  BLUR,       blur);
    ADD_STANDARD_COMPONENT(7,  BSPIN,      bassspin);
    ADD_STANDARD_COMPONENT(8,  PARTS,      movingparticles);
    ADD_STANDARD_COMPONENT(9,  ROTBLT,     rotoblitter);
    ADD_STANDARD_COMPONENT(10, SVP,        svp);
    ADD_STANDARD_COMPONENT(11, COLORFADE,  colorfade);
    ADD_STANDARD_COMPONENT(12, COLORCLIP,  colorclip);
    ADD_STANDARD_COMPONENT(13, ROTSTAR,    rotstar);
    ADD_STANDARD_COMPONENT(14, OSCRING,    oscring);
    ADD_STANDARD_COMPONENT(15, TRANS,      movement);
    ADD_STANDARD_COMPONENT(16, SCAT,       scatter);
    ADD_STANDARD_COMPONENT(17, DOTGRID,    dotgrid);
    ADD_STANDARD_COMPONENT(18, STACK,      buffersave);
    ADD_STANDARD_COMPONENT(19, DOTPLANE,   dotfountain);
    ADD_STANDARD_COMPONENT(20, WATER,      water);
    ADD_STANDARD_COMPONENT(21, COMMENT,    comment);
    ADD_STANDARD_COMPONENT(22, BRIGHTNESS, brightness);
    ADD_STANDARD_COMPONENT(23, INTERLEAVE, interleave);
    ADD_STANDARD_COMPONENT(24, GRAIN,      grain);
    ADD_STANDARD_COMPONENT(25, CLEAR,      clear);
    ADD_STANDARD_COMPONENT(26, MIRROR,     mirror);
    ADD_STANDARD_COMPONENT(27, STARFIELD,  starfield);
    ADD_STANDARD_COMPONENT(28, TEXT,       text);
    ADD_STANDARD_COMPONENT(29, BUMP,       bump);
    ADD_STANDARD_COMPONENT(30, MOSAIC,     mosaic);
    ADD_STANDARD_COMPONENT(31, WATERBUMP,  waterbump);
    ADD_STANDARD_COMPONENT(32, AVI,        avi);
    ADD_STANDARD_COMPONENT(33, BPM,        custombpm);
    ADD_STANDARD_COMPONENT(34, PICTURE,    picture);
    ADD_STANDARD_COMPONENT(35, DDM,        dynamicdistancemodifier);
    ADD_STANDARD_COMPONENT(36, SSCOPE,     superscope);
    ADD_STANDARD_COMPONENT(37, INVERT,     invert);
    ADD_STANDARD_COMPONENT(38, ONETONE,    uniquetone);
    ADD_STANDARD_COMPONENT(39, TIMESCOPE,  timescope);
    ADD_STANDARD_COMPONENT(40, LINEMODE,   setrendermode);
    ADD_STANDARD_COMPONENT(41, INTERF,     interferences);
    ADD_STANDARD_COMPONENT(42, SHIFT,      dynamicshift);
    ADD_STANDARD_COMPONENT(43, DMOVE,      dynamicmovement);
    ADD_STANDARD_COMPONENT(44, FASTBRIGHT, fastbrightness);
    ADD_STANDARD_COMPONENT(45, COLORMOD,   colormodifier);
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
    ADD_APE_COMPONENT("Jheriko: Global",              GLOBALVARS,     globalvars);
    ADD_APE_COMPONENT("Jheriko : MULTIFILTER",        MULTIFILTER,    multifilter);
    ADD_APE_COMPONENT("Picture II",                   PICTUREII,      picture2);
    // clang-format on
    this->size = i;

    this->unknown.id = UNKN_ID;
    this->unknown.idstring[0] = '\0';
    this->unknown.dialog_resource_id = IDD_CFG_UNKN;
    this->unknown.ui_handler = (DLGPROC)win32_dlgproc_unknown;

    this->effectlist.id = LIST_ID;
    this->effectlist.idstring[0] = '\0';
    this->effectlist.dialog_resource_id = IDD_CFG_LIST;
    this->effectlist.ui_handler = (DLGPROC)win32_dlgproc_effectlist;
    // the root EL is the same component, but has "isroot" attr set that disambiguates
    // -> special handling in this->get().
    this->root_effectlist.id = LIST_ID;
    this->root_effectlist.idstring[0] = '\0';
    this->root_effectlist.dialog_resource_id = IDD_CFG_LISTROOT;
    this->root_effectlist.ui_handler = (DLGPROC)win32_dlgproc_root_effectlist;
}

C_GLibrary::~C_GLibrary() {
    free(this->components);
}

C_Win32GuiComponent* C_GLibrary::get(int id_or_idstring, void* render_component) {
    if(id_or_idstring == -1) {
        return &this->unknown;
    }
    if(id_or_idstring > APE_ID_BASE) {
        return this->get_by_idstring((char*)id_or_idstring);
    }
    for(unsigned int i=0; i<this->size; i++) {
        if(this->components[i].id == id_or_idstring) {
            return &this->components[i];
        }
    }
    if(id_or_idstring == LIST_ID) {
        if(((C_RenderListClass*)render_component)->isroot) {
            return &this->root_effectlist;
        } else {
            return &this->effectlist;
        }
    }
    return NULL;
}

C_Win32GuiComponent* C_GLibrary::get_by_idstring(char* idstring) {
    for(unsigned int i=0; i<this->size; i++) {
        if(strncmp(this->components[i].idstring, idstring, COMPONENT_IDSTRING_LEN)==0) {
            return &this->components[i];
        }
    }
    return NULL;
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
