#pragma once

#include "effect.h"

#include <string>

#define LIST_ID -2  // 0xfffffffe

class C_RBASE {
   public:
    C_RBASE(){};
    virtual ~C_RBASE(){};
    virtual int render(char visdata[2][2][576],
                       int is_beat,
                       int* framebuffer,
                       int* fbout,
                       int w,
                       int h) {
        (void)visdata, (void)is_beat, (void)framebuffer, (void)fbout, (void)w, (void)h;
        return 0;  // returns 1 if fbout has dest
    };
    virtual char* get_desc() = 0;
    virtual void load_config(unsigned char* data, int len) { (void)data, (void)len; };
    virtual int save_config(unsigned char* data) {
        (void)data;
        return 0;
    }

    void load_string(std::string& s, unsigned char* data, int& pos, int len);
    void save_string(unsigned char* data, int& pos, std::string& text);
};

class C_RBASE2 : public C_RBASE {
   public:
    C_RBASE2() {}
    virtual ~C_RBASE2(){};
    virtual int smp_getflags() { return 0; }  // return 1 to enable smp support

    // returns # of threads you desire, <= max_threads, or 0 to not do anything
    // default should return max_threads if you are flexible
    virtual int smp_begin(int max_threads,
                          char visdata[2][2][576],
                          int is_beat,
                          int* framebuffer,
                          int* fbout,
                          int w,
                          int h) {
        (void)max_threads, (void)visdata, (void)is_beat, (void)framebuffer, (void)fbout,
            (void)w, (void)h;
        return 0;
    }
    virtual void smp_render(int this_thread,
                            int max_threads,
                            char visdata[2][2][576],
                            int is_beat,
                            int* framebuffer,
                            int* fbout,
                            int w,
                            int h) {
        (void)this_thread, (void)max_threads, (void)visdata, (void)is_beat,
            (void)framebuffer, (void)fbout, (void)w, (void)h;
    };
    virtual int smp_finish(char visdata[2][2][576],
                           int is_beat,
                           int* framebuffer,
                           int* fbout,
                           int w,
                           int h) {
        (void)visdata, (void)is_beat, (void)framebuffer, (void)fbout, (void)w, (void)h;
        return 0;  // returns 1 if fbout has dest
    };
};
