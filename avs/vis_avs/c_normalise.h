#pragma once

#include "c__base.h"

#define MOD_NAME "Trans / Normalise"


class C_Normalise : public C_RBASE {
    public:
        C_Normalise();
        virtual ~C_Normalise();
        virtual int render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int);
        virtual char *get_desc();
        virtual void load_config(unsigned char *data, int len);
        virtual int  save_config(unsigned char *data);
        bool enabled;

    protected:
        int scan_min_max(int* framebuffer, int fb_length, unsigned char * max_out, unsigned char * min_out);
        int scan_min_max_sse2(int* framebuffer, int fb_length, unsigned char * max_out, unsigned char * min_out);
        void make_scale_table(unsigned char max, unsigned char min, unsigned int scale_table_out[]);
        void apply(int* framebuffer, int fb_length, unsigned int scale_table[]);
};
