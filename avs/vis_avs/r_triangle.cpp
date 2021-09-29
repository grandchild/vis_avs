#include "c_triangle.h"

#include "r_defs.h"

#define MAX_CODE_LEN (1 << 16)  // 64k is the maximum component size in AVS
#define NUM_COLOR_VALUES 256    // 2 ^ BITS_PER_CHANNEL (i.e. 8)
#define IS_BEAT_MASK 0x01

#define TRIANGLE_NUM_POINTS 3
#define TRIANGLE_NUM_SHORT_EDGES 2
#define TRIANGLE_NUM_CODE_SECTIONS 4

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
        // TODO: lock(triangle_depth_buffer)
        if (this->depth_buffer == NULL) {
            this->depth_buffer = new TriangleDepthBuffer(w, h);
        }
        // TODO: unlock(triangle_depth_buffer)
        this->need_depth_buffer = false;
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
                       int* fbout,
                       int w,
                       int h) {
    this->init_depthbuffer_if_needed(w, h);
    this->code.recompile_if_needed();
    this->code.init_variables(w, h);
    if (this->code.need_init) {
        this->code.init.run(visdata);
        this->code.need_init = false;
    }
    this->code.frame.run(visdata);
    if (is_beat & IS_BEAT_MASK) {
        this->code.beat.run(visdata);
    }
    int triangle_count = *this->code.vars.n;
    this->depth_buffer->reset_if_needed(w, h, *this->code.vars.zbclear != 0.0);
    u_int blendmode = *g_triangle_extinfo->lineblendmode & 0xff;
    u_int adjustable_blend = *g_triangle_extinfo->lineblendmode >> 8 & 0xff;
    if (triangle_count > 0) {
        double step = 0.0f;
        if (triangle_count > 1) {
            step = 1.0 / (*this->code.vars.n - 1);
        }
        double i = 0.0;
        double w_half = ((double)w) / 2.0;
        double h_half = ((double)h) / 2.0;
        *this->code.vars.skip = 0.0;
        for (int k = 0; k < triangle_count; ++k, i += step) {
            *this->code.vars.i = i;
            this->code.point.run(visdata);
            if (*this->code.vars.skip != 0.0) {
                continue;
            }
            Vertex vertices[3] = {
                {w2p(*this->code.vars.x1, w_half), w2p(*this->code.vars.y1, h_half)},
                {w2p(*this->code.vars.x2, w_half), w2p(*this->code.vars.y2, h_half)},
                {w2p(*this->code.vars.x3, w_half), w2p(*this->code.vars.y3, h_half)},
            };
            for (int p = 0; p < 3; p++) {
                framebuffer[vertices[p].x + vertices[p].y * w] = 0xff;
            }

            unsigned int color;
            unsigned char* color_bytes = (unsigned char*)&color;
            color_bytes[0] = *this->code.vars.blue1 * 255.0f;
            color_bytes[1] = *this->code.vars.green1 * 255.0f;
            color_bytes[2] = *this->code.vars.red1 * 255.0f;

            this->draw_triangle2(framebuffer,
                                 fbout,
                                 w,
                                 h,
                                 vertices,
                                 *this->code.vars.zbuf != 0.0f,
                                 *this->code.vars.z1,
                                 blendmode,
                                 adjustable_blend,
                                 color);
        }
    }

    return blendmode == OUT_REPLACE ? 0 : 1;
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
                               int* fbout,
                               int w,
                               int h,
                               int points[3][2],
                               bool use_depthbuffer,
                               double z1,
                               u_int blendmode,
                               u_int adjustable_blend,
                               u_int color) {
    Edge edges[3] = {
        Edge(points[0], points[1]),
        Edge(points[1], points[2]),
        Edge(points[2], points[0]),
    };
    // find longest y-edge
    int max_y_length = -1;
    u_int long_edge_index = 0;
    for (u_int i = 0; i < TRIANGLE_NUM_POINTS; i++) {
        if (edges[i].y_length > max_y_length) {
            max_y_length = edges[i].y_length;
            long_edge_index = i;
        }
    }
    Edge* long_edge = &edges[long_edge_index];
    int long_edge_x_length = long_edge->x2 - long_edge->x1;
    int long_edge_y_length = long_edge->y2 - long_edge->y1;
    if (long_edge_y_length == 0) {
        return;
    }
    // Make a y-sorted list of the two short edges, so that we can go straight down from
    // top to bottom (i.e. from low y-values to high y-values), across the two short
    // edges.
    Edge* short_edges[2] = {
        &edges[(long_edge_index + 1) % 3],
        &edges[(long_edge_index + 2) % 3],
    };
    if (short_edges[0]->y1 > short_edges[1]->y1) {
        Edge* tmp = short_edges[0];
        short_edges[0] = short_edges[1];
        short_edges[1] = tmp;
    }
    // Render all spans between the long edge and the two short edges in succession.
    // int span_direction = long_edge->x2 < short_edges[0]->x2 ? 1 : -1;
    int y = max(long_edge->y1, 0);
    for (u_int e = 0; e < TRIANGLE_NUM_SHORT_EDGES; e++) {
        Edge* short_edge = short_edges[e];
        int short_edge_y_length = short_edge->y2 - short_edge->y1;
        if (short_edge_y_length == 0) {
            continue;
        }
        int short_edge_x_length = short_edge->x2 - short_edge->x1;
        for (; y <= short_edge->y2 && y < h; y++) {
            int span_x1 =
                (long_edge_x_length * (y - long_edge->y1)) / long_edge_y_length
                + long_edge->x1;
            int span_x2 =
                (short_edge_x_length * (y - short_edge->y1)) / short_edge_y_length
                + short_edge->x1;
            if (span_x1 > span_x2) {
                int tmp = span_x1;
                span_x1 = span_x2;
                span_x2 = tmp;
            }
            if (span_x1 > w || span_x2 < 0) {
                continue;
            }
            int x = span_x1;
            while (x < 0 && x < span_x2) {
                x++;
            }
            for (; x <= span_x2 && x >= 0 && x < w; x++) {
                int pixel_index = x + y * w;
                if (use_depthbuffer) {
                    uint64_t z1int = 0;
                    if (z1 > 40000) {
                        z1int = (uint64_t)40000 * 100000;
                    } else if (z1 > 0) {
                        z1int = (uint64_t)z1 * 100000.0;
                    }
                    if (z1int < this->depth_buffer->buffer[pixel_index]) {
                        continue;
                    } else {
                        this->depth_buffer->buffer[pixel_index] = z1int;
                    }
                }
                this->draw_pixel(framebuffer,
                                 fbout,
                                 pixel_index,
                                 blendmode,
                                 adjustable_blend,
                                 color);
            }
        }
    }
}

inline void C_Triangle::draw_pixel(int* source_fb,
                                   int* dest_fb,
                                   int pixel_index,
                                   u_int blendmode,
                                   u_int adjustable_blend,
                                   u_int color) {
    switch (blendmode) {
        case OUT_REPLACE: {
            source_fb[pixel_index] = color;
            break;
        }
        case OUT_ADDITIVE: {
            dest_fb[pixel_index] = BLEND(color, source_fb[pixel_index]);
            break;
        }
        case OUT_MAXIMUM: {
            dest_fb[pixel_index] = BLEND_MAX(color, source_fb[pixel_index]);
            break;
        }
        case OUT_5050: {
            dest_fb[pixel_index] = BLEND_AVG(color, source_fb[pixel_index]);
            break;
        }
        case OUT_SUB1: {
            dest_fb[pixel_index] = BLEND_SUB(source_fb[pixel_index], color);
            break;
        }
        case OUT_SUB2: {
            dest_fb[pixel_index] = BLEND_SUB(color, source_fb[pixel_index]);
            break;
        }
        case OUT_MULTIPLY: {
            dest_fb[pixel_index] = BLEND_MUL(color, source_fb[pixel_index]);
            break;
        }
        case OUT_ADJUSTABLE: {
            dest_fb[pixel_index] =
                BLEND_ADJ(color, source_fb[pixel_index], adjustable_blend);
            break;
        }
        case OUT_XOR: {
            dest_fb[pixel_index] = color ^ source_fb[pixel_index];
            break;
        }
        case OUT_MINIMUM: {
            dest_fb[pixel_index] = BLEND_MIN(color, source_fb[pixel_index]);
            break;
        }
    }
}

void C_Triangle::draw_triangle2(int* framebuffer,
                                int* fbout,
                                int w,
                                int h,
                                Vertex vertices[3],
                                bool use_depthbuffer,
                                double z1,
                                u_int blendmode,
                                u_int adjustable_blend,
                                u_int color) {
    // Sort vertices by y value
    Vertex tmp_vertex;
    char p12 = vertices[0].y <= vertices[1].y;
    char p13 = vertices[0].y <= vertices[2].y;
    char p23 = vertices[1].y <= vertices[2].y;
    char p = p12 + p13 * 2 + p23 * 4;
    switch (p) {
        case 0: {
            // 321
            tmp_vertex = vertices[2];
            vertices[2] = vertices[0];
            vertices[0] = tmp_vertex;
            break;
        }
        case 1: {
            // 312
            tmp_vertex = vertices[2];
            vertices[2] = vertices[1];
            vertices[1] = vertices[0];
            vertices[0] = tmp_vertex;
            break;
        }
        case 3: {
            // 132
            tmp_vertex = vertices[2];
            vertices[2] = vertices[1];
            vertices[1] = tmp_vertex;
            break;
        }
        case 4: {
            // 231
            tmp_vertex = vertices[2];
            vertices[2] = vertices[0];
            vertices[0] = vertices[1];
            vertices[1] = tmp_vertex;
            break;
        }
        case 6: {
            // 213
            tmp_vertex = vertices[1];
            vertices[1] = vertices[0];
            vertices[0] = tmp_vertex;
            break;
        }
        case 7:   // 123 -- NOOP
        default:  // p = 0b010 and p = 0b101 are impossible
            break;
    }
    int y = max(0, min(h, vertices[0].y));
    int midy = max(0, min(h, vertices[1].y));
    int endy = max(0, min(h, vertices[2].y));
    int fb_index = w * y;
    Vertex v1 = vertices[0];
    Vertex v2 = vertices[1];
    Vertex v3 = vertices[2];
    for (; y < midy && y < h; fb_index += w, y++) {
        if (y == v1.y || y == v2.y || y == v3.y) {
            continue;
        }
        int startx = ((v2.x - v1.x) * (y - v1.y)) / (v2.y - v1.y) + v1.x;
        int endx = ((v3.x - v1.x) * (y - v1.y)) / (v3.y - v1.y) + v1.x;
        if (startx > endx) {
            int tmp = startx;
            startx = endx;
            endx = tmp;
        }
        if (endx < 0 || startx >= w) {
            continue;
        }
        startx = max(startx, 0);
        endx = min(endx, w - 1);
        fb_index += startx;
        int fb_endx = fb_index + (endx - startx);
        for (; fb_index <= fb_endx; fb_index++) {
            if (use_depthbuffer) {
                uint64_t z1int = 0;
                if (z1 > 40000) {
                    z1int = (uint64_t)40000 * 100000;
                } else if (z1 > 0) {
                    z1int = (uint64_t)z1 * 100000.0;
                }
                if (z1int < this->depth_buffer->buffer[fb_index]) {
                    continue;
                } else {
                    this->depth_buffer->buffer[fb_index] = z1int;
                }
            }
            this->draw_pixel(
                framebuffer, fbout, fb_index, blendmode, adjustable_blend, color);
        }
        fb_index -= endx + 1;
    }
    for (; y < endy && y < h; fb_index += w, y++) {
        int startx = ((v3.x - v2.x) * (y - v2.y)) / (v3.y - v2.y) + v2.x;
        int endx = ((v3.x - v1.x) * (y - v1.y)) / (v3.y - v1.y) + v1.x;
        if (startx > endx) {
            int tmp = startx;
            startx = endx;
            endx = tmp;
        }
        if (endx < 0 || startx >= w) {
            continue;
        }
        startx = max(startx, 0);
        endx = min(endx, w - 1);
        fb_index += startx;
        int fb_endx = fb_index + (endx - startx);
        for (; fb_index <= fb_endx; fb_index++) {
            if (use_depthbuffer) {
                uint64_t z1int = 0;
                if (z1 > 40000) {
                    z1int = (uint64_t)40000 * 100000;
                } else if (z1 > 0) {
                    z1int = (uint64_t)z1 * 100000.0;
                }
                if (z1int < this->depth_buffer->buffer[fb_index]) {
                    continue;
                } else {
                    this->depth_buffer->buffer[fb_index] = z1int;
                }
            }
            this->draw_pixel(
                framebuffer, fbout, fb_index, blendmode, adjustable_blend, color);
        }
        fb_index -= endx + 1;
    }
}

void TriangleDepthBuffer::reset_if_needed(u_int w, u_int h, bool clear) {
    // TODO: lock(triangle_depth_buffer)
    if (clear || this->w != w || this->h != h) {
        // TODO [feature]: The existing depth-buffer could be resized here.
        free(this->buffer);
        this->w = w;
        this->h = h;
        this->buffer = (u_int*)calloc(w * h, sizeof(u_int));
    }
    // TODO: unlock(triangle_depth_buffer)
}

// Code
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

/**/
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

// Code Section

TriangleCodeSection::TriangleCodeSection() : code(NULL), need_recompile(false) {
    this->string = new char[1];
    this->string[0] = '\0';
}

TriangleCodeSection::~TriangleCodeSection() {
    delete[] this->string;
    g_triangle_extinfo->freeCode(this->code);
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

void TriangleCodeSection::run(char visdata[2][2][576]) {
    if (!this->code) {
        return;
    }
    g_triangle_extinfo->executeCode(code, visdata);
}

// Config Loading & Effect Registration

char* C_Triangle::get_desc(void) { return MOD_NAME; }

void C_Triangle::load_config(unsigned char* data, int len) {
    char* str_data = (char*)data;
    u_int pos = 0;
    this->code = TriangleCode();
    TriangleCodeSection* sections[TRIANGLE_NUM_CODE_SECTIONS] = {
        &this->code.init,
        &this->code.frame,
        &this->code.beat,
        &this->code.point,
    };
    for (u_int i = 0; i < TRIANGLE_NUM_CODE_SECTIONS; i++) {
        u_int max_len = max(0, len - pos);
        u_int code_len = strnlen(&str_data[pos], max_len);
        sections[i]->set(&str_data[pos], code_len + 1);
        sections[i]->need_recompile = true;
        pos += code_len + 1;
        if (code_len == max_len) {
            break;
        }
    }
}

int C_Triangle::save_config(unsigned char* data) {
    char* str_data = (char*)data;
    u_int pos = 0;
    char* section_strings[TRIANGLE_NUM_CODE_SECTIONS] = {
        this->code.init.string,
        this->code.frame.string,
        this->code.beat.string,
        this->code.point.string,
    };
    for (u_int i = 0; i < TRIANGLE_NUM_CODE_SECTIONS; ++i) {
        u_int max_len = MAX_CODE_LEN - 1 - pos;
        u_int code_len = strnlen(section_strings[i], max_len);
        bool code_too_long = code_len == max_len;
        if (code_too_long) {
            strncpy(&str_data[pos], section_strings[i], code_len);
            data[pos + code_len] = '\0';
        } else {
            strncpy(&str_data[pos], section_strings[i], code_len + 1);
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
