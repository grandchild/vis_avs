#pragma once

#include "c__base.h"

#define MOD_NAME "Misc / Custom BPM"
#define C_THISCLASS C_BpmClass
#define SET_BEAT		0x10000000
#define CLR_BEAT		0x20000000


class C_THISCLASS : public C_RBASE {
	protected:
	public:
		C_THISCLASS();
		virtual ~C_THISCLASS();
		virtual int render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h);
		virtual char *get_desc() { return MOD_NAME; }
		virtual void load_config(unsigned char *data, int len);
		virtual int  save_config(unsigned char *data);
		void SliderStep(int Ctl, int *slide);
    int enabled;
	int arbitrary, skip, invert;		// Type of action, adapt = detect beat
	unsigned int arbVal, skipVal;		// Values of arbitrary beat and beat skip
	unsigned int arbLastTC;				// Last tick count used for arbitrary beat
	unsigned int skipCount;				// Beat counter used by beat skipper
	int inInc, outInc;					// +1/-1, Used by the nifty beatsynced sliders
	int inSlide, outSlide;				// Positions of sliders
	int oldInSlide, oldOutSlide;		// Used by timer to detect changes in sliders
	int skipfirst;
	int count;
	};
