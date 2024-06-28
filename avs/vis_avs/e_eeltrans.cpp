#include "e_eeltrans.h"
#include "e_eeltrans_translate.h"

#include "r_defs.h"

#include "instance.h"

#include "../ns-eel/ns-eel.h"

#include <stdio.h>  // for logging
#include <vector>

#define PUT_INT(y)                   \
    data[pos] = (y) & 255;           \
    data[pos + 1] = (y >> 8) & 255;  \
    data[pos + 2] = (y >> 16) & 255; \
    data[pos + 3] = (y >> 24) & 255

#define GET_INT() \
    (data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24))

constexpr Parameter EelTrans_Info::parameters[];

// TODO [bug][feature]: NSEEL-per-AVS-instance. NSEEL is currently one global instance,
// and is not separated per AVS instance. Instance-global configuration of EELTrans
// works, but any interaction with NSEEL currently still only uses preprocessor code
// from EELTrans effects in the first AVS instance, and conversely applies to all code
// sections everywhere.
std::vector<void*> comp_ctxs;
char* newbuf;

void E_EelTrans::on_enable(bool enabled) {
    this->global->config.translate_enabled = enabled;
    for (auto it : this->global->instances) {
        it->enabled = enabled;
    }
}

E_EelTrans::E_EelTrans(AVS_Instance* avs) : Configurable_Effect(avs) {
    if (this->global->config.need_new_first_instance) {
        this->is_first_instance = true;
        this->global->config.need_new_first_instance = false;
        this->global->config.avs_base_path = this->avs->base_path;
        this->enabled = this->global->config.translate_enabled;

        std::string filename = this->avs->base_path + "\\eeltrans.ini";
        FILE* ini = fopen(filename.c_str(), "r");
        if (ini) {
            char* tmp = new char[2048];
            fscanf(ini, "%s", tmp);
            this->global->config.log_path = tmp;
            fclose(ini);
        }

        NSEEL_set_compile_hooks(this->pre_compile_hook, this->post_compile_hook);
    } else {
        this->is_first_instance = false;
    }
}

E_EelTrans::~E_EelTrans() {
    if (this->is_first_instance) {
        this->global->config.need_new_first_instance = true;
        std::string filename = this->avs->base_path + "\\eeltrans.ini";
        FILE* ini = fopen(filename.c_str(), "w");
        if (ini) {
            fprintf(ini, "%s", this->global->config.log_path.c_str());
            fclose(ini);
        }
    }
    if (this->global->instances.size() == 1) {
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
    // TODO [bug]: When does this list get cleared?
    comp_ctxs.push_back(ctx);
    return i;
}

char* E_EelTrans::pre_compile_hook(void* ctx, char* expression) {
    static void* lastctx = 0;
    static int num = 0;
    if (expression == NULL) {
        return NULL;
    }
    // TODO [bug][feature]: NSEEL-per-AVS-instance. See above for details.
    // This line simply selects the first available global config. Should be fixed to
    // select one based on the AVS-instance the NSEEL compiler instance is for.
    auto g = E_EelTrans::globals.begin()->lock();
    if (g->config.log_enabled) {
        if (ctx == lastctx) {
            num = (num % 4) + 1;
        } else {
            num = 1;
            lastctx = ctx;
        }
        char filename[200];
        sprintf(filename,
                "%s\\comp%02dpart%d.log",
                g->config.log_path.c_str(),
                compnum(ctx),
                num);
        FILE* logfile = fopen(filename, "w");
        if (logfile) {
            fprintf(logfile, "%s", expression);
            fclose(logfile);
        } else {
            if (!create_directory((char*)g->config.log_path.c_str())) {
                printf("Could not create log directory, deactivating Code Logger.\n");
                g->config.log_enabled = 0;
            }
        }
    }
    if (g->config.translate_enabled && (strncmp(expression, "//$notrans", 10) != 0)) {
        std::string all_code;
        for (auto it : g->instances) {
            all_code += it->config.code;
            all_code += "\r\n";
        }
        std::string tmp = translate(all_code,
                                    expression,
                                    g->config.translate_firstlevel,
                                    g->config.avs_base_path);
        newbuf = new char[tmp.size() + 1];
        strcpy(newbuf, tmp.c_str());
        return newbuf;
    }
    return expression;
}

void E_EelTrans::post_compile_hook() { delete[] newbuf; }

int E_EelTrans::render(char[2][2][576], int, int*, int*, int, int) {
    if (this->global->config.need_new_first_instance && !this->is_first_instance) {
        this->global->config.need_new_first_instance = false;
        this->is_first_instance = true;
    }
    return 0;
}

void E_EelTrans::load_legacy(unsigned char* data, int len) {
    int pos = 0;

    if (len - pos >= 4) {
        this->global->config.translate_enabled = (GET_INT() != 0);
        this->enabled = this->global->config.translate_enabled;
        pos += 4;
    }
    if (len - pos >= 4) {
        this->global->config.log_enabled = (GET_INT() != 0);
        pos += 4;
    }
    if (len - pos >= 4) {
        this->global->config.translate_firstlevel = (GET_INT() != 0);
        pos += 4;
    }
    if (len - pos >= 4) {
        this->global->config.read_comment_codes = (GET_INT() != 0);
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

    PUT_INT(this->global->config.log_enabled);
    pos += 4;

    PUT_INT(((int)this->global->config.translate_firstlevel));
    pos += 4;

    PUT_INT(this->global->config.read_comment_codes);
    pos += 4;

    char* str_data = (char*)data;
    pos += string_nt_save_legacy(
        this->config.code, &str_data[pos], MAX_CODE_LEN - 1 - pos);

    return pos;
}

Effect_Info* create_EelTrans_Info() { return new EelTrans_Info(); }
Effect* create_EelTrans(AVS_Instance* avs) { return new E_EelTrans(avs); }
void set_EelTrans_desc(char* desc) { E_EelTrans::set_desc(desc); }
