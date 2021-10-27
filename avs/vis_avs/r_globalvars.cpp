#include "c_globalvars.h"

#include "avs_eelif.h"
#include "r_defs.h"

#include <algorithm>

#define MAX_COMPONENT_SAVE_LEN (1 << 16)  // 64k is the maximum component size in AVS
#define IS_BEAT_MASK 0x01  // something else might be encoded in the higher bytes

APEinfo* g_globalvars_extinfo = NULL;

const int C_GlobalVars::max_regs_index = 99;
const int C_GlobalVars::max_gmb_index = MEGABUF_BLOCKS * MEGABUF_ITEMSPERBLOCK;

C_GlobalVars::C_GlobalVars()
    : load_option(GLOBALVARS_LOAD_NONE),
      is_first_frame(true),
      load_code_next_frame(false),
      filepath(""),
      reg_ranges_str(""),
      buf_ranges_str("") {
    for (int i = 0; i < 10; i++) {
        this->filepath += ('a' + rand() % ('z' - 'a' + 1));
    }
    this->filepath += ".gvm";
}

C_GlobalVars::~C_GlobalVars() {
    for (auto range : this->reg_ranges) {
        delete[] range;
    }
    for (auto range : this->buf_ranges) {
        delete[] range;
    }
}

int C_GlobalVars::render(char visdata[2][2][576],
                         int is_beat,
                         int*,
                         int*,
                         int w,
                         int h) {
    if (this->load_code_next_frame) {
        this->exec_code_from_file(visdata);
        this->load_code_next_frame = false;
    }
    this->code.recompile_if_needed();
    if (this->code.need_init) {
        this->code.init.exec(visdata);
        this->code.init_variables(w, h, is_beat & IS_BEAT_MASK, 5);
        this->code.need_init = false;
    }
    this->code.frame.exec(visdata);
    if (is_beat & IS_BEAT_MASK) {
        this->code.beat.exec(visdata);
    }
    if (this->load_option == GLOBALVARS_LOAD_FRAME
        || (this->load_option == GLOBALVARS_LOAD_CODE && *this->code.vars.load != 0.0)
        || this->is_first_frame) {
        this->load_code_next_frame = true;
    }
    if (*this->code.vars.save != 0.0) {
        this->save_ranges_to_file();
    }
    this->is_first_frame = false;
    return 0;
}

void GlobalVarsVars::register_variables(void* vm_context) {
    this->w = NSEEL_VM_regvar(vm_context, "w");
    this->h = NSEEL_VM_regvar(vm_context, "h");
    this->b = NSEEL_VM_regvar(vm_context, "b");
    this->load = NSEEL_VM_regvar(vm_context, "load");
    this->save = NSEEL_VM_regvar(vm_context, "save");
}

void GlobalVarsVars::init_variables(int w, int h, int is_beat, ...) {
    *this->w = w;
    *this->h = h;
    *this->b = is_beat ? 1.0f : 0.0f;
    *this->load = 0.0f;
    *this->save = 0.0f;
}

void C_GlobalVars::exec_code_from_file(char visdata[2][2][576]) {
    FILE* file;
    size_t file_size;
    char* file_code;
    void* file_code_handle;

    file = fopen(this->filepath.c_str(), "r");
    if (!file) {
        return;
    }
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    rewind(file);
    if (file_size > 0) {
        file_code = (char*)calloc((file_size + 1), sizeof(char));
        fread(file_code, sizeof(char), file_size, file);
    } else {
        return;
    }
    file_code_handle = NSEEL_code_compile(this->code.vm_context, file_code);
    if (file_code_handle) {
        AVS_EEL_IF_Execute(file_code_handle, visdata);
    }
    NSEEL_code_free(file_code_handle);
    free(file_code);
}

bool compare_ranges(const void* a, const void* b) {
    return ((int*)a)[0] < ((int*)b)[0];
}

void C_GlobalVars::save_ranges_to_file() {
    int i;
    FILE* file = fopen(this->filepath.c_str(), "w");
    if (!file) {
        return;
    }
    this->check_set_range(this->reg_ranges_str, this->reg_ranges, this->max_regs_index);
    std::sort(this->reg_ranges.begin(), this->reg_ranges.end(), compare_ranges);
    this->check_set_range(this->buf_ranges_str, this->buf_ranges, this->max_gmb_index);
    std::sort(this->buf_ranges.begin(), this->buf_ranges.end(), compare_ranges);
    i = 0;
    for (auto range : this->reg_ranges) {
        if (i < range[0]) {
            i = range[0];
        }
        while (i <= range[1]) {
            fprintf(file, "reg%02d=%0.6f;\n", i, this->reg_value(i));
            i++;
        }
    }
    i = 0;
    for (auto range : this->buf_ranges) {
        if (i < range[0]) {
            i = range[0];
        }
        while (i <= range[1]) {
            fprintf(file, "assign(gmegabuf(%d),%0.6f);\n", i, this->buf_value(i));
            i++;
        }
    }
    fclose(file);
}

double C_GlobalVars::reg_value(int index) {
    if (index >= 0 && index < 100) {
        return NSEEL_getglobalregs()[index];
    }
    return 0.0f;
}

double C_GlobalVars::buf_value(int index) {
    if (index >= 0 && index < MEGABUF_BLOCKS * MEGABUF_ITEMSPERBLOCK) {
        return AVS_EEL_IF_gmb_value(index);
    }
    return 0.0f;
}

/* Check entered ranges for reg and gmegabuf values.

   Rough state machine:

   START ──"1...9"─▶ first number ─┬─"-"─▶ hyphen ──"1..9"─▶ second number ─┐
     ▲                             │                                        │
     └───────── comma ◀───","──────┘◀───────────────────────────────────────┘

   Additionally, spaces are ignored, and any other symbol than the ones indicated result
   in an abort and discards the current range. Previous valid ranges are kept. */
enum RangeCheckStates {
    RCS_START = 0,
    RCS_FIRST_NUM,
    RCS_HYPHEN,
    RCS_SECOND_NUM,
    RCS_COMMA,
};

bool C_GlobalVars::check_set_range(std::string input,
                                   std::vector<int*>& ranges,
                                   int max_index) {
    RangeCheckStates state = RCS_START;
    int prev_number = -1;
    int number;
    std::string number_str = "";
    for (auto range : ranges) {
        delete[] range;
    }
    ranges.clear();
    for (auto c : input) {
        if (c >= '0' && c <= '9') {
            if (state == RCS_START) {
                state = RCS_FIRST_NUM;
            } else if (state == RCS_HYPHEN) {
                state = RCS_SECOND_NUM;
            }  // else `state` stays the same
            number_str += c;
        } else if (c == '-' && state == RCS_FIRST_NUM) {
            number = atoi(number_str.c_str());
            if (number > max_index) {
                return false;
            }
            prev_number = number;
            number_str.clear();
            state = RCS_HYPHEN;
        } else if (c == ',') {
            number = atoi(number_str.c_str());
            if (number > max_index) {
                return false;
            }
            if (state == RCS_FIRST_NUM) {
                prev_number = number;
            } else if (state == RCS_SECOND_NUM) {
                if (prev_number > number) {
                    return false;
                }
            } else {
                return false;
            }
            number_str.clear();
            state = RCS_COMMA;
        } else if (c == ' ') {
            continue;
        } else {
            return false;
        }
        if (state == RCS_COMMA) {
            ranges.push_back(new int[2]{prev_number, number});
            state = RCS_START;
        }
    }
    if (!number_str.empty()) {
        number = atoi(number_str.c_str());
        if (number > max_index) {
            return false;
        }
        if (state == RCS_FIRST_NUM) {
            prev_number = number;
        } else if (state == RCS_SECOND_NUM) {
            if (prev_number > number) {
                return false;
            }
        }
        if (prev_number < 0) {  // this should never occur, but let's be safe.
            prev_number = number;
        }
        ranges.push_back(new int[2]{prev_number, number});
    }
    return true;
}

void C_GlobalVars::load_config(unsigned char* data, int len) {
    char* str = (char*)data;
    int pos = 0;
    this->load_option = (GlobalVarsLoadOpts)(*(int32_t*)data);
    pos += 4;
    // 24 bytes of unused (or unknown?) data
    pos += 24;
    pos += this->code.init.load(&str[pos], max(0, len - pos));
    pos += this->code.frame.load(&str[pos], max(0, len - pos));
    pos += this->code.beat.load(&str[pos], max(0, len - pos));
    if (pos >= len) {
        return;
    }
    this->filepath = &str[pos];
    pos += strnlen(&str[pos], max(0, len - pos)) + 1;
    if (pos >= len) {
        return;
    }
    this->reg_ranges_str = &str[pos];
    this->check_set_range(this->reg_ranges_str, this->reg_ranges, 99);
    pos += strnlen(&str[pos], max(0, len - pos)) + 1;
    if (pos >= len) {
        return;
    }
    this->buf_ranges_str = &str[pos];
    this->check_set_range(
        this->buf_ranges_str, this->buf_ranges, MEGABUF_BLOCKS * MEGABUF_ITEMSPERBLOCK);
}

int C_GlobalVars::save_config(unsigned char* data) {
    char* str = (char*)data;
    int pos = 0;
    *(int32_t*)data = (int)this->load_option;
    pos += 4;
    // 24 bytes of unused (or unknown?) data
    pos += 24;
    pos += this->code.init.save(&str[pos], max(0, MAX_COMPONENT_SAVE_LEN - 1 - pos));
    pos += this->code.frame.save(&str[pos], max(0, MAX_COMPONENT_SAVE_LEN - 1 - pos));
    pos += this->code.beat.save(&str[pos], max(0, MAX_COMPONENT_SAVE_LEN - 1 - pos));
    memcpy(&str[pos], this->filepath.c_str(), this->filepath.length() + 1);
    pos += this->filepath.length() + 1;
    memcpy(&str[pos], this->reg_ranges_str.c_str(), this->reg_ranges_str.length() + 1);
    pos += this->reg_ranges_str.length() + 1;
    memcpy(&str[pos], this->buf_ranges_str.c_str(), this->buf_ranges_str.length() + 1);
    pos += this->buf_ranges_str.length() + 1;
    return pos;
}

char* C_GlobalVars::get_desc(void) { return MOD_NAME; }

C_RBASE* R_GlobalVars(char* desc) {
    if (desc) {
        strcpy(desc, MOD_NAME);
        return NULL;
    }
    return (C_RBASE*)new C_GlobalVars();
}

void R_GlobalVars_SetExtInfo(APEinfo* info) { g_globalvars_extinfo = info; }
