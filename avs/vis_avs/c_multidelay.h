#pragma once

#include "c__base.h"

#define MOD_NAME               "Trans / Multi Delay"
#define C_DELAY                C_MultiDelayClass
#define MULTIDELAY_NUM_BUFFERS 6

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

    bool usebeats(int buf);
    void usebeats(int buf, bool value);
    int delay(int buf);
    void delay(int buf, int value);
    void framedelay(int buf, unsigned long value);
    unsigned long framesperbeat();

    // saved members
    int mode;
    int activebuffer;

    // unsaved members
    unsigned int creationid;
};
