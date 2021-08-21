#include "c_triangle.h"
#include "r_defs.h"

#define MAX_CODE_LEN (1 << 16)  // 64k is the maximum component size in AVS
#define NUM_COLOR_VALUES 256    // 2 ^ BITS_PER_CHANNEL (i.e. 8)

C_Triangle::C_Triangle() : code() {}

C_Triangle::~C_Triangle() {}

int C_Triangle::render(char[2][2][576], int, int* framebuffer, int*, int w, int h) {
    return 0;
}

char* C_Triangle::get_desc(void) { return MOD_NAME; }

void C_Triangle::load_config(unsigned char* data, int len) {
    char* str_data = (char*)data;
    u_int pos = 0;
    this->code = Code();
    char* sections[] = {
        this->code.init,
        this->code.frame,
        this->code.beat,
        this->code.point,
    };
    for (u_int i = 0; i < sizeof(sections); i++) {
        u_int max_len = len - pos;
        u_int code_len = strnlen(&str_data[pos], max_len);
        delete[] sections[i];
        sections[i] = new char[code_len + 1];
        sections[i][code_len] = 0;
        pos += code_len + 1;
        if (code_len == max_len) {
            break;
        }
    }
}

int C_Triangle::save_config(unsigned char* data) {
    char* str_data = (char*)data;
    u_int pos = 0;
    char* sections[] = {
        this->code.init,
        this->code.frame,
        this->code.beat,
        this->code.point,
    };
    for (u_int i = 0; i < sizeof(sections); ++i) {
        u_int max_len = MAX_CODE_LEN - 1 - pos;
        u_int code_len = strnlen(sections[i], max_len);
        bool code_too_long = false;
        if (code_len == max_len) {
            strncpy(&str_data[pos], sections[i], code_len);
            data[pos + code_len] = '\0';
            code_too_long = true;
        } else {
            strncpy(&str_data[pos], sections[i], code_len + 1);
        }
        pos += code_len + 1;
        if (code_too_long) {
            break;
        }
    }
    return pos;
}

C_RBASE* R_Triangle(char* desc) {
    if (desc) {
        strcpy(desc, MOD_NAME);
        return NULL;
    }
    return (C_RBASE*)new C_Triangle();
}
