#pragma once

#include "c__base.h"

#define MOD_NAME "Trans / Video Delay"
#define C_DELAY  C_VideoDelayClass

class C_DELAY : public C_RBASE {
   protected:
   public:
    // standard ape members
    C_DELAY();
    virtual ~C_DELAY();
    virtual int render(char visdata[2][2][576],
                       int isBeat,
                       int* framebuffer,
                       int* fbout,
                       int w,
                       int h);
    virtual char* get_desc();
    virtual void load_config(unsigned char* data, int len);
    virtual int save_config(unsigned char* data);

    // saved members
    bool enabled;
    bool usebeats;
    unsigned int delay;

    // unsaved members
    void* buffer;
    void* inoutpos;
    unsigned long buffersize;
    unsigned long virtualbuffersize;
    unsigned long oldvirtualbuffersize;
    unsigned long framessincebeat;
    unsigned long framedelay;
    unsigned long framemem;
    unsigned long oldframemem;
};
