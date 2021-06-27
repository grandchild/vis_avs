#pragma once

#include "c__base.h"
#include <windows.h>
#include <string>

#define C_THISCLASS C_DColorModClass
#define MOD_NAME "Trans / Color Modifier"


typedef struct {
  char *name;
  char *init;
  char *point;
  char *frame;
  char *beat;
  int recompute;
} colormodPresetType;

class C_THISCLASS : public C_RBASE {
	public:
		C_THISCLASS();
		virtual ~C_THISCLASS();
		virtual int render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h);
		virtual char *get_desc() { return MOD_NAME; }
		virtual void load_config(unsigned char *data, int len);
		virtual int  save_config(unsigned char *data);
    std::string effect_exp[4];

    int m_recompute;
    
    int m_tab_valid;
    unsigned char m_tab[768];
    int AVS_EEL_CONTEXTNAME;
    double *var_r, *var_g, *var_b, *var_beat;
		int inited;
    int codehandle[4];
    int need_recompile;
    CRITICAL_SECTION rcs;

		char *help_text="Color Modifier\0"
				"The color modifier allows you to modify the intensity of each color\r\n"
				"channel with respect to itself. For example, you could reverse the red\r\n"
				"channel, double the green channel, or half the blue channel.\r\n"
				"\r\n"
				"The code in the 'level' section should adjust the variables\r\n"
				"'red', 'green', and 'blue', whose value represent the channel\r\n"
				"intensity (0..1).\r\n"
				"Code in the 'frame' or 'level' sections can also use the variable\r\n"
				"'beat' to detect if it is currently a beat.\r\n"
				"\r\n"
				"Try loading an example via the 'Load Example' button for examples.";

		colormodPresetType presets[10] = {
				// Name, Init, Level, Frame, Beat, Recalc
				{"4x Red Brightness, 2x Green, 1x Blue","","red=4*red; green=2*green;","","",0},
				{"Solarization","","red=(min(1,red*2)-red)*2;\r\ngreen=red; blue=red;","","",0},
				{"Double Solarization","","red=(min(1,red*2)-red)*2;\r\nred=(min(1,red*2)-red)*2;\r\ngreen=red; blue=red;","","",0},
				{"Inverse Solarization (Soft)","","red=abs(red - .5) * 1.5;\r\ngreen=red; blue=red;","","",0},
				{"Big Brightness on Beat","scale=1.0","red=red*scale;\r\ngreen=red; blue=red;","scale=0.07 + (scale*0.93)","scale=16",1},
				{"Big Brightness on Beat (Interpolative)","c = 200; f = 0;","red = red * t;\r\ngreen=red;blue=red;","f = f + 1;\r\nt = (1.025 - (f / c)) * 5;","c = f;f = 0;",1},
				{"Pulsing Brightness (Beat Interpolative)","c = 200; f = 0;","red = red * st;\r\ngreen=red;blue=red;","f = f + 1;\r\nt = (f * 2 * $PI) / c;\r\nst = sin(t) + 1;","c = f;f = 0;",1},
				{"Rolling Solarization (Beat Interpolative)","c = 200; f = 0;","red=(min(1,red*st)-red)*st;\r\nred=(min(1,red*2)-red)*2;\r\ngreen=red; blue=red;","f = f + 1;\r\nt = (f * 2 * $PI) / c;\r\nst = ( sin(t) * .75 ) + 2;","c = f;f = 0;",1},
				{"Rolling Tone (Beat Interpolative)","c = 200; f = 0;","red = red * st;\r\ngreen = green * ct;\r\nblue = (blue * 4 * ti) - red - green;","f = f + 1;\r\nt = (f * 2 * $PI) / c;\r\nti = (f / c);\r\nst = sin(t) + 1.5;\r\nct = cos(t) + 1.5;","c = f;f = 0;",1},
				{"Random Inverse Tone (Switch on Beat)","","dd = red * 1.5;\r\nred = pow(dd, dr);\r\ngreen = pow(dd, dg);\r\nblue = pow(dd, db);","","token = rand(99) % 3;\r\ndr = if (equal(token, 0), -1, 1);\r\ndg = if (equal(token, 1), -1, 1);\r\ndb = if (equal(token, 2), -1, 1);",1},
		};
};
