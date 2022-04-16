#include "e_eeltrans.h"
#include "e_eeltrans_translate.h"

#include "r_defs.h"

#include "../ns-eel/ns-eel.h"

#include <stdio.h>  // for logging
#include <vector>

#define PUT_INT(y)                   \
    data[pos] = (y)&255;             \
    data[pos + 1] = (y >> 8) & 255;  \
    data[pos + 2] = (y >> 16) & 255; \
    data[pos + 3] = (y >> 24) & 255

#define GET_INT() \
    (data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24))

constexpr Parameter EelTrans_Info::parameters[];

bool E_EelTrans::translate_enabled = false;
std::string E_EelTrans::log_path = "";
bool E_EelTrans::log_enabled = false;
bool E_EelTrans::translate_firstlevel = false;
bool E_EelTrans::read_comment_codes = false;
bool E_EelTrans::need_new_first_instance = true;

std::set<E_EelTrans*> E_EelTrans::instances;

std::vector<void*> comp_ctxs;
char* newbuf;

void EelTrans_Info::apply_global(Effect* component,
                                 const Parameter*,
                                 std::vector<int64_t>) {
    auto eeltrans = (E_EelTrans*)component;
    E_EelTrans::log_enabled = eeltrans->config.log_enabled;
    E_EelTrans::log_path = eeltrans->config.log_path;
    E_EelTrans::translate_firstlevel = eeltrans->config.translate_firstlevel;
    E_EelTrans::read_comment_codes = eeltrans->config.read_comment_codes;
    for (auto it : E_EelTrans::instances) {
        it->config.log_enabled = E_EelTrans::log_enabled;
        it->config.log_path = E_EelTrans::log_path;
        it->config.translate_firstlevel = E_EelTrans::translate_firstlevel;
        it->config.read_comment_codes = E_EelTrans::read_comment_codes;
    }
}

void E_EelTrans::on_enable(bool enabled) {
    E_EelTrans::translate_enabled = enabled;
    for (auto it : E_EelTrans::instances) {
        it->enabled = enabled;
    }
}

E_EelTrans::E_EelTrans() {
    if (this->need_new_first_instance) {
        this->need_new_first_instance = false;
        this->enabled = false;
        this->translate_enabled = 0;
        this->log_enabled = 0;

        this->translate_firstlevel = 0;
        this->read_comment_codes = 1;

        this->is_first_instance = true;

        std::string filename = std::string(g_path) + "\\eeltrans.ini";
        FILE* ini = fopen(filename.c_str(), "r");
        if (ini) {
            char* tmp = new char[2048];
            fscanf(ini, "%s", tmp);
            log_path = tmp;
            fclose(ini);
        } else {
            log_path = "C:\\avslog";
        }

        NSEEL_set_compile_hooks(this->pre_compile_hook, this->post_compile_hook);
    } else {
        this->is_first_instance = false;
    }
    this->instances.insert(this);
}

E_EelTrans::~E_EelTrans() {
    if (this->is_first_instance) {
        this->need_new_first_instance = true;
        std::string filename = std::string(g_path) + "\\eeltrans.ini";
        FILE* ini = fopen(filename.c_str(), "w");
        if (ini) {
            fprintf(ini, "%s", log_path.c_str());
            fclose(ini);
        }
    }
    this->instances.erase(this);
    if (this->instances.empty()) {
        NSEEL_unset_compile_hooks();
    }
}

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

char* E_EelTrans::pre_compile_hook(void* ctx, char* expression) {
    static void* lastctx = 0;
    static int num = 0;
    if (expression) {
        if (E_EelTrans::log_enabled) {
            if (ctx == lastctx) {
                num = (num % 4) + 1;
            } else {
                num = 1;
                lastctx = ctx;
            }
            char filename[200];
            sprintf(filename,
                    "%s\\comp%02dpart%d.log",
                    log_path.c_str(),
                    compnum(ctx),
                    num);
            FILE* logfile = fopen(filename, "w");
            if (logfile) {
                fprintf(logfile, "%s", expression);
                fclose(logfile);
            } else {
                if (!create_directory((char*)log_path.c_str())) {
                    printf(
                        "Could not create log directory, deactivating Code Logger.\n");
                    E_EelTrans::log_enabled = 0;
                }
            }
        }
        if (E_EelTrans::translate_enabled
            && (strncmp(expression, "//$notrans", 10) != 0)) {
            std::string tmp = translate(
                E_EelTrans::all_code(), expression, E_EelTrans::translate_firstlevel);
            newbuf = new char[tmp.size() + 1];
            strcpy(newbuf, tmp.c_str());
            return newbuf;
        }
    }
    return expression;
}

void E_EelTrans::post_compile_hook() { delete[] newbuf; }

std::string E_EelTrans::all_code() {
    std::string all;
    for (auto it : E_EelTrans::instances) {
        all += it->config.code;
        all += "\r\n";
    }
    return all;
}

int E_EelTrans::render(char[2][2][576], int, int*, int*, int, int) {
    if (this->need_new_first_instance && !this->is_first_instance) {
        this->need_new_first_instance = false;
        this->is_first_instance = true;
    }
    return 0;
}

void E_EelTrans::load_legacy(unsigned char* data, int len) {
    int pos = 0;

    if (len - pos >= 4) {
        this->translate_enabled = (GET_INT() != 0);
        this->enabled = this->translate_enabled;
        pos += 4;
    }
    if (len - pos >= 4) {
        this->log_enabled = (GET_INT() != 0);
        this->config.log_enabled = this->log_enabled;
        pos += 4;
    }
    if (len - pos >= 4) {
        this->translate_firstlevel = (GET_INT() != 0);
        this->config.translate_firstlevel = this->translate_firstlevel;
        pos += 4;
    }
    if (len - pos >= 4) {
        this->read_comment_codes = (GET_INT() != 0);
        this->config.read_comment_codes = this->read_comment_codes;
        pos += 4;
    }
    if (len > pos) {
        char* str_data = (char*)data;
        this->string_nt_load_legacy(&str_data[pos], this->config.code, len - pos);
    } else {
        this->config.code = "";
    }
}

int E_EelTrans::save_legacy(unsigned char* data) {
    int pos = 0;

    PUT_INT(this->enabled);
    pos += 4;

    PUT_INT(this->log_enabled);
    pos += 4;

    PUT_INT(((int)this->translate_firstlevel));
    pos += 4;

    PUT_INT(this->read_comment_codes);
    pos += 4;

    char* str_data = (char*)data;
    pos += string_nt_save_legacy(
        this->config.code, &str_data[pos], MAX_CODE_LEN - 1 - pos);

    return pos;
}

Effect_Info* create_EelTrans_Info() { return new EelTrans_Info(); }
Effect* create_EelTrans() { return new E_EelTrans(); }
