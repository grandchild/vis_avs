#pragma once

#include "effect.h"

#include <string>

#define LIST_ID -2  // 0xfffffffe

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

// TODO [clean]: Remove this class when all effects have been changed to the new layout.
class Legacy_Effect_Proxy {
   public:
    C_RBASE* legacy_effect;
    Effect* effect;
    int32_t legacy_effect_index;
    const char* legacy_ape_id;

    Legacy_Effect_Proxy() : legacy_effect(NULL), effect(NULL){};
    Legacy_Effect_Proxy(C_RBASE* legacy_effect,
                        Effect* effect,
                        int legacy_effect_index = -3)
        : legacy_effect(legacy_effect),
          effect(effect),
          legacy_effect_index(legacy_effect_index){};
    Legacy_Effect_Proxy& operator=(Effect* other) {
        if (this->effect == other) {
            return *this;
        };
        this->legacy_effect = NULL;
        this->effect = other;
        return *this;
    };
    Legacy_Effect_Proxy& operator=(C_RBASE* other) {
        if (this->legacy_effect == other) {
            return *this;
        };
        this->legacy_effect = other;
        this->effect = NULL;
        return *this;
    };
    bool operator==(Legacy_Effect_Proxy& other) {
        return this->effect == other.effect
               && this->legacy_effect == other.legacy_effect;
    };
    bool operator!=(Legacy_Effect_Proxy& other) {
        return this->effect != other.effect
               || this->legacy_effect != other.legacy_effect;
    };

    enum Type {
        NEW_EFFECT = 0,
        LEGACY_EFFECT = 1,
    };
    Type type() {
        return this->legacy_effect != NULL ? Type::LEGACY_EFFECT : Type::NEW_EFFECT;
    };
    bool is_list() {
        return this->type() != NEW_EFFECT && this->legacy_effect_index == LIST_ID;
    };
    bool valid() { return this->legacy_effect || this->effect; };
    void invalidate() {
        this->effect = NULL;
        this->legacy_effect = NULL;
    };
    void* void_ref() {
        switch (this->type()) {
            case NEW_EFFECT: return (void*)this->effect;
            default: return (void*)this->legacy_effect;
        }
    };

    int render(char visdata[2][2][576],
               int isBeat,
               int* framebuffer,
               int* fbout,
               int w,
               int h) {
        switch (this->type()) {
            case NEW_EFFECT:
                if (!this->effect->enabled) {
                    return 0;
                }
                return this->effect->render(visdata, isBeat, framebuffer, fbout, w, h);
            default:
                return this->legacy_effect->render(
                    visdata, isBeat, framebuffer, fbout, w, h);
        }
    };
    char* get_desc() {
        switch (this->type()) {
            case NEW_EFFECT: return this->effect->get_desc();
            default: return this->legacy_effect->get_desc();
        }
    };
    void load_config(unsigned char* data, int len) {
        switch (this->type()) {
            case NEW_EFFECT: return this->effect->load_legacy(data, len);
            default: return this->legacy_effect->load_config(data, len);
        }
    };
    int save_config(unsigned char* data) {
        switch (this->type()) {
            case NEW_EFFECT: return this->effect->save_legacy(data);
            default: return this->legacy_effect->save_config(data);
        }
    };

    bool can_multithread() {
        switch (this->type()) {
            case NEW_EFFECT: return this->effect->can_multithread();
            default:
            case LEGACY_EFFECT: return false;
        }
    };
    int smp_begin(int max_threads,
                  char visdata[2][2][576],
                  int isBeat,
                  int* framebuffer,
                  int* fbout,
                  int w,
                  int h) {
        switch (this->type()) {
            case NEW_EFFECT:
                if (!this->effect->enabled) {
                    return 0;
                }
                return this->effect->smp_begin(
                    max_threads, visdata, isBeat, framebuffer, fbout, w, h);
            default:
            case LEGACY_EFFECT: return 0;
        }
    };
    void smp_render(int this_thread,
                    int max_threads,
                    char visdata[2][2][576],
                    int isBeat,
                    int* framebuffer,
                    int* fbout,
                    int w,
                    int h) {
        switch (this->type()) {
            case NEW_EFFECT:
                if (!this->effect->enabled) {
                    return;
                }
                return this->effect->smp_render(this_thread,
                                                max_threads,
                                                visdata,
                                                isBeat,
                                                framebuffer,
                                                fbout,
                                                w,
                                                h);
            default:
            case LEGACY_EFFECT: return;
        }
    };
    int smp_finish(char visdata[2][2][576],
                   int isBeat,
                   int* framebuffer,
                   int* fbout,
                   int w,
                   int h) {
        switch (this->type()) {
            case NEW_EFFECT:
                if (!this->effect->enabled) {
                    return 0;
                }
                return this->effect->smp_finish(
                    visdata, isBeat, framebuffer, fbout, w, h);
            default:
            case LEGACY_EFFECT: return 0;
        }
    };
};
