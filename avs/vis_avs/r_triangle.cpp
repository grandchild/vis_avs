#include "c_triangle.h"
#include "r_defs.h"

#define MAX_CODE_LEN (1 << 16)  // 64k is the maximum component size in AVS
#define NUM_COLOR_VALUES 256    // 2 ^ BITS_PER_CHANNEL (i.e. 8)

APEinfo* g_triangle_extinfo;
TriangleDepthBuffer* C_Triangle::depth_buffer = NULL;
u_int C_Triangle::instance_count = 0;

C_Triangle::C_Triangle() : code() {
    this->instance_count += 1;
    if (this->depth_buffer == NULL) {
        this->need_depth_buffer = true;
    }
}
C_Triangle::~C_Triangle() {
    this->instance_count -= 1;
    if (this->instance_count == 0) {
        delete this->depth_buffer;
        this->depth_buffer = NULL;
    }
}

void TriangleCode::recompile() {
    g_triangle_extinfo->resetVM(this->vm_context);
    this->vars.w = g_triangle_extinfo->regVMvariable(this->vm_context, "w");
    this->vars.h = g_triangle_extinfo->regVMvariable(this->vm_context, "h");
    this->vars.n = g_triangle_extinfo->regVMvariable(this->vm_context, "n");
    this->vars.i = g_triangle_extinfo->regVMvariable(this->vm_context, "i");
    this->vars.skip = g_triangle_extinfo->regVMvariable(this->vm_context, "skip");
    this->vars.x1 = g_triangle_extinfo->regVMvariable(this->vm_context, "x1");
    this->vars.y1 = g_triangle_extinfo->regVMvariable(this->vm_context, "y1");
    this->vars.red1 = g_triangle_extinfo->regVMvariable(this->vm_context, "red1");
    this->vars.green1 = g_triangle_extinfo->regVMvariable(this->vm_context, "green1");
    this->vars.blue1 = g_triangle_extinfo->regVMvariable(this->vm_context, "blue1");
    this->vars.x2 = g_triangle_extinfo->regVMvariable(this->vm_context, "x2");
    this->vars.y2 = g_triangle_extinfo->regVMvariable(this->vm_context, "y2");
    this->vars.red2 = g_triangle_extinfo->regVMvariable(this->vm_context, "red2");
    this->vars.green2 = g_triangle_extinfo->regVMvariable(this->vm_context, "green2");
    this->vars.blue2 = g_triangle_extinfo->regVMvariable(this->vm_context, "blue2");
    this->vars.x3 = g_triangle_extinfo->regVMvariable(this->vm_context, "x3");
    this->vars.y3 = g_triangle_extinfo->regVMvariable(this->vm_context, "y3");
    this->vars.red3 = g_triangle_extinfo->regVMvariable(this->vm_context, "red3");
    this->vars.green3 = g_triangle_extinfo->regVMvariable(this->vm_context, "green3");
    this->vars.blue3 = g_triangle_extinfo->regVMvariable(this->vm_context, "blue3");
    this->vars.z1 = g_triangle_extinfo->regVMvariable(this->vm_context, "z1");
    this->vars.zbuf = g_triangle_extinfo->regVMvariable(this->vm_context, "zbuf");
    this->vars.zbclear = g_triangle_extinfo->regVMvariable(this->vm_context, "zbclear");

    g_triangle_extinfo->freeCode(this->init);
    g_triangle_extinfo->freeCode(this->frame);
    g_triangle_extinfo->freeCode(this->beat);
    g_triangle_extinfo->freeCode(this->point);
    this->init = g_triangle_extinfo->compileVMcode(this->vm_context, this->init_str);
    this->frame = g_triangle_extinfo->compileVMcode(this->vm_context, this->frame_str);
    this->beat = g_triangle_extinfo->compileVMcode(this->vm_context, this->beat_str);
    this->point = g_triangle_extinfo->compileVMcode(this->vm_context, this->point_str);
}

int C_Triangle::render(char[2][2][576], int, int* framebuffer, int*, int w, int h) {
    (void)framebuffer;
    if (this->need_depth_buffer) {
        if (this->depth_buffer == NULL) {
            this->depth_buffer = new TriangleDepthBuffer(w, h);
        }
        this->need_depth_buffer = false;
    }
    return 0;
}

char* C_Triangle::get_desc(void) { return MOD_NAME; }

void C_Triangle::load_config(unsigned char* data, int len) {
    char* str_data = (char*)data;
    u_int pos = 0;
    this->code = TriangleCode();
    char* sections[] = {
        this->code.init_str,
        this->code.frame_str,
        this->code.beat_str,
        this->code.point_str,
    };
    for (u_int i = 0; i < sizeof(sections); i++) {
        u_int max_len = max(0, len - pos);
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
        this->code.init_str,
        this->code.frame_str,
        this->code.beat_str,
        this->code.point_str,
    };
    for (u_int i = 0; i < sizeof(sections); ++i) {
        u_int max_len = MAX_CODE_LEN - 1 - pos;
        u_int code_len = strnlen(sections[i], max_len);
        bool code_too_long = code_len == max_len;
        if (code_too_long) {
            strncpy(&str_data[pos], sections[i], code_len);
            data[pos + code_len] = '\0';
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

void R_Triangle_SetExtInfo(APEinfo* info) { g_triangle_extinfo = info; }
