#pragma once

#include "effect.h"
#include "effect_info.h"
#include "effect_programmable.h"

#include "../platform.h"

#include <string>

struct SuperScope_Color_Config : public Effect_Config {
    uint64_t color = 0x000000;
    SuperScope_Color_Config() = default;
    SuperScope_Color_Config(uint64_t color) : color(color){};
};

struct SuperScope_Config : public Effect_Config {
    std::string init = "n=800";
    std::string frame = "t=t-0.05";
    std::string beat = "";
    std::string point = "d=i+v*0.2; r=t+i*$PI*4; x=cos(r)*d; y=sin(r)*d";
    std::vector<SuperScope_Color_Config> colors;
    int64_t audio_source = 0;
    int64_t audio_channel = 0;
    int64_t draw_mode = 0;
    int64_t example = 0;
    SuperScope_Config() { this->colors.emplace_back(0xffffff); }
};

struct SuperScope_Info : public Effect_Info {
    static constexpr char* group = "Render";
    static constexpr char* name = "SuperScope";
    static constexpr char* help =
        "Superscope tutorial goes here\r\n"
        "But for now, here is the old text:\r\n"
        "You can specify expressions that run on Init, Frame, and on Beat.\r\n"
        "  'n' specifies the number of points to render (set this in Init, Beat, or "
        "Frame).\r\n"
        "For the 'Per Point' expression (which happens 'n' times per frame), use:\r\n"
        "  'x' and 'y' are the coordinates to draw to (-1..1)\r\n"
        "  'i' is the position of the scope (0..1)\r\n"
        "  'v' is the value at that point (-1..1).\r\n"
        "  'b' is 1 if beat, 0 if not.\r\n"
        "  'red', 'green' and 'blue' are all (0..1) and can be modified\r\n"
        "  'linesize' can be set from 1.0 to 255.0\r\n"
        "  'skip' can be set to >0 to skip drawing the current item\r\n"
        "  'drawmode' can be set to > 0 for lines, <= 0 for points\r\n"
        "  'w' and 'h' are the width and height of the screen, in pixels.\r\n"
        " Anybody want to send me better text to put here? Please :)\r\n";
    static constexpr int32_t legacy_id = 36;
    static constexpr char* legacy_ape_id = NULL;

    static const char* const* sources(int64_t* length_out) {
        *length_out = 2;
        static const char* const options[2] = {"Waveform", "Spectrum"};
        return options;
    };
    static const char* const* channels(int64_t* length_out) {
        *length_out = 3;
        static const char* const options[3] = {"Center", "Left", "Right"};
        return options;
    };
    static const char* const* drawmodes(int64_t* length_out) {
        *length_out = 2;
        static const char* const options[2] = {"Dots", "Lines"};
        return options;
    };
    static const char* const* examples(int64_t* length_out) {
        *length_out = 14;
        static const char* const options[14] = {
            "Spiral",
            "3D Scope Dish",
            "Rotating Bow Thing",
            "Vertical Bouncing Scope",
            "Spiral Graph Fun",
            "Alternating Diagonal Scope",
            "Vibrating Worm",
            "Wandering Simple",
            "Flitterbug",
            "Spirostar",
            "Exploding Daisy",
            "Swirlie Dots",
            "Sweep",
            "Whiplash Spiral",
        };
        return options;
    };

    static constexpr uint32_t num_color_params = 1;
    static constexpr Parameter color_params[num_color_params] = {
        P_COLOR(offsetof(SuperScope_Color_Config, color), "Color"),
    };

    static void recompile(Effect*, const Parameter*, std::vector<int64_t>);
    static void load_example(Effect*, const Parameter*, std::vector<int64_t>);
    static constexpr uint32_t num_parameters = 10;
    static constexpr Parameter parameters[num_parameters] = {
        P_STRING(offsetof(SuperScope_Config, init), "Init", NULL, recompile),
        P_STRING(offsetof(SuperScope_Config, frame), "Frame", NULL, recompile),
        P_STRING(offsetof(SuperScope_Config, beat), "Beat", NULL, recompile),
        P_STRING(offsetof(SuperScope_Config, point), "Point", NULL, recompile),
        P_LIST<SuperScope_Color_Config>(offsetof(SuperScope_Config, colors),
                                        "Colors",
                                        color_params,
                                        num_color_params,
                                        1,
                                        16),
        P_SELECT(offsetof(SuperScope_Config, audio_source), "Audio Source", sources),
        P_SELECT(offsetof(SuperScope_Config, audio_channel), "Audio Channel", channels),
        P_SELECT(offsetof(SuperScope_Config, draw_mode), "Draw Mode", drawmodes),
        P_SELECT_X(offsetof(SuperScope_Config, example), "Example", examples),
        P_ACTION("Load Example", load_example, "Load the selected example code"),
    };

    EFFECT_INFO_GETTERS;
};

struct Code_Example {
    const char* name;
    const char* init;
    const char* point;
    const char* frame;
    const char* beat;
};

struct SuperScope_Vars : public Variables {
    double* n;
    double* b;
    double* x;
    double* y;
    double* i;
    double* v;
    double* w;
    double* h;
    double* red;
    double* green;
    double* blue;
    double* linesize;
    double* skip;
    double* drawmode;

    virtual void register_(void*);
    virtual void init(int, int, int, va_list);
};

class E_SuperScope
    : public Programmable_Effect<SuperScope_Info, SuperScope_Config, SuperScope_Vars> {
   public:
    E_SuperScope();
    virtual ~E_SuperScope();
    virtual int render(char visdata[2][2][576],
                       int isBeat,
                       int* framebuffer,
                       int* fbout,
                       int w,
                       int h);
    virtual void load_legacy(unsigned char* data, int len);
    virtual int save_legacy(unsigned char* data);

    uint32_t color_pos;

    char* help_text =
        "Superscope\0"
        "Superscope tutorial goes here\r\n"
        "But for now, here is the old text:\r\n"
        "You can specify expressions that run on Init, Frame, and on Beat.\r\n"
        "  'n' specifies the number of points to render (set this in Init, Beat, or "
        "Frame).\r\n"
        "For the 'Per Point' expression (which happens 'n' times per frame), use:\r\n"
        "  'x' and 'y' are the coordinates to draw to (-1..1)\r\n"
        "  'i' is the position of the scope (0..1)\r\n"
        "  'v' is the value at that point (-1..1).\r\n"
        "  'b' is 1 if beat, 0 if not.\r\n"
        "  'red', 'green' and 'blue' are all (0..1) and can be modified\r\n"
        "  'linesize' can be set from 1.0 to 255.0\r\n"
        "  'skip' can be set to >0 to skip drawing the current item\r\n"
        "  'drawmode' can be set to > 0 for lines, <= 0 for points\r\n"
        "  'w' and 'h' are the width and height of the screen, in pixels.\r\n"
        " Anybody want to send me better text to put here? Please :)\r\n";

    static constexpr Code_Example examples[14] = {
        {
            "Spiral",
            "n=800",
            "d=i+v*0.2; r=t+i*$PI*4; x=cos(r)*d; y=sin(r)*d",
            "t=t-0.05",
            "",
        },
        {
            "3D Scope Dish",
            "n=200",
            "iz=1.3+sin(r+i*$PI*2)*(v+0.5)*0.88; ix=cos(r+i*$PI*2)*(v+0.5)*.88; "
            "iy=-0.3+abs(cos(v*$PI)); x=ix/iz;y=iy/iz;",
            "",
            "",
        },
        {
            "Rotating Bow Thing",
            "n=80;t=0.0;",
            "r=i*$PI*2; d=sin(r*3)+v*0.5; x=cos(t+r)*d; y=sin(t-r)*d",
            "t=t+0.01",
            "",
        },
        {
            "Vertical Bouncing Scope",
            "n=100; t=0; tv=0.1;dt=1;",
            "x=t+v*pow(sin(i*$PI),2); y=i*2-1.0;",
            "t=t*0.9+tv*0.1",
            "tv=((rand(50.0)/50.0))*dt; dt=-dt;",
        },
        {
            "Spiral Graph Fun",
            "n=100;t=0;",
            "r=i*$PI*128+t; x=cos(r/64)*0.7+sin(r)*0.3; y=sin(r/64)*0.7+cos(r)*0.3",
            "t=t+0.01;",
            "n=80+rand(120.0)",
        },
        {
            "Alternating Diagonal Scope",
            "n=64; t=1;",
            "sc=0.4*sin(i*$PI); x=2*(i-0.5-v*sc)*t; y=2*(i-0.5+v*sc);",
            "",
            "t=-t;",
        },
        {
            "Vibrating Worm",
            "n=w; dt=0.01; t=0; sc=1;",
            "x=cos(2*i+t)*0.9*(v*0.5+0.5); y=sin(i*2+t)*0.9*(v*0.5+0.5);",
            "t=t+dt;dt=0.9*dt+0.001; t=if(above(t,$PI*2),t-$PI*2,t);",
            "dt=sc;sc=-sc;",
        },
        {
            "Wandering Simple",
            "n=800;xa=-0.5;ya=0.0;xb=-0.0;yb=0.75;c=200;f=0;\r\nnxa=(rand(100)-50)*.02;"
            "nya=(rand(100)-50)*.02;\r\nnxb=(rand(100)-50)*.02;nyb=(rand(100)-50)*.02;",
            "//primary render\r\nx=(ex*i)+xa;\r\ny=(ey*i)+ya;\r\n\r\n//volume "
            "offset\r\nx=x+ ( cos(r) * v * dv);\r\ny=y+ ( sin(r) * v * "
            "dv);\r\n\r\n//color values\r\nred=i;\r\ngreen=(1-i);\r\nblue=abs(v*6);",
            "f=f+1;\r\nt=1-((cos((f*3.1415)/"
            "c)+1)*.5);\r\nxa=((nxa-lxa)*t)+lxa;\r\nya=((nya-lya)*t)+lya;\r\nxb=((nxb-"
            "lxb)"
            "*t)+lxb;\r\nyb=((nyb-lyb)*t)+lyb;\r\nex=(xb-xa);\r\ney=(yb-ya);\r\nd=sqrt("
            "sqr(ex)+sqr(ey));\r\nr=atan(ey/ex)+(3.1415/2);\r\ndv=d*2",
            "c=f;\r\nf=0;\r\nlxa=nxa;\r\nlya=nya;\r\nlxb=nxb;\r\nlyb=nyb;\r\nnxa=(rand("
            "100)-50)*.02;\r\nnya=(rand(100)-50)*.02;\r\nnxb=(rand(100)-50)*.02;"
            "\r\nnyb=("
            "rand(100)-50)*.02",
        },
        {
            "Flitterbug",
            "n=180;t=0.0;lx=0;ly=0;vx=rand(200)-100;vy=rand(200)-100;cf=.97;c=200;f=0",
            "x=nx;y=ny;\r\nr=i*3.14159*2; "
            "d=(sin(r*5*(1-s))+i*0.5)*(.3-s);\r\ntx=(t*(1-(s*(i-.5))));\r\nx=x+cos(tx+"
            "r)*"
            "d; "
            "y=y+sin(t-y)*d;\r\nred=abs(x-nx)*5;\r\ngreen=abs(y-ny)*5;\r\nblue=1-s-red-"
            "green;",
            "f=f+1;t=(f*2*3.1415)/"
            "c;\r\nvx=(vx-(lx*.1))*cf;\r\nvy=(vy-(ly*.1))*cf;\r\nlx=lx+vx;ly=ly+vy;"
            "\r\nnx="
            "lx*.001;ny=ly*.001;\r\ns=abs(nx*ny)",
            "c=f;f=0;\r\nvx=vx+rand(600)-300;vy=vy+rand(600)-300",
        },
        {
            "Spirostar",
            "n=20;t=0;f=0;c=200;mn=10;dv=2;dn=0",
            "r=if(b,0,((i*dv)*3.14159*128)+(t/2));\r\nx=cos(r)*sz;\r\ny=sin(r)*sz;",
            "f=f+1;t=(f*3.1415*2)/c;\r\nsz=abs(sin(t-3.1415));\r\ndv=if(below(n,12),(n/"
            "2)-1,\r\n    if(equal(12,n),3,\r\n    if(equal(14,n),6,\r\n    "
            "if(below(n,20),2,4))))",
            "bb = bb + 1;\r\nbeatdiv = "
            "8;\r\nc=if(equal(bb%beatdiv,0),f,c);\r\nf=if(equal(bb%beatdiv,0),0,f);"
            "\r\ng="
            "if(equal(bb%beatdiv,0),g+1,g);\r\nn=if(equal(bb%beatdiv,0),(abs((g%17)-8) "
            "*2)+4,n);",
        },
        {
            "Exploding Daisy",
            "n = 380 + rand(200) ; k = 0.0; l = 0.0; m = ( rand( 10 ) + 2 ) * .5; c = "
            "0; "
            "f = 0",
            "r=(i*3.14159*2)+(a * 3.1415);\r\nd=sin(r*m)*.3;\r\nx=cos(k+r)*d*2;y=(  "
            "(sin(k-r)*d) + ( sin(l*(i-.5) ) ) ) * "
            ".7;\r\nred=abs(x);\r\ngreen=abs(y);\r\nblue=d",
            "a = a + 0.002 ; k = k + 0.04 ; l = l + 0.03",
            "bb = bb + 1;\r\nbeatdiv = 16;\r\nn=if(equal(bb%beatdiv,0),380 + "
            "rand(200),n);\r\nt=if(equal(bb%beatdiv,0),0.0,t);\r\na=if(equal(bb%"
            "beatdiv,0)"
            ",0.0,a);\r\nk=if(equal(bb%beatdiv,0),0.0,k);\r\nl=if(equal(bb%beatdiv,0),"
            "0.0,"
            "l);\r\nm=if(equal(bb%beatdiv,0),(( rand( 100  ) + 2 ) * .1) + 2,m);",
        },
        {
            "Swirlie Dots",
            "n=45;t=rand(100);u=rand(100)",
            "di = ( i - .5) * 2;\r\nx = di;sin(u*di) * .4;\r\ny = cos(u*di) * .6;\r\nx "
            "= "
            "x + ( cos(t) * .05 );\r\ny = y + ( sin(t) * .05 );",
            "t = t + .15; u = u + .05",
            "bb = bb + 1;\r\nbeatdiv = 16;\r\nn = if(equal(bb%beatdiv,0),30 + rand( 30 "
            "),n);",
        },
        {
            "Sweep",
            "n=180;lsv=100;sv=200;ssv=200;c=200;f=0",
            "sv=(sv*abs(cos(lsv)))+(lsv*abs(cos(sv)));\r\nfv=fv+(sin(sv)*dv);\r\nd=i; "
            "r=t+(fv * sin(t) * "
            ".3)*3.14159*4;\r\nx=cos(r)*d;\r\ny=sin(r)*d;\r\nred=i;\r\ngreen=abs(sin(r)"
            ")-("
            "red*.15);\r\nblue=fv",
            "f=f+1;t=(f*2*3.1415)/c;\r\nlsv=slsv;sv=ssv;fv=0",
            "bb = bb + 1;\r\nbeatdiv = "
            "8;\r\nc=if(equal(bb%beatdiv,0),f,c);\r\nf=if(equal(bb%beatdiv,0),0,f);"
            "\r\ndv="
            "if(equal(bb%beatdiv,0),((rand(100)*.01) * .1) + "
            ".02,dv);\r\nn=if(equal(bb%beatdiv,0),80+rand(100),n);\r\nssv=if(equal(bb%"
            "beatdiv,0),rand(200)+100,ssv);\r\nslsv=if(equal(bb%beatdiv,0),rand(200)+"
            "100,"
            "slsv);",
        },
        {
            "Whiplash Spiral",
            "n=80;c=200;f=0",
            "d=i;\r\nr=t+i*3.14159*4;\r\nsdt=sin(dt+(i*3.1415*2));\r\ncdt=cos(dt+(i*3."
            "1415*2));\r\nx=(cos(r)*d) + (sdt * .6 * sin(t) );\r\ny=(sin(r)*d) + ( cdt "
            "*.6 * sin(t) );\r\nblue=abs(x);\r\ngreen=abs(y);\r\nred=cos(dt*4)",
            "t=t-0.05;f=f+1;dt=(f*2*3.1415)/c",
            "bb = bb + 1;\r\nbeatdiv = "
            "8;\r\nc=if(equal(bb%beatdiv,0),f,c);\r\nf=if(equal(bb%beatdiv,0),0,f);",
        },
    };
};
