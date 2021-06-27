#pragma once

#include "c__base.h"

#define MOD_NAME "Trans / Channel Shift"
#define C_THISCLASS C_ChannelShiftClass


typedef struct {
	int	mode;
	int onbeat;
} channelShiftConfig;


class C_THISCLASS : public C_RBASE 
{
	protected:
	public:
		C_THISCLASS();
		virtual ~C_THISCLASS();
		virtual int render(char visdata[2][2][576], int isBeat,	int *framebuffer, int *fbout, int w, int h);		
		virtual char *get_desc();
		virtual void load_config(unsigned char *data, int len);
		virtual int  save_config(unsigned char *data);

		channelShiftConfig config;
};
