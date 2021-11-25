#pragma once

#include "c__base.h"

#define MOD_NAME "Misc / Framerate Limiter"

class C_FramerateLimiter : public C_RBASE {
   public:
    C_FramerateLimiter();
    virtual ~C_FramerateLimiter();
    virtual int render(char visdata[2][2][576],
                       int isBeat,
                       int* framebuffer,
                       int* fbout,
                       int w,
                       int h);
    virtual char* get_desc();
    virtual void load_config(unsigned char* data, int len);
    virtual int save_config(unsigned char* data);

    void update_time_diff();

    bool enabled;
    int framerate_limit;
    int time_diff;
    int64_t last_time;
    static const int default_framerate_limit = 30;

   private:
    int64_t now();
    void sleep(int32_t duration_ms);
};
