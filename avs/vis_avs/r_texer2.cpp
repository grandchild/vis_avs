#include <windows.h>
#include <math.h>
#include <stdio.h>
#include "r_defs.h"
#include "r_texer2.h"
#include "resource.h"
#include "../gcc-hacks.h"

class C_Texer2 : public C_RBASE
{
    protected:
    public:
        C_Texer2(int default_version);
        virtual ~C_Texer2();
        virtual int render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h);
        virtual HWND conf(HINSTANCE hInstance, HWND hwndParent);
        virtual char *get_desc();
        virtual void load_config(unsigned char *data, int len);
        virtual int  save_config(unsigned char *data);

        virtual void InitTexture();
        virtual void DeleteTexture();

        virtual void Recompile();
        virtual void DrawParticle(int *framebuffer, int *texture, int w, int h, double x, double y, double sizex, double sizey, unsigned int color);
        virtual void LoadExamples(HWND button, HWND ctl, bool is_init);

        texer2_apeconfig config;

        VM_CONTEXT context;
        VM_CODEHANDLE codeinit;
        VM_CODEHANDLE codeframe;
        VM_CODEHANDLE codebeat;
        VM_CODEHANDLE codepoint;
        Vars vars;

        Code code;

        HWND hwndDlg;
        HBITMAP bmp;
        HDC bmpdc;
        HBITMAP bmpold;
        int iw;
        int ih;
        int *texbits_normal;
        int *texbits_flipped;
        int *texbits_mirrored;
        int *texbits_rot180;
        bool init;

        CRITICAL_SECTION imageload;
        CRITICAL_SECTION codestuff;
        
        char* help_text = "Texer II\0"
            "Texer II is a rendering component that draws what is commonly known as particles.\r\n"
            "At specified positions on screen, a copy of the source image is placed and blended in various ways.\r\n"
            "\r\n"
            "\r\n"
            "The usage is similar to that of a dot-superscope.\r\n"
            "The following variables are available:\r\n"
            "\r\n"
            "* n: Contains the amount of particles to draw. Usually set in the init code or the frame code.\r\n"
            "\r\n"
            "* w,h: Contain the width and height of the window in pixels.\r\n"
            "\r\n"
            "* i: Contains the percentage of progress of particles drawn. Varies from 0 (first particle) to 1 (last particle).\r\n"
            "\r\n"
            "* x,y: Specify the position of the center of the particle. Range -1 to 1.\r\n"
            "\r\n"
            "* v: Contains the i'th sample of the oscilloscope waveform. Use getspec(...) or getosc(...) for other data (check the function reference for their usage).\r\n"
            "\r\n"
            "* b: Contains 1 if a beat is occuring, 0 otherwise.\r\n"
            "\r\n"
            "* iw, ih: Contain the width and height of the Texer bitmap in pixels\r\n"
            "\r\n"
            "* sizex, sizey: Contain the relative horizontal and vertical size of the current particle. Use 1.0 for original size. Negative values cause the image to be mirrored or flipped. Changing size only works if Resizing is on; mirroring/flipping works in all modes.\r\n"
            "\r\n"
            "* red, green, blue: Set the color of the current particle in terms of its red, green and blue component. Only works if color filtering is on.\r\n"
            "\r\n"
            "* skip: Default is 0. If set to 1, indicates that the current particle should not be drawn.\r\n"
            "\r\n"
            "\r\n"
            "\r\n"
            "The options available are:\r\n"
            "\r\n"
            "* Color filtering: blends the image multiplicatively with the color specified by the red, green and blue variables.\r\n"
            "\r\n"
            "* Resizing: resizes the image according to the variables sizex and sizey.\r\n"
            "\r\n"
            "* Wrap around: wraps any image data that falls off the border of the screen around to the other side. Useful for making tiled patterns.\r\n"
            "\r\n"
            "\r\n"
            "\r\n"
            "Texer II supports the standard AVS blend modes. Just place a Misc / Set Render Mode before the Texer II and choose the appropriate setting. You will most likely use Additive or Maximum blend.\r\n"
            "\r\n"
            "Texer II was written by Steven Wittens. Thanks to everyone at the Winamp.com forums for feedback, especially Deamon and gaekwad2 for providing the examples and Tuggummi for providing the default image.\r\n";
};

// extended APE api support
APEinfo *g_extinfo = 0;

// global configuration dialog pointer
static C_Texer2 *g_ConfigThis;

void C_Texer2::LoadExamples(HWND button, HWND ctl, bool is_init) {
    RECT r;
    GetWindowRect(ctl, &r);

    HMENU m = CreatePopupMenu();
    AppendMenu(m, MF_STRING, ID_TEXER2_EXAMPLE_COLOROSC, "Colored Oscilloscope");
    AppendMenu(m, MF_STRING, ID_TEXER2_EXAMPLE_FLUMMYSPEC, "Flummy Spectrum");
    AppendMenu(m, MF_STRING, ID_TEXER2_EXAMPLE_BEATCIRCLE, "Beat-responsive Circle");
    AppendMenu(m, MF_STRING, ID_TEXER2_EXAMPLE_3DRINGS, "3D Beat Rings");

    int ret = TrackPopupMenu(m, TPM_RETURNCMD, r.left+1, r.bottom+1, 0, ctl, 0);
    switch(ret) {
        case ID_TEXER2_EXAMPLE_COLOROSC:
            this->code.SetInit("// This example needs Maximum render mode\r\n"
                               "n=300;");
            this->code.SetFrame("");
            this->code.SetBeat("");
            this->code.SetPoint("x=(i*2-1)*2;y=v;\r\n"
                                "red=1-y*2;green=abs(y)*2;blue=y*2-1;");
            this->config.mask = 1;
            this->config.resize = 0;
            this->config.wrap = 0;
            break;
        case ID_TEXER2_EXAMPLE_FLUMMYSPEC:
            this->code.SetInit("// This example needs Maximum render mode");
            this->code.SetFrame("");
            this->code.SetBeat("");
            this->code.SetPoint("x=i*1.8-.9;\r\n"
                                "y=0;\r\n"
                                "vol=1.001-getspec(abs(x)*.5,.05,0)*min(1,abs(x)+.5)*2;\r\n"
                                "sizex=vol;sizey=(1/vol)*2;\r\n"
                                "j=abs(x);red=1-j;green=1-abs(.5-j);blue=j");
            this->config.mask = 1;
            this->config.resize = 1;
            this->config.wrap = 0;
            break;
        case ID_TEXER2_EXAMPLE_BEATCIRCLE:
            this->code.SetInit("// This example needs Maximum render mode\r\n"
                               "n=30;newradius=.5;");
            this->code.SetFrame("rotation=rotation+step;step=step*.9;\r\n"
                                "radius=radius*.9+newradius*.1;\r\n"
                                "point=0;\r\n"
                                "aspect=h/w;");
            this->code.SetBeat("step=.05;\r\n"
                               "newradius=rand(100)*.005+.5;");
            this->code.SetPoint("angle=rotation+point/n*$pi*2;\r\n"
                                "x=cos(angle)*radius*aspect;y=sin(angle)*radius;\r\n"
                                "red=sin(i*$pi*2)*.5+.5;green=1-red;blue=.5;\r\n"
                                "point=point+1;");
            this->config.mask = 1;
            this->config.resize = 0;
            this->config.wrap = 0;
            break;
        case ID_TEXER2_EXAMPLE_3DRINGS:
            this->code.SetInit("// This shows how to use texer for 3D particles\r\n"
                               "// Additive or maximum blend mode should be used\r\n"
                               "xr=(rand(50)/500)-0.05;\r\n"
                               "yr=(rand(50)/500)-0.05;\r\n"
                               "zr=(rand(50)/500)-0.05;");
            this->code.SetFrame("// Rotation along x/y/z axes\r\n"
                                "xt=xt+xr;yt=yt+yr;zt=zt+zr;\r\n"
                                "// Shrink rings\r\n"
                                "bt=max(0,bt*.95+.01);\r\n"
                                "// Aspect correction\r\n"
                                "asp=w/h;\r\n"
                                "// Dynamically adjust particle count based on ring size\r\n"
                                "n=((bt*40)|0)*3;");
            this->code.SetBeat("// New rotation speeds\r\n"
                               "xr=(rand(50)/500)-0.05;\r\n"
                               "yr=(rand(50)/500)-0.05;\r\n"
                               "zr=(rand(50)/500)-0.05;\r\n"
                               "// Ring size\r\n"
                               "bt=1.2;\r\n"
                               "n=((bt*40)|0)*3;");
            this->code.SetPoint("// 3D object\r\n"
                                "x1=sin(i*$pi*6)/2*bt;\r\n"
                                "y1=above(i,.66)-below(i,.33);\r\n"
                                "z1=cos(i*$pi*6)/2*bt;\r\n"
                                "\r\n"
                                "// 3D rotations\r\n"
                                "x2=x1*sin(zt)-y1*cos(zt);y2=x1*cos(zt)+y1*sin(zt);\r\n"
                                "z2=x2*cos(yt)+z1*sin(yt);x3=x2*sin(yt)-z1*cos(yt);\r\n"
                                "y3=y2*sin(xt)-z2*cos(xt);z3=y2*cos(xt)+z2 *sin(xt);\r\n"
                                "\r\n"
                                "// 2D Projection\r\n"
                                "iz=1/(z3+2);\r\n"
                                "x=x3*iz;y=y3*iz*asp;\r\n"
                                "sizex=iz*2;sizey=iz*2;");
            this->config.mask = 0;
            this->config.resize = 1;
            this->config.wrap = 0;
            break;
        default:
            break;
    }
    CheckDlgButton(button, IDC_TEXERII_OWRAP, this->config.wrap ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(button, IDC_TEXERII_OMASK, this->config.mask ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(button, IDC_TEXERII_ORESIZE, this->config.resize ? BST_CHECKED : BST_UNCHECKED);
    SetDlgItemTextA(button, IDC_TEXERII_CINIT, this->code.init);
    SetDlgItemTextA(button, IDC_TEXERII_CFRAME, this->code.frame);
    SetDlgItemTextA(button, IDC_TEXERII_CBEAT, this->code.beat);
    SetDlgItemTextA(button, IDC_TEXERII_CPOINT, this->code.point);
    // select the default texture image
    SendDlgItemMessageA(this->hwndDlg, IDC_TEXERII_TEXTURE, CB_SETCURSEL, 0, 0);
    this->Recompile();
    this->InitTexture();
}

// this is where we deal with the configuration screen
static BOOL CALLBACK g_DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    char buf[MAX_PATH];
    switch (uMsg)
    {
        case WM_COMMAND:
        {
            int wNotifyCode = HIWORD(wParam);
            int wID = LOWORD(wParam);
            HWND h = (HWND) lParam;

            if (wNotifyCode == CBN_SELCHANGE) {
                switch (LOWORD(wParam)) {
                case IDC_TEXERII_TEXTURE:
                    HWND h = (HWND)lParam;
                    int p = SendMessage(h, CB_GETCURSEL, 0, 0);
                    if (p >= 1) {
                        SendMessage(h, CB_GETLBTEXT, p, (LPARAM)g_ConfigThis->config.img);
                        g_ConfigThis->InitTexture();
                    } else {
                        g_ConfigThis->config.img[0] = 0;
                        g_ConfigThis->InitTexture();
                    }
                    break;
                }
            } else if (wNotifyCode == BN_CLICKED) {
                g_ConfigThis->config.wrap = IsDlgButtonChecked(hwndDlg, IDC_TEXERII_OWRAP) == BST_CHECKED;
                g_ConfigThis->config.resize = IsDlgButtonChecked(hwndDlg, IDC_TEXERII_ORESIZE) == BST_CHECKED;
                g_ConfigThis->config.mask = IsDlgButtonChecked(hwndDlg, IDC_TEXERII_OMASK) == BST_CHECKED;

                if (LOWORD(wParam) == IDC_TEXERII_ABOUT) {
                    g_extinfo->doscripthelp(hwndDlg, g_ConfigThis->help_text);
                } else if (LOWORD(wParam) == IDC_TEXERII_EXAMPLE) {
                    HWND examplesButton = GetDlgItem(hwndDlg, IDC_TEXERII_EXAMPLE);
                    g_ConfigThis->LoadExamples(hwndDlg, examplesButton, false);
                }
            } else if (wNotifyCode == EN_CHANGE) {
                char *buf;
                int l = GetWindowTextLength(h);
                buf = new char[l+1];
                GetWindowText(h, buf, l+1);

                switch (LOWORD(wParam)) {
                case IDC_TEXERII_CINIT:
                    g_ConfigThis->code.SetInit(buf);
                    g_ConfigThis->Recompile();
                    break;
                case IDC_TEXERII_CFRAME:
                    g_ConfigThis->code.SetFrame(buf);
                    g_ConfigThis->Recompile();
                    break;
                case IDC_TEXERII_CBEAT:
                    g_ConfigThis->code.SetBeat(buf);
                    g_ConfigThis->Recompile();
                    break;
                case IDC_TEXERII_CPOINT:
                    g_ConfigThis->code.SetPoint(buf);
                    g_ConfigThis->Recompile();
                    break;
                default:
                    delete buf;
                    break;
                }
            }
            return 1;
        }

        case WM_INITDIALOG:
            g_ConfigThis->hwndDlg = hwndDlg;

            {
                WIN32_FIND_DATA wfd;
                HANDLE h;

                wsprintf(buf,"%s\\*.bmp",g_path);

                bool found = false;
                SendDlgItemMessage(hwndDlg, IDC_TEXERII_TEXTURE, CB_ADDSTRING, 0, (LPARAM)"(default image)");
                h = FindFirstFile(buf, &wfd);
                if (h != INVALID_HANDLE_VALUE) {
                    bool rep = true;
                    while (rep) {
                        int p = SendDlgItemMessage(hwndDlg, IDC_TEXERII_TEXTURE, CB_ADDSTRING, 0, (LPARAM)wfd.cFileName);
                        if (stricmp(wfd.cFileName, g_ConfigThis->config.img) == 0) {
                            SendDlgItemMessage(hwndDlg, IDC_TEXERII_TEXTURE, CB_SETCURSEL, p, 0);
                            found = true;
                        }
                        if (!FindNextFile(h, &wfd)) {
                            rep = false;
                        }
                    };
                    FindClose(h);
                }
                if (!found)
                    SendDlgItemMessage(hwndDlg, IDC_TEXERII_TEXTURE, CB_SETCURSEL, 0, 0);
            }

            SetDlgItemText(hwndDlg, IDC_TEXERII_CINIT, g_ConfigThis->code.init);
            SetDlgItemText(hwndDlg, IDC_TEXERII_CFRAME, g_ConfigThis->code.frame);
            SetDlgItemText(hwndDlg, IDC_TEXERII_CBEAT, g_ConfigThis->code.beat);
            SetDlgItemText(hwndDlg, IDC_TEXERII_CPOINT, g_ConfigThis->code.point);

            CheckDlgButton(hwndDlg, IDC_TEXERII_OWRAP, g_ConfigThis->config.wrap ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwndDlg, IDC_TEXERII_OMASK, g_ConfigThis->config.mask ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwndDlg, IDC_TEXERII_ORESIZE, g_ConfigThis->config.resize ? BST_CHECKED : BST_UNCHECKED);

            return 1;

        case WM_DESTROY:
            return 1;
    }
    return 0;
}


void C_Texer2::Recompile() {
    EnterCriticalSection(&codestuff);

    init = true;

    g_extinfo->resetVM(context);
    vars.n     = g_extinfo->regVMvariable(context, "n");
    vars.i     = g_extinfo->regVMvariable(context, "i");
    vars.x     = g_extinfo->regVMvariable(context, "x");
    vars.y     = g_extinfo->regVMvariable(context, "y");
    vars.v     = g_extinfo->regVMvariable(context, "v");
    vars.w     = g_extinfo->regVMvariable(context, "w");
    vars.h     = g_extinfo->regVMvariable(context, "h");
    vars.sizex = g_extinfo->regVMvariable(context, "sizex");
    vars.sizey = g_extinfo->regVMvariable(context, "sizey");
    vars.red   = g_extinfo->regVMvariable(context, "red");
    vars.green = g_extinfo->regVMvariable(context, "green");
    vars.blue  = g_extinfo->regVMvariable(context, "blue");
    vars.skip  = g_extinfo->regVMvariable(context, "skip");

    if (codeinit)
        g_extinfo->freeCode(codeinit);
    if (codeframe)
        g_extinfo->freeCode(codeframe);
    if (codebeat)
        g_extinfo->freeCode(codebeat);
    if (codepoint)
        g_extinfo->freeCode(codepoint);

    codeinit = g_extinfo->compileVMcode(context, code.init);
    codeframe = g_extinfo->compileVMcode(context, code.frame);
    codebeat = g_extinfo->compileVMcode(context, code.beat);
    codepoint = g_extinfo->compileVMcode(context, code.point);

    LeaveCriticalSection(&codestuff);
}

// set up default configuration
C_Texer2::C_Texer2(int default_version)
{
    memset(&config, 0, sizeof(texer2_apeconfig));
    hwndDlg = 0;
    bmp = 0;
    iw = 0;
    ih = 0;
    texbits_normal = 0;
    texbits_flipped = 0;
    texbits_mirrored = 0;
    texbits_rot180 = 0;
    init = true;
    config.version = default_version;

    InitializeCriticalSection(&imageload);
    InitializeCriticalSection(&codestuff);

    if (g_extinfo) {
        context = g_extinfo->allocVM();
        codeinit = codeframe = codebeat = codepoint = 0;
        Recompile();
    }
}

// virtual destructor
C_Texer2::~C_Texer2()
{
    if (bmp)
        DeleteTexture();
    DeleteCriticalSection(&imageload);
    DeleteCriticalSection(&codestuff);

    if (codeinit)
        g_extinfo->freeCode(codeinit);
    if (codeframe)
        g_extinfo->freeCode(codeframe);
    if (codebeat)
        g_extinfo->freeCode(codebeat);
    if (codepoint)
        g_extinfo->freeCode(codepoint);

    g_extinfo->freeVM(context);
}

void C_Texer2::DeleteTexture()
{
    EnterCriticalSection(&imageload);
    if (bmp) {
        SelectObject(bmpdc, bmpold);
        DeleteObject(bmp);
        DeleteDC(bmpdc);
        iw = ih = 0;
        delete this->texbits_normal;
        delete this->texbits_flipped;
        delete this->texbits_mirrored;
        delete this->texbits_rot180;
    }
    bmp = NULL;
    LeaveCriticalSection(&imageload);
}

extern unsigned char rawData[1323]; // example pic
void C_Texer2::InitTexture()
{
    EnterCriticalSection(&imageload);
    if (bmp)
        DeleteTexture();
    bool loaddefault = false;
    if (strlen(config.img)) {
        char buf[MAX_PATH];

        wsprintf(buf, "%s\\%s", g_path, config.img);

        HANDLE f = CreateFile(buf, 0, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
        if (f != INVALID_HANDLE_VALUE) {
            CloseHandle(f);
            bmp = (HBITMAP)LoadImage(0, buf, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE|LR_CREATEDIBSECTION);

            DIBSECTION dib;
            BITMAPINFO bi;

            GetObject(bmp, sizeof(dib), &dib);

            if(dib.dsBmih.biWidth < 2) {
                dib.dsBmih.biWidth = 2;
            }
            iw = dib.dsBmih.biWidth;
            if(dib.dsBmih.biHeight < 2) {
                dib.dsBmih.biHeight = 2;
            }
            ih = dib.dsBmih.biHeight;

            bmpdc = CreateCompatibleDC(NULL);
            bmpold = (HBITMAP)SelectObject(bmpdc, bmp);

            bi.bmiHeader.biSizeImage = iw*ih*4;
            bi.bmiHeader.biHeight = -ih;
            bi.bmiHeader.biWidth = iw;
            bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bi.bmiHeader.biPlanes = 1;
            bi.bmiHeader.biBitCount = 32;
            bi.bmiHeader.biCompression = BI_RGB;
            bi.bmiHeader.biXPelsPerMeter = 0;
            bi.bmiHeader.biYPelsPerMeter = 0;
            bi.bmiHeader.biClrUsed = 0xffffff;
            bi.bmiHeader.biClrImportant = 0xffffff;

            this->texbits_normal = (int *)new unsigned char[bi.bmiHeader.biSizeImage];
            this->texbits_flipped = (int *)new unsigned char[bi.bmiHeader.biSizeImage];
            this->texbits_mirrored = (int *)new unsigned char[bi.bmiHeader.biSizeImage];
            this->texbits_rot180 = (int *)new unsigned char[bi.bmiHeader.biSizeImage];
            GetDIBits(bmpdc, bmp, 0, ih, texbits_normal, &bi, DIB_RGB_COLORS);

            int y = 0;
            while (y < ih) {
                int x = 0;
                while (x < iw) {
                    int value = texbits_normal[y * iw + x];
                    texbits_flipped[(ih - y - 1) * iw + x] = value;
                    texbits_mirrored[(y + 1) * iw - x - 1] = value;
                    texbits_rot180[(ih - y) * iw - x - 1] = value;
                    x++;
                }
                y++;
            }
        } else {
            loaddefault = true;
        }
    } else {
        loaddefault = true;
    }
    if (loaddefault) {
        iw = 21;
        ih = 21;
        this->texbits_normal = (int *)new unsigned char[iw * ih * 4];
        this->texbits_flipped = (int *)new unsigned char[iw * ih * 4];
        this->texbits_mirrored = (int *)new unsigned char[iw * ih * 4];
        this->texbits_rot180 = (int *)new unsigned char[iw * ih * 4];
        for (int i = 0; i < iw*ih; ++i) {
            // the default image is symmetrical in all directions
            texbits_normal[i] = *(int *)&rawData[i * 3];
            texbits_flipped[i] = *(int *)&rawData[i * 3];
            texbits_mirrored[i] = *(int *)&rawData[i * 3];
            texbits_rot180[i] = *(int *)&rawData[i * 3];
        }
        bmp = (HBITMAP)0xcdcdcdcd;
    }
    LeaveCriticalSection(&imageload);
}

struct RECTf {
    double left;
    double top;
    double right;
    double bottom;
};


void C_Texer2::DrawParticle(int *framebuffer, int *texture, int w, int h, double x, double y, double sizex, double sizey, unsigned int color) {
    // Adjust width/height
    --w;
    --h;
    --(this->iw); // member vars, restore later!
    --(this->ih);

    // Texture Coordinates
    double x0 = 0.0;
    double y0 = 0.0;

/***************************************************************************/
/***************************************************************************/
/*   Scaling renderer                                                      */
/***************************************************************************/
/***************************************************************************/
    if (config.resize) {
        RECTf r;
        // Determine area rectangle,
        // correct with half pixel for correct pixel coverage
        r.left = -iw*.5*sizex + 0.5 + (x *.5 + .5)*w;
        r.top = -ih*.5*sizey + 0.5 + (y *.5 + .5)*h;
        r.right = (iw-1)*.5*sizex + 0.5 + (x *.5 + .5)*w;
        r.bottom = (ih-1)*.5*sizey + 0.5 + (y *.5 + .5)*h;

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
            x0 = - r.left / (r.right - r.left);
            r2.left = 0;
        }
        if (r.top < 0.0f) {
            y0 = - r.top / (r.bottom - r.top);
            r2.top = 0;
        }
        if (r.right > w) {
            r2.right = w;
        }
        if (r.bottom > h) {
            r2.bottom = h;
        }

        {
            double fx0 = x0*iw;
            double fy0 = y0*ih;

            int cx0 = RoundToInt(fx0);
            int cy0 = RoundToInt(fy0);

            // fixed point fractional part of first coordinate
            int dx = 65535 - FloorToInt((.5f-(fx0 - cx0))*65536.0);
            int dy = 65535 - FloorToInt((.5f-(fy0 - cy0))*65536.0);

            // 'texel per pixel' steps
            double scx = (iw-1) / (r.right - r.left + 1);
            double scy = (ih-1) / (r.bottom - r.top + 1);
            int sdx = FloorToInt(scx*65536.0);
            int sdy = FloorToInt(scy*65536.0);

            // fixed point corrected coordinate
            cx0 = (cx0<<16) + dx;
            cy0 = (cy0<<16) + dy;

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

            int imagewidth = iw;

            // Prepare filter color
            T2_PREP_MASK_COLOR

            switch (*(g_extinfo->lineblendmode) & 0xFF) {
                case OUT_REPLACE:
                {
                    __int64 mmxxor = 0x00FF00FF00FF00FF;
                    __int64 *p = &mmxxor;
                    int tot = r2.right - r2.left;
                    int *outp = &framebuffer[r2.top*(w+1)+r2.left];
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
                        outp += w+1;
                    }
                    break;
                }

                case OUT_ADDITIVE:
                {
                    __int64 mmxxor = 0x00FF00FF00FF00FF;
                    __int64 *p = &mmxxor;
                    int tot = r2.right - r2.left;
                    int *outp = &framebuffer[r2.top*(w+1)+r2.left];
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
                        outp += w+1;
                    }
                    break;
                }

                case OUT_MAXIMUM:
                {
                    int maxmask = 0xFFFFFF;
                    int signmask = 0x808080;
                    T2_SCALE_MINMAX_SIGNMASK
                    __int64 mmxxor = 0x00FF00FF00FF00FF;
                    __int64 *p = &mmxxor;
                    int tot = r2.right - r2.left;
                    int *outp = &framebuffer[r2.top*(w+1)+r2.left];
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
                        outp += w+1;
                    }
                    break;
                }

                case OUT_5050:
                {
                    __int64 mmxxor = 0x00FF00FF00FF00FF;
                    __int64 *p = &mmxxor;
                    int tot = r2.right - r2.left;
                    int *outp = &framebuffer[r2.top*(w+1)+r2.left];
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
                        outp += w+1;
                    }
                    break;
                }

                case OUT_SUB1:
                {
                    __int64 mmxxor = 0x00FF00FF00FF00FF;
                    __int64 *p = &mmxxor;
                    int tot = r2.right - r2.left;
                    int *outp = &framebuffer[r2.top*(w+1)+r2.left];
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        T2_SCALE_BLEND_ASM_ENTER(t2_scale_loop_sub1)
#ifdef _MSC_VER
                        __asm {
                            psrlw mm0, 8
                            packuswb mm0, mm0

                            // save
                            movd mm1, dword ptr [edi]
                            psubusb mm1, mm0
                        }
#else  // GCC
                            "psrlw mm0, 8\n\t"
                            "packuswb mm0, mm0\n\t"

                            // save
                            "movd mm1, dword ptr [edi]\n\t"
                            "psubusb mm1, mm0\n\t"
#endif
                        T2_SCALE_BLEND_ASM_LEAVE(t2_scale_loop_sub1)
                        cy0 += sdy;
                        outp += w+1;
                    }
                    break;
                }

                case OUT_SUB2:
                {
                    __int64 mmxxor = 0x00FF00FF00FF00FF;
                    __int64 *p = &mmxxor;
                    int tot = r2.right - r2.left;
                    int *outp = &framebuffer[r2.top*(w+1)+r2.left];
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
                        outp += w+1;
                    }
                    break;
                }

                case OUT_MULTIPLY:
                {
                    __int64 mmxxor = 0x00FF00FF00FF00FF;
                    __int64 *p = &mmxxor;
                    int tot = r2.right - r2.left;
                    int *outp = &framebuffer[r2.top*(w+1)+r2.left];
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
                        outp += w+1;
                    }
                    break;
                }

                case OUT_ADJUSTABLE:
                {
                    __int64 alphavalue = 0x0;
                    __int64 *alpha = &alphavalue;
                    // TODO [bugfix]: shouldn't salpha be 255, not 256?
                    // it's used to calculate 256 - alpha, and if max_alpha = 255, then...
                    __int64 salpha = 0x0100010001000100;
                    int t = ((*g_extinfo->lineblendmode) & 0xFF00)>>8;
                    T2_SCALE_BLEND_AND_STORE_ALPHA
                    __int64 mmxxor = 0x00FF00FF00FF00FF;
                    __int64 *p = &mmxxor;
                    int tot = r2.right - r2.left;
                    int *outp = &framebuffer[r2.top*(w+1)+r2.left];
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
                        outp += w+1;
                    }
                    break;
                }

                case OUT_XOR:
                {
                    __int64 mmxxor = 0x00FF00FF00FF00FF;
                    __int64 *p = &mmxxor;
                    int tot = r2.right - r2.left;
                    int *outp = &framebuffer[r2.top*(w+1)+r2.left];
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        T2_SCALE_BLEND_ASM_ENTER(t2_scale_loop_xor)
#ifdef _MSC_VER
                        __asm {
                            psrlw mm0, 8

                            // save
                            pxor mm0, qword ptr [edi]
                        }
#else  // GCC
                            "psrlw  %%mm0, 8\n\t"

                            // save
                            "pxor   %%mm0, qword ptr [%%edi]\n\t"
#endif
                        T2_SCALE_BLEND_ASM_LEAVE(t2_scale_loop_xor)
                        cy0 += sdy;
                        outp += w+1;
                    }
                    break;
                }

                case OUT_MINIMUM:
                {
                    int maxmask = 0xFFFFFF;
                    int signmask = 0x808080;
                    __int64 mmxxor = 0x00FF00FF00FF00FF;
                    __int64 *p = &mmxxor;
                    int tot = r2.right - r2.left;
                    int *outp = &framebuffer[r2.top*(w+1)+r2.left];
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
                        outp += w+1;
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
        r.left = (RoundToInt((x *.5f + .5f)*w) - iw/2);
        r.top = (RoundToInt((y *.5f + .5f)*h) - ih/2);
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
            x0 = - ((double)r.left) / (double)(r.right - r.left);
            r2.left = 0;
        }
        if (r.top < 0) {
            y0 = - ((double)r.top) / (double)(r.bottom - r.top);
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

        int cx0 = RoundToInt(x0*iw);
        int cy0 = RoundToInt(y0*ih);

        int ty = cy0;

        if (config.mask) {
            // Second easiest path, masking, but no scaling
            T2_PREP_MASK_COLOR
            switch (*(g_extinfo->lineblendmode) & 0xFF) {
                case OUT_REPLACE:
                {
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = &texture[ty*(iw+1)+cx0];
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

                case OUT_ADDITIVE:
                {
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = &texture[ty*(iw+1)+cx0];
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

                case OUT_MAXIMUM:
                {
                    int maxmask = 0xFFFFFF;  // -> mm4
                    int signmask = 0x808080;  // -> mm6
                    T2_NONSCALE_PUSH_ESI_EDI
                    T2_NONSCALE_MINMAX_MASKS
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = &texture[ty*(iw+1)+cx0];
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

                case OUT_5050:
                {
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = &texture[ty*(iw+1)+cx0];
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

                case OUT_SUB1:
                {
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = &texture[ty*(iw+1)+cx0];
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

                case OUT_SUB2:
                {
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = &texture[ty*(iw+1)+cx0];
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

                case OUT_MULTIPLY:
                {
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = &texture[ty*(iw+1)+cx0];
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

                case OUT_ADJUSTABLE:
                {
                    __int64 alphavalue = 0x0;
                    __int64 *alpha = &alphavalue;
                    // TODO [bugfix]: shouldn't salpha be 255, not 256?
                    // it's used to calculate 256 - alpha, and if max_alpha = 255, then...
                    __int64 salpha = 0x0100010001000100;
                    int t = ((*g_extinfo->lineblendmode) & 0xFF00)>>8;
                    T2_NONSCALE_BLEND_AND_STORE_ALPHA
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = &texture[ty*(iw+1)+cx0];
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

                            pmullw    mm0, mm6; // * alph
                            pmullw    mm1, mm3; // * 256 - alph

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

                case OUT_XOR:
                {
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = &texture[ty*(iw+1)+cx0];
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

                case OUT_MINIMUM:
                {
                    int maxmask = 0xFFFFFF;
                    int signmask = 0x808080;
                    T2_NONSCALE_PUSH_ESI_EDI
                    T2_NONSCALE_MINMAX_MASKS
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = &texture[ty*(iw+1)+cx0];
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
                case OUT_REPLACE:
                {
                    // the push order was reversed (edi, esi) in the original code here
                    // (and only here) -- i assume that was not intentional...
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = &texture[ty*(iw+1)+cx0];
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

                case OUT_ADDITIVE:
                {
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = &texture[ty*(iw+1)+cx0];
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

                case OUT_MAXIMUM:
                {
                    int maxmask = 0xFFFFFF;
                    int signmask = 0x808080;
                    T2_NONSCALE_PUSH_ESI_EDI
                    T2_NONSCALE_MINMAX_MASKS
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = &texture[ty*(iw+1)+cx0];
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

                case OUT_5050:
                {
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = &texture[ty*(iw+1)+cx0];
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

                case OUT_SUB1:
                {
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = &texture[ty*(iw+1)+cx0];
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

                case OUT_SUB2:
                {
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = &texture[ty*(iw+1)+cx0];
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

                case OUT_MULTIPLY:
                {
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = &texture[ty*(iw+1)+cx0];
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

                case OUT_ADJUSTABLE:
                {
                    __int64 alphavalue = 0x0;
                    __int64 *alpha = &alphavalue;
                    // TODO [bugfix]: shouldn't salpha be 255, not 256?
                    // it's used to calculate 256 - alpha, and if max_alpha = 255, then...
                    __int64 salpha = 0x0100010001000100;
                    int t = ((*g_extinfo->lineblendmode) & 0xFF00)>>8;
                    T2_NONSCALE_BLEND_AND_STORE_ALPHA
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = &texture[ty*(iw+1)+cx0];
                        int tot = (r2.right - r2.left);
                        T2_NONSCALE_BLEND_ASM_ENTER(t2_nonscale_nonmask_loop_adj)
#ifdef _MSC_VER
                        __asm {
                            movd mm0, dword ptr [edi]
                            movd mm1, dword ptr [esi]

                            punpcklbw mm0, mm5
                            punpcklbw mm1, mm5

                            pmullw    mm0, mm3; // * alph
                            pmullw    mm1, mm6; // * 256 - alph

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

                case OUT_XOR:
                {
                    T2_NONSCALE_PUSH_ESI_EDI
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = &texture[ty*(iw+1)+cx0];
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

                case OUT_MINIMUM:
                {
                    int maxmask = 0xFFFFFF;
                    int signmask = 0x808080;
                    T2_NONSCALE_PUSH_ESI_EDI
                    T2_NONSCALE_MINMAX_MASKS
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = &texture[ty*(iw+1)+cx0];
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
    ++(this->iw); // restore member vars!
    ++(this->ih);

    // Perhaps removing all fpu stuff here and doing one emms globally would be better?
    EMMS
}

inline double wrap_diff_to_plusminus1(double x) {
    return round(x / 2.0) * 2.0;
}

/* For AVS 2.81d compatibility. TexerII v1.0 only wrapped once. */
inline double wrap_once_diff_to_plusminus1(double x) {
    return (x > 1.0) ? +2.0 : (x < -1.0 ? -2.0 : 0.0);
}

bool overlaps_edge(double wrapped_coord, double img_size, int img_size_px, int screen_size_px) {
    double abs_wrapped_coord = fabs(wrapped_coord);
    // a /2 seems missing, but screen has size 2, hence /2(half) *2(screen size) => *1
    // image pixel size needs reduction by one pixel
    double rel_size_half = img_size * (img_size_px - 1) / screen_size_px;
    return ((abs_wrapped_coord + rel_size_half) > 1.0) && ((abs_wrapped_coord - rel_size_half) < 1.0);
}

int C_Texer2::render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h)
{
    EnterCriticalSection(&codestuff);

    *vars.w = w;
    *vars.h = h;

    if (((isBeat & 0x80000000) || (init)) && (codeinit)) {
        g_extinfo->executeCode(codeinit, visdata);
        init = false;
    }

    *vars.red = 1.0;
    *vars.green = 1.0;
    *vars.blue = 1.0;
    *vars.sizex = 1.0;
    *vars.sizey = 1.0;

    if (codeframe)
        g_extinfo->executeCode(codeframe, visdata);

    if ((isBeat & 0x00000001) && (codebeat)) {
        g_extinfo->executeCode(codebeat, visdata);
    }

    int n = RoundToInt((double)*vars.n);
    n = max(0, min(65536, n));

    EnterCriticalSection(&imageload);
    if (n) {
        double step = 1.0/(n-1);
        double i = 0.0;
        for (int j = 0; j < n; ++j) {
            *vars.i = i;
            *vars.skip = 0.0;
            *vars.v = ((int)visdata[1][0][j * 575 / n] + (int)visdata[1][1][j * 575 / n]) / 256.0;
            i += step;

            g_extinfo->executeCode(codepoint, visdata);

            // TODO [cleanup]: invert to if+continue
            if (*vars.skip == 0.0) {
                unsigned int color = min(255, max(0, RoundToInt(255.0f*(double)*vars.blue)));
                color |= min(255, max(0, RoundToInt(255.0f*(double)*vars.green))) << 8;
                color |= min(255, max(0, RoundToInt(255.0f*(double)*vars.red))) << 16;

                if (config.mask == 0) color = 0xFFFFFF;

                int* texture = this->texbits_normal;
                if (*vars.sizex < 0 && *vars.sizey > 0) {
                    texture = this->texbits_mirrored;
                } else if(*vars.sizex > 0 && *vars.sizey < 0) {
                    texture = this->texbits_flipped;
                } else if(*vars.sizex < 0 && *vars.sizey < 0) {
                    texture = this->texbits_rot180;
                }

                double szx = (double)*vars.sizex;
                szx = fabs(szx);
                double szy = (double)*vars.sizey;
                szy = fabs(szy);

                // TODO [cleanup]: put more of the above into this if-case, or invert to if+continue
                // TODO [bugfix]: really large images would be clipped while still potentially visible, should be relative to image size
                if ((szx > .01) && (szy > .01)) {
                    double x = *vars.x;
                    double y = *vars.y;
                    if (config.wrap != 0) {
                        bool overlaps_x, overlaps_y;
                        if(this->config.version == TEXER_II_VERSION_V2_81D) {
                            /* Wrap only once (when crossing +/-1, not when crossing +/-3 etc. */
                            x -= wrap_once_diff_to_plusminus1(*vars.x);
                            y -= wrap_once_diff_to_plusminus1(*vars.y);
                            overlaps_x = overlaps_edge(*vars.x, szx, this->iw, w);
                            overlaps_y = overlaps_edge(*vars.y, szy, this->ih, h);
                        } else {
                            x = *vars.x - wrap_diff_to_plusminus1(*vars.x);
                            y = *vars.y - wrap_diff_to_plusminus1(*vars.y);
                            overlaps_x = overlaps_edge(x, szx, this->iw, w);
                            overlaps_y = overlaps_edge(y, szy, this->ih, h);
                        }
                        double sign_x2 = x > 0 ? 2.0 : -2.0;
                        double sign_y2 = y > 0 ? 2.0 : -2.0;
                        if (overlaps_x) {
                            DrawParticle(framebuffer, texture, w, h, x - sign_x2, y, szx, szy, color);
                        }
                        if (overlaps_y) {
                            DrawParticle(framebuffer, texture, w, h, x, y - sign_y2, szx, szy, color);
                        }
                        if (overlaps_x && overlaps_y) {
                            DrawParticle(framebuffer, texture, w, h, x - sign_x2, y - sign_y2, szx, szy, color);
                        }
                    }
                    DrawParticle(framebuffer, texture, w, h, x, y, szx, szy, color);
                }
            }
        }
    }

    LeaveCriticalSection(&codestuff);
    LeaveCriticalSection(&imageload);
    return 0;
}

HWND C_Texer2::conf(HINSTANCE hInstance, HWND hwndParent)
{
    g_ConfigThis = this;
    return CreateDialog(hInstance, MAKEINTRESOURCE(IDD_CFG_TEXERII), hwndParent, (DLGPROC)g_DlgProc);
}


char *C_Texer2::get_desc(void)
{
    return MOD_NAME;
}

void C_Texer2::load_config(unsigned char *data, int len)
{
    if (len >= sizeof(texer2_apeconfig))
        memcpy(&this->config, data, sizeof(texer2_apeconfig));
    if (this->config.version < TEXER_II_VERSION_V2_81D
        || this->config.version > TEXER_II_VERSION_CURRENT) {
        // If the version value is not in the known set, assume an old preset version.
        this->config.version = TEXER_II_VERSION_V2_81D;
    }

    unsigned char *p = &data[sizeof(texer2_apeconfig)];
    char *buf;

    // Check size
    if ((p + 4 - data) >= len) return;

    // Init
    len = max(0, *(int *)p);
    p+=4;
    buf = new char[len+1];
    if (len)
        strncpy(buf, (const char *)p, len);
    p+=len;
    buf[len] = 0;
    code.SetInit(buf);

    // Frame
    len = max(0, *(int *)p);
    p+=4;
    buf = new char[len+1];
    if (len)
        strncpy(buf, (const char *)p, len);
    p+=len;
    buf[len] = 0;
    code.SetFrame(buf);

    // Beat
    len = max(0, *(int *)p);
    p+=4;
    buf = new char[len+1];
    if (len)
        strncpy(buf, (const char *)p, len);
    p+=len;
    buf[len] = 0;
    code.SetBeat(buf);

    // Point
    len = max(0, *(int *)p);
    p+=4;
    buf = new char[len+1];
    if (len)
        strncpy(buf, (const char *)p, len);
    p+=len;
    buf[len] = 0;
    code.SetPoint(buf);

    Recompile();
    InitTexture();
}


int  C_Texer2::save_config(unsigned char *data)
{
    memcpy(data, &this->config, sizeof(texer2_apeconfig));
    int l = 0, size = sizeof(texer2_apeconfig);
    char *p = (char *)(data+sizeof(texer2_apeconfig));
    int tot = 16;

    // Init
    l = strlen(code.init);
    tot += l;
    *((int *)p) = l;
    p+=4;
    strncpy(p, code.init, l);
    p+=l;

    // Frame
    l = strlen(code.frame);
    tot += l;
    *((int *)p) = l;
    p+=4;
    strncpy(p, code.frame, l);
    p+=l;

    // Beat
    l = strlen(code.beat);
    tot += l;
    *((int *)p) = l;
    p+=4;
    strncpy(p, code.beat, l);
    p+=l;

    // Point
    l = strlen(code.point);
    tot += l;
    *((int *)p) = l;
    p+=4;
    strncpy(p, code.point, l);
    p+=l;

    return sizeof(texer2_apeconfig) + tot;
}


/* APE interface */
C_RBASE *R_Texer2(char *desc)
{
    if (desc) {
        strcpy(desc,MOD_NAME);
        return NULL;
    }
    return (C_RBASE *) new C_Texer2(TEXER_II_VERSION_CURRENT);
}

void R_Texer2_SetExtInfo(APEinfo *ptr) {
    g_extinfo = ptr;
}
