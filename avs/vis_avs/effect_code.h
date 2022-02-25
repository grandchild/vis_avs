#pragma once

#include <cstdarg>
#include <cstdlib>
#include <string>

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
    void set(const char* str);
    void set(const char* str, unsigned int length);
    bool recompile_if_needed(void* vm_context);
    void exec(char visdata[2][2][576]);
    int load(char* src, unsigned int max_len);
    int save(char* dest, unsigned int max_len);
    int load_length_prefixed(char* src, unsigned int max_len);
    int save_length_prefixed(char* src, unsigned int max_len);

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

template <class Vars_T>
class ComponentCode : public ComponentCodeBase {
   public:
    Vars_T vars;
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
