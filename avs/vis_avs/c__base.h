#pragma once

#include <cstdlib>
#include <string>

#define MAX_PATH 260
#define M_PI 3.14159265358979323846


class C_RBASE {
    public:
        C_RBASE() { }
        virtual ~C_RBASE() { };
        virtual int render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h)=0; // returns 1 if fbout has dest
        virtual char *get_desc()=0;
        virtual void load_config(unsigned char *data, int len) { }
        virtual int  save_config(unsigned char *data) { return 0; }

        void load_string(std::string &s,unsigned char *data, int &pos, int len);
        void save_string(unsigned char *data, int &pos, std::string &text);
};

class C_RBASE2 : public C_RBASE {
    public:
        C_RBASE2() { }
        virtual ~C_RBASE2() { };

    int getRenderVer2() { return 2; }


    virtual int smp_getflags() { return 0; } // return 1 to enable smp support

    // returns # of threads you desire, <= max_threads, or 0 to not do anything
    // default should return max_threads if you are flexible
    virtual int smp_begin(int max_threads, char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h) { return 0; }  
    virtual void smp_render(int this_thread, int max_threads, char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h) { }; 
    virtual int smp_finish(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h) { return 0; }; // return value is that of render() for fbstuff etc
};
