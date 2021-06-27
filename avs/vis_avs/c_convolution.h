#pragma once

#include "c__base.h"
#include <mmintrin.h>

#define MOD_NAME "Trans / Convolution Filter"
#define MAX_FILENAME_SIZE 1024

typedef int (* FunctionType)(void *, void *, void *);	// framebuffer, fbout, m64farray


class C_CONVOLUTION : public C_RBASE 
{
	public:
		// standard ape members
		C_CONVOLUTION();
		virtual ~C_CONVOLUTION();
		virtual int render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h);
		virtual char *get_desc();
		virtual void load_config(unsigned char *data, int len);
		virtual int  save_config(unsigned char *data);

		// file access members
		char szFile[MAX_FILENAME_SIZE];				// buffer for file name

		// functions
		void createdraw(void);			// create the draw function
		_inline void deletedraw(void);	// delete it
		FunctionType draw;				// the draw function

		// main filter data (saved)
		bool enabled;					// toggles plug-in on and off
		bool wraparound;				// toggles wrap around subtraction
		bool absolute;					// toggles absolute values in subtraction results
		bool twopass;					// toggles two pass mode
		int farray[51];					// box data

		// non-saved values
		__m64 m64farray[50];			// abs(farray) packed into the four unsigned shorts that make up an m64 
		int width,height;				// does what it says on the tin
		int oldprotect;					// needed for virtual protect stuff
		int drawstate;					// 0 not allocated 1 creating 2 created
		bool updatedraw;				// true if draw needs updating
		unsigned int codelength;		// length of draw
};
