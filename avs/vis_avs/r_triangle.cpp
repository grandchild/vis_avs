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

/**
 * Convert world coordinates (-1 to +1) to screen/pixel coordinates (0 to w/h).
 * Note that `x` and `width` are just the parameter names here, it's also used for y.
 */
inline int w2p(double x, double width_half) { return (int)((x + 1.0) * width_half); };

int C_Triangle::render(char visdata[2][2][576],
                       int is_beat,
                       int* framebuffer,
                       int*,
                       int w,
                       int h) {
    this->init_depthbuffer_if_needed(w, h);
    this->code.recompile_if_needed();
    *this->code.vars.w = w;
    *this->code.vars.h = h;
    *this->code.vars.red1 = 1.0f;
    *this->code.vars.green1 = 1.0f;
    *this->code.vars.blue1 = 1.0f;
    *this->code.vars.red2 = 1.0f;
    *this->code.vars.green2 = 1.0f;
    *this->code.vars.blue2 = 1.0f;
    *this->code.vars.red3 = 1.0f;
    *this->code.vars.green3 = 1.0f;
    *this->code.vars.blue3 = 1.0f;
    if (this->code.need_init) {
        this->code.run(this->code.init, visdata);
    }
    this->code.run(this->code.frame, visdata);
    if (is_beat & 0x01) {
        this->code.run(this->code.beat, visdata);
    }
    int triangle_count = *this->code.vars.n;
    this->depth_buffer->reset_if_needed(w, h, *this->code.vars.zbclear != 0.0);
    if (triangle_count) {
        double step = 1.0 / (*this->code.vars.n - 1);
        double i = 0.0;
        double w_half = ((double)w) / 2.0;
        double h_half = ((double)h) / 2.0;
        *this->code.vars.skip = 0.0;
        for (int k = 0; k < triangle_count; ++k, i += step) {
            *this->code.vars.i = i;
            this->code.run(this->code.point, visdata);
            if (*this->code.vars.skip != 0.0) {
                continue;
            }
            int points[3][2] = {
                {w2p(*this->code.vars.x1, w_half), w2p(*this->code.vars.y1, h_half)},
                {w2p(*this->code.vars.x2, w_half), w2p(*this->code.vars.y2, h_half)},
                {w2p(*this->code.vars.x3, w_half), w2p(*this->code.vars.y3, h_half)}};

            unsigned int color;
            unsigned char* color_bytes = (unsigned char*)&color;
            color_bytes[0] = *this->code.vars.blue1 * 255.0f;
            color_bytes[1] = *this->code.vars.green1 * 255.0f;
            color_bytes[2] = *this->code.vars.red1 * 255.0f;

            this->draw_triangle(framebuffer, w, h, points, *this->code.vars.z1, color);
        }
    }

    return 0;
}

/**
 * Drawing triangles works by successively drawing the horizontal lines between one
 * longer and two shorter edges, where length is _only_ measured vertically, i.e. the
 * absolute difference |y1-y2|.
 *
 * Consider the following triangle:
 *
 *    1—__
 *    |+++——__
 *    |+++++++2
 *    |=====/
 *    |===/
 *    |=/
 *    3
 *
 * First the edge 1-3 is identified as the longest edge in the y-direction (the fact
 * that it's also geometrically the longest edge in this triangle is purely
 * coincidental). The lines between edges 1-3 and 1-2 (filled with +) will then get
 * drawn first, and the lines between edges 1-3 and 2-3 (filled with =) after that.
 *
 * See https://joshbeam.com/articles/triangle_rasterization/ for a full description.
 *
 * In our case, an additional depth-buffer check is performed and pixels will only be
 * actually drawn to the framebuffer if the triangle has a lower z1 value than the
 * depth-buffer value at that pixel.
 */
void C_Triangle::draw_triangle(int* framebuffer,
                               int w,
                               int h,
                               int points[3][2],
                               double z1,
                               u_int color) {
    (void)*framebuffer;
    Edge edges[3] = {Edge(points[0], points[1]),
                     Edge(points[1], points[2]),
                     Edge(points[2], points[0])};
    // find longest y-edge
    int max_y_length = -1;
    u_int long_edge_index = 0;
    for (u_int i = 0; i < 3; i++) {
        if (edges[i].y_length > max_y_length) {
            max_y_length = edges[i].y_length;
            long_edge_index = i;
        }
    }
    Edge* long_edge = &edges[long_edge_index];
    u_int short_edge_indices[2] = {(long_edge_index + 1) % 3,
                                   (long_edge_index + 2) % 3};
    for (int e = 0; e < 2; e++) {
        Edge* short_edge = &edges[short_edge_indices[e]];
        for (int y = short_edge->y1; y <= short_edge->y2; y++) {
            Span span(short_edge->x1, long_edge->x1);
            for (int x = span.x1; x <= span.x2; x++) {
            }
        }
    }
}

void C_Triangle::init_depthbuffer_if_needed(int w, int h) {
    if (this->need_depth_buffer) {
        // TODO: lock(triangle_depth_buffer)
        if (this->depth_buffer == NULL) {
            this->depth_buffer = new TriangleDepthBuffer(w, h);
        }
        // TODO: unlock(triangle_depth_buffer)
        this->need_depth_buffer = false;
    }
}

void TriangleDepthBuffer::reset_if_needed(u_int w, u_int h, bool clear) {
    // TODO: lock(triangle_depth_buffer)
    if (this->w != w || this->h != h) {
        // TODO [feature]: The existing depth-buffer could be resized here.
        delete[] this->buffer;
        this->w = w;
        this->h = h;
        this->buffer = new double[w * h];
    } else if (clear) {
        memset(this->buffer, 0, w * h * sizeof(double));
    }
    // TODO: unlock(triangle_depth_buffer)
}

void TriangleCode::recompile_if_needed() {
    if (!this->need_recompile) {
        return;
    }

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

    this->need_init = true;
}

void TriangleCode::run(VM_CODEHANDLE code, char visdata[2][2][576]) {
    if (!code) {
        return;
    }
    g_triangle_extinfo->executeCode(code, visdata);
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
