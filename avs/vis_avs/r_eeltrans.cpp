#include "c_eeltrans.h"

#include "r_defs.h"

#include "../ns-eel/ns-eel.h"

#include <windows.h>
#include <stdio.h>  // for logging
#include <vector>

APEinfo* g_eeltrans_extinfo = 0;

char* C_EelTrans::logpath = NULL;
bool C_EelTrans::log_enabled = false;
bool C_EelTrans::translate_enabled = false;
bool C_EelTrans::translate_firstlevel = false;
bool C_EelTrans::read_comment_codes = false;
bool C_EelTrans::need_new_first_instance = true;
int C_EelTrans::num_instances = 0;

std::set<C_EelTrans*> C_EelTrans::instances;

std::vector<void*> comp_ctxs;
char* newbuf;

unsigned int compnum(void* ctx) {
    unsigned int i = 0;
    for (; i < comp_ctxs.size(); i++) {
        if (comp_ctxs[i] == ctx) {
            return i;
        }
    }
    comp_ctxs.push_back(ctx);
    return i;
}

char* C_EelTrans::pre_compile_hook(void* ctx, char* expression) {
    static void* lastctx = 0;
    static int num = 0;
    if (expression) {
        if (C_EelTrans::log_enabled) {
            if (ctx == lastctx) {
                num = (num % 4) + 1;
            } else {
                num = 1;
                lastctx = ctx;
            }
            char filename[200];
            sprintf(filename, "%s\\comp%02dpart%d.log", logpath, compnum(ctx), num);
            FILE* logfile = fopen(filename, "w");
            if (logfile) {
                fprintf(logfile, "%s", expression);
                fclose(logfile);
            } else {
                if (!CreateDirectory(logpath, NULL)) {
                    MessageBox(
                        NULL,
                        "Could not create log directory, deactivating Code Logger.",
                        "Code Logger Error",
                        0);
                    C_EelTrans::log_enabled = 0;
                }
            }
        }
        if (C_EelTrans::translate_enabled
            && (strncmp(expression, "//$notrans", 10) != 0)) {
            std::string tmp = translate(
                C_EelTrans::all_code(), expression, C_EelTrans::translate_firstlevel);
            newbuf = new char[tmp.size() + 1];
            strcpy(newbuf, tmp.c_str());
            return newbuf;
        }
    }
    return expression;
}

void C_EelTrans::post_compile_hook() { delete[] newbuf; }

std::string C_EelTrans::all_code() {
    std::string all;
    for (auto it : C_EelTrans::instances) {
        all += it->code;
    }
    return all;
}

// set up default configuration
C_EelTrans::C_EelTrans() {
    if (this->need_new_first_instance) {
        this->need_new_first_instance = false;
        this->translate_enabled = 0;
        this->log_enabled = 0;

        this->translate_firstlevel = 0;
        this->read_comment_codes = 1;
        this->instances.insert(this);
        this->code = "";

        this->is_first_instance = true;

        std::string filename = std::string(g_path) + "\\eeltrans.ini";
        FILE* ini = fopen(filename.c_str(), "r");
        if (logpath) delete[] logpath;
        if (ini) {
            char* tmp = new char[2048];
            fscanf(ini, "%s", tmp);
            logpath = new char[strlen(tmp) + 1];
            strcpy(logpath, tmp);
            fclose(ini);
        } else {
            logpath = new char[10];
            strcpy(logpath, "C:\\avslog");
        }

        g_eeltrans_extinfo->set_compile_hooks(this->pre_compile_hook,
                                              this->post_compile_hook);
    } else {
        this->is_first_instance = false;
    }
    this->num_instances++;
}

// virtual destructor
C_EelTrans::~C_EelTrans() {
    this->num_instances--;
    if (this->is_first_instance) {
        this->need_new_first_instance = true;
        std::string filename = std::string(g_path) + "\\eeltrans.ini";
        FILE* ini = fopen(filename.c_str(), "w");
        if (ini) {
            fprintf(ini, "%s", logpath);
            fclose(ini);
        }
    }
    if (this->num_instances == 0) {
        g_eeltrans_extinfo->unset_compile_hooks();
    }
    this->instances.erase(this);
}

int C_EelTrans::render(char[2][2][576], int, int*, int*, int, int) {
    if (this->need_new_first_instance && !this->is_first_instance) {
        this->need_new_first_instance = false;
        this->is_first_instance = true;
    }
    return 0;
}

char* C_EelTrans::get_desc(void) { return MOD_NAME; }

#define PUT_INT(y)                   \
    data[pos] = (y)&255;             \
    data[pos + 1] = (y >> 8) & 255;  \
    data[pos + 2] = (y >> 16) & 255; \
    data[pos + 3] = (y >> 24) & 255
#define GET_INT() \
    (data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24))

void C_EelTrans::load_config(unsigned char* data, int len) {
    int pos = 0;

    // always ensure there is data to be loaded
    if (len - pos >= 4) {
        this->translate_enabled = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        this->log_enabled = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        this->translate_firstlevel = (GET_INT() != 0);
        pos += 4;
    }
    if (len - pos >= 4) {
        this->read_comment_codes = GET_INT();
        pos += 4;
    }
    this->instances.insert(this);
    if (len > pos) {
        this->code = (char*)(data + pos);
    } else {
        this->code = "";
    }
}

int C_EelTrans::save_config(unsigned char* data) {
    int pos = 0;

    PUT_INT(this->translate_enabled);
    pos += 4;

    PUT_INT(this->log_enabled);
    pos += 4;

    PUT_INT(((int)this->translate_firstlevel));
    pos += 4;

    PUT_INT(this->read_comment_codes);
    pos += 4;

    strcpy((char*)(data + pos), this->code.c_str());
    pos += this->code.length() + 1;

    return pos;
}

/* APE interface */
C_RBASE* R_EelTrans(char* desc) {
    if (desc) {
        strcpy(desc, MOD_NAME);
        return NULL;
    }
    return (C_RBASE*)new C_EelTrans();
}

void R_EelTrans_SetExtInfo(APEinfo* ptr) { g_eeltrans_extinfo = ptr; }
