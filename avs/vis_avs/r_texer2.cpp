#include "c_texer2.h"
#include "r_texer2.h"

#include "r_defs.h"

#include "avs_eelif.h"
#include "files.h"
#include "image.h"
#include "pixel_format.h"

#include "../util.h"

#include <math.h>
#include <stdio.h>

APEinfo* g_extinfo = 0;

int C_Texer2::instance_count = 0;
std::vector<char*> C_Texer2::file_list;

C_Texer2::C_Texer2(int default_version)
    : config({default_version,
              "",
              this->examples[0].resize,
              this->examples[0].wrap,
              this->examples[0].mask,
              0}),
      iw(0),
      ih(0),
      image_normal(0),
      image_flipped(0),
      image_mirrored(0),
      image_rot180(0),
      image_lock(lock_init()),
      code_lock(lock_init()) {
    this->code.init.set(this->examples[0].init,
                        strnlen(this->examples[0].init, 65334) + 1);
    this->code.frame.set(this->examples[0].frame,
                         strnlen(this->examples[0].frame, 65334) + 1);
    this->code.beat.set(this->examples[0].beat,
                        strnlen(this->examples[0].beat, 65334) + 1);
    this->code.point.set(this->examples[0].point,
                         strnlen(this->examples[0].point, 65334) + 1);
    this->find_image_files();
}

C_Texer2::~C_Texer2() {
    lock(this->image_lock);
    if (this->image_normal) {
        this->delete_image();
    }
    lock_unlock(this->image_lock);
    lock_destroy(this->image_lock);
    lock_destroy(this->code_lock);
}

static void add_file_callback(const char* file, void* data) {
    C_Texer2* texer2 = (C_Texer2*)data;
    size_t filename_length = strnlen(file, MAX_PATH);
    char* filename = (char*)calloc(filename_length + 1, sizeof(char));
    strncpy(filename, file, filename_length + 1);
    texer2->file_list.push_back(filename);
}

void C_Texer2::find_image_files() {
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

void C_Texer2::clear_image_files() {
    for (auto file : this->file_list) {
        free(file);
    }
    this->file_list.clear();
}

void C_Texer2::delete_image() {
    if (this->image_normal) {
        iw = 0;
        ih = 0;
        delete this->image_normal;
        delete this->image_flipped;
        delete this->image_mirrored;
        delete this->image_rot180;
    }
}

extern unsigned char rawData[1323];  // example pic
void C_Texer2::load_image() {
    lock(this->image_lock);
    if (this->image_normal) this->delete_image();
    if (!strlen(this->config.image)) {
        this->load_default_image();
        lock_unlock(this->image_lock);
        return;
    }
    char filename[MAX_PATH];
    wsprintf(filename, "%s\\%s", g_path, this->config.image);
    AVS_image* tmp_image = image_load(filename);
    if (tmp_image->data == NULL || tmp_image->w == 0 || tmp_image->h == 0
        || tmp_image->error != NULL) {
        image_free(tmp_image);
        this->load_default_image();
        lock_unlock(this->image_lock);
        return;
    }
    this->iw = tmp_image->w;
    this->ih = tmp_image->h;
    int image_size = this->ih * this->iw;
    this->image_normal = new pixel_rgb8[image_size];
    this->image_flipped = new pixel_rgb8[image_size];
    this->image_mirrored = new pixel_rgb8[image_size];
    this->image_rot180 = new pixel_rgb8[image_size];
    memcpy(this->image_normal, tmp_image->data, image_size * sizeof(pixel_rgb8));
    int y = 0;
    while (y < ih) {
        int x = 0;
        while (x < iw) {
            pixel_rgb8 value = this->image_normal[y * iw + x];
            this->image_flipped[(ih - y - 1) * iw + x] = value;
            this->image_mirrored[(y + 1) * iw - x - 1] = value;
            this->image_rot180[(ih - y) * iw - x - 1] = value;
            x++;
        }
        y++;
    }
    lock_unlock(this->image_lock);
}

void C_Texer2::load_default_image() {
    this->iw = 21;
    this->ih = 21;
    this->image_normal = new pixel_rgb8[this->iw * this->ih];
    this->image_flipped = new pixel_rgb8[this->iw * this->ih];
    this->image_mirrored = new pixel_rgb8[this->iw * this->ih];
    this->image_rot180 = new pixel_rgb8[this->iw * this->ih];
    for (int i = 0; i < this->iw * this->ih; ++i) {
        // the default image is symmetrical in all directions
        image_normal[i] = *(int*)&rawData[i * 3];
        image_flipped[i] = *(int*)&rawData[i * 3];
        image_mirrored[i] = *(int*)&rawData[i * 3];
        image_rot180[i] = *(int*)&rawData[i * 3];
    }
}

struct RECTf {
    double left;
    double top;
    double right;
    double bottom;
};

void C_Texer2::DrawParticle(int* framebuffer,
                            pixel_rgb8* texture,
                            int w,
                            int h,
                            double x,
                            double y,
                            double sizex,
                            double sizey,
                            unsigned int color) {
    // Adjust width/height
    --w;
    --h;
    --(this->iw);  // member vars, restore later!
    --(this->ih);

    // Texture Coordinates
    double x0 = 0.0;
    double y0 = 0.0;

    /***************************************************************************/
    /***************************************************************************/
    /*   Scaling renderer                                                      */
    /***************************************************************************/
    /***************************************************************************/
    if (this->config.resize) {
        RECTf r;
        // Determine area rectangle,
        // correct with half pixel for correct pixel coverage
        r.left = -iw * .5 * sizex + 0.5 + (x * .5 + .5) * w;
        r.top = -ih * .5 * sizey + 0.5 + (y * .5 + .5) * h;
        r.right = (iw - 1) * .5 * sizex + 0.5 + (x * .5 + .5) * w;
        r.bottom = (ih - 1) * .5 * sizey + 0.5 + (y * .5 + .5) * h;

        RECT r2;
        r2.left = RoundToInt(r.left);
        r2.top = RoundToInt(r.top);
        r2.right = RoundToInt(r.right);
        r2.bottom = RoundToInt(r.bottom);

        // Visiblity culling
        if ((r2.right < 0.0f) || (r2.left > w) || (r2.bottom < 0.0f) || (r2.top > h)) {
            goto skippart;
        }

        // Subpixel adjustment for first texel
        x0 = (0.5 - Fractional(r.left + 0.5)) / (r.right - r.left);
        y0 = (0.5 - Fractional(r.top + 0.5)) / (r.bottom - r.top);

        // Window edge clipping
        if (r.left < 0.0f) {
            x0 = -r.left / (r.right - r.left);
            r2.left = 0;
        }
        if (r.top < 0.0f) {
            y0 = -r.top / (r.bottom - r.top);
            r2.top = 0;
        }
        if (r.right > w) {
            r2.right = w;
        }
        if (r.bottom > h) {
            r2.bottom = h;
        }

        {
            double fx0 = x0 * iw;
            double fy0 = y0 * ih;

            int cx0 = RoundToInt(fx0);
            int cy0 = RoundToInt(fy0);

            // fixed point fractional part of first coordinate
            int dx = 65535 - FloorToInt((.5f - (fx0 - cx0)) * 65536.0);
            int dy = 65535 - FloorToInt((.5f - (fy0 - cy0)) * 65536.0);

            // 'texel per pixel' steps
            double scx = (iw - 1) / (r.right - r.left + 1);
            double scy = (ih - 1) / (r.bottom - r.top + 1);
            int sdx = FloorToInt(scx * 65536.0);
            int sdy = FloorToInt(scy * 65536.0);

            // fixed point corrected coordinate
            cx0 = (cx0 << 16) + dx;
            cy0 = (cy0 << 16) + dy;

            if (cx0 < 0) {
                cx0 += sdx;
                ++r2.left;
            }
            if (cy0 < 0) {
                cy0 += sdy;
                ++r2.top;
            }

            // Cull subpixel sized particles
            if ((r2.right <= r2.left) || (r2.bottom <= r2.top)) {
                goto skippart;
            }

            // Prepare filter color
            T2_PREP_MASK_COLOR

            switch (*(g_extinfo->lineblendmode) & 0xFF) {
                case OUT_REPLACE: {
                    __int64 mmxxor = 0x00FF00FF00FF00FF;
                    int tot = r2.right - r2.left;
                    int* outp = &framebuffer[r2.top * (w + 1) + r2.left];
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        T2_SCALE_BLEND_ASM_ENTER(t2_scale_loop_rep)
#ifdef _MSC_VER
                        __asm {
                            psrlw mm0, 8
                            packuswb mm0, mm0
                        }
#else  // GCC
                        "psrlw     %%mm0, 8\n\t"
                        "packuswb  %%mm0, %%mm0\n\t"
#endif
                        T2_SCALE_BLEND_ASM_LEAVE(t2_scale_loop_rep)
                        cy0 += sdy;
                        outp += w + 1;
                    }
                    break;
                }

                case OUT_ADDITIVE: {
                    __int64 mmxxor = 0x00FF00FF00FF00FF;
                    int tot = r2.right - r2.left;
                    int* outp = &framebuffer[r2.top * (w + 1) + r2.left];
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        T2_SCALE_BLEND_ASM_ENTER(t2_scale_loop_add)
#ifdef _MSC_VER
                        __asm {
                            psrlw mm0, 8
                            packuswb mm0, mm0

                                // save
                            paddusb mm0, qword ptr [edi]
                        }
#else  // GCC
                        "psrlw      %%mm0, 8\n\t"
                        "packuswb   %%mm0, %%mm0\n\t"
                        "paddusb    %%mm0, qword ptr [%%edi]\n\t"
#endif
                        T2_SCALE_BLEND_ASM_LEAVE(t2_scale_loop_add)
                        cy0 += sdy;
                        outp += w + 1;
                    }
                    break;
                }

                case OUT_MAXIMUM: {
                    int signmask = 0x808080;
                    T2_SCALE_MINMAX_SIGNMASK
                    __int64 mmxxor = 0x00FF00FF00FF00FF;
                    int tot = r2.right - r2.left;
                    int* outp = &framebuffer[r2.top * (w + 1) + r2.left];
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        T2_SCALE_BLEND_ASM_ENTER(t2_scale_loop_max)
#ifdef _MSC_VER
                        __asm {
                            // movd mm6, signmask;
                            psrlw mm0, 8
                            packuswb mm0, mm0

                                // save
                            movd mm1, dword ptr [edi]
                            pxor mm0, mm6
                            mov eax, 0xFFFFFF
                            pxor mm1, mm6
                            movd mm5, eax
                            movq mm2, mm1
                            pcmpgtb mm2, mm0
                            movq mm3, mm2
                            pxor mm3, mm5
                            pxor mm5, mm5
                            pand mm0, mm3
                            pand mm1, mm2
                            por mm0, mm1
                            pxor mm0, mm6
                        }
#else  // GCC
                        /*"movd      %%mm6, %[signmask]\n\t"*/
                        "psrlw     %%mm0, 8\n\t"
                        "packuswb  %%mm0, %%mm0\n\t"

                        // save
                        "movd      %%mm1, dword ptr [%%edi]\n\t"
                        "pxor      %%mm0, %%mm6\n\t"
                        "mov       %%eax, 0xFFFFFF\n\t"
                        "pxor      %%mm1, %%mm6\n\t"
                        "movd      %%mm5, %%eax\n\t"
                        "movq      %%mm2, %%mm1\n\t"
                        "pcmpgtb   %%mm2, %%mm0\n\t"
                        "movq      %%mm3, %%mm2\n\t"
                        "pxor      %%mm3, %%mm5\n\t"
                        "pxor      %%mm5, %%mm5\n\t"
                        "pand      %%mm0, %%mm3\n\t"
                        "pand      %%mm1, %%mm2\n\t"
                        "por       %%mm0, %%mm1\n\t"
                        "pxor      %%mm0, %%mm6\n\t"
#endif
                        T2_SCALE_BLEND_ASM_LEAVE(t2_scale_loop_max)
                        cy0 += sdy;
                        outp += w + 1;
                    }
                    break;
                }

                case OUT_5050: {
                    __int64 mmxxor = 0x00FF00FF00FF00FF;
                    int tot = r2.right - r2.left;
                    int* outp = &framebuffer[r2.top * (w + 1) + r2.left];
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        T2_SCALE_BLEND_ASM_ENTER(t2_scale_loop_50)
#ifdef _MSC_VER
                        __asm {

                            // save
                            movd mm1, dword ptr [edi]
                            psrlw mm0, 8
                            punpcklbw mm1, mm5
                            paddusw mm0, mm1
                            psrlw mm0, 1
                            packuswb mm0, mm0
                        }
#else  // GCC
                        "movd       %%mm1, dword ptr [%%edi]\n\t"
                        "psrlw      %%mm0, 8\n\t"
                        "punpcklbw  %%mm1, %%mm5\n\t"
                        "paddusw    %%mm0, %%mm1\n\t"
                        "psrlw      %%mm0, 1\n\t"
                        "packuswb   %%mm0, %%mm0\n\t"
#endif
                        T2_SCALE_BLEND_ASM_LEAVE(t2_scale_loop_50)
                        cy0 += sdy;
                        outp += w + 1;
                    }
                    break;
                }

                case OUT_SUB1: {
                    __int64 mmxxor = 0x00FF00FF00FF00FF;
                    int tot = r2.right - r2.left;
                    int* outp = &framebuffer[r2.top * (w + 1) + r2.left];
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        T2_SCALE_BLEND_ASM_ENTER(t2_scale_loop_sub1)
#ifdef _MSC_VER
                        __asm {
                            psrlw mm0, 8
                            packuswb mm0, mm0

                                // save
                            movd mm1, dword ptr [edi]
                            psubusb mm1, mm0
                            movq mm0, mm1
                        }
#else  // GCC
                        "psrlw mm0, 8\n\t"
                        "packuswb mm0, mm0\n\t"

                        // save
                        "movd mm1, dword ptr [edi]\n\t"
                        "psubusb mm1, mm0\n\t"
                        "movq mm0, mm1\n\t"
#endif
                        T2_SCALE_BLEND_ASM_LEAVE(t2_scale_loop_sub1)
                        cy0 += sdy;
                        outp += w + 1;
                    }
                    break;
                }

                case OUT_SUB2: {
                    __int64 mmxxor = 0x00FF00FF00FF00FF;
                    int tot = r2.right - r2.left;
                    int* outp = &framebuffer[r2.top * (w + 1) + r2.left];
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        T2_SCALE_BLEND_ASM_ENTER(t2_scale_loop_sub2)
#ifdef _MSC_VER
                        __asm {
                            psrlw mm0, 8
                            packuswb mm0, mm0

                                // save
                            psubusb mm0, qword ptr [edi]
                        }
#else  // GCC
                        "psrlw mm0, 8\n\t"
                        "packuswb mm0, mm0\n\t"

                        // save
                        "psubusb mm0, qword ptr [edi]\n\t"
#endif
                        T2_SCALE_BLEND_ASM_LEAVE(t2_scale_loop_sub2)
                        cy0 += sdy;
                        outp += w + 1;
                    }
                    break;
                }

                case OUT_MULTIPLY: {
                    __int64 mmxxor = 0x00FF00FF00FF00FF;
                    int tot = r2.right - r2.left;
                    int* outp = &framebuffer[r2.top * (w + 1) + r2.left];
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        T2_SCALE_BLEND_ASM_ENTER(t2_scale_loop_mul)
#ifdef _MSC_VER
                        __asm {
                            psrlw mm0, 8

                            // save
                            movd mm1, dword ptr [edi]
                            punpcklbw mm1, mm5
                            pmullw mm1, mm0
                            psrlw mm1, 8
                            packuswb mm1, mm1
                        }
#else  // GCC
                        "psrlw mm0, 8\n\t"

                        // save
                        "movd mm1, dword ptr [edi]\n\t"
                        "punpcklbw mm1, mm5\n\t"
                        "pmullw mm1, mm0\n\t"
                        "psrlw mm1, 8\n\t"
                        "packuswb mm1, mm1\n\t"
#endif
                        T2_SCALE_BLEND_ASM_LEAVE(t2_scale_loop_mul)
                        cy0 += sdy;
                        outp += w + 1;
                    }
                    break;
                }

                case OUT_ADJUSTABLE: {
                    __int64 alphavalue = 0x0;
                    __int64* alpha = &alphavalue;
                    // TODO [bugfix]: shouldn't salpha be 255, not 256?
                    // it's used to calculate 256 - alpha, and if max_alpha = 255,
                    // then...
                    __int64 salpha = 0x0100010001000100;
                    int t = ((*g_extinfo->lineblendmode) & 0xFF00) >> 8;
                    T2_SCALE_BLEND_AND_STORE_ALPHA
                    __int64 mmxxor = 0x00FF00FF00FF00FF;
                    int tot = r2.right - r2.left;
                    int* outp = &framebuffer[r2.top * (w + 1) + r2.left];
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        T2_SCALE_BLEND_ASM_ENTER(t2_scale_loop_adj)
#ifdef _MSC_VER
                        __asm {
                            psrlw mm0, 8

                            // Merged filter/alpha
                            // save
                            movd mm1, dword ptr [edi]
                            punpcklbw mm1, mm5
                            pmullw mm1, mm6
                            psrlw mm1, 8
                            pmullw mm0, alpha
                            psrlw mm0, 8
                            paddusw mm0, mm1
                            packuswb mm0, mm0
                        }
#else  // GCC
                        "psrlw      %%mm0, 8\n\t"

                        // Merged filter/alpha
                        // save
                        "movd       %%mm1, dword ptr [edi]\n\t"
                        "punpcklbw  %%mm1, %%mm5\n\t"
                        "pmullw     %%mm1, %%mm6\n\t"
                        "psrlw      %%mm1, 8\n\t"
                        "pmullw     %%mm0, qword ptr %[alpha]\n\t"
                        "psrlw      %%mm0, 8\n\t"
                        "paddusw    %%mm0, %%mm1\n\t"
                        "packuswb   %%mm0, %%mm0\n\t"
#endif
                        T2_SCALE_BLEND_ASM_LEAVE(t2_scale_loop_adj, ASM_M_ARG(alpha))
                        cy0 += sdy;
                        outp += w + 1;
                    }
                    break;
                }

                case OUT_XOR: {
                    __int64 mmxxor = 0x00FF00FF00FF00FF;
                    int tot = r2.right - r2.left;
                    int* outp = &framebuffer[r2.top * (w + 1) + r2.left];
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        T2_SCALE_BLEND_ASM_ENTER(t2_scale_loop_xor)
#ifdef _MSC_VER
                        __asm {
                            psrlw mm0, 8
                            packuswb mm0, mm0
                            pxor mm0, qword ptr [edi]
                        }
#else  // GCC
                        "psrlw  %%mm0, 8\n\t"
                        "packuswb %%mm0, %%mm0\n\t"
                        "pxor   %%mm0, qword ptr [%%edi]\n\t"
#endif
                        T2_SCALE_BLEND_ASM_LEAVE(t2_scale_loop_xor)
                        cy0 += sdy;
                        outp += w + 1;
                    }
                    break;
                }

                case OUT_MINIMUM: {
                    int signmask = 0x808080;
                    __int64 mmxxor = 0x00FF00FF00FF00FF;
                    int tot = r2.right - r2.left;
                    int* outp = &framebuffer[r2.top * (w + 1) + r2.left];
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        T2_SCALE_BLEND_ASM_ENTER(t2_scale_loop_min)
#ifdef _MSC_VER
                        __asm {
                            movd mm6, signmask
                            psrlw mm0, 8
                            packuswb mm0, mm0

                                // save
                            movd mm1, dword ptr [edi]
                            pxor mm0, mm6
                            mov eax, 0xFFFFFF
                            pxor mm1, mm6
                            movd mm5, eax
                            movq mm2, mm1
                            pcmpgtb mm2, mm0
                            movq mm3, mm2
                            pxor mm3, mm5
                            pxor mm5, mm5
                            pand mm0, mm2
                            pand mm1, mm3
                            por mm0, mm1
                            pxor mm0, mm6
                        }
#else  // GCC
                        "movd      %%mm6, %[signmask]\n\t"
                        "psrlw     %%mm0, 8\n\t"
                        "packuswb  %%mm0, %%mm0\n\t"

                        // save
                        "movd      %%mm1, dword ptr [%%edi]\n\t"
                        "pxor      %%mm0, %%mm6\n\t"
                        "mov       %%eax, 0xFFFFFF\n\t"
                        "pxor      %%mm1, %%mm6\n\t"
                        "movd      %%mm5, %%eax\n\t"
                        "movq      %%mm2, %%mm1\n\t"
                        "pcmpgtb   %%mm2, %%mm0\n\t"
                        "movq      %%mm3, %%mm2\n\t"
                        "pxor      %%mm3, %%mm5\n\t"
                        "pxor      %%mm5, %%mm5\n\t"
                        "pand      %%mm0, %%mm2\n\t"
                        "pand      %%mm1, %%mm3\n\t"
                        "por       %%mm0, %%mm1\n\t"
                        "pxor      %%mm0, %%mm6\n\t"
#endif
                        T2_SCALE_BLEND_ASM_LEAVE(t2_scale_loop_min, ASM_M_ARG(signmask))
                        cy0 += sdy;
                        outp += w + 1;
                    }
                    break;
                }
            }
        }
    } else {
        /***************************************************************************/
        /***************************************************************************/
        /*   Non-scaling renderer                                                  */
        /***************************************************************************/
        /***************************************************************************/

        RECT r;
        // Determine exact position, original size
        r.left = (RoundToInt((x * .5f + .5f) * w) - iw / 2);
        r.top = (RoundToInt((y * .5f + .5f) * h) - ih / 2);
        r.right = r.left + iw - 1;
        r.bottom = r.top + ih - 1;

        RECT r2;
        memcpy(&r2, &r, sizeof(RECT));

        // Visiblity culling
        if ((r2.right < 0) || (r2.left > w) || (r2.bottom < 0) || (r2.top > h)) {
            goto skippart;
        }

        // Window edge clipping
        if (r.left < 0) {
            x0 = -((double)r.left) / (double)(r.right - r.left);
            r2.left = 0;
        }
        if (r.top < 0) {
            y0 = -((double)r.top) / (double)(r.bottom - r.top);
            r2.top = 0;
        }
        if (r.right > w) {
            r2.right = w;
        }
        if (r.bottom > h) {
            r2.bottom = h;
        }

        if ((r2.right <= r2.left) || (r2.bottom <= r2.top)) {
            goto skippart;
        }

        int cx0 = RoundToInt(x0 * iw);
        int cy0 = RoundToInt(y0 * ih);

        int ty = cy0;

        if (this->config.mask) {
            // Second easiest path, masking, but no scaling
            T2_PREP_MASK_COLOR
            switch (*(g_extinfo->lineblendmode) & 0xFF) {
                case OUT_REPLACE: {
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int* outp = &framebuffer[y * (w + 1) + r2.left];
                        pixel_rgb8* inp = &texture[ty * (iw + 1) + cx0];
                        int tot = (r2.right - r2.left);
                        T2_NONSCALE_BLEND_ASM_ENTER(t2_nonscale_mask_loop_rep)
#ifdef _MSC_VER
                        __asm {
                            movd mm0, dword ptr [esi]

                            punpcklbw mm0, mm5
                            pmullw mm0, mm7
                            psrlw mm0, 8
                            packuswb mm0, mm0

                            movd dword ptr [edi], mm0
                        }
#else  // GCC
                        "movd       %%mm0, dword ptr [%%esi]\n\t"

                            "punpcklbw  %%mm0, %%mm5\n\t"
                            "pmullw     %%mm0, %%mm7\n\t"
                            "psrlw      %%mm0, 8\n\t"
                            "packuswb   %%mm0, %%mm0\n\t"

                            "movd       dword ptr [%%edi], %%mm0\n\t"
#endif
                        T2_NONSCALE_BLEND_ASM_LEAVE(t2_nonscale_mask_loop_rep)
                        ++ty;
                    }
                    T2_NONSCALE_POP_EDI_ESI
                    break;
                }

                case OUT_ADDITIVE: {
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int* outp = &framebuffer[y * (w + 1) + r2.left];
                        pixel_rgb8* inp = &texture[ty * (iw + 1) + cx0];
                        int tot = (r2.right - r2.left);
                        T2_NONSCALE_BLEND_ASM_ENTER(t2_nonscale_mask_loop_add)
#ifdef _MSC_VER
                        __asm {
                            movd mm0, dword ptr [edi]
                            movd mm1, dword ptr [esi]

                            punpcklbw mm1, mm5
                            pmullw mm1, mm7
                            psrlw mm1, 8
                            packuswb mm1, mm1

                            paddusb mm0, mm1
                            movd dword ptr [edi], mm0
                        }
#else  // GCC
                        "movd       %%mm0, dword ptr [%%edi]\n\t"
                            "movd       %%mm1, dword ptr [%%esi]\n\t"

                            "punpcklbw  %%mm1, %%mm5\n\t"
                            "pmullw     %%mm1, %%mm7\n\t"
                            "psrlw      %%mm1, 8\n\t"
                            "packuswb   %%mm1, %%mm1\n\t"

                            "paddusb    %%mm0, %%mm1\n\t"
                            "movd       dword ptr [%%edi], %%mm0\n\t"
#endif
                        T2_NONSCALE_BLEND_ASM_LEAVE(t2_nonscale_mask_loop_add)
                        ++ty;
                    }
                    T2_NONSCALE_POP_EDI_ESI
                    break;
                }

                case OUT_MAXIMUM: {
                    int maxmask = 0xFFFFFF;   // -> mm4
                    int signmask = 0x808080;  // -> mm6
                    T2_NONSCALE_PUSH_ESI_EDI
                    T2_NONSCALE_MINMAX_MASKS
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int* outp = &framebuffer[y * (w + 1) + r2.left];
                        pixel_rgb8* inp = &texture[ty * (iw + 1) + cx0];
                        int tot = (r2.right - r2.left);
                        T2_NONSCALE_BLEND_ASM_ENTER(t2_nonscale_mask_loop_max)
#ifdef _MSC_VER
                        __asm {
                            movd mm0, dword ptr [edi]
                            movd mm1, dword ptr [esi]

                            punpcklbw mm1, mm5
                            pmullw mm1, mm7
                            psrlw mm1, 8
                            packuswb mm1, mm1

                            pxor mm0, mm6
                            pxor mm1, mm6
                            movq mm2, mm1
                            pcmpgtb mm2, mm0
                            movq mm3, mm2
                            pxor mm3, mm4
                            pand mm0, mm3
                            pand mm1, mm2
                            por mm0, mm1
                            pxor mm0, mm6
                            movd dword ptr [edi], mm0
                        }
#else  // GCC
                        "movd       %%mm0, dword ptr [%%edi]\n\t"
                            "movd       %%mm1, dword ptr [%%esi]\n\t"

                            "punpcklbw  %%mm1, %%mm5\n\t"
                            "pmullw     %%mm1, %%mm7\n\t"
                            "psrlw      %%mm1, 8\n\t"
                            "packuswb   %%mm1, %%mm1\n\t"

                            "pxor       %%mm0, %%mm6\n\t"
                            "pxor       %%mm1, %%mm6\n\t"
                            "movq       %%mm2, %%mm1\n\t"
                            "pcmpgtb    %%mm2, %%mm0\n\t"
                            "movq       %%mm3, %%mm2\n\t"
                            "pxor       %%mm3, %%mm4\n\t"
                            "pand       %%mm0, %%mm3\n\t"
                            "pand       %%mm1, %%mm2\n\t"
                            "por        %%mm0, %%mm1\n\t"
                            "pxor       %%mm0, %%mm6\n\t"
                            "movd       dword ptr [%%edi], %%mm0\n\t"
#endif
                        T2_NONSCALE_BLEND_ASM_LEAVE(t2_nonscale_mask_loop_max)
                        ++ty;
                    }
                    T2_NONSCALE_POP_EDI_ESI
                    break;
                }

                case OUT_5050: {
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int* outp = &framebuffer[y * (w + 1) + r2.left];
                        pixel_rgb8* inp = &texture[ty * (iw + 1) + cx0];
                        int tot = (r2.right - r2.left);
                        T2_NONSCALE_BLEND_ASM_ENTER(t2_nonscale_mask_loop_50)
#ifdef _MSC_VER
                        __asm {
                            movd mm0, dword ptr [edi]
                            movd mm1, dword ptr [esi]

                            punpcklbw mm1, mm5
                            pmullw mm1, mm7
                            psrlw mm1, 8
                            packuswb mm1, mm1

                            punpcklbw mm0, mm5
                            punpcklbw mm1, mm5
                            paddusw mm0, mm1
                            psrlw mm0, 1
                            packuswb mm0, mm0
                            movd dword ptr [edi], mm0
                        }
#else  // GCC
                        "movd       %%mm0, dword ptr [%%edi]\n\t"
                            "movd       %%mm1, dword ptr [%%esi]\n\t"

                            "punpcklbw  %%mm1, %%mm5\n\t"
                            "pmullw     %%mm1, %%mm7\n\t"
                            "psrlw      %%mm1, 8\n\t"
                            "packuswb   %%mm1, %%mm1\n\t"

                            "punpcklbw  %%mm0, %%mm5\n\t"
                            "punpcklbw  %%mm1, %%mm5\n\t"
                            "paddusw    %%mm0, %%mm1\n\t"
                            "psrlw      %%mm0, 1\n\t"
                            "packuswb   %%mm0, %%mm0\n\t"
                            "movd       dword ptr [%%edi], %%mm0\n\t"
#endif
                        T2_NONSCALE_BLEND_ASM_LEAVE(t2_nonscale_mask_loop_50)
                        ++ty;
                    }
                    T2_NONSCALE_POP_EDI_ESI
                    break;
                }

                case OUT_SUB1: {
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int* outp = &framebuffer[y * (w + 1) + r2.left];
                        pixel_rgb8* inp = &texture[ty * (iw + 1) + cx0];
                        int tot = (r2.right - r2.left);
                        T2_NONSCALE_BLEND_ASM_ENTER(t2_nonscale_mask_loop_sub1)
#ifdef _MSC_VER
                        __asm {
                            movd mm0, dword ptr [edi]
                            movd mm1, dword ptr [esi]

                            punpcklbw mm1, mm5
                            pmullw mm1, mm7
                            psrlw mm1, 8
                            packuswb mm1, mm1

                            psubusb mm0, mm1
                            movd dword ptr [edi], mm0
                        }
#else  // GCC
                        "movd       %%mm0, dword ptr [%%edi]\n\t"
                            "movd       %%mm1, dword ptr [%%esi]\n\t"

                            "punpcklbw  %%mm1, %%mm5\n\t"
                            "pmullw     %%mm1, %%mm7\n\t"
                            "psrlw      %%mm1, 8\n\t"
                            "packuswb   %%mm1, %%mm1\n\t"

                            "psubusb    %%mm0, %%mm1\n\t"
                            "movd       dword ptr [%%edi], %%mm0\n\t"
#endif
                        T2_NONSCALE_BLEND_ASM_LEAVE(t2_nonscale_mask_loop_sub1)
                        ++ty;
                    }
                    T2_NONSCALE_POP_EDI_ESI
                    break;
                }

                case OUT_SUB2: {
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int* outp = &framebuffer[y * (w + 1) + r2.left];
                        pixel_rgb8* inp = &texture[ty * (iw + 1) + cx0];
                        int tot = (r2.right - r2.left);
                        T2_NONSCALE_BLEND_ASM_ENTER(t2_nonscale_mask_loop_sub2)
#ifdef _MSC_VER
                        __asm {
                            movd mm0, dword ptr [esi]

                            punpcklbw mm0, mm5
                            pmullw mm0, mm7
                            psrlw mm0, 8
                            packuswb mm0, mm0

                            psubusb mm0, qword ptr [edi]
                            movd dword ptr [edi], mm0
                        }
#else  // GCC
                        "movd       %%mm0, dword ptr [%%esi]\n\t"

                            "punpcklbw  %%mm0, %%mm5\n\t"
                            "pmullw     %%mm0, %%mm7\n\t"
                            "psrlw      %%mm0, 8\n\t"
                            "packuswb   %%mm0, %%mm0\n\t"

                            "psubusb    %%mm0, qword ptr [%%edi]\n\t"
                            "movd       dword ptr [%%edi], %%mm0\n\t"
#endif
                        T2_NONSCALE_BLEND_ASM_LEAVE(t2_nonscale_mask_loop_sub2)
                        ++ty;
                    }
                    T2_NONSCALE_POP_EDI_ESI
                    break;
                }

                case OUT_MULTIPLY: {
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int* outp = &framebuffer[y * (w + 1) + r2.left];
                        pixel_rgb8* inp = &texture[ty * (iw + 1) + cx0];
                        int tot = (r2.right - r2.left);
                        T2_NONSCALE_BLEND_ASM_ENTER(t2_nonscale_mask_loop_mul)
#ifdef _MSC_VER
                        __asm {
                            movd mm0, dword ptr [edi]
                            movd mm1, dword ptr [esi]

                            punpcklbw mm0, mm5
                            punpcklbw mm1, mm5
                            pmullw mm0, mm1
                            psrlw mm0, 8
                            // Merged filter/mul
                            pmullw mm0, mm7
                            psrlw mm0, 8
                            packuswb mm0, mm0
                            movd dword ptr [edi], mm0
                        }
#else  // GCC
                        "movd       %%mm0, dword ptr [%%edi]\n\t"
                            "movd       %%mm1, dword ptr [%%esi]\n\t"
                            "punpcklbw  %%mm0, %%mm5\n\t"
                            "punpcklbw  %%mm1, %%mm5\n\t"
                            "pmullw     %%mm0, %%mm1\n\t"
                            "psrlw      %%mm0, 8\n\t"
                            // Merged filter/mul
                            "pmullw     %%mm0, %%mm7\n\t"
                            "psrlw      %%mm0, 8\n\t"
                            "packuswb   %%mm0, %%mm0\n\t"
                            "movd       dword ptr [%%edi], %%mm0\n\t"
#endif
                        T2_NONSCALE_BLEND_ASM_LEAVE(t2_nonscale_mask_loop_mul)
                        ++ty;
                    }
                    T2_NONSCALE_POP_EDI_ESI
                    break;
                }

                case OUT_ADJUSTABLE: {
                    __int64 alphavalue = 0x0;
                    __int64* alpha = &alphavalue;
                    // TODO [bugfix]: shouldn't salpha be 255, not 256?
                    // it's used to calculate 256 - alpha, and if max_alpha = 255,
                    // then...
                    __int64 salpha = 0x0100010001000100;
                    int t = ((*g_extinfo->lineblendmode) & 0xFF00) >> 8;
                    T2_NONSCALE_BLEND_AND_STORE_ALPHA
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int* outp = &framebuffer[y * (w + 1) + r2.left];
                        pixel_rgb8* inp = &texture[ty * (iw + 1) + cx0];
                        int tot = (r2.right - r2.left);
                        T2_NONSCALE_BLEND_ASM_ENTER(t2_nonscale_mask_loop_adj)
#ifdef _MSC_VER
                        __asm {
                            movd mm0, dword ptr [edi]
                            movd mm1, dword ptr [esi]

                            punpcklbw mm0, mm5
                            punpcklbw mm1, mm5
                                // Merged filter/alpha
                            pmullw    mm1, mm7
                            psrlw     mm1, 8

                            pmullw    mm0, mm6;  // * alph
                            pmullw    mm1, mm3;  // * 256 - alph

                            paddusw   mm0, mm1
                            psrlw     mm0, 8
                            packuswb  mm0, mm0
                            movd dword ptr [edi], mm0
                        }
#else  // GCC
                        "movd       %%mm0, dword ptr [%%edi]\n\t"
                            "movd       %%mm1, dword ptr [%%esi]\n\t"

                            "punpcklbw  %%mm0, %%mm5\n\t"
                            "punpcklbw  %%mm1, %%mm5\n\t"
                            // Merged filter/alpha
                            "pmullw     %%mm1, %%mm7\n\t"
                            "psrlw      %%mm1, 8\n\t"

                            "pmullw     %%mm0, %%mm6\n\t" // * alpha
                            "pmullw     %%mm1, %%mm3\n\t" // * 256 - alpha

                            "paddusw    %%mm0, %%mm1\n\t"
                            "psrlw      %%mm0, 8\n\t"
                            "packuswb   %%mm0, %%mm0\n\t"
                            "movd       dword ptr [%%edi], %%mm0\n\t"
#endif
                        T2_NONSCALE_BLEND_ASM_LEAVE(t2_nonscale_mask_loop_adj)
                        ++ty;
                    }
                    T2_NONSCALE_POP_EDI_ESI
                    break;
                }

                case OUT_XOR: {
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int* outp = &framebuffer[y * (w + 1) + r2.left];
                        pixel_rgb8* inp = &texture[ty * (iw + 1) + cx0];
                        int tot = (r2.right - r2.left);
                        T2_NONSCALE_BLEND_ASM_ENTER(t2_nonscale_mask_loop_xor)
#ifdef _MSC_VER
                        __asm {
                            movd mm0, dword ptr [edi]
                            movd mm1, dword ptr [esi]

                            punpcklbw mm1, mm5
                            pmullw mm1, mm7
                            psrlw mm1, 8
                            packuswb mm1, mm1

                            pxor mm0, mm1
                            movd dword ptr [edi], mm0
                        }
#else  // GCC
                        "movd       %%mm0, dword ptr [%%edi]\n\t"
                            "movd       %%mm1, dword ptr [%%esi]\n\t"

                            "punpcklbw  %%mm1, %%mm5\n\t"
                            "pmullw     %%mm1, %%mm7\n\t"
                            "psrlw      %%mm1, 8\n\t"
                            "packuswb   %%mm1, %%mm1\n\t"

                            "pxor       %%mm0, %%mm1\n\t"
                            "movd       dword ptr [%%edi], %%mm0\n\t"
#endif
                        T2_NONSCALE_BLEND_ASM_LEAVE(t2_nonscale_mask_loop_xor)
                        ++ty;
                    }
                    T2_NONSCALE_POP_EDI_ESI
                    break;
                }

                case OUT_MINIMUM: {
                    int maxmask = 0xFFFFFF;
                    int signmask = 0x808080;
                    T2_NONSCALE_PUSH_ESI_EDI
                    T2_NONSCALE_MINMAX_MASKS
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int* outp = &framebuffer[y * (w + 1) + r2.left];
                        pixel_rgb8* inp = &texture[ty * (iw + 1) + cx0];
                        int tot = (r2.right - r2.left);
                        T2_NONSCALE_BLEND_ASM_ENTER(t2_nonscale_mask_loop_min)
#ifdef _MSC_VER
                        __asm {
                            movd mm0, dword ptr [edi]
                            movd mm1, dword ptr [esi]

                            punpcklbw mm1, mm5
                            pmullw mm1, mm7
                            psrlw mm1, 8
                            packuswb mm1, mm1

                            pxor mm0, mm6
                            pxor mm1, mm6
                            movq mm2, mm1
                            pcmpgtb mm2, mm0
                            movq mm3, mm2
                            pxor mm3, mm4
                            pand mm0, mm2
                            pand mm1, mm3
                            por mm0, mm1
                            pxor mm0, mm6
                            movd dword ptr [edi], mm0
                        }
#else  // GCC
                        "movd       %%mm0, dword ptr [%%edi]\n\t"
                            "movd       %%mm1, dword ptr [%%esi]\n\t"

                            "punpcklbw  %%mm1, %%mm5\n\t"
                            "pmullw     %%mm1, %%mm7\n\t"
                            "psrlw      %%mm1, 8\n\t"
                            "packuswb   %%mm1, %%mm1\n\t"

                            "pxor       %%mm0, %%mm6\n\t"
                            "pxor       %%mm1, %%mm6\n\t"
                            "movq       %%mm2, %%mm1\n\t"
                            "pcmpgtb    %%mm2, %%mm0\n\t"
                            "movq       %%mm3, %%mm2\n\t"
                            "pxor       %%mm3, %%mm4\n\t"
                            "pand       %%mm0, %%mm2\n\t"
                            "pand       %%mm1, %%mm3\n\t"
                            "por        %%mm0, %%mm1\n\t"
                            "pxor       %%mm0, %%mm6\n\t"
                            "movd       dword ptr [%%edi], %%mm0\n\t"
#endif
                        T2_NONSCALE_BLEND_ASM_LEAVE(t2_nonscale_mask_loop_min)
                        ++ty;
                    }
                    T2_NONSCALE_POP_EDI_ESI
                    break;
                }
            }
        } else {
            // Most basic path, no scaling or masking
            T2_ZERO_MM5
            switch (*(g_extinfo->lineblendmode) & 0xFF) {
                case OUT_REPLACE: {
                    // the push order was reversed (edi, esi) in the original code here
                    // (and only here) -- i assume that was not intentional...
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int* outp = &framebuffer[y * (w + 1) + r2.left];
                        pixel_rgb8* inp = &texture[ty * (iw + 1) + cx0];
                        int tot = (r2.right - r2.left);
                        T2_NONSCALE_BLEND_ASM_ENTER(t2_nonscale_nonmask_loop_rep)
#ifdef _MSC_VER
                        __asm {
                            movd mm0, dword ptr [esi]
                            movd dword ptr [edi], mm0
                        }
#else  // GCC
                        "movd  %%mm0, dword ptr [%%esi]\n\t"
                            "movd  dword ptr [%%edi], %%mm0\n\t"
#endif
                        T2_NONSCALE_BLEND_ASM_LEAVE(t2_nonscale_nonmask_loop_rep)
                        ++ty;
                    }
                    T2_NONSCALE_POP_EDI_ESI
                    break;
                }

                case OUT_ADDITIVE: {
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int* outp = &framebuffer[y * (w + 1) + r2.left];
                        pixel_rgb8* inp = &texture[ty * (iw + 1) + cx0];
                        int tot = (r2.right - r2.left);
                        T2_NONSCALE_BLEND_ASM_ENTER(t2_nonscale_nonmask_loop_add)
#ifdef _MSC_VER
                        __asm {
                            movd mm0, dword ptr [edi]
                            paddusb mm0, qword ptr [esi]
                            movd dword ptr [edi], mm0
                        }
#else  // GCC
                        "movd     %%mm0, dword ptr [%%edi]\n\t"
                            "paddusb  %%mm0, qword ptr [%%esi]\n\t"
                            "movd     dword ptr [%%edi], %%mm0\n\t"
#endif
                        T2_NONSCALE_BLEND_ASM_LEAVE(t2_nonscale_nonmask_loop_add)
                        ++ty;
                    }
                    T2_NONSCALE_POP_EDI_ESI
                    break;
                }

                case OUT_MAXIMUM: {
                    int maxmask = 0xFFFFFF;
                    int signmask = 0x808080;
                    T2_NONSCALE_PUSH_ESI_EDI
                    T2_NONSCALE_MINMAX_MASKS
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int* outp = &framebuffer[y * (w + 1) + r2.left];
                        pixel_rgb8* inp = &texture[ty * (iw + 1) + cx0];
                        int tot = (r2.right - r2.left);
                        T2_NONSCALE_BLEND_ASM_ENTER(t2_nonscale_nonmask_loop_max)
#ifdef _MSC_VER
                        __asm {
                            movd mm0, dword ptr [edi]
                            movd mm1, dword ptr [esi]
                            pxor mm0, mm6
                            pxor mm1, mm6
                            movq mm2, mm1
                            pcmpgtb mm2, mm0
                            movq mm3, mm2
                            pxor mm3, mm4
                            pand mm0, mm3
                            pand mm1, mm2
                            por mm0, mm1
                            pxor mm0, mm6
                            movd dword ptr [edi], mm0
                        }
#else  // GCC
                        "movd     %%mm0, dword ptr [%%edi]\n\t"
                            "movd     %%mm1, dword ptr [%%esi]\n\t"
                            "pxor     %%mm0, %%mm6\n\t"
                            "pxor     %%mm1, %%mm6\n\t"
                            "movq     %%mm2, %%mm1\n\t"
                            "pcmpgtb  %%mm2, %%mm0\n\t"
                            "movq     %%mm3, %%mm2\n\t"
                            "pxor     %%mm3, %%mm4\n\t"
                            "pand     %%mm0, %%mm3\n\t"
                            "pand     %%mm1, %%mm2\n\t"
                            "por      %%mm0, %%mm1\n\t"
                            "pxor     %%mm0, %%mm6\n\t"
                            "movd     dword ptr [%%edi], %%mm0\n\t"
#endif
                        T2_NONSCALE_BLEND_ASM_LEAVE(t2_nonscale_nonmask_loop_max)
                        ++ty;
                    }
                    T2_NONSCALE_POP_EDI_ESI
                    break;
                }

                case OUT_5050: {
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int* outp = &framebuffer[y * (w + 1) + r2.left];
                        pixel_rgb8* inp = &texture[ty * (iw + 1) + cx0];
                        int tot = (r2.right - r2.left);
                        T2_NONSCALE_BLEND_ASM_ENTER(t2_nonscale_nonmask_loop_50)
#ifdef _MSC_VER
                        __asm {
                            movd mm0, dword ptr [edi]
                            movd mm1, dword ptr [esi]
                            punpcklbw mm0, mm5
                            punpcklbw mm1, mm5
                            paddusw mm0, mm1
                            psrlw mm0, 1
                            packuswb mm0, mm0
                            movd dword ptr [edi], mm0
                        }
#else  // GCC
                        "movd       %%mm0, dword ptr [%%edi]\n\t"
                            "movd       %%mm1, dword ptr [%%esi]\n\t"
                            "punpcklbw  %%mm0, %%mm5\n\t"
                            "punpcklbw  %%mm1, %%mm5\n\t"
                            "paddusw    %%mm0, %%mm1\n\t"
                            "psrlw      %%mm0, 1\n\t"
                            "packuswb   %%mm0, %%mm0\n\t"
                            "movd       dword ptr [%%edi], %%mm0\n\t"
#endif
                        T2_NONSCALE_BLEND_ASM_LEAVE(t2_nonscale_nonmask_loop_50)
                        ++ty;
                    }
                    T2_NONSCALE_POP_EDI_ESI
                    break;
                }

                case OUT_SUB1: {
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int* outp = &framebuffer[y * (w + 1) + r2.left];
                        pixel_rgb8* inp = &texture[ty * (iw + 1) + cx0];
                        int tot = (r2.right - r2.left);
                        T2_NONSCALE_BLEND_ASM_ENTER(t2_nonscale_nonmask_loop_sub1)
#ifdef _MSC_VER
                        __asm {
                            movd mm0, dword ptr [edi]
                            psubusb mm0, qword ptr [esi]
                            movd dword ptr [edi], mm0
                        }
#else  // GCC
                        "movd     %%mm0, dword ptr [%%edi]\n\t"
                            "psubusb  %%mm0, qword ptr [%%esi]\n\t"
                            "movd     dword ptr [%%edi], %%mm0\n\t"
#endif
                        T2_NONSCALE_BLEND_ASM_LEAVE(t2_nonscale_nonmask_loop_sub1)
                        ++ty;
                    }
                    T2_NONSCALE_POP_EDI_ESI
                    break;
                }

                case OUT_SUB2: {
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int* outp = &framebuffer[y * (w + 1) + r2.left];
                        pixel_rgb8* inp = &texture[ty * (iw + 1) + cx0];
                        int tot = (r2.right - r2.left);
                        T2_NONSCALE_BLEND_ASM_ENTER(t2_nonscale_nonmask_loop_sub2)
#ifdef _MSC_VER
                        __asm {
                            movd mm0, dword ptr [esi]
                            psubusb mm0, qword ptr [edi]
                            movd dword ptr [edi], mm0
                        }
#else  // GCC
                        "movd     %%mm0, dword ptr [%%esi]\n\t"
                            "psubusb  %%mm0, qword ptr [%%edi]\n\t"
                            "movd     dword ptr [%%edi], %%mm0\n\t"
#endif
                        T2_NONSCALE_BLEND_ASM_LEAVE(t2_nonscale_nonmask_loop_sub2)
                        ++ty;
                    }
                    T2_NONSCALE_POP_EDI_ESI
                    break;
                }

                case OUT_MULTIPLY: {
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int* outp = &framebuffer[y * (w + 1) + r2.left];
                        pixel_rgb8* inp = &texture[ty * (iw + 1) + cx0];
                        int tot = (r2.right - r2.left);
                        T2_NONSCALE_BLEND_ASM_ENTER(t2_nonscale_nonmask_loop_mul)
#ifdef _MSC_VER
                        __asm {
                            movd mm0, dword ptr [edi]
                            movd mm1, dword ptr [esi]
                            punpcklbw mm0, mm5
                            punpcklbw mm1, mm5
                            pmullw mm0, mm1
                            psrlw mm0, 8
                            packuswb mm0, mm0
                            movd dword ptr [edi], mm0
                        }
#else  // GCC
                        "movd       %%mm0, dword ptr [%%edi]\n\t"
                            "movd       %%mm1, dword ptr [%%esi]\n\t"
                            "punpcklbw  %%mm0, %%mm5\n\t"
                            "punpcklbw  %%mm1, %%mm5\n\t"
                            "pmullw     %%mm0, %%mm1\n\t"
                            "psrlw      %%mm0, 8\n\t"
                            "packuswb   %%mm0, %%mm0\n\t"
                            "movd       dword ptr [%%edi], %%mm0\n\t"
#endif
                        T2_NONSCALE_BLEND_ASM_LEAVE(t2_nonscale_nonmask_loop_mul)
                        ++ty;
                    }
                    T2_NONSCALE_POP_EDI_ESI
                    break;
                }

                case OUT_ADJUSTABLE: {
                    __int64 alphavalue = 0x0;
                    __int64* alpha = &alphavalue;
                    // TODO [bugfix]: shouldn't salpha be 255, not 256?
                    // it's used to calculate 256 - alpha, and if max_alpha = 255,
                    // then...
                    __int64 salpha = 0x0100010001000100;
                    int t = ((*g_extinfo->lineblendmode) & 0xFF00) >> 8;
                    T2_NONSCALE_BLEND_AND_STORE_ALPHA
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int* outp = &framebuffer[y * (w + 1) + r2.left];
                        pixel_rgb8* inp = &texture[ty * (iw + 1) + cx0];
                        int tot = (r2.right - r2.left);
                        T2_NONSCALE_BLEND_ASM_ENTER(t2_nonscale_nonmask_loop_adj)
#ifdef _MSC_VER
                        __asm {
                            movd mm0, dword ptr [edi]
                            movd mm1, dword ptr [esi]

                            punpcklbw mm0, mm5
                            punpcklbw mm1, mm5

                            pmullw    mm0, mm3;  // * alph
                            pmullw    mm1, mm6;  // * 256 - alph

                            paddusw   mm0, mm1
                            psrlw     mm0, 8
                            packuswb  mm0, mm0
                            movd dword ptr [edi], mm0
                        }
#else  // GCC
                        "movd       %%mm0, dword ptr [%%edi]\n\t"
                            "movd       %%mm1, dword ptr [%%esi]\n\t"

                            "punpcklbw  %%mm0, %%mm5\n\t"
                            "punpcklbw  %%mm1, %%mm5\n\t"

                            "pmullw     %%mm0, %%mm3\n\t" // * alpha
                            "pmullw     %%mm1, %%mm6\n\t" // * 256 - alpha

                            "paddusw    %%mm0, %%mm1\n\t"
                            "psrlw      %%mm0, 8\n\t"
                            "packuswb   %%mm0, %%mm0\n\t"
                            "movd       dword ptr [%%edi], %%mm0\n\t"
#endif
                        T2_NONSCALE_BLEND_ASM_LEAVE(t2_nonscale_nonmask_loop_adj)
                        ++ty;
                    }
                    T2_NONSCALE_POP_EDI_ESI
                    break;
                }

                case OUT_XOR: {
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int* outp = &framebuffer[y * (w + 1) + r2.left];
                        pixel_rgb8* inp = &texture[ty * (iw + 1) + cx0];
                        int tot = (r2.right - r2.left);
                        T2_NONSCALE_BLEND_ASM_ENTER(t2_nonscale_nonmask_loop_xor)
#ifdef _MSC_VER
                        __asm {
                            movd mm0, dword ptr [edi]
                            pxor mm0, qword ptr [esi]
                            movd dword ptr [edi], mm0
                        }
#else  // GCC
                        "movd  %%mm0, dword ptr [%%edi]\n\t"
                            "pxor  %%mm0, qword ptr [%%esi]\n\t"
                            "movd  dword ptr [%%edi], %%mm0\n\t"
#endif
                        T2_NONSCALE_BLEND_ASM_LEAVE(t2_nonscale_nonmask_loop_xor)
                        ++ty;
                    }
                    T2_NONSCALE_POP_EDI_ESI
                    break;
                }

                case OUT_MINIMUM: {
                    int maxmask = 0xFFFFFF;
                    int signmask = 0x808080;
                    T2_NONSCALE_PUSH_ESI_EDI
                    T2_NONSCALE_MINMAX_MASKS
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int* outp = &framebuffer[y * (w + 1) + r2.left];
                        pixel_rgb8* inp = &texture[ty * (iw + 1) + cx0];
                        int tot = (r2.right - r2.left);
                        T2_NONSCALE_BLEND_ASM_ENTER(t2_nonscale_nonmask_loop_min)
#ifdef _MSC_VER
                        __asm {
                            movd mm0, dword ptr [edi]
                            movd mm1, dword ptr [esi]
                            pxor mm0, mm6
                            pxor mm1, mm6
                            movq mm2, mm1
                            pcmpgtb mm2, mm0
                            movq mm3, mm2
                            pxor mm3, mm4
                            pand mm0, mm2
                            pand mm1, mm3
                            por mm0, mm1
                            pxor mm0, mm6
                            movd dword ptr [edi], mm0
                        }
#else  // GCC
                        "movd     %%mm0, dword ptr [%%edi]\n\t"
                            "movd     %%mm1, dword ptr [%%esi]\n\t"
                            "pxor     %%mm0, %%mm6\n\t"
                            "pxor     %%mm1, %%mm6\n\t"
                            "movq     %%mm2, %%mm1\n\t"
                            "pcmpgtb  %%mm2, %%mm0\n\t"
                            "movq     %%mm3, %%mm2\n\t"
                            "pxor     %%mm3, %%mm4\n\t"
                            "pand     %%mm0, %%mm2\n\t"
                            "pand     %%mm1, %%mm3\n\t"
                            "por      %%mm0, %%mm1\n\t"
                            "pxor     %%mm0, %%mm6\n\t"
                            "movd     dword ptr [%%edi], %%mm0\n\t"
#endif
                        T2_NONSCALE_BLEND_ASM_LEAVE(t2_nonscale_nonmask_loop_min)
                        ++ty;
                    }
                    T2_NONSCALE_POP_EDI_ESI
                    break;
                }
            }
        }
    }

skippart:
    ++(this->iw);  // restore member vars!
    ++(this->ih);

    // Perhaps removing all fpu stuff here and doing one emms globally would be better?
    EMMS
}

inline double wrap_diff_to_plusminus1(double x) { return round(x / 2.0) * 2.0; }

/* For AVS 2.81d compatibility. TexerII v1.0 only wrapped once. */
inline double wrap_once_diff_to_plusminus1(double x) {
    return (x > 1.0) ? +2.0 : (x < -1.0 ? -2.0 : 0.0);
}

bool overlaps_edge(double wrapped_coord,
                   double img_size,
                   int img_size_px,
                   int screen_size_px) {
    double abs_wrapped_coord = fabs(wrapped_coord);
    // a /2 seems missing, but screen has size 2, hence /2(half) *2(screen size) => *1
    // image pixel size needs reduction by one pixel
    double rel_size_half = img_size * (img_size_px - 1) / screen_size_px;
    return ((abs_wrapped_coord + rel_size_half) > 1.0)
           && ((abs_wrapped_coord - rel_size_half) < 1.0);
}

int C_Texer2::render(char visdata[2][2][576],
                     int is_beat,
                     int* framebuffer,
                     int*,
                     int w,
                     int h) {
    lock(this->code_lock);
    this->code.recompile_if_needed();
    this->code.init_variables(w, h, is_beat, this->iw, this->ih);
    if (this->code.need_init || (is_beat & 0x80000000)) {
        *this->code.vars.n = 0.0f;
        this->code.init.exec(visdata);
        this->code.need_init = false;
    }
    this->code.frame.exec(visdata);
    if ((is_beat & 0x00000001)) {
        this->code.beat.exec(visdata);
    }
    int n = RoundToInt((double)*this->code.vars.n);
    n = max(0, min(65536, n));

    lock(this->image_lock);
    if (n) {
        double step = 1.0 / (n - 1);
        double i = 0.0;
        for (int j = 0; j < n; ++j) {
            *this->code.vars.i = i;
            *this->code.vars.skip = 0.0;
            *this->code.vars.v =
                ((int)visdata[1][0][j * 575 / n] + (int)visdata[1][1][j * 575 / n])
                / 256.0;
            i += step;

            this->code.point.exec(visdata);

            // TODO [cleanup]: invert to if+continue
            if (*this->code.vars.skip == 0.0) {
                unsigned int color = min(
                    255, max(0, RoundToInt(255.0f * (double)*this->code.vars.blue)));
                color |=
                    min(255,
                        max(0, RoundToInt(255.0f * (double)*this->code.vars.green)))
                    << 8;
                color |=
                    min(255, max(0, RoundToInt(255.0f * (double)*this->code.vars.red)))
                    << 16;

                if (this->config.mask == 0) color = 0xFFFFFF;

                pixel_rgb8* texture = this->image_normal;
                if (*this->code.vars.sizex < 0 && *this->code.vars.sizey > 0) {
                    texture = this->image_mirrored;
                } else if (*this->code.vars.sizex > 0 && *this->code.vars.sizey < 0) {
                    texture = this->image_flipped;
                } else if (*this->code.vars.sizex < 0 && *this->code.vars.sizey < 0) {
                    texture = this->image_rot180;
                }

                double szx = fabs(*this->code.vars.sizex);
                double szy = fabs(*this->code.vars.sizey);

                // TODO [cleanup]: put more of the above into this if-case, or invert to
                // if+continue
                // TODO [bugfix]: really large images would be clipped while still
                // potentially visible, should be relative to image size
                if ((szx > .01) && (szy > .01)) {
                    double x = *this->code.vars.x;
                    double y = *this->code.vars.y;
                    if (this->config.wrap != 0) {
                        bool overlaps_x, overlaps_y;
                        if (this->config.version == TEXER_II_VERSION_V2_81D) {
                            /* Wrap only once (when crossing +/-1, not when crossing
                             * +/-3 etc. */
                            x -= wrap_once_diff_to_plusminus1(*this->code.vars.x);
                            y -= wrap_once_diff_to_plusminus1(*this->code.vars.y);
                            overlaps_x =
                                overlaps_edge(*this->code.vars.x, szx, this->iw, w);
                            overlaps_y =
                                overlaps_edge(*this->code.vars.y, szy, this->ih, h);
                        } else {
                            x = *this->code.vars.x
                                - wrap_diff_to_plusminus1(*this->code.vars.x);
                            y = *this->code.vars.y
                                - wrap_diff_to_plusminus1(*this->code.vars.y);
                            overlaps_x = overlaps_edge(x, szx, this->iw, w);
                            overlaps_y = overlaps_edge(y, szy, this->ih, h);
                        }
                        double sign_x2 = x > 0 ? 2.0 : -2.0;
                        double sign_y2 = y > 0 ? 2.0 : -2.0;
                        if (overlaps_x) {
                            DrawParticle(framebuffer,
                                         texture,
                                         w,
                                         h,
                                         x - sign_x2,
                                         y,
                                         szx,
                                         szy,
                                         color);
                        }
                        if (overlaps_y) {
                            DrawParticle(framebuffer,
                                         texture,
                                         w,
                                         h,
                                         x,
                                         y - sign_y2,
                                         szx,
                                         szy,
                                         color);
                        }
                        if (overlaps_x && overlaps_y) {
                            DrawParticle(framebuffer,
                                         texture,
                                         w,
                                         h,
                                         x - sign_x2,
                                         y - sign_y2,
                                         szx,
                                         szy,
                                         color);
                        }
                    }
                    DrawParticle(framebuffer, texture, w, h, x, y, szx, szy, color);
                }
            }
        }
    }

    lock_unlock(this->code_lock);
    lock_unlock(this->image_lock);
    return 0;
}

void Texer2Vars::register_variables(void* vm_context) {
    this->n = NSEEL_VM_regvar(vm_context, "n");
    this->i = NSEEL_VM_regvar(vm_context, "i");
    this->x = NSEEL_VM_regvar(vm_context, "x");
    this->y = NSEEL_VM_regvar(vm_context, "y");
    this->v = NSEEL_VM_regvar(vm_context, "v");
    this->b = NSEEL_VM_regvar(vm_context, "b");
    this->w = NSEEL_VM_regvar(vm_context, "w");
    this->h = NSEEL_VM_regvar(vm_context, "h");
    this->iw = NSEEL_VM_regvar(vm_context, "iw");
    this->ih = NSEEL_VM_regvar(vm_context, "ih");
    this->sizex = NSEEL_VM_regvar(vm_context, "sizex");
    this->sizey = NSEEL_VM_regvar(vm_context, "sizey");
    this->red = NSEEL_VM_regvar(vm_context, "red");
    this->green = NSEEL_VM_regvar(vm_context, "green");
    this->blue = NSEEL_VM_regvar(vm_context, "blue");
    this->skip = NSEEL_VM_regvar(vm_context, "skip");
}

void Texer2Vars::init_variables(int w, int h, int is_beat, va_list extra_args) {
    *this->i = 0.0f;
    *this->x = 0.0f;
    *this->y = 0.0f;
    *this->v = 0.0f;
    *this->w = w;
    *this->h = h;
    *this->b = is_beat ? 1.0f : 0.0f;
    *this->iw = va_arg(extra_args, int);
    *this->ih = va_arg(extra_args, int);
    *this->sizex = 1.0f;
    *this->sizey = 1.0f;
    *this->red = 1.0f;
    *this->green = 1.0f;
    *this->blue = 1.0f;
    *this->skip = 0.0f;
}

char* C_Texer2::get_desc(void) { return MOD_NAME; }

void C_Texer2::load_config(unsigned char* data, int len) {
    if (len >= ssizeof32(texer2_apeconfig)) {
        memcpy(&this->config, data, sizeof(texer2_apeconfig));
    }
    if (this->config.version < TEXER_II_VERSION_V2_81D
        || this->config.version > TEXER_II_VERSION_CURRENT) {
        // If the version value is not in the known set, assume an old preset version.
        this->config.version = TEXER_II_VERSION_V2_81D;
    }
    u_int pos = sizeof(texer2_apeconfig);
    char* str_data = (char*)data;
    pos += this->code.init.load_length_prefixed(&str_data[pos], max(0, len - pos));
    pos += this->code.frame.load_length_prefixed(&str_data[pos], max(0, len - pos));
    pos += this->code.beat.load_length_prefixed(&str_data[pos], max(0, len - pos));
    pos += this->code.point.load_length_prefixed(&str_data[pos], max(0, len - pos));
    load_image();
}

int C_Texer2::save_config(unsigned char* data) {
    memcpy(data, &this->config, sizeof(texer2_apeconfig));
    u_int pos = sizeof(texer2_apeconfig);
    char* str_data = (char*)data;
    pos += this->code.init.save_length_prefixed(&str_data[pos],
                                                max(0, MAX_CODE_LEN - 1 - pos));
    pos += this->code.frame.save_length_prefixed(&str_data[pos],
                                                 max(0, MAX_CODE_LEN - 1 - pos));
    pos += this->code.beat.save_length_prefixed(&str_data[pos],
                                                max(0, MAX_CODE_LEN - 1 - pos));
    pos += this->code.point.save_length_prefixed(&str_data[pos],
                                                 max(0, MAX_CODE_LEN - 1 - pos));
    return pos;
}

/* APE interface */
C_RBASE* R_Texer2(char* desc) {
    if (desc) {
        strcpy(desc, MOD_NAME);
        return NULL;
    }
    return (C_RBASE*)new C_Texer2(TEXER_II_VERSION_CURRENT);
}

void R_Texer2_SetExtInfo(APEinfo* ptr) { g_extinfo = ptr; }
