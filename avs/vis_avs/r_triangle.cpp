#include "c_triangle.h"

#include "r_defs.h"

#include <math.h>

#define MAX_CODE_LEN (1 << 16)  // 64k is the maximum component size in AVS
#define NUM_COLOR_VALUES 256    // 2 ^ BITS_PER_CHANNEL (i.e. 8)
#define IS_BEAT_MASK 0x01       // something else might be encoded in the higher bytes

#define TRIANGLE_NUM_POINTS 3
#define TRIANGLE_NUM_SHORT_EDGES 2
#define TRIANGLE_NUM_CODE_SECTIONS 4
#define TRIANGLE_MAX_Z 40000
#define TRIANGLE_Z_INT_RESOLUTION 100000

APEinfo* g_triangle_extinfo;

TriangleDepthBuffer* C_Triangle::depth_buffer = NULL;
u_int C_Triangle::instance_count = 0;

C_Triangle::C_Triangle() {
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

void C_Triangle::init_depthbuffer_if_needed(int w, int h) {
    if (this->need_depth_buffer) {
        // TODO [bug]: lock(triangle_depth_buffer)
        if (this->depth_buffer == NULL) {
            this->depth_buffer = new TriangleDepthBuffer(w, h);
        }
        // TODO [bug]: unlock(triangle_depth_buffer)
        this->need_depth_buffer = false;
    }
}

/* Convert world coordinates (-1 to +1) to screen/pixel coordinates (0 to w/h). Note
   that `x` and `width` are just the parameter names here, it's also used for y. */
inline int world_to_screen(double x, double width_half) {
    return (x + 1.0) * width_half;
};

/* Convert double z-buffer values to the depth buffer's format. Values in the depth
   buffer are stored as fixed-point values (i.e. integers with a mulitplier). */
inline uint64_t z_to_depthbuffer(double z1) {
    if (z1 > TRIANGLE_MAX_Z) {
        return (uint64_t)TRIANGLE_MAX_Z * TRIANGLE_Z_INT_RESOLUTION;
    } else if (z1 > 0) {
        return (uint64_t)z1 * TRIANGLE_Z_INT_RESOLUTION;
    }
    return 0;
}

inline unsigned char col_to_int(double col) {
    return max(0.0, min(255.0, col * 255.0f));
}

int C_Triangle::render(char visdata[2][2][576],
                       int is_beat,
                       int* framebuffer,
                       int*,
                       int w,
                       int h) {
    this->init_depthbuffer_if_needed(w, h);
    this->code.recompile_if_needed();
    if (this->code.need_init) {
        this->code.init.exec(visdata);
        this->code.init_variables(w, h);
        this->code.need_init = false;
    }
    this->code.frame.exec(visdata);
    if (is_beat & IS_BEAT_MASK) {
        this->code.beat.exec(visdata);
    }
    this->depth_buffer->reset_if_needed(w, h, *this->code.vars.zbclear != 0.0);
    int triangle_count = round(*this->code.vars.n);
    if (triangle_count > 0) {
        u_int blendmode = *g_triangle_extinfo->lineblendmode & 0xff;
        u_int adjustable_blend = *g_triangle_extinfo->lineblendmode >> 8 & 0xff;
        double step = 1.0;
        if (triangle_count > 1) {
            step = 1.0 / (triangle_count - 1.0);
        }
        double i = 0.0;
        double w_half = ((double)(w - 1)) / 2.0;
        double h_half = ((double)(h - 1)) / 2.0;
        *this->code.vars.i = i;
        for (int k = 0; k < triangle_count; ++k) {
            *this->code.vars.skip = 0.0;
            this->code.point.exec(visdata);
            if (*this->code.vars.skip != 0.0) {
                continue;
            }
            Vertex vertices[3] = {
                {world_to_screen(*this->code.vars.x1, w_half),
                 world_to_screen(*this->code.vars.y1, h_half),
                 z_to_depthbuffer(*this->code.vars.z1)},
                {world_to_screen(*this->code.vars.x2, w_half),
                 world_to_screen(*this->code.vars.y2, h_half),
                 z_to_depthbuffer(*this->code.vars.z1)},
                {world_to_screen(*this->code.vars.x3, w_half),
                 world_to_screen(*this->code.vars.y3, h_half),
                 z_to_depthbuffer(*this->code.vars.z1)},
            };

            unsigned int color;
            unsigned char* color_bytes = (unsigned char*)&color;
            color_bytes[0] = col_to_int(*this->code.vars.blue1);
            color_bytes[1] = col_to_int(*this->code.vars.green1);
            color_bytes[2] = col_to_int(*this->code.vars.red1);

            this->draw_triangle(framebuffer,
                                w,
                                h,
                                vertices,
                                *this->code.vars.zbuf != 0.0f,
                                blendmode,
                                adjustable_blend,
                                color);

            i += step;
            *this->code.vars.i = i;
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
 * First the 3 vertices are sorted according to their y-coordinates. The lines between
 * edges 1-3 and 1-2 (filled with +) will then get drawn first, and the lines between
 * edges 1-3 and 2-3 (filled with =) after that.
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
                               Vertex vertices[3],
                               bool use_depthbuffer,
                               u_int blendmode,
                               u_int adjustable_blend,
                               u_int color) {
    this->sort_vertices(vertices);
    Vertex v1 = vertices[0];
    Vertex v2 = vertices[1];
    Vertex v3 = vertices[2];
    int y = max(0, min(h, v1.y));
    int midy = max(0, min(h, v2.y));
    int endy = max(0, min(h, v3.y));
    int fb_index = w * y;

    for (; y <= midy; fb_index += w, y++) {
        if (y == v1.y) {
            continue;
        }
        int startx = ((v2.x - v1.x) * (y - v1.y)) / (v2.y - v1.y) + v1.x;
        int endx = ((v3.x - v1.x) * (y - v1.y)) / (v3.y - v1.y) + v1.x;
        this->draw_line(framebuffer,
                        fb_index,
                        startx,
                        endx,
                        v1.z,
                        w,
                        use_depthbuffer,
                        blendmode,
                        adjustable_blend,
                        color);
    }
    for (; y < endy; fb_index += w, y++) {
        int startx = ((v3.x - v2.x) * (y - v2.y)) / (v3.y - v2.y) + v2.x;
        int endx = ((v3.x - v1.x) * (y - v1.y)) / (v3.y - v1.y) + v1.x;
        this->draw_line(framebuffer,
                        fb_index,
                        startx,
                        endx,
                        v1.z,
                        w,
                        use_depthbuffer,
                        blendmode,
                        adjustable_blend,
                        color);
    }
}

inline void C_Triangle::draw_line(int* framebuffer,
                                  int fb_index,
                                  int startx,
                                  int endx,
                                  uint64_t z,
                                  int w,
                                  bool use_depthbuffer,
                                  u_int blendmode,
                                  u_int adjustable_blend,
                                  u_int color) {
    if (startx > endx) {
        int tmp = startx;
        startx = endx;
        endx = tmp;
    }
    if (endx < 0 || startx >= w) {
        return;
    }
    startx = max(startx, 0);
    endx = min(endx, w - 1);
    fb_index += startx;
    int fb_endx = fb_index + (endx - startx);
    for (; fb_index < fb_endx; fb_index++) {
        if (use_depthbuffer) {
            if (z >= this->depth_buffer->buffer[fb_index]) {
                this->depth_buffer->buffer[fb_index] = z;
                framebuffer[fb_index] = color;
            } else {
                continue;
            }
        } else {
            this->draw_pixel(
                &framebuffer[fb_index], blendmode, adjustable_blend, color);
        }
    }
}

inline void C_Triangle::draw_pixel(int* pixel,
                                   u_int blendmode,
                                   u_int adjustable_blend,
                                   u_int color) {
    switch (blendmode) {
        case OUT_REPLACE: {
            *pixel = color;
            break;
        }
        case OUT_ADDITIVE: {
            *pixel = BLEND(color, *pixel);
            break;
        }
        case OUT_MAXIMUM: {
            *pixel = BLEND_MAX(color, *pixel);
            break;
        }
        case OUT_5050: {
            *pixel = BLEND_AVG(color, *pixel);
            break;
        }
        case OUT_SUB1: {
            *pixel = BLEND_SUB(*pixel, color);
            break;
        }
        case OUT_SUB2: {
            *pixel = BLEND_SUB(color, *pixel);
            break;
        }
        case OUT_MULTIPLY: {
            *pixel = BLEND_MUL(color, *pixel);
            break;
        }
        case OUT_ADJUSTABLE: {
            *pixel = BLEND_ADJ(color, *pixel, adjustable_blend);
            break;
        }
        case OUT_XOR: {
            *pixel = color ^ *pixel;
            break;
        }
        case OUT_MINIMUM: {
            *pixel = BLEND_MIN(color, *pixel);
            break;
        }
    }
}

inline void C_Triangle::sort_vertices(Vertex v[3]) {
    Vertex tmp_vertex;
    char p12 = v[0].y <= v[1].y;
    char p13 = v[0].y <= v[2].y;
    char p23 = v[1].y <= v[2].y;
    char p = p12 + p13 * 2 + p23 * 4;
    switch (p) {
        case 0: {
            // 321
            tmp_vertex = v[2];
            v[2] = v[0];
            v[0] = tmp_vertex;
            break;
        }
        case 1: {
            // 312
            tmp_vertex = v[2];
            v[2] = v[1];
            v[1] = v[0];
            v[0] = tmp_vertex;
            break;
        }
        case 3: {
            // 132
            tmp_vertex = v[2];
            v[2] = v[1];
            v[1] = tmp_vertex;
            break;
        }
        case 4: {
            // 231
            tmp_vertex = v[2];
            v[2] = v[0];
            v[0] = v[1];
            v[1] = tmp_vertex;
            break;
        }
        case 6: {
            // 213
            tmp_vertex = v[1];
            v[1] = v[0];
            v[0] = tmp_vertex;
            break;
        }
        case 7:   // 123 -- NOOP
        default:  // p = 0b010 and p = 0b101 are impossible
            break;
    }
}

/* Depth buffer */
TriangleDepthBuffer::TriangleDepthBuffer(u_int w, u_int h) : w(w), h(h) {
    this->buffer = new u_int[w * h];
    memset(this->buffer, 0, w * h * sizeof(u_int));
}
TriangleDepthBuffer::~TriangleDepthBuffer() { delete[] this->buffer; }

void TriangleDepthBuffer::reset_if_needed(u_int w, u_int h, bool clear) {
    // TODO [bug]: lock(triangle_depth_buffer)
    if (clear || this->w != w || this->h != h) {
        // TODO [feature]: The existing depth-buffer could be resized here.
        free(this->buffer);
        this->w = w;
        this->h = h;
        /* Allocating with calloc() is noticeably faster than with new[]() */
        this->buffer = (u_int*)calloc(w * h, sizeof(u_int));
    }
    // TODO [bug]: unlock(triangle_depth_buffer)
}

/* Code */
TriangleCode::TriangleCode() : vm_context(NULL) {}

TriangleCode::~TriangleCode() {
    if (g_triangle_extinfo && this->vm_context) {
        g_triangle_extinfo->freeVM(this->vm_context);
        this->vm_context = NULL;
    }
}

void TriangleCode::register_variables() {
    if (!g_triangle_extinfo || !this->vm_context) {
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
}

void TriangleCode::recompile_if_needed() {
    if (this->vm_context == NULL) {
        this->vm_context = g_triangle_extinfo->allocVM();
        if (this->vm_context == NULL) {
            return;
        }
        this->register_variables();
        *this->vars.n = 0.0f;
    }
    if (this->init.recompile_if_needed(this->vm_context)) {
        this->need_init = true;
    }
    this->frame.recompile_if_needed(this->vm_context);
    this->beat.recompile_if_needed(this->vm_context);
    this->point.recompile_if_needed(this->vm_context);
}

void TriangleCode::init_variables(int w, int h) {
    *this->vars.w = w;
    *this->vars.h = h;
    *this->vars.x1 = 0.0f;
    *this->vars.y1 = 0.0f;
    *this->vars.x2 = 0.0f;
    *this->vars.y2 = 0.0f;
    *this->vars.x3 = 0.0f;
    *this->vars.y3 = 0.0f;
    *this->vars.red1 = 1.0f;
    *this->vars.green1 = 1.0f;
    *this->vars.blue1 = 1.0f;
    *this->vars.red2 = 1.0f;
    *this->vars.green2 = 1.0f;
    *this->vars.blue2 = 1.0f;
    *this->vars.red3 = 1.0f;
    *this->vars.green3 = 1.0f;
    *this->vars.blue3 = 1.0f;
}

/* Code Section */
TriangleCodeSection::TriangleCodeSection()
    : string(new char[1]), need_recompile(false), code(NULL) {
    this->string[0] = '\0';
}

TriangleCodeSection::~TriangleCodeSection() {
    delete[] this->string;
    if (g_triangle_extinfo) {
        g_triangle_extinfo->freeCode(this->code);
    }
    this->code = NULL;
}

/** `length` must include the zero byte! */
void TriangleCodeSection::set(char* string, u_int length) {
    delete[] this->string;
    this->string = new char[length];
    strncpy(this->string, string, length);
    this->string[length - 1] = '\0';
    this->need_recompile = true;
}

bool TriangleCodeSection::recompile_if_needed(VM_CONTEXT vm_context) {
    if (!g_triangle_extinfo || !this->need_recompile) {
        return false;
    }
    g_triangle_extinfo->freeCode(this->code);
    this->code = g_triangle_extinfo->compileVMcode(vm_context, this->string);
    this->need_recompile = false;
    return true;
}

void TriangleCodeSection::exec(char visdata[2][2][576]) {
    if (!this->code) {
        return;
    }
    g_triangle_extinfo->executeCode(code, visdata);
}

int TriangleCodeSection::load(char* src, u_int max_len) {
    u_int code_len = strnlen(src, max_len);
    this->set(src, code_len + 1);
    return code_len + 1;
}

int TriangleCodeSection::save(char* dest, u_int max_len) {
    u_int code_len = strnlen(this->string, max_len);
    bool code_too_long = code_len == max_len;
    if (code_too_long) {
        strncpy(dest, this->string, code_len);
        dest[code_len] = '\0';
    } else {
        strncpy(dest, this->string, code_len + 1);
    }
    return code_len + 1;
}

/* Config Loading & Effect Registration */
void C_Triangle::load_config(unsigned char* data, int len) {
    char* str = (char*)data;
    u_int pos = 0;
    pos += this->code.init.load(&str[pos], max(0, len - pos));
    pos += this->code.frame.load(&str[pos], max(0, len - pos));
    pos += this->code.beat.load(&str[pos], max(0, len - pos));
    pos += this->code.point.load(&str[pos], max(0, len - pos));
}

int C_Triangle::save_config(unsigned char* data) {
    char* str = (char*)data;
    u_int pos = 0;
    pos += this->code.init.save(&str[pos], max(0, MAX_CODE_LEN - 1 - pos));
    pos += this->code.frame.save(&str[pos], max(0, MAX_CODE_LEN - 1 - pos));
    pos += this->code.beat.save(&str[pos], max(0, MAX_CODE_LEN - 1 - pos));
    pos += this->code.point.save(&str[pos], max(0, MAX_CODE_LEN - 1 - pos));
    return pos;
}

char* C_Triangle::get_desc(void) { return MOD_NAME; }

C_RBASE* R_Triangle(char* desc) {
    if (desc) {
        strcpy(desc, MOD_NAME);
        return NULL;
    }
    return (C_RBASE*)new C_Triangle();
}

void R_Triangle_SetExtInfo(APEinfo* info) { g_triangle_extinfo = info; }
