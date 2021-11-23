#pragma once

#include <cstdarg>
#include <cstdlib>
#include <string>

class C_RBASE {
   public:
    C_RBASE(){};
    virtual ~C_RBASE(){};
    virtual int render(char visdata[2][2][576],
                       int isBeat,
                       int* framebuffer,
                       int* fbout,
                       int w,
                       int h) {
        (void)visdata, (void)isBeat, (void)framebuffer, (void)fbout, (void)w, (void)h;
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
    int getRenderVer2() { return 2; }
    virtual int smp_getflags() { return 0; }  // return 1 to enable smp support

    // returns # of threads you desire, <= max_threads, or 0 to not do anything
    // default should return max_threads if you are flexible
    virtual int smp_begin(int max_threads,
                          char visdata[2][2][576],
                          int isBeat,
                          int* framebuffer,
                          int* fbout,
                          int w,
                          int h) {
        (void)max_threads, (void)visdata, (void)isBeat, (void)framebuffer, (void)fbout,
            (void)w, (void)h;
        return 0;
    }
    virtual void smp_render(int this_thread,
                            int max_threads,
                            char visdata[2][2][576],
                            int isBeat,
                            int* framebuffer,
                            int* fbout,
                            int w,
                            int h) {
        (void)this_thread, (void)max_threads, (void)visdata, (void)isBeat,
            (void)framebuffer, (void)fbout, (void)w, (void)h;
    };
    virtual int smp_finish(char visdata[2][2][576],
                           int isBeat,
                           int* framebuffer,
                           int* fbout,
                           int w,
                           int h) {
        (void)visdata, (void)isBeat, (void)framebuffer, (void)fbout, (void)w, (void)h;
        return 0;  // returns 1 if fbout has dest
    };
};

/* Inherit from this class to define available EEL variables:

    #include "avs_eelif.h"  // for NSEEL_VM_regvar()

    class MyVars : public VarsBase {
       public:
        double* stuff;
        double* w;
        double* h;
        double* n;

        virtual void register_variables(void* vm_context) {
            this->stuff = NSEEL_VM_regvar(vm_context, "stuff");
            this->w = NSEEL_VM_regvar(vm_context, "w");
            this->h = NSEEL_VM_regvar(vm_context, "h");
            this->n = NSEEL_VM_regvar(vm_context, "n");
        };

        virtual void init_variables(int w, int h, int is_beat, va_list extra_args) {
            *this->w = w;
            *this->h = h;
            *this->n = 0.0f;
            *this->foo = va_arg(extra_args, int);
            *this->bar = va_arg(extra_args, double);
        };
    }

   Then you can declare the code member in the component class like this:

    class MyEffect : C_RBASE {
        // ... (the usual interface)

        ComponentCode<MyVars> code;

        // ...
    }
*/
class VarsBase {
   public:
    virtual void register_variables(void* vm_context) = 0;
    // Overriding this method is not strictly necessary, but probably helpful.
    // Pass in additional parameters to ComponentCode.init_variables() as needed, and
    // retrieve them with va_arg() like above.
    virtual void init_variables(int w, int h, int is_beat, va_list) {
        (void)w, (void)h, (void)is_beat;
    };
};

class CodeSection {
   public:
    char* string;
    bool need_recompile;

    CodeSection();
    ~CodeSection();
    void set(char* str, unsigned int length);
    bool recompile_if_needed(void* vm_context);
    void exec(char visdata[2][2][576]);
    int load(char* src, unsigned int max_len);
    int save(char* dest, unsigned int max_len);

   private:
    void* code;
};

class ComponentCodeBase {
   public:
    CodeSection init;
    CodeSection frame;
    CodeSection beat;
    CodeSection point;
    bool need_init;
    void* vm_context;

    ComponentCodeBase();
    ~ComponentCodeBase();
    void recompile_if_needed();
    virtual void register_variables() = 0;
    virtual void init_variables(int w, int h, int is_beat, ...) = 0;
};

template <class VarsT>
class ComponentCode : public ComponentCodeBase {
   public:
    VarsT vars;
    virtual void register_variables() {
        this->vars.register_variables(this->vm_context);
    };
    virtual void init_variables(int w, int h, int is_beat, ...) {
        va_list extra_args;
        va_start(extra_args, is_beat);
        this->vars.init_variables(w, h, is_beat, extra_args);
        va_end(extra_args);
    };
};
