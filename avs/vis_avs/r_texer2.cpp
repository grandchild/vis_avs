#define WINVER 0x5000
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include "r_texer2.h"
#include "r_texer2_resource.h"

class C_THISCLASS : public C_RBASE 
{
    protected:
    public:
        C_THISCLASS();
        virtual ~C_THISCLASS();
        virtual int render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h);        
        virtual HWND conf(HINSTANCE hInstance, HWND hwndParent);
        virtual char *get_desc();
        virtual void load_config(unsigned char *data, int len);
        virtual int  save_config(unsigned char *data);

        virtual void InitTexture();
        virtual void DeleteTexture();

        virtual void Recompile();
        virtual void Allocate(int n);

        virtual void DrawParticle(int *framebuffer, int w, int h, double x, double y, double sizex, double sizey, unsigned int color);

//      Particle *particles;
//      int npart;

        apeconfig config;

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
        int *texbits;
        bool init;

        CRITICAL_SECTION imageload;
        CRITICAL_SECTION codestuff;
};

// extended APE api support
APEinfo *g_extinfo = 0;
extern "C"
{
  void __declspec(dllexport) _AVS_APE_SetExtInfo(HINSTANCE hDllInstance, APEinfo *ptr)
  {
    g_extinfo = ptr;
  }
}

// global configuration dialog pointer 
static C_THISCLASS *g_ConfigThis; 
static HINSTANCE g_hDllInstance; 

typedef LRESULT CALLBACK WINDOWPROC(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK URLProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    WINDOWPROC *RealProc;
    char buffer[32];
    static int hovered = 0;

    RealProc = (WINDOWPROC *)GetWindowLong(hwnd, GWL_USERDATA);

    switch (uMsg) {
        case WM_TIMER:
        {
            POINT p;
            RECT r;
            GetCursorPos(&p);
            ScreenToClient(hwnd, &p);
            GetClientRect(hwnd, &r);
            if ((p.x < 0) || (p.x >= r.right) || (p.y < 0) || (p.y >= r.bottom)) {
                KillTimer(hwnd, 1);
                hovered = 0;
                InvalidateRect(hwnd, NULL, FALSE);
                UpdateWindow(hwnd);
                HCURSOR c = LoadCursor(NULL, IDC_ARROW);
                SetCursor(c);
            }
        }
        return 1;

        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        {
            HCURSOR c = LoadCursor(NULL, IDC_HAND);
            SetCursor(c);
        }
        return 1;
        
        case WM_LBUTTONDOWN:
        {
            HCURSOR c = LoadCursor(NULL, IDC_HAND);
            SetCursor(c);
            ShellExecute(NULL, "open", "http://avs.acko.net/", NULL, "", SW_SHOW);
        }
        return 1;

        case WM_MOUSEMOVE:
        {
            hovered = 1;
            HCURSOR c = LoadCursor(NULL, IDC_HAND);
            SetCursor(c);
            SetTimer(hwnd, 1, 50, NULL);
            InvalidateRect(hwnd, NULL, TRUE);
            UpdateWindow(hwnd);
        }
        return 1;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;
            RECT r;
            HFONT font;
            SIZE size;
            HPEN pen, penold;
            HBRUSH br;
        
            hdc = BeginPaint(hwnd, &ps);

            GetWindowText(hwnd, buffer, 32);
            
            GetClientRect(hwnd, &r);
            br = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
            FillRect(hdc, &r, br);
            DeleteObject(br);

            font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            SelectObject(hdc, font);
            SetTextColor(hdc, (COLORREF)0xFF0000);
            SetBkMode(hdc, TRANSPARENT);
            DrawText(hdc, buffer, strlen(buffer), &r, DT_LEFT);

            if (hovered) {
                GetTextExtentPoint32(hdc, buffer, strlen(buffer), &size);
                pen = CreatePen(PS_SOLID, 1, (COLORREF)0xFF0000);
                penold = (HPEN)SelectObject(hdc, pen);
                MoveToEx(hdc, 1, size.cy-1, NULL);
                LineTo(hdc, size.cx+1, size.cy-1);
                SelectObject(hdc, penold);
                DeleteObject(pen);
            }

            EndPaint(hwnd, &ps);
        }
        return 1;
    }
    return RealProc(hwnd, uMsg, wParam, lParam);
}

#define ID_EX_1  31337
void DoExamples(HWND ctl) {
    RECT r;
    GetWindowRect(ctl, &r);

    HMENU m = CreatePopupMenu();
    AppendMenu(m, MF_STRING, ID_EX_1, "(no examples)");
    
    int ret = TrackPopupMenu(m, TPM_RETURNCMD, r.left+1, r.bottom+1, 0, ctl, 0);

    if (ret == ID_EX_1) {
    }
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
                case IDC_TEXTURE:
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
                g_ConfigThis->config.bilinear = IsDlgButtonChecked(hwndDlg, IDC_OBILINEAR) == BST_CHECKED;
                g_ConfigThis->config.resize = IsDlgButtonChecked(hwndDlg, IDC_ORESIZE) == BST_CHECKED;
                g_ConfigThis->config.mask = IsDlgButtonChecked(hwndDlg, IDC_OMASK) == BST_CHECKED;

                if (LOWORD(wParam) == IDC_ABOUT) {
                    MessageBox(hwndDlg,
                        "Texer II works like a dot-superscope, except it draws a bitmap instead of a dot at each location.\n\nVariables: n, i, x, y, w, h, v, sizex, sizey, red, green, blue.\n\n", "Texer II", MB_OK);
                } else if (LOWORD(wParam) == IDC_EXAMPLE) {
                    DoExamples(GetDlgItem(hwndDlg, IDC_EXAMPLE));
                }
            } else if (wNotifyCode == EN_CHANGE) {
                char *buf;
                int l = GetWindowTextLength(h);
                buf = new char[l+1];
                GetWindowText(h, buf, l+1);

                switch (LOWORD(wParam)) {
                case IDC_CINIT:
                    g_ConfigThis->code.SetInit(buf);
                    g_ConfigThis->Recompile();
                    break;
                case IDC_CFRAME:
                    g_ConfigThis->code.SetFrame(buf);
                    g_ConfigThis->Recompile();
                    break;
                case IDC_CBEAT:
                    g_ConfigThis->code.SetBeat(buf);
                    g_ConfigThis->Recompile();
                    break;
                case IDC_CPOINT:
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

                GetModuleFileName(g_hDllInstance, buf, MAX_PATH);
                strcpy(strrchr(buf, '\\') + 1, "*.bmp");

                bool found = false;
                SendDlgItemMessage(hwndDlg, IDC_TEXTURE, CB_ADDSTRING, 0, (LPARAM)"(default image)");
                h = FindFirstFile(buf, &wfd);
                if (h != INVALID_HANDLE_VALUE) {
                    bool rep = true;
                    while (rep) {
                        int p = SendDlgItemMessage(hwndDlg, IDC_TEXTURE, CB_ADDSTRING, 0, (LPARAM)wfd.cFileName);
                        if (stricmp(wfd.cFileName, g_ConfigThis->config.img) == 0) {
                            SendDlgItemMessage(hwndDlg, IDC_TEXTURE, CB_SETCURSEL, p, 0);
                            found = true;
                        }
                        if (!FindNextFile(h, &wfd)) {
                            rep = false;
                        }
                    };
                    FindClose(h);
                }
                if (!found)
                    SendDlgItemMessage(hwndDlg, IDC_TEXTURE, CB_SETCURSEL, 0, 0);
            }

            {
                long gwl;
                HWND url = GetDlgItem(hwndDlg, IDC_URL);
                gwl = GetWindowLong(url, GWL_WNDPROC);
                SetWindowLong(url, GWL_USERDATA, gwl);
                SetWindowLong(url, GWL_WNDPROC, (long)URLProc);
            }

            SetDlgItemText(hwndDlg, IDC_CINIT, g_ConfigThis->code.init);
            SetDlgItemText(hwndDlg, IDC_CFRAME, g_ConfigThis->code.frame);
            SetDlgItemText(hwndDlg, IDC_CBEAT, g_ConfigThis->code.beat);
            SetDlgItemText(hwndDlg, IDC_CPOINT, g_ConfigThis->code.point);

            CheckDlgButton(hwndDlg, IDC_OBILINEAR, g_ConfigThis->config.bilinear ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwndDlg, IDC_OMASK, g_ConfigThis->config.mask ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwndDlg, IDC_ORESIZE, g_ConfigThis->config.resize ? BST_CHECKED : BST_UNCHECKED);

            return 1;

        case WM_DESTROY:
            return 1;
    }
    return 0;
}

void C_THISCLASS::Allocate(int n) {
/*  if (n <= npart) return;
    Particle *p = new Particle[n];
    if (npart > 0) {
        memcpy(p, particles, npart*sizeof(Particle));
    }
    memset(p + npart, 0, sizeof(Particle)*(n - npart));
    if (particles) {
        delete particles;
    }
    particles = p;*/
}

void C_THISCLASS::Recompile() {
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
C_THISCLASS::C_THISCLASS() 
{
    memset(&config, 0, sizeof(apeconfig));
    hwndDlg = 0;
    bmp = 0;
    iw = 0;
    ih = 0;
    texbits = 0;
    init = true;
//  npart = 0;
//  particles = 0;

    char DEVMSG[] = "/* This a development alpha version.\r\nDo not distribute */";
    char *DEVVER = new char[strlen(DEVMSG)+1];
    strcpy(DEVVER, DEVMSG);
    code.SetInit(DEVVER);

    Allocate(10);

    InitializeCriticalSection(&imageload);
    InitializeCriticalSection(&codestuff);

    if (g_extinfo) {
        context = g_extinfo->allocVM();
        codeinit = codeframe = codebeat = codepoint = 0;
        Recompile();
    }
}

// virtual destructor
C_THISCLASS::~C_THISCLASS() 
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

void C_THISCLASS::DeleteTexture()
{
    EnterCriticalSection(&imageload);
    if (bmp) {
        SelectObject(bmpdc, bmpold);
        DeleteObject(bmp);
        DeleteDC(bmpdc);
        iw = ih = 0;
        delete texbits;
    }
    bmp = 0;
    LeaveCriticalSection(&imageload);
}

extern unsigned char rawData[1323]; // example pic
void C_THISCLASS::InitTexture()
{
    EnterCriticalSection(&imageload);
    if (bmp)
        DeleteTexture();
    bool loaddefault = false;
    if (strlen(config.img)) {
        char buf[MAX_PATH];
        
        GetModuleFileName(g_hDllInstance, buf, MAX_PATH);
        strcpy(strrchr(buf, '\\') + 1, config.img);

        HANDLE f = CreateFile(buf, 0, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
        if (f != INVALID_HANDLE_VALUE) {
            CloseHandle(f);
            bmp = (HBITMAP)LoadImage(0, buf, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE|LR_CREATEDIBSECTION);

            DIBSECTION dib;
            BITMAPINFO bi;

            GetObject(bmp, sizeof(dib), &dib);
            
            iw = dib.dsBmih.biWidth;
            ih = dib.dsBmih.biHeight;

            bmpdc = CreateCompatibleDC(NULL);
            bmpold = (HBITMAP)SelectObject(bmpdc, bmp);

            bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bi.bmiHeader.biWidth = iw;
            bi.bmiHeader.biHeight = -ih;
            bi.bmiHeader.biPlanes = 1;
            bi.bmiHeader.biBitCount = 32;
            bi.bmiHeader.biCompression = BI_RGB;
            bi.bmiHeader.biSizeImage = iw*ih*4;
            bi.bmiHeader.biXPelsPerMeter = 0;
            bi.bmiHeader.biYPelsPerMeter = 0;
            bi.bmiHeader.biClrUsed = 16777215;
            bi.bmiHeader.biClrImportant = 16777215;
            
            texbits = (int *)new unsigned char[bi.bmiHeader.biSizeImage];
            GetDIBits(bmpdc, bmp, 0, ih, texbits, &bi, DIB_RGB_COLORS);
        } else {
            loaddefault = true;
        }
    } else {
            loaddefault = true;
    }
    if (loaddefault) {
        iw = 21;
        ih = 21;
        texbits = (int *)new unsigned char[iw*ih*4];
        for (int i = 0; i < iw*ih; ++i) {
            texbits[i] = *(int *)&rawData[i*3];
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

void C_THISCLASS::DrawParticle(int *framebuffer, int w, int h, double x, double y, double sizex, double sizey, unsigned int color) {
    config.bilinear = 1;

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
        r2.left = DoubleToInt(r.left);
        r2.top = DoubleToInt(r.top);
        r2.right = DoubleToInt(r.right);
        r2.bottom = DoubleToInt(r.bottom);

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

            int cx0 = DoubleToInt(fx0);
            int cy0 = DoubleToInt(fy0);

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
            int texdata = *(int *)&texbits;

            // Prepare filter color
            __asm {
                pxor mm5, mm5;
                movd mm7, color;
                punpcklbw mm7, mm5;
            }

            switch (*(g_extinfo->lineblendmode) & 0xFF) {
                case OUT_REPLACE:
                {
                    __int64 mmxxor = 0x00FF00FF00FF00FF;
                    __int64 *p = &mmxxor;
                    int tot = r2.right - r2.left;
                    int *outp = &framebuffer[r2.top*(w+1)+r2.left];
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        __asm {
                            push esi;
                            push edi;
                            push edx;
                            // esi = texture width
                            mov esi, imagewidth;
                            inc esi;
                            pxor mm5, mm5;
                        }
                        __asm {
                            // bilinear
                            push ebx;

                            pxor mm5, mm5;

                            // calculate dy coefficient for this scanline
                            // store in mm4 = dy cloned into all bytes
                            movd mm4, cy0;
                            psrlw mm4, 8;
                            punpcklwd mm4, mm4;
                            punpckldq mm4, mm4;

                            // loop counter
                            mov ecx, tot;
                            inc ecx;

                            // set output pointer
                            mov edi, outp;

                            // beginning x tex coordinate
                            mov ebx, cx0;

                            // calculate y combined address for first point a for this scanline
                            // store in ebp = texture start address for this scanline
                            mov eax, cy0;
                            shr eax, 16;
                            imul eax, esi;
                            shl eax, 2;
                            mov edx, texdata;
                            add edx, eax;

                            // begin loop
                            p0looprep:

                            // calculate dx, fractional part of tex coord
                            movd mm3, ebx;
                            psrlw mm3, 8;
                            punpcklwd mm3, mm3;
                            punpckldq mm3, mm3;

                            // convert fixed point into floor and load pixel address
                            mov eax, ebx;
                            shr eax, 16;
                            lea eax, dword ptr [edx+eax*4];

                            // mm0 = b*dx
                            movd mm0, dword ptr [eax+4]; //b
                            punpcklbw mm0, mm5;
                            pmullw mm0, mm3;
                            psrlw mm0, 8;
                            // mm1 = a*(1-dx)
                            movd mm1, dword ptr [eax]; //a
                            pxor mm3, mmxxor;
                            punpcklbw mm1, mm5;
                            pmullw mm1, mm3;
                            psrlw mm1, 8;
                            paddw mm0, mm1;

                            // mm1 = c*(1-dx)
                            movd mm1, dword ptr [eax+esi*4]; //c
                            punpcklbw mm1, mm5;
                            pmullw mm1, mm3;
                            psrlw mm1, 8;
                            // mm2 = d*dx
                            movd mm2, dword ptr [eax+esi*4+4]; //d
                            pxor mm3, mmxxor;
                            punpcklbw mm2, mm5;
                            pmullw mm2, mm3;
                            psrlw mm2, 8;
                            paddw mm1, mm2;

                            // combine
                            pmullw mm1, mm4;
                            psrlw mm1, 8;
                            pxor mm4, mmxxor;
                            pmullw mm0, mm4;
                            psrlw mm0, 8;
                            paddw mm0, mm1;
                            pxor mm4, mmxxor;

                            // filter color
                            // (already unpacked) punpcklbw mm0, mm5;
                            pmullw mm0, mm7;
                            psrlw mm0, 8;
                            packuswb mm0, mm0;

                            // save
                            movd dword ptr [edi], mm0;
                            add edi, 4;
                            
                            // advance tex coords, cx += sdx;
                            add ebx, sdx;

                            dec ecx;
                            jnz p0looprep;

                            pop ebx;
                        }
                        __asm {
                            pop edx;
                            pop edi;
                            pop esi;
                        }
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
                        __asm {
                            push esi;
                            push edi;
                            push edx;
                            // esi = texture width
                            mov esi, imagewidth;
                            inc esi;
                            pxor mm5, mm5;
                        }
                        __asm {
                            // bilinear
                            push ebx;

                            pxor mm5, mm5;

                            // calculate dy coefficient for this scanline
                            // store in mm4 = dy cloned into all bytes
                            movd mm4, cy0;
                            psrlw mm4, 8;
                            punpcklwd mm4, mm4;
                            punpckldq mm4, mm4;

                            // loop counter
                            mov ecx, tot;
                            inc ecx;

                            // set output pointer
                            mov edi, outp;

                            // beginning x tex coordinate
                            mov ebx, cx0;

                            // calculate y combined address for first point a for this scanline
                            // store in ebp = texture start address for this scanline
                            mov eax, cy0;
                            shr eax, 16;
                            imul eax, esi;
                            shl eax, 2;
                            mov edx, texdata;
                            add edx, eax;

                            // begin loop
                            p0loopadd:

                            // calculate dx, fractional part of tex coord
                            movd mm3, ebx;
                            psrlw mm3, 8;
                            punpcklwd mm3, mm3;
                            punpckldq mm3, mm3;

                            // convert fixed point into floor and load pixel address
                            mov eax, ebx;
                            shr eax, 16;
                            lea eax, dword ptr [edx+eax*4];

                            // mm0 = b*dx
                            movd mm0, dword ptr [eax+4]; //b
                            punpcklbw mm0, mm5;
                            pmullw mm0, mm3;
                            psrlw mm0, 8;
                            // mm1 = a*(1-dx)
                            movd mm1, dword ptr [eax]; //a
                            pxor mm3, mmxxor;
                            punpcklbw mm1, mm5;
                            pmullw mm1, mm3;
                            psrlw mm1, 8;
                            paddw mm0, mm1;

                            // mm1 = c*(1-dx)
                            movd mm1, dword ptr [eax+esi*4]; //c
                            punpcklbw mm1, mm5;
                            pmullw mm1, mm3;
                            psrlw mm1, 8;
                            // mm2 = d*dx
                            movd mm2, dword ptr [eax+esi*4+4]; //d
                            pxor mm3, mmxxor;
                            punpcklbw mm2, mm5;
                            pmullw mm2, mm3;
                            psrlw mm2, 8;
                            paddw mm1, mm2;

                            // combine
                            pmullw mm1, mm4;
                            psrlw mm1, 8;
                            pxor mm4, mmxxor;
                            pmullw mm0, mm4;
                            psrlw mm0, 8;
                            paddw mm0, mm1;
                            pxor mm4, mmxxor;

                            // filter color
                            // (already unpacked) punpcklbw mm0, mm5;
                            pmullw mm0, mm7;
                            psrlw mm0, 8;
                            packuswb mm0, mm0;

                            // save
                            paddusb mm0, dword ptr [edi];
                            movd dword ptr [edi], mm0;
                            add edi, 4;
                            
                            // advance tex coords, cx += sdx;
                            add ebx, sdx;

                            dec ecx;
                            jnz p0loopadd;

                            pop ebx;
                        }
                        __asm {
                            pop edx;
                            pop edi;
                            pop esi;
                        }
                        cy0 += sdy;
                        outp += w+1;
                    }
                    break;
                }
            
                case OUT_MAXIMUM:
                {
                    int maxmask = 0xFFFFFF;
                    int signmask = 0x808080;
                    __int64 mmxxor = 0x00FF00FF00FF00FF;
                    __int64 *p = &mmxxor;
                    int tot = r2.right - r2.left;
                    int *outp = &framebuffer[r2.top*(w+1)+r2.left];
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        __asm {
                            push esi;
                            push edi;
                            push edx;
                            // esi = texture width
                            mov esi, imagewidth;
                            inc esi;
                            pxor mm5, mm5;
                            movd mm6, signmask;
                        }
                        __asm {
                            // bilinear
                            push ebx;

                            pxor mm5, mm5;

                            // calculate dy coefficient for this scanline
                            // store in mm4 = dy cloned into all bytes
                            movd mm4, cy0;
                            psrlw mm4, 8;
                            punpcklwd mm4, mm4;
                            punpckldq mm4, mm4;

                            // loop counter
                            mov ecx, tot;
                            inc ecx;

                            // set output pointer
                            mov edi, outp;

                            // beginning x tex coordinate
                            mov ebx, cx0;

                            // calculate y combined address for first point a for this scanline
                            // store in ebp = texture start address for this scanline
                            mov eax, cy0;
                            shr eax, 16;
                            imul eax, esi;
                            shl eax, 2;
                            mov edx, texdata;
                            add edx, eax;

                            // begin loop
                            p0loopmax:

                            // calculate dx, fractional part of tex coord
                            movd mm3, ebx;
                            psrlw mm3, 8;
                            punpcklwd mm3, mm3;
                            punpckldq mm3, mm3;

                            // convert fixed point into floor and load pixel address
                            mov eax, ebx;
                            shr eax, 16;
                            lea eax, dword ptr [edx+eax*4];

                            // mm0 = b*dx
                            movd mm0, dword ptr [eax+4]; //b
                            punpcklbw mm0, mm5;
                            pmullw mm0, mm3;
                            psrlw mm0, 8;
                            // mm1 = a*(1-dx)
                            movd mm1, dword ptr [eax]; //a
                            pxor mm3, mmxxor;
                            punpcklbw mm1, mm5;
                            pmullw mm1, mm3;
                            psrlw mm1, 8;
                            paddw mm0, mm1;

                            // mm1 = c*(1-dx)
                            movd mm1, dword ptr [eax+esi*4]; //c
                            punpcklbw mm1, mm5;
                            pmullw mm1, mm3;
                            psrlw mm1, 8;
                            // mm2 = d*dx
                            movd mm2, dword ptr [eax+esi*4+4]; //d
                            pxor mm3, mmxxor;
                            punpcklbw mm2, mm5;
                            pmullw mm2, mm3;
                            psrlw mm2, 8;
                            paddw mm1, mm2;

                            // combine
                            pmullw mm1, mm4;
                            psrlw mm1, 8;
                            pxor mm4, mmxxor;
                            pmullw mm0, mm4;
                            psrlw mm0, 8;
                            paddw mm0, mm1;
                            pxor mm4, mmxxor;

                            // filter color
                            // (already unpacked) punpcklbw mm0, mm5;
                            pmullw mm0, mm7;
                            psrlw mm0, 8;
                            packuswb mm0, mm0;

                            // save
                            movd mm1, dword ptr [edi];
                            pxor mm0, mm6;
                            mov eax, 0xFFFFFF;
                            pxor mm1, mm6;
                            movd mm5, eax;
                            movq mm2, mm1;
                            pcmpgtb mm2, mm0;
                            movq mm3, mm2;
                            pxor mm3, mm5;
                            pxor mm5, mm5;
                            pand mm0, mm3;
                            pand mm1, mm2;
                            por mm0, mm1;
                            pxor mm0, mm6;
                            movd dword ptr [edi], mm0;
                            add edi, 4;
                            
                            // advance tex coords, cx += sdx;
                            add ebx, sdx;

                            dec ecx;
                            jnz p0loopmax;

                            pop ebx;
                        }
                        __asm {
                            pop edx;
                            pop edi;
                            pop esi;
                        }
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
                        __asm {
                            push esi;
                            push edi;
                            push edx;
                            // esi = texture width
                            mov esi, imagewidth;
                            inc esi;
                            pxor mm5, mm5;
                        }
                        __asm {
                            // bilinear
                            push ebx;

                            pxor mm5, mm5;

                            // calculate dy coefficient for this scanline
                            // store in mm4 = dy cloned into all bytes
                            movd mm4, cy0;
                            psrlw mm4, 8;
                            punpcklwd mm4, mm4;
                            punpckldq mm4, mm4;

                            // loop counter
                            mov ecx, tot;
                            inc ecx;

                            // set output pointer
                            mov edi, outp;

                            // beginning x tex coordinate
                            mov ebx, cx0;

                            // calculate y combined address for first point a for this scanline
                            // store in ebp = texture start address for this scanline
                            mov eax, cy0;
                            shr eax, 16;
                            imul eax, esi;
                            shl eax, 2;
                            mov edx, texdata;
                            add edx, eax;

                            // begin loop
                            p0loop50:

                            // calculate dx, fractional part of tex coord
                            movd mm3, ebx;
                            psrlw mm3, 8;
                            punpcklwd mm3, mm3;
                            punpckldq mm3, mm3;

                            // convert fixed point into floor and load pixel address
                            mov eax, ebx;
                            shr eax, 16;
                            lea eax, dword ptr [edx+eax*4];

                            // mm0 = b*dx
                            movd mm0, dword ptr [eax+4]; //b
                            punpcklbw mm0, mm5;
                            pmullw mm0, mm3;
                            psrlw mm0, 8;
                            // mm1 = a*(1-dx)
                            movd mm1, dword ptr [eax]; //a
                            pxor mm3, mmxxor;
                            punpcklbw mm1, mm5;
                            pmullw mm1, mm3;
                            psrlw mm1, 8;
                            paddw mm0, mm1;

                            // mm1 = c*(1-dx)
                            movd mm1, dword ptr [eax+esi*4]; //c
                            punpcklbw mm1, mm5;
                            pmullw mm1, mm3;
                            psrlw mm1, 8;
                            // mm2 = d*dx
                            movd mm2, dword ptr [eax+esi*4+4]; //d
                            pxor mm3, mmxxor;
                            punpcklbw mm2, mm5;
                            pmullw mm2, mm3;
                            psrlw mm2, 8;
                            paddw mm1, mm2;

                            // combine
                            pmullw mm1, mm4;
                            psrlw mm1, 8;
                            pxor mm4, mmxxor;
                            pmullw mm0, mm4;
                            psrlw mm0, 8;
                            paddw mm0, mm1;
                            pxor mm4, mmxxor;

                            // filter color
                            // (already unpacked) punpcklbw mm0, mm5;
                            pmullw mm0, mm7;

                            // save
                            movd mm1, dword ptr [edi];
                            psrlw mm0, 8;
                            punpcklbw mm1, mm5;
                            paddusw mm0, mm1;
                            psrlw mm0, 1;
                            packuswb mm0, mm0;
                            movd dword ptr [edi], mm0;
                            add edi, 4;
                            
                            // advance tex coords, cx += sdx;
                            add ebx, sdx;

                            dec ecx;
                            jnz p0loop50;

                            pop ebx;
                        }
                        __asm {
                            pop edx;
                            pop edi;
                            pop esi;
                        }
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
                        __asm {
                            push esi;
                            push edi;
                            push edx;
                            // esi = texture width
                            mov esi, imagewidth;
                            inc esi;
                            pxor mm5, mm5;
                        }
                        __asm {
                            // bilinear
                            push ebx;

                            pxor mm5, mm5;

                            // calculate dy coefficient for this scanline
                            // store in mm4 = dy cloned into all bytes
                            movd mm4, cy0;
                            psrlw mm4, 8;
                            punpcklwd mm4, mm4;
                            punpckldq mm4, mm4;

                            // loop counter
                            mov ecx, tot;
                            inc ecx;

                            // set output pointer
                            mov edi, outp;

                            // beginning x tex coordinate
                            mov ebx, cx0;

                            // calculate y combined address for first point a for this scanline
                            // store in ebp = texture start address for this scanline
                            mov eax, cy0;
                            shr eax, 16;
                            imul eax, esi;
                            shl eax, 2;
                            mov edx, texdata;
                            add edx, eax;

                            // begin loop
                            p0loopsub1:

                            // calculate dx, fractional part of tex coord
                            movd mm3, ebx;
                            psrlw mm3, 8;
                            punpcklwd mm3, mm3;
                            punpckldq mm3, mm3;

                            // convert fixed point into floor and load pixel address
                            mov eax, ebx;
                            shr eax, 16;
                            lea eax, dword ptr [edx+eax*4];

                            // mm0 = b*dx
                            movd mm0, dword ptr [eax+4]; //b
                            punpcklbw mm0, mm5;
                            pmullw mm0, mm3;
                            psrlw mm0, 8;
                            // mm1 = a*(1-dx)
                            movd mm1, dword ptr [eax]; //a
                            pxor mm3, mmxxor;
                            punpcklbw mm1, mm5;
                            pmullw mm1, mm3;
                            psrlw mm1, 8;
                            paddw mm0, mm1;

                            // mm1 = c*(1-dx)
                            movd mm1, dword ptr [eax+esi*4]; //c
                            punpcklbw mm1, mm5;
                            pmullw mm1, mm3;
                            psrlw mm1, 8;
                            // mm2 = d*dx
                            movd mm2, dword ptr [eax+esi*4+4]; //d
                            pxor mm3, mmxxor;
                            punpcklbw mm2, mm5;
                            pmullw mm2, mm3;
                            psrlw mm2, 8;
                            paddw mm1, mm2;

                            // combine
                            pmullw mm1, mm4;
                            psrlw mm1, 8;
                            pxor mm4, mmxxor;
                            pmullw mm0, mm4;
                            psrlw mm0, 8;
                            paddw mm0, mm1;
                            pxor mm4, mmxxor;

                            // filter color
                            // (already unpacked) punpcklbw mm0, mm5;
                            pmullw mm0, mm7;
                            psrlw mm0, 8;
                            packuswb mm0, mm0;

                            // save
                            movd mm1, dword ptr [edi];
                            psubusb mm1, mm0;
                            movd dword ptr [edi], mm1;
                            add edi, 4;
                            
                            // advance tex coords, cx += sdx;
                            add ebx, sdx;

                            dec ecx;
                            jnz p0loopsub1;

                            pop ebx;
                        }
                        __asm {
                            pop edx;
                            pop edi;
                            pop esi;
                        }
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
                        __asm {
                            push esi;
                            push edi;
                            push edx;
                            // esi = texture width
                            mov esi, imagewidth;
                            inc esi;
                            pxor mm5, mm5;
                        }
                        __asm {
                            // bilinear
                            push ebx;

                            pxor mm5, mm5;

                            // calculate dy coefficient for this scanline
                            // store in mm4 = dy cloned into all bytes
                            movd mm4, cy0;
                            psrlw mm4, 8;
                            punpcklwd mm4, mm4;
                            punpckldq mm4, mm4;

                            // loop counter
                            mov ecx, tot;
                            inc ecx;

                            // set output pointer
                            mov edi, outp;

                            // beginning x tex coordinate
                            mov ebx, cx0;

                            // calculate y combined address for first point a for this scanline
                            // store in ebp = texture start address for this scanline
                            mov eax, cy0;
                            shr eax, 16;
                            imul eax, esi;
                            shl eax, 2;
                            mov edx, texdata;
                            add edx, eax;

                            // begin loop
                            p0loopsub2:

                            // calculate dx, fractional part of tex coord
                            movd mm3, ebx;
                            psrlw mm3, 8;
                            punpcklwd mm3, mm3;
                            punpckldq mm3, mm3;

                            // convert fixed point into floor and load pixel address
                            mov eax, ebx;
                            shr eax, 16;
                            lea eax, dword ptr [edx+eax*4];

                            // mm0 = b*dx
                            movd mm0, dword ptr [eax+4]; //b
                            punpcklbw mm0, mm5;
                            pmullw mm0, mm3;
                            psrlw mm0, 8;
                            // mm1 = a*(1-dx)
                            movd mm1, dword ptr [eax]; //a
                            pxor mm3, mmxxor;
                            punpcklbw mm1, mm5;
                            pmullw mm1, mm3;
                            psrlw mm1, 8;
                            paddw mm0, mm1;

                            // mm1 = c*(1-dx)
                            movd mm1, dword ptr [eax+esi*4]; //c
                            punpcklbw mm1, mm5;
                            pmullw mm1, mm3;
                            psrlw mm1, 8;
                            // mm2 = d*dx
                            movd mm2, dword ptr [eax+esi*4+4]; //d
                            pxor mm3, mmxxor;
                            punpcklbw mm2, mm5;
                            pmullw mm2, mm3;
                            psrlw mm2, 8;
                            paddw mm1, mm2;

                            // combine
                            pmullw mm1, mm4;
                            psrlw mm1, 8;
                            pxor mm4, mmxxor;
                            pmullw mm0, mm4;
                            psrlw mm0, 8;
                            paddw mm0, mm1;
                            pxor mm4, mmxxor;

                            // filter color
                            // (already unpacked) punpcklbw mm0, mm5;
                            pmullw mm0, mm7;
                            psrlw mm0, 8;
                            packuswb mm0, mm0;

                            // save
                            psubusb mm0, dword ptr [edi];
                            movd dword ptr [edi], mm0;
                            add edi, 4;
                            
                            // advance tex coords, cx += sdx;
                            add ebx, sdx;

                            dec ecx;
                            jnz p0loopsub2;

                            pop ebx;
                        }
                        __asm {
                            pop edx;
                            pop edi;
                            pop esi;
                        }
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
                        __asm {
                            push esi;
                            push edi;
                            push edx;
                            // esi = texture width
                            mov esi, imagewidth;
                            inc esi;
                            pxor mm5, mm5;
                        }
                        __asm {
                            // bilinear
                            push ebx;

                            pxor mm5, mm5;

                            // calculate dy coefficient for this scanline
                            // store in mm4 = dy cloned into all bytes
                            movd mm4, cy0;
                            psrlw mm4, 8;
                            punpcklwd mm4, mm4;
                            punpckldq mm4, mm4;

                            // loop counter
                            mov ecx, tot;
                            inc ecx;

                            // set output pointer
                            mov edi, outp;

                            // beginning x tex coordinate
                            mov ebx, cx0;

                            // calculate y combined address for first point a for this scanline
                            // store in ebp = texture start address for this scanline
                            mov eax, cy0;
                            shr eax, 16;
                            imul eax, esi;
                            shl eax, 2;
                            mov edx, texdata;
                            add edx, eax;

                            // begin loop
                            p0loopmul:

                            // calculate dx, fractional part of tex coord
                            movd mm3, ebx;
                            psrlw mm3, 8;
                            punpcklwd mm3, mm3;
                            punpckldq mm3, mm3;

                            // convert fixed point into floor and load pixel address
                            mov eax, ebx;
                            shr eax, 16;
                            lea eax, dword ptr [edx+eax*4];

                            // mm0 = b*dx
                            movd mm0, dword ptr [eax+4]; //b
                            punpcklbw mm0, mm5;
                            pmullw mm0, mm3;
                            psrlw mm0, 8;
                            // mm1 = a*(1-dx)
                            movd mm1, dword ptr [eax]; //a
                            pxor mm3, mmxxor;
                            punpcklbw mm1, mm5;
                            pmullw mm1, mm3;
                            psrlw mm1, 8;
                            paddw mm0, mm1;

                            // mm1 = c*(1-dx)
                            movd mm1, dword ptr [eax+esi*4]; //c
                            punpcklbw mm1, mm5;
                            pmullw mm1, mm3;
                            psrlw mm1, 8;
                            // mm2 = d*dx
                            movd mm2, dword ptr [eax+esi*4+4]; //d
                            pxor mm3, mmxxor;
                            punpcklbw mm2, mm5;
                            pmullw mm2, mm3;
                            psrlw mm2, 8;
                            paddw mm1, mm2;

                            // combine
                            pmullw mm1, mm4;
                            psrlw mm1, 8;
                            pxor mm4, mmxxor;
                            pmullw mm0, mm4;
                            psrlw mm0, 8;
                            paddw mm0, mm1;
                            pxor mm4, mmxxor;

                            // filter color
                            // (already unpacked) punpcklbw mm0, mm5;
                            pmullw mm0, mm7;
                            psrlw mm0, 8;

                            // save
                            movd mm1, dword ptr [edi];
                            punpcklbw mm1, mm5;
                            pmullw mm1, mm0;
                            psrlw mm1, 8;
                            packuswb mm1, mm1;
                            movd dword ptr [edi], mm1;
                            add edi, 4;
                            
                            // advance tex coords, cx += sdx;
                            add ebx, sdx;

                            dec ecx;
                            jnz p0loopmul;

                            pop ebx;
                        }
                        __asm {
                            pop edx;
                            pop edi;
                            pop esi;
                        }
                        cy0 += sdy;
                        outp += w+1;
                    }
                    break;
                }
            
                case OUT_ADJUSTABLE:
                {
                    __int64 alphavalue = 0x0;
                    __int64 *alpha = &alphavalue;
                    __int64 salpha = 0x0100010001000100;
                    int t = ((*g_extinfo->lineblendmode) & 0xFF00)>>8;
                    __asm {
                        // duplicate blend factor into all channels
                        mov eax, t;
                        mov edx, eax;
                        shl eax, 16;
                        or  eax, edx;
                        mov [alpha], eax;
                        mov [alpha+4], eax;

                        // store alpha and (256 - alpha)
                        movq mm6, salpha;
                        psubusw mm6, alpha;
                    }
                    __int64 mmxxor = 0x00FF00FF00FF00FF;
                    __int64 *p = &mmxxor;
                    int tot = r2.right - r2.left;
                    int *outp = &framebuffer[r2.top*(w+1)+r2.left];
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        __asm {
                            push esi;
                            push edi;
                            push edx;
                            // esi = texture width
                            mov esi, imagewidth;
                            inc esi;
                            pxor mm5, mm5;
                        }
                        __asm {
                            // bilinear
                            push ebx;

                            pxor mm5, mm5;

                            // calculate dy coefficient for this scanline
                            // store in mm4 = dy cloned into all bytes
                            movd mm4, cy0;
                            psrlw mm4, 8;
                            punpcklwd mm4, mm4;
                            punpckldq mm4, mm4;

                            // loop counter
                            mov ecx, tot;
                            inc ecx;

                            // set output pointer
                            mov edi, outp;

                            // beginning x tex coordinate
                            mov ebx, cx0;

                            // calculate y combined address for first point a for this scanline
                            // store in ebp = texture start address for this scanline
                            mov eax, cy0;
                            shr eax, 16;
                            imul eax, esi;
                            shl eax, 2;
                            mov edx, texdata;
                            add edx, eax;

                            // begin loop
                            p0loopadj:

                            // calculate dx, fractional part of tex coord
                            movd mm3, ebx;
                            psrlw mm3, 8;
                            punpcklwd mm3, mm3;
                            punpckldq mm3, mm3;

                            // convert fixed point into floor and load pixel address
                            mov eax, ebx;
                            shr eax, 16;
                            lea eax, dword ptr [edx+eax*4];

                            // mm0 = b*dx
                            movd mm0, dword ptr [eax+4]; //b
                            punpcklbw mm0, mm5;
                            pmullw mm0, mm3;
                            psrlw mm0, 8;
                            // mm1 = a*(1-dx)
                            movd mm1, dword ptr [eax]; //a
                            pxor mm3, mmxxor;
                            punpcklbw mm1, mm5;
                            pmullw mm1, mm3;
                            psrlw mm1, 8;
                            paddw mm0, mm1;

                            // mm1 = c*(1-dx)
                            movd mm1, dword ptr [eax+esi*4]; //c
                            punpcklbw mm1, mm5;
                            pmullw mm1, mm3;
                            psrlw mm1, 8;
                            // mm2 = d*dx
                            movd mm2, dword ptr [eax+esi*4+4]; //d
                            pxor mm3, mmxxor;
                            punpcklbw mm2, mm5;
                            pmullw mm2, mm3;
                            psrlw mm2, 8;
                            paddw mm1, mm2;

                            // combine
                            pmullw mm1, mm4;
                            psrlw mm1, 8;
                            pxor mm4, mmxxor;
                            pmullw mm0, mm4;
                            psrlw mm0, 8;
                            paddw mm0, mm1;
                            pxor mm4, mmxxor;

                            // filter color
                            // (already unpacked) punpcklbw mm0, mm5;
                            pmullw mm0, mm7;
                            psrlw mm0, 8;

                            // Merged filter/alpha
                            // save
                            movd mm1, dword ptr [edi];
                            punpcklbw mm1, mm5;
                            pmullw mm1, mm6;
                            psrlw mm1, 8;
                            pmullw mm0, alpha;
                            psrlw mm0, 8;
                            paddusw mm0, mm1;
                            packuswb mm0, mm0;
                            movd dword ptr [edi], mm0;
                            add edi, 4;
                            
                            // advance tex coords, cx += sdx;
                            add ebx, sdx;

                            dec ecx;
                            jnz p0loopadj;

                            pop ebx;
                        }
                        __asm {
                            pop edx;
                            pop edi;
                            pop esi;
                        }
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
                        __asm {
                            push esi;
                            push edi;
                            push edx;
                            // esi = texture width
                            mov esi, imagewidth;
                            inc esi;
                            pxor mm5, mm5;
                        }
                        __asm {
                            // bilinear
                            push ebx;

                            pxor mm5, mm5;

                            // calculate dy coefficient for this scanline
                            // store in mm4 = dy cloned into all bytes
                            movd mm4, cy0;
                            psrlw mm4, 8;
                            punpcklwd mm4, mm4;
                            punpckldq mm4, mm4;

                            // loop counter
                            mov ecx, tot;
                            inc ecx;

                            // set output pointer
                            mov edi, outp;

                            // beginning x tex coordinate
                            mov ebx, cx0;

                            // calculate y combined address for first point a for this scanline
                            // store in ebp = texture start address for this scanline
                            mov eax, cy0;
                            shr eax, 16;
                            imul eax, esi;
                            shl eax, 2;
                            mov edx, texdata;
                            add edx, eax;

                            // begin loop
                            p0loopxor:

                            // calculate dx, fractional part of tex coord
                            movd mm3, ebx;
                            psrlw mm3, 8;
                            punpcklwd mm3, mm3;
                            punpckldq mm3, mm3;

                            // convert fixed point into floor and load pixel address
                            mov eax, ebx;
                            shr eax, 16;
                            lea eax, dword ptr [edx+eax*4];

                            // mm0 = b*dx
                            movd mm0, dword ptr [eax+4]; //b
                            punpcklbw mm0, mm5;
                            pmullw mm0, mm3;
                            psrlw mm0, 8;
                            // mm1 = a*(1-dx)
                            movd mm1, dword ptr [eax]; //a
                            pxor mm3, mmxxor;
                            punpcklbw mm1, mm5;
                            pmullw mm1, mm3;
                            psrlw mm1, 8;
                            paddw mm0, mm1;

                            // mm1 = c*(1-dx)
                            movd mm1, dword ptr [eax+esi*4]; //c
                            punpcklbw mm1, mm5;
                            pmullw mm1, mm3;
                            psrlw mm1, 8;
                            // mm2 = d*dx
                            movd mm2, dword ptr [eax+esi*4+4]; //d
                            pxor mm3, mmxxor;
                            punpcklbw mm2, mm5;
                            pmullw mm2, mm3;
                            psrlw mm2, 8;
                            paddw mm1, mm2;

                            // combine
                            pmullw mm1, mm4;
                            psrlw mm1, 8;
                            pxor mm4, mmxxor;
                            pmullw mm0, mm4;
                            psrlw mm0, 8;
                            paddw mm0, mm1;
                            pxor mm4, mmxxor;

                            // filter color
                            // (already unpacked) punpcklbw mm0, mm5;
                            pmullw mm0, mm7;
                            psrlw mm0, 8;

                            // save
                            pxor mm0, dword ptr [edi];
                            movd dword ptr [edi], mm0;
                            add edi, 4;
                            
                            // advance tex coords, cx += sdx;
                            add ebx, sdx;

                            dec ecx;
                            jnz p0loopxor;

                            pop ebx;
                        }
                        __asm {
                            pop edx;
                            pop edi;
                            pop esi;
                        }
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
                        __asm {
                            push esi;
                            push edi;
                            push edx;
                            // esi = texture width
                            mov esi, imagewidth;
                            inc esi;
                            pxor mm5, mm5;
                            movd mm6, signmask;
                        }
                        __asm {
                            // bilinear
                            push ebx;

                            pxor mm5, mm5;

                            // calculate dy coefficient for this scanline
                            // store in mm4 = dy cloned into all bytes
                            movd mm4, cy0;
                            psrlw mm4, 8;
                            punpcklwd mm4, mm4;
                            punpckldq mm4, mm4;

                            // loop counter
                            mov ecx, tot;
                            inc ecx;

                            // set output pointer
                            mov edi, outp;

                            // beginning x tex coordinate
                            mov ebx, cx0;

                            // calculate y combined address for first point a for this scanline
                            // store in ebp = texture start address for this scanline
                            mov eax, cy0;
                            shr eax, 16;
                            imul eax, esi;
                            shl eax, 2;
                            mov edx, texdata;
                            add edx, eax;

                            // begin loop
                            p0loopmin:

                            // calculate dx, fractional part of tex coord
                            movd mm3, ebx;
                            psrlw mm3, 8;
                            punpcklwd mm3, mm3;
                            punpckldq mm3, mm3;

                            // convert fixed point into floor and load pixel address
                            mov eax, ebx;
                            shr eax, 16;
                            lea eax, dword ptr [edx+eax*4];

                            // mm0 = b*dx
                            movd mm0, dword ptr [eax+4]; //b
                            punpcklbw mm0, mm5;
                            pmullw mm0, mm3;
                            psrlw mm0, 8;
                            // mm1 = a*(1-dx)
                            movd mm1, dword ptr [eax]; //a
                            pxor mm3, mmxxor;
                            punpcklbw mm1, mm5;
                            pmullw mm1, mm3;
                            psrlw mm1, 8;
                            paddw mm0, mm1;

                            // mm1 = c*(1-dx)
                            movd mm1, dword ptr [eax+esi*4]; //c
                            punpcklbw mm1, mm5;
                            pmullw mm1, mm3;
                            psrlw mm1, 8;
                            // mm2 = d*dx
                            movd mm2, dword ptr [eax+esi*4+4]; //d
                            pxor mm3, mmxxor;
                            punpcklbw mm2, mm5;
                            pmullw mm2, mm3;
                            psrlw mm2, 8;
                            paddw mm1, mm2;

                            // combine
                            pmullw mm1, mm4;
                            psrlw mm1, 8;
                            pxor mm4, mmxxor;
                            pmullw mm0, mm4;
                            psrlw mm0, 8;
                            paddw mm0, mm1;
                            pxor mm4, mmxxor;

                            // filter color
                            // (already unpacked) punpcklbw mm0, mm5;
                            pmullw mm0, mm7;
                            psrlw mm0, 8;
                            packuswb mm0, mm0;

                            // save
                            movd mm1, dword ptr [edi];
                            pxor mm0, mm6;
                            mov eax, 0xFFFFFF;
                            pxor mm1, mm6;
                            movd mm5, eax;
                            movq mm2, mm1;
                            pcmpgtb mm2, mm0;
                            movq mm3, mm2;
                            pxor mm3, mm5;
                            pxor mm5, mm5;
                            pand mm0, mm2;
                            pand mm1, mm3;
                            por mm0, mm1;
                            pxor mm0, mm6;
                            movd dword ptr [edi], mm0;
                            add edi, 4;
                            
                            // advance tex coords, cx += sdx;
                            add ebx, sdx;

                            dec ecx;
                            jnz p0loopmin;

                            pop ebx;
                        }
                        __asm {
                            pop edx;
                            pop edi;
                            pop esi;
                        }
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
        r.left = (DoubleToInt((x *.5f + .5f)*w) - iw/2);
        r.top = (DoubleToInt((y *.5f + .5f)*h) - ih/2);
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

        int cx0 = DoubleToInt(x0*iw);
        int cy0 = DoubleToInt(y0*ih);

        int ty = cy0;

        if (config.mask) {
            // Second easiest path, masking, but no scaling
            __asm {
                pxor mm5, mm5;
                movd mm7, color;
                punpcklbw mm7, mm5;
            }
            switch (*(g_extinfo->lineblendmode) & 0xFF) {
                case OUT_REPLACE:
                {
                    __asm {
                        push esi;
                        push edi;
                    }
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = (int *)&texbits[ty*(iw+1)+cx0];
                        int tot = (r2.right - r2.left);
                        __asm {
                            mov ecx, tot;
                            inc ecx;
                            mov edi, outp;
                            mov esi, inp;

                            p2looprep:
                            movd mm0, dword ptr [esi];

                            punpcklbw mm0, mm5;
                            pmullw mm0, mm7;
                            psrlw mm0, 8;
                            packuswb mm0, mm0;

                            movd dword ptr [edi], mm0;
                            add esi, 4;
                            add edi, 4;

                            dec ecx;
                            jnz p2looprep;
                        }
                        ++ty;
                    }
                    __asm {
                        pop edi;
                        pop esi;
                    }
                    break;
                }

                case OUT_ADDITIVE:
                {
                    __asm {
                        push esi;
                        push edi;
                    }
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = (int *)&texbits[ty*(iw+1)+cx0];
                        int tot = (r2.right - r2.left);
                        __asm {
                            mov ecx, tot;
                            inc ecx;
                            mov edi, outp;
                            mov esi, inp;

                            p2loopadd:
                            movd mm0, dword ptr [edi];
                            movd mm1, dword ptr [esi];
                            
                            punpcklbw mm1, mm5;
                            pmullw mm1, mm7;
                            psrlw mm1, 8;
                            packuswb mm1, mm1;
                            
                            paddusb mm0, mm1;
                            movd dword ptr [edi], mm0;
                            add esi, 4;
                            add edi, 4;

                            dec ecx;
                            jnz p2loopadd;
                        }
                        ++ty;
                    }
                    __asm {
                        pop edi;
                        pop esi;
                    }
                    break;
                }

                case OUT_MAXIMUM:
                {
                    int maxmask = 0xFFFFFF;
                    int signmask = 0x808080;
                    __asm {
                        push esi;
                        push edi;
                        movd mm4, maxmask;
                        movd mm6, signmask;
                    }
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = (int *)&texbits[ty*(iw+1)+cx0];
                        int tot = (r2.right - r2.left);
                        __asm {
                            mov ecx, tot;
                            inc ecx;
                            mov edi, outp;
                            mov esi, inp;

                            p2loopmax:
                            movd mm0, dword ptr [edi];
                            movd mm1, dword ptr [esi];
                            
                            punpcklbw mm1, mm5;
                            pmullw mm1, mm7;
                            psrlw mm1, 8;
                            packuswb mm1, mm1;
                            
                            pxor mm0, mm6;
                            pxor mm1, mm6;
                            movq mm2, mm1;
                            pcmpgtb mm2, mm0;
                            movq mm3, mm2;
                            pxor mm3, mm4;
                            pand mm0, mm3;
                            pand mm1, mm2;
                            por mm0, mm1;
                            pxor mm0, mm6;
                            movd dword ptr [edi], mm0;
                            add esi, 4;
                            add edi, 4;

                            dec ecx;
                            jnz p2loopmax;
                        }
                        ++ty;
                    }
                    __asm {
                        pop edi;
                        pop esi;
                    }
                    break;
                }

                case OUT_5050:
                {
                    __asm {
                        push esi;
                        push edi;
                    }
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = (int *)&texbits[ty*(iw+1)+cx0];
                        int tot = (r2.right - r2.left);
                        __asm {
                            mov ecx, tot;
                            inc ecx;
                            mov edi, outp;
                            mov esi, inp;

                            p2loop50:
                            movd mm0, dword ptr [edi];
                            movd mm1, dword ptr [esi];
                            
                            punpcklbw mm1, mm5;
                            pmullw mm1, mm7;
                            psrlw mm1, 8;
                            packuswb mm1, mm1;
                            
                            punpcklbw mm0, mm5;
                            punpcklbw mm1, mm5;
                            paddusw mm0, mm1;
                            psrlw mm0, 1;
                            packuswb mm0, mm0;
                            movd dword ptr [edi], mm0;
                            add esi, 4;
                            add edi, 4;

                            dec ecx;
                            jnz p2loop50;
                        }
                        ++ty;
                    }
                    __asm {
                        pop edi;
                        pop esi;
                    }
                    break;
                }

                case OUT_SUB1:
                {
                    __asm {
                        push esi;
                        push edi;
                    }
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = (int *)&texbits[ty*(iw+1)+cx0];
                        int tot = (r2.right - r2.left);
                        __asm {
                            mov ecx, tot;
                            inc ecx;
                            mov edi, outp;
                            mov esi, inp;

                            p2loopsub1:
                            movd mm0, dword ptr [edi];
                            movd mm1, dword ptr [esi];

                            punpcklbw mm1, mm5;
                            pmullw mm1, mm7;
                            psrlw mm1, 8;
                            packuswb mm1, mm1;

                            psubusb mm0, mm1;
                            movd dword ptr [edi], mm0;
                            add esi, 4;
                            add edi, 4;

                            dec ecx;
                            jnz p2loopsub1;
                        }
                        ++ty;
                    }
                    __asm {
                        pop edi;
                        pop esi;
                    }
                    break;
                }

                case OUT_SUB2:
                {
                    __asm {
                        push esi;
                        push edi;
                    }
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = (int *)&texbits[ty*(iw+1)+cx0];
                        int tot = (r2.right - r2.left);
                        __asm {
                            mov ecx, tot;
                            inc ecx;
                            mov edi, outp;
                            mov esi, inp;

                            p2loopsub2:
                            movd mm0, dword ptr [esi];
                            
                            punpcklbw mm0, mm5;
                            pmullw mm0, mm7;
                            psrlw mm0, 8;
                            packuswb mm0, mm0;
                            
                            psubusb mm0, dword ptr [edi];
                            movd dword ptr [edi], mm0;
                            add esi, 4;
                            add edi, 4;

                            dec ecx;
                            jnz p2loopsub2;
                        }
                        ++ty;
                    }
                    __asm {
                        pop edi;
                        pop esi;
                    }
                    break;
                }

                case OUT_MULTIPLY:
                {
                    __asm {
                        push esi;
                        push edi;
                    }
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = (int *)&texbits[ty*(iw+1)+cx0];
                        int tot = (r2.right - r2.left);
                        __asm {
                            mov ecx, tot;
                            inc ecx;
                            mov edi, outp;
                            mov esi, inp;

                            p2loopmul:
                            movd mm0, dword ptr [edi];
                            movd mm1, dword ptr [esi];
                            punpcklbw mm0, mm5;
                            punpcklbw mm1, mm5;
                            pmullw mm0, mm1;
                            psrlw mm0, 8;
                            // Merged filter/mul
                            pmullw mm0, mm7;
                            psrlw mm0, 8;
                            packuswb mm0, mm0;
                            movd dword ptr [edi], mm0;
                            add esi, 4;
                            add edi, 4;

                            dec ecx;
                            jnz p2loopmul;
                        }
                        ++ty;
                    }
                    __asm {
                        pop edi;
                        pop esi;
                    }
                    break;
                }

                case OUT_ADJUSTABLE:
                {
                    __int64 alphavalue = 0x0;
                    __int64 *alpha = &alphavalue;
                    __int64 salpha = 0x0100010001000100;
                    int t = ((*g_extinfo->lineblendmode) & 0xFF00)>>8;
                    __asm {
                        // duplicate blend factor into all channels
                        mov eax, t;
                        mov edx, eax;
                        shl eax, 16;
                        or  eax, edx;
                        mov [alpha], eax;
                        mov [alpha+4], eax;

                        // store alpha and (256 - alpha)
                        movq mm2, alpha;
                        movq mm3, salpha;
                        psubusw mm3, alpha;
                
                        push esi;
                        push edi;
                    }
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = (int *)&texbits[ty*(iw+1)+cx0];
                        int tot = (r2.right - r2.left);
                        __asm {
                            mov ecx, tot;
                            inc ecx;
                            mov edi, outp;
                            mov esi, inp;

                            p2loopadj:
                            movd mm0, dword ptr [edi];
                            movd mm1, dword ptr [esi];

                            punpcklbw mm0, mm5;
                            punpcklbw mm1, mm5;
                            // Merged filter/alpha
                            pmullw    mm1, mm7;
                            psrlw     mm1, 8;

                            pmullw    mm0, mm2;
                            pmullw    mm1, mm3;

                            paddusw   mm0, mm1;
                            psrlw     mm0, 8;
                            packuswb  mm0, mm0;
                            movd dword ptr [edi], mm0;
                            add esi, 4;
                            add edi, 4;

                            dec ecx;
                            jnz p2loopadj;
                        }
                        ++ty;
                    }
                    __asm {
                        pop edi;
                        pop esi;
                    }
                    break;
                }
                
                case OUT_XOR:
                {
                    __asm {
                        push esi;
                        push edi;
                    }
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = (int *)&texbits[ty*(iw+1)+cx0];
                        int tot = (r2.right - r2.left);
                        __asm {
                            mov ecx, tot;
                            inc ecx;
                            mov edi, outp;
                            mov esi, inp;

                            p2loopxor:
                            movd mm0, dword ptr [edi];
                            movd mm1, dword ptr [esi];

                            punpcklbw mm1, mm5;
                            pmullw mm1, mm7;
                            psrlw mm1, 8;
                            packuswb mm1, mm1;

                            pxor mm0, mm1;
                            movd dword ptr [edi], mm0;
                            add esi, 4;
                            add edi, 4;

                            dec ecx;
                            jnz p2loopxor;
                        }
                        ++ty;
                    }
                    __asm {
                        pop edi;
                        pop esi;
                    }
                    break;
                }

                case OUT_MINIMUM:
                {
                    int maxmask = 0xFFFFFF;
                    int signmask = 0x808080;
                    __asm {
                        push esi;
                        push edi;
                        movd mm4, maxmask;
                        movd mm6, signmask;
                    }
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = (int *)&texbits[ty*(iw+1)+cx0];
                        int tot = (r2.right - r2.left);
                        __asm {
                            mov ecx, tot;
                            inc ecx;
                            mov edi, outp;
                            mov esi, inp;

                            p2loopmin:
                            movd mm0, dword ptr [edi];
                            movd mm1, dword ptr [esi];

                            punpcklbw mm1, mm5;
                            pmullw mm1, mm7;
                            psrlw mm1, 8;
                            packuswb mm1, mm1;

                            pxor mm0, mm6;
                            pxor mm1, mm6;
                            movq mm2, mm1;
                            pcmpgtb mm2, mm0;
                            movq mm3, mm2;
                            pxor mm3, mm4;
                            pand mm0, mm2;
                            pand mm1, mm3;
                            por mm0, mm1;
                            pxor mm0, mm6;
                            movd dword ptr [edi], mm0;
                            add esi, 4;
                            add edi, 4;

                            dec ecx;
                            jnz p2loopmin;
                        }
                        ++ty;
                    }
                    __asm {
                        pop edi;
                        pop esi;
                    }
                    break;
                }
            }
        } else {
            // Most basic path, no scaling or masking
            __asm pxor mm5, mm5
            switch (*(g_extinfo->lineblendmode) & 0xFF) {
                case OUT_REPLACE:
                {
                    __asm {
                        push edi;
                        push esi;
                    }
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = (int *)&texbits[ty*(iw+1)+cx0];
                        int tot = (r2.right - r2.left);
                        __asm {
                            mov ecx, tot;
                            inc ecx;
                            mov edi, outp;
                            mov esi, inp;

                            p3looprep:
                            movd mm0, dword ptr [esi];
                            movd dword ptr [edi], mm0;
                            add esi, 4;
                            add edi, 4;

                            dec ecx;
                            jnz p3looprep;
                        }
                        ++ty;
                    }
                    __asm {
                        pop edi;
                        pop esi;
                    }
                    break;
                }

                case OUT_ADDITIVE:
                {
                    __asm {
                        push esi;
                        push edi;
                    }
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = (int *)&texbits[ty*(iw+1)+cx0];
                        int tot = (r2.right - r2.left);
                        __asm {
                            mov ecx, tot;
                            inc ecx;
                            mov edi, outp;
                            mov esi, inp;

                            p3loopadd:
                            movd mm0, dword ptr [edi];
                            paddusb mm0, dword ptr [esi];
                            movd dword ptr [edi], mm0;
                            add esi, 4;
                            add edi, 4;

                            dec ecx;
                            jnz p3loopadd;
                        }
                        ++ty;
                    }
                    __asm {
                        pop edi;
                        pop esi;
                    }
                    break;
                }

                case OUT_MAXIMUM:
                {
                    int maxmask = 0xFFFFFF;
                    int signmask = 0x808080;
                    __asm {
                        push esi;
                        push edi;
                        movd mm4, maxmask;
                        movd mm6, signmask;
                    }
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = (int *)&texbits[ty*(iw+1)+cx0];
                        int tot = (r2.right - r2.left);
                        __asm {
                            mov ecx, tot;
                            inc ecx;
                            mov edi, outp;
                            mov esi, inp;

                            p3loopmax:
                            movd mm0, dword ptr [edi];
                            movd mm1, dword ptr [esi];
                            pxor mm0, mm6;
                            pxor mm1, mm6;
                            movq mm2, mm1;
                            pcmpgtb mm2, mm0;
                            movq mm3, mm2;
                            pxor mm3, mm4;
                            pand mm0, mm3;
                            pand mm1, mm2;
                            por mm0, mm1;
                            pxor mm0, mm6;
                            movd dword ptr [edi], mm0;
                            add esi, 4;
                            add edi, 4;

                            dec ecx;
                            jnz p3loopmax;
                        }
                        ++ty;
                    }
                    __asm {
                        pop edi;
                        pop esi;
                    }
                    break;
                }

                case OUT_5050:
                {
                    __asm {
                        push esi;
                        push edi;
                    }
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = (int *)&texbits[ty*(iw+1)+cx0];
                        int tot = (r2.right - r2.left);
                        __asm {
                            mov ecx, tot;
                            inc ecx;
                            mov edi, outp;
                            mov esi, inp;

                            p3loop50:
                            movd mm0, dword ptr [edi];
                            movd mm1, dword ptr [esi];
                            punpcklbw mm0, mm5;
                            punpcklbw mm1, mm5;
                            paddusw mm0, mm1;
                            psrlw mm0, 1;
                            packuswb mm0, mm0;
                            movd dword ptr [edi], mm0;
                            add esi, 4;
                            add edi, 4;

                            dec ecx;
                            jnz p3loop50;
                        }
                        ++ty;
                    }
                    __asm {
                        pop edi;
                        pop esi;
                    }
                    break;
                }

                case OUT_SUB1:
                {
                    __asm {
                        push esi;
                        push edi;
                    }
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = (int *)&texbits[ty*(iw+1)+cx0];
                        int tot = (r2.right - r2.left);
                        __asm {
                            mov ecx, tot;
                            inc ecx;
                            mov edi, outp;
                            mov esi, inp;

                            p3loopsub1:
                            movd mm0, dword ptr [edi];
                            psubusb mm0, dword ptr [esi];
                            movd dword ptr [edi], mm0;
                            add esi, 4;
                            add edi, 4;

                            dec ecx;
                            jnz p3loopsub1;
                        }
                        ++ty;
                    }
                    __asm {
                        pop edi;
                        pop esi;
                    }
                    break;
                }

                case OUT_SUB2:
                {
                    __asm {
                        push esi;
                        push edi;
                    }
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = (int *)&texbits[ty*(iw+1)+cx0];
                        int tot = (r2.right - r2.left);
                        __asm {
                            mov ecx, tot;
                            inc ecx;
                            mov edi, outp;
                            mov esi, inp;

                            p3loopsub2:
                            movd mm0, dword ptr [esi];
                            psubusb mm0, dword ptr [edi];
                            movd dword ptr [edi], mm0;
                            add esi, 4;
                            add edi, 4;

                            dec ecx;
                            jnz p3loopsub2;
                        }
                        ++ty;
                    }
                    __asm {
                        pop edi;
                        pop esi;
                    }
                    break;
                }

                case OUT_MULTIPLY:
                {
                    __asm {
                        push esi;
                        push edi;
                    }
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = (int *)&texbits[ty*(iw+1)+cx0];
                        int tot = (r2.right - r2.left);
                        __asm {
                            mov ecx, tot;
                            inc ecx;
                            mov edi, outp;
                            mov esi, inp;

                            p3loopmul:
                            movd mm0, dword ptr [edi];
                            movd mm1, dword ptr [esi];
                            punpcklbw mm0, mm5;
                            punpcklbw mm1, mm5;
                            pmullw mm0, mm1;
                            psrlw mm0, 8;
                            packuswb mm0, mm0;
                            movd dword ptr [edi], mm0;
                            add esi, 4;
                            add edi, 4;

                            dec ecx;
                            jnz p3loopmul;
                        }
                        ++ty;
                    }
                    __asm {
                        pop edi;
                        pop esi;
                    }
                    break;
                }

                case OUT_ADJUSTABLE:
                {
                    __int64 alphavalue = 0x0;
                    __int64 *alpha = &alphavalue;
                    __int64 salpha = 0x0100010001000100;
                    int t = ((*g_extinfo->lineblendmode) & 0xFF00)>>8;
                    __asm {
                        // duplicate blend factor into all channels
                        mov eax, t;
                        mov edx, eax;
                        shl eax, 16;
                        or  eax, edx;
                        mov [alpha], eax;
                        mov [alpha+4], eax;

                        // store alpha and (256 - alpha)
                        movq mm2, alpha;
                        movq mm3, salpha;
                        psubusw mm3, alpha;
                
                        push esi;
                        push edi;
                    }
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = (int *)&texbits[ty*(iw+1)+cx0];
                        int tot = (r2.right - r2.left);
                        __asm {
                            mov ecx, tot;
                            inc ecx;
                            mov edi, outp;
                            mov esi, inp;

                            p3loopadj:
                            movd mm0, dword ptr [edi];
                            movd mm1, dword ptr [esi];

                            punpcklbw mm0, mm5;
                            punpcklbw mm1, mm5;

                            pmullw    mm0, mm2;
                            pmullw    mm1, mm3;

                            paddusw   mm0, mm1;
                            psrlw     mm0, 8;
                            packuswb  mm0, mm0;
                            movd dword ptr [edi], mm0;
                            add esi, 4;
                            add edi, 4;

                            dec ecx;
                            jnz p3loopadj;
                        }
                        ++ty;
                    }
                    __asm {
                        pop edi;
                        pop esi;
                    }
                    break;
                }
                
                case OUT_XOR:
                {
                    __asm {
                        push esi;
                        push edi;
                    }
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = (int *)&texbits[ty*(iw+1)+cx0];
                        int tot = (r2.right - r2.left);
                        __asm {
                            mov ecx, tot;
                            inc ecx;
                            mov edi, outp;
                            mov esi, inp;

                            p3loopxor:
                            movd mm0, dword ptr [edi];
                            pxor mm0, dword ptr [esi];
                            movd dword ptr [edi], mm0;
                            add esi, 4;
                            add edi, 4;

                            dec ecx;
                            jnz p3loopxor;
                        }
                        ++ty;
                    }
                    __asm {
                        pop edi;
                        pop esi;
                    }
                    break;
                }

                case OUT_MINIMUM:
                {
                    int maxmask = 0xFFFFFF;
                    int signmask = 0x808080;
                    __asm {
                        push esi;
                        push edi;
                        movd mm4, maxmask;
                        movd mm6, signmask;
                    }
                    for (int y = r2.top; y <= r2.bottom; ++y) {
                        int *outp = &framebuffer[y*(w+1)+r2.left];
                        int *inp = (int *)&texbits[ty*(iw+1)+cx0];
                        int tot = (r2.right - r2.left);
                        __asm {
                            mov ecx, tot;
                            inc ecx;
                            mov edi, outp;
                            mov esi, inp;

                            p3loopmin:
                            movd mm0, dword ptr [edi];
                            movd mm1, dword ptr [esi];
                            pxor mm0, mm6;
                            pxor mm1, mm6;
                            movq mm2, mm1;
                            pcmpgtb mm2, mm0;
                            movq mm3, mm2;
                            pxor mm3, mm4;
                            pand mm0, mm2;
                            pand mm1, mm3;
                            por mm0, mm1;
                            pxor mm0, mm6;
                            movd dword ptr [edi], mm0;
                            add esi, 4;
                            add edi, 4;

                            dec ecx;
                            jnz p3loopmin;
                        }
                        ++ty;
                    }
                    __asm {
                        pop edi;
                        pop esi;
                    }
                    break;
                }
            }
        }
    }

    skippart:
    ++(this->iw); // restore member vars!
    ++(this->ih);

    // Perhaps removing all fpu stuff here and doing one emms globally would be better?
    __asm emms;
}


int C_THISCLASS::render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h)
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

    int n = DoubleToInt((double)*vars.n);   
    n = max(0, min(65536, n));
    Allocate(n);

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

            if (*vars.skip == 0.0) {
                unsigned int color = min(255, max(0, DoubleToInt(255.0f*(double)*vars.blue)));
                color |= min(255, max(0, DoubleToInt(255.0f*(double)*vars.green))) << 8;
                color |= min(255, max(0, DoubleToInt(255.0f*(double)*vars.red))) << 16;

                if (config.mask == 0) color = 0xFFFFFF;

                double szx = (double)*vars.sizex;
                szx = fabs(szx);
                double szy = (double)*vars.sizey;
                szy = fabs(szy);
                if ((szx > .01) && (szy > .01))
                    DrawParticle(framebuffer, w, h, (double)*vars.x, (double)*vars.y, (double)szx, (double)szy, color);
            }
        }
    }

    LeaveCriticalSection(&codestuff);
    LeaveCriticalSection(&imageload);
    return 0;
}

HWND C_THISCLASS::conf(HINSTANCE hInstance, HWND hwndParent) 
{
    g_ConfigThis = this;
    return CreateDialog(hInstance, MAKEINTRESOURCE(IDD_CONFIG), hwndParent, (DLGPROC)g_DlgProc);
}


char *C_THISCLASS::get_desc(void)
{ 
    return MOD_NAME; 
}

void C_THISCLASS::load_config(unsigned char *data, int len) 
{
    if (len >= sizeof(apeconfig))
        memcpy(&this->config, data, sizeof(apeconfig));
    
    unsigned char *p = &data[sizeof(apeconfig)];
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


int  C_THISCLASS::save_config(unsigned char *data) 
{
    memcpy(data, &this->config, sizeof(apeconfig));
    int l = 0, size = sizeof(apeconfig);
    char *p = (char *)(data+sizeof(apeconfig));
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

    return sizeof(apeconfig) + tot;
}

C_RBASE *R_RetrFunc(char *desc) 
{
    if (desc) { 
        strcpy(desc,MOD_NAME); 
        return NULL; 
    }
    return (C_RBASE *) new C_THISCLASS();
}

extern "C"
{
    __declspec (dllexport) int _AVS_APE_RetrFunc(HINSTANCE hDllInstance, char **info, int *create)
    {
        g_hDllInstance=hDllInstance;
        *info=UNIQUEIDSTRING;
        *create=(int)(void*)R_RetrFunc;
        return 1;
    }
};

