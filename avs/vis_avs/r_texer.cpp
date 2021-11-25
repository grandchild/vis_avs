#include "c_texer.h"

#include "r_defs.h"  // g_path

#include "files.h"
#include "image.h"
#include "pixel_format.h"

#include "../util.h"  // ssizeof32()

#include <emmintrin.h>  // SSE2 SIMD intrinsics

// IMAGE_DATA_ROW_EXPAND depends on the stepping of the SIMD-enabled render method.
// Currently 4 pixels are processed at once, so we are at most 3 pixels outside of the
// image. In the future this might be (8 - 1) if the method is upgraded to use 256-bit
// registers and instructions.
#define IMAGE_DATA_ROW_EXPAND (4 - 1)

int C_Texer::instance_count = 0;
std::vector<char*> C_Texer::file_list;

C_Texer::C_Texer()
    : image(""),
      input_mode(TEXER_INPUT_IGNORE),
      output_mode(TEXER_OUTPUT_NORMAL),
      num_particles(100),
      image_data(NULL),
      need_image_refresh(false) {
    this->instance_count++;
    this->find_image_files();
}

C_Texer::~C_Texer() {
    this->instance_count--;
    if (this->instance_count <= 0) {
        this->clear_image_files();
    }
}

static void add_file_callback(const char* file, void* data) {
    C_Texer* texer = (C_Texer*)data;
    size_t filename_length = strnlen(file, MAX_PATH);
    char* filename = (char*)calloc(filename_length + 1, sizeof(char));
    strncpy(filename, file, filename_length + 1);
    texer->file_list.push_back(filename);
}

void C_Texer::find_image_files() {
    this->clear_image_files();
    const int num_extensions = 5;
    char* extensions[num_extensions] = {".bmp", ".png", ".jpg", ".jpeg", ".gif"};
    find_files_by_extensions(g_path,
                             extensions,
                             num_extensions,
                             add_file_callback,
                             this,
                             sizeof(*this),
                             /*recursive*/ false,
                             /*background*/ false);
}

void C_Texer::clear_image_files() {
    for (auto file : this->file_list) {
        free(file);
    }
    this->file_list.clear();
}

bool C_Texer::load_image() {
    if (this->image[0] == 0) {
        return true;
    }
    char filename[MAX_PATH];
    int printed = snprintf(filename, MAX_PATH, "%s\\%s", g_path, this->image);
    if (printed >= MAX_PATH) {
        filename[MAX_PATH - 1] = '\0';
    }
    AVS_image* tmp_image = image_load(filename);
    if (tmp_image->data == NULL || tmp_image->w == 0 || tmp_image->h == 0
        || tmp_image->error != NULL) {
        image_free(tmp_image);
        return false;
    }

    this->image_data_width = tmp_image->w + IMAGE_DATA_ROW_EXPAND * 2;
    this->image_width = tmp_image->w;
    this->image_width_half = this->image_width / 2;
    this->image_width_other_half = this->image_width - this->image_width_half;
    this->image_height = tmp_image->h;
    this->image_height_half = this->image_height / 2;
    this->image_height_other_half = this->image_height - this->image_height_half;

    delete[] this->image_data;
    this->image_data = new pixel_rgb8[this->image_data_width * this->image_height];
    for (int y = 0; y < this->image_height; y++) {
        int x = 0;
        // Prepend each row by black pixels, so that we can render 4 pixels at once and
        // not worry about overshooting the right framebuffer edge.
        // This only works because Texer only does additive blend. With any other
        // blendmode, should it be supported in the future, this would no longer work.
        for (; x < IMAGE_DATA_ROW_EXPAND; x++) {
            this->image_data[x + y * this->image_data_width] = 0;
        }
        for (; x < this->image_width + IMAGE_DATA_ROW_EXPAND; x++) {
            this->image_data[x + y * this->image_data_width] =
                ((pixel_rgb8*)
                     tmp_image->data)[x - IMAGE_DATA_ROW_EXPAND + y * tmp_image->w];
        }
        // Expand each row by black pixels, so that we can render 4 pixels at once and
        // don't need to worry about overshooting the image width.
        for (; x < this->image_data_width; x++) {
            this->image_data[x + y * this->image_data_width] = 0;
        }
        // By always blindly adding 3 pixels we waste a bit of memory at the cost of
        // cleaner code.
    }
    image_free(tmp_image);

    this->need_image_refresh = false;
    return true;
}

int C_Texer::render(char[2][2][576], int, int* framebuffer, int* fbout, int w, int h) {
    if (this->need_image_refresh) {
        this->load_image();
    }
    if (this->image_data == NULL) {
        return 0;
    }
    if (this->input_mode == TEXER_INPUT_IGNORE) {
        memset(fbout, 0, w * h * sizeof(uint32_t));
    } else {
        memcpy(fbout, framebuffer, w * h * sizeof(uint32_t));
    }
    int p = 0;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if ((framebuffer[x + y * w] & AVS_PXL_COLOR_MASK_RGB8) != 0) {
                this->render_particle_sse2(framebuffer, fbout, x, y, w, h);
                p++;
                if (p >= this->num_particles) {
                    return 1;
                }
            }
        }
    }
    return 1;
}

inline void C_Texer::render_particle(int* framebuffer,
                                     int* fbout,
                                     int x,
                                     int y,
                                     int w,
                                     int h) {
    int fb_start_x = x - this->image_width_half;
    int fb_start_y = y - this->image_height_half;
    int fb_end_x = x + this->image_width_other_half;
    int fb_end_y = y + this->image_height_other_half;
    int image_start_x = -min(0, fb_start_x);
    int image_start_y = -min(0, fb_start_y);
    int image_end_x = this->image_width + min(0, w - fb_end_x);
    int image_end_y = this->image_height + min(0, h - fb_end_y);
    fb_start_x = max(0, fb_start_x);
    fb_start_y = max(0, fb_start_y);
    pixel_rgb8 mask = AVS_PXL_COLOR_MASK_RGB8;  // a sane, but unused, default value
    if (this->output_mode == TEXER_OUTPUT_MASKED) {
        mask = framebuffer[x + y * w];
    }
    for (int iy = image_start_y, fby = fb_start_y; iy < image_end_y; iy++, fby++) {
        for (int ix = image_start_x, fbx = fb_start_x; ix < image_end_x; ix++, fbx++) {
            pixel_rgb8 dest_pixel = this->image_data[ix + IMAGE_DATA_ROW_EXPAND
                                                     + iy * this->image_data_width];
            if (this->output_mode == TEXER_OUTPUT_MASKED) {
                dest_pixel = BLEND_MUL(dest_pixel, mask);
            }
            fbout[fbx + fby * w] = BLEND(fbout[fbx + fby * w], dest_pixel);
        }
    }
}

inline void C_Texer::render_particle_sse2(int* framebuffer,
                                          int* fbout,
                                          int x,
                                          int y,
                                          int w,
                                          int h) {
    int fb_start_x = x - this->image_width_half;
    int fb_start_y = y - this->image_height_half;
    int fb_end_x = x + this->image_width_other_half;
    int fb_end_y = y + this->image_height_other_half;
    int image_start_x = -min(0, fb_start_x) + IMAGE_DATA_ROW_EXPAND;
    int image_start_y = -min(0, fb_start_y);
    int image_end_x = this->image_width + min(0, w - fb_end_x) + IMAGE_DATA_ROW_EXPAND;
    int image_end_y = this->image_height + min(0, h - fb_end_y);
    fb_start_x = max(0, fb_start_x);
    fb_start_y = max(0, fb_start_y);
    // Don't overshoot the framebuffer row, if we start close to the right edge. This
    // can happen because we always process the texer 4 pixels at once, and might not
    // start on a multiple-by-four column index.
    //
    // To prevent that, round `fb_start_x` down to a multiple of four. The framebuffer's
    // width is required to always a multiple of four, so this guarantees that we stay
    // within bounds. Setting the least significant two bits to zero accomplishes this.
    //
    // Before that, to offset the shift in the framebuffer index, we also have to start
    // rendering the texer image 0 to 3 pixels before the actual image, depending on
    // how many pixels we rounded `fb_start_x` down by. For that, we added 3 pixels of
    // black on both sides of the image in `load_image()`. Subtracting the least
    // significant two bits of `fb_start_x` from `image_start_x` accomplishes this.
    image_start_x -= fb_start_x & 0b11;
    fb_start_x &= ~0b11;
    __m128i four_source_px;
    __m128i four_dest_px;
    __m128i two_16bit_px1;
    __m128i two_16bit_px2;
    __m128i zero = _mm_setzero_si128();
    __m128i mask = _mm_set1_epi32(-1);  // unused default value
    if (this->output_mode == TEXER_OUTPUT_MASKED) {
        mask = _mm_set1_epi32(framebuffer[x + y * w]);
        mask = _mm_unpacklo_epi8(mask, zero);
    }
    for (int iy = image_start_y, fby = fb_start_y; iy < image_end_y; iy++, fby++) {
        for (int ix = image_start_x, fbx = fb_start_x; ix < image_end_x;
             ix += 4, fbx += 4) {
            four_source_px = _mm_loadu_si128(
                (__m128i*)&this->image_data[ix + iy * this->image_data_width]);
            if (this->output_mode == TEXER_OUTPUT_MASKED) {
                two_16bit_px1 = _mm_unpacklo_epi8(four_source_px, zero);
                two_16bit_px2 = _mm_unpackhi_epi8(four_source_px, zero);
                two_16bit_px1 = _mm_mullo_epi16(two_16bit_px1, mask);
                two_16bit_px1 = _mm_srli_epi16(two_16bit_px1, 8);
                two_16bit_px2 = _mm_mullo_epi16(two_16bit_px2, mask);
                two_16bit_px2 = _mm_srli_epi16(two_16bit_px2, 8);
                four_source_px = _mm_packus_epi16(two_16bit_px1, two_16bit_px2);
            }
            four_dest_px = _mm_loadu_si128((__m128i*)&fbout[fbx + fby * w]);
            four_dest_px = _mm_adds_epu8(four_dest_px, four_source_px);
            _mm_store_si128((__m128i*)&fbout[fbx + fby * w], four_dest_px);
        }
    }
}

char* C_Texer::get_desc(void) { return MOD_NAME; }

/* The legacy save format has a few holes:
 *     16 bytes [unused]
 *    260 bytes image filename
 *      1 byte input/output modes
 *               - input mode at first nibble ("1" and "2" bit),
 *               - output mode at second nibble ("4" and "8" bit)
 *      3 bytes [padding]
 *      4 bytes [unused]
 */
void C_Texer::load_config(unsigned char* data, int len) {
    if (len >= (ssizeof32(uint32_t) * 4 + MAX_PATH)) {
        strncpy(this->image, (char*)&data[16], MAX_PATH);
        this->image[MAX_PATH - 1] = '\0';
        this->need_image_refresh = true;
    }
    if (len >= (ssizeof32(uint32_t) * 5 + MAX_PATH)) {
        int32_t input_output = *(uint32_t*)&data[16 + MAX_PATH];
        this->input_mode =
            (input_output & 0b11) == 2 ? TEXER_INPUT_REPLACE : TEXER_INPUT_IGNORE;
        this->output_mode =
            (input_output & 0b1100) == 8 ? TEXER_OUTPUT_MASKED : TEXER_OUTPUT_NORMAL;
    }
    if (len >= (ssizeof32(uint32_t) * 6 + MAX_PATH)) {
        this->num_particles = *(uint32_t*)&data[16 + MAX_PATH + 4];
    }
}

int C_Texer::save_config(unsigned char* data) {
    memset(data, 0, 16);  // unused
    strncpy((char*)&data[16], this->image, MAX_PATH);
    data[16 + MAX_PATH - 1] = '\0';
    *(uint32_t*)&data[16 + MAX_PATH] =
        (uint32_t)((this->input_mode == TEXER_INPUT_REPLACE ? 2 : 1)
                   | (this->output_mode == TEXER_OUTPUT_MASKED ? 8 : 4));

    *(uint32_t*)&data[16 + MAX_PATH + 4] = this->num_particles;
    memset(&data[16 + MAX_PATH + 4 + 4], 0, 4);  // unused
    return /*unused*/ 16 + /*image file*/ MAX_PATH + /*input/output*/ 4
           + /*num particles*/ 4 + /*unused*/ 4;
}

C_RBASE* R_Texer(char* desc) {
    if (desc) {
        strcpy(desc, MOD_NAME);
        return NULL;
    }
    return (C_RBASE*)new C_Texer();
}
