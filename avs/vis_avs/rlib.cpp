/*
  LICENSE
  -------
Copyright 2005 Nullsoft, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer. 

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 

  * Neither the name of Nullsoft nor the names of its contributors may be used to 
    endorse or promote products derived from this software without specific prior written permission. 
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR 
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#include <windows.h>
#include "../util.h"
#include "r_defs.h"
#include "c__base.h"
#include "c_unkn.h"
#include "c_list.h"
#include "rlib.h"

#include "avs_eelif.h"

#define PUT_INT(y) data[pos]=(y)&255; data[pos+1]=(y>>8)&255; data[pos+2]=(y>>16)&255; data[pos+3]=(y>>24)&255
#define GET_INT() (data[pos]|(data[pos+1]<<8)|(data[pos+2]<<16)|(data[pos+3]<<24))

void C_RBASE::load_string(std::string &s, unsigned char *data, int &pos, int len) {
  int size=GET_INT(); pos += 4;
  if (size > 0 && len-pos >= size) {
	  s.assign((char*)(data+pos), size);
	  pos+=size;
	} else {
    s.assign("");
  }
}
void C_RBASE::save_string(unsigned char *data, int &pos, std::string &text) {
  if (!text.empty()) {
    if (text.length() > 32768) {
      MessageBox(NULL,"Yo, this is some long ass shit","FUCK!",MB_OK);
      //FUCKO
    }
    int l = text.length() + 1;
	  PUT_INT(l); pos+=4;
	  memcpy(data+pos, text.c_str(), l);
	  pos += l;
	}
  else
  { 
    PUT_INT(0); 
    pos+=4;
  }
}

void C_RLibrary::add_dofx(void *rf, int has_r2)
{
  if ((NumRetrFuncs&7)==0||!RetrFuncs)
  {
    void *newdl=(void *)malloc(sizeof(rfStruct)*(NumRetrFuncs+8));
    if (!newdl) 
    {
      MessageBox(NULL,"Out of memory","AVS Critical Error",MB_OK);
      ExitProcess(0);
    }
    memset(newdl,0,sizeof(rfStruct)*(NumRetrFuncs+8));
    if (RetrFuncs) 
    {
      memcpy(newdl,RetrFuncs,NumRetrFuncs*sizeof(rfStruct));
      free(RetrFuncs);
    }
    RetrFuncs=(rfStruct*)newdl;    
  }
  RetrFuncs[NumRetrFuncs].is_r2=has_r2;
  *((void**)&RetrFuncs[NumRetrFuncs].rf) = rf;
  NumRetrFuncs++;
}

// declarations for built-in effects
#define DECLARE_EFFECT(name) extern C_RBASE *(name)(char *desc); add_dofx((void*)name,0);
#define DECLARE_EFFECT2(name) extern C_RBASE *(name)(char *desc); add_dofx((void*)name,1);

void C_RLibrary::initfx(void)
{
  DECLARE_EFFECT(R_SimpleSpectrum);
  DECLARE_EFFECT(R_DotPlane);
  DECLARE_EFFECT(R_OscStars);
  DECLARE_EFFECT(R_FadeOut);
  DECLARE_EFFECT(R_BlitterFB);
  DECLARE_EFFECT(R_NFClear);
  DECLARE_EFFECT2(R_Blur);
  DECLARE_EFFECT(R_BSpin);
  DECLARE_EFFECT(R_Parts);
  DECLARE_EFFECT(R_RotBlit);
  DECLARE_EFFECT(R_SVP);
  DECLARE_EFFECT2(R_ColorFade);
  DECLARE_EFFECT(R_ColorClip);
  DECLARE_EFFECT(R_RotStar);
  DECLARE_EFFECT(R_OscRings);
  DECLARE_EFFECT2(R_Trans);
  DECLARE_EFFECT(R_Scat);
  DECLARE_EFFECT(R_DotGrid);
  DECLARE_EFFECT(R_Stack);
  DECLARE_EFFECT(R_DotFountain);
  DECLARE_EFFECT2(R_Water);
  DECLARE_EFFECT(R_Comment);
  DECLARE_EFFECT2(R_Brightness);
  DECLARE_EFFECT(R_Interleave);
  DECLARE_EFFECT(R_Grain);
  DECLARE_EFFECT(R_Clear);
  DECLARE_EFFECT(R_Mirror);
  DECLARE_EFFECT(R_StarField);
  DECLARE_EFFECT(R_Text);
  DECLARE_EFFECT(R_Bump);
  DECLARE_EFFECT(R_Mosaic);
  DECLARE_EFFECT(R_WaterBump);
  DECLARE_EFFECT(R_AVI);
  DECLARE_EFFECT(R_Bpm);
  DECLARE_EFFECT(R_Picture);
  DECLARE_EFFECT(R_DDM);
  DECLARE_EFFECT(R_SScope);
  DECLARE_EFFECT(R_Invert);
  DECLARE_EFFECT(R_Onetone);
  DECLARE_EFFECT(R_Timescope);
  DECLARE_EFFECT(R_LineMode);
  DECLARE_EFFECT(R_Interferences);
  DECLARE_EFFECT(R_Shift);
  DECLARE_EFFECT2(R_DMove);
  DECLARE_EFFECT(R_FastBright);
  DECLARE_EFFECT(R_DColorMod);  
}

static const struct 
{
  char id[33];
  int newidx;
} NamedApeToBuiltinTrans[] = 
{
  {"Winamp Brightness APE v1", 22},
  {"Winamp Interleave APE v1", 23},
  {"Winamp Grain APE v1", 24 },
  {"Winamp ClearScreen APE v1", 25},
  {"Nullsoft MIRROR v1", 26},
  {"Winamp Starfield v1", 27},
  {"Winamp Text v1", 28 },
  {"Winamp Bump v1", 29 },
  {"Winamp Mosaic v1", 30 },
  {"Winamp AVIAPE v1", 32},
	{"Nullsoft Picture Rendering v1", 34},
	{"Winamp Interf APE v1", 41}
};

static APEinfo ext_info=
{
  4,
  0,
  &g_line_blend_mode,
  NSEEL_VM_alloc,
  AVS_EEL_IF_VM_free,
  AVS_EEL_IF_resetvars,
  NSEEL_VM_regvar,
  NSEEL_code_compile,
  AVS_EEL_IF_Execute,
  NSEEL_code_free,
  NULL,
  getGlobalBuffer,
  NSEEL_set_compile_hooks,
  NSEEL_unset_compile_hooks,
};

void C_RLibrary::initbuiltinape(void)
{
#define ADD(sym) \
  extern C_RBASE * sym(char *desc); \
  _add_dll(0, sym, "Builtin_" #sym, 0, NULL)
#define ADD2(sym,name) \
  extern C_RBASE * sym(char *desc); \
  _add_dll(0, sym, name, 0, NULL)
#define ADD_EXT(sym,name) \
  extern C_RBASE * sym(char *desc); \
  extern void sym##_SetExtInfo(APEinfo* ape_info); \
  _add_dll(0, sym, name, 0, sym##_SetExtInfo)
#ifdef LASER
  ADD(RLASER_Cone);
  ADD(RLASER_BeatHold);
  ADD(RLASER_Line);
  ADD(RLASER_Bren); // not including it for now
  ADD(RLASER_Transform);
#else
  ADD2(R_ChannelShift,"Channel Shift");
  ADD2(R_ColorReduction,"Color Reduction");
  ADD2(R_Multiplier,"Multiplier");
  ADD2(R_VideoDelay,"Holden04: Video Delay");
  ADD2(R_MultiDelay,"Holden05: Multi Delay");
  ADD2(R_Convolution,"Holden03: Convolution Filter");
  ADD_EXT(R_Texer2,"Acko.net: Texer II");
  ADD2(R_Normalise,"Trans: Normalise");
  ADD2(R_ColorMap,"Color Map");
  ADD2(R_AddBorders,"Virtual Effect: Addborders");
  ADD_EXT(R_Triangle,"Render: Triangle");
  ADD_EXT(R_EelTrans, "Misc: AVSTrans Automation");
  ADD_EXT(R_GlobalVars, "Jheriko: Global");
  ADD2(R_MultiFilter, "Jheriko : MULTIFILTER");
#endif
#undef ADD
#undef ADD2
}


void C_RLibrary::_add_dll(HINSTANCE hlib,class C_RBASE *(__cdecl *cre)(char *),char *inf, int is_r2, void (*set_info)(APEinfo*))
{
  if ((NumDLLFuncs&7)==0||!DLLFuncs)
  {
    DLLInfo *newdl=(DLLInfo *)malloc(sizeof(DLLInfo)*(NumDLLFuncs+8));
    if (!newdl) 
    {
      MessageBox(NULL,"Out of memory","AVS Critical Error",MB_OK);
      ExitProcess(0);
    }
    memset(newdl,0,sizeof(DLLInfo)*(NumDLLFuncs+8));
    if (DLLFuncs) 
    {
      memcpy(newdl,DLLFuncs,NumDLLFuncs*sizeof(DLLInfo));
      free(DLLFuncs);
    }
    DLLFuncs=newdl;
  
  }
  DLLFuncs[NumDLLFuncs].hDllInstance=hlib;
  DLLFuncs[NumDLLFuncs].createfunc=cre;
  DLLFuncs[NumDLLFuncs].idstring=inf;
  DLLFuncs[NumDLLFuncs].is_r2=is_r2;
  DLLFuncs[NumDLLFuncs].set_info_func=set_info;
  NumDLLFuncs++;
}


void C_RLibrary::initdll()
{
  HANDLE h;
  WIN32_FIND_DATA d;
  char dirmask[MAX_PATH*2];
#ifdef LASER
  wsprintf(dirmask,"%s\\*.lpe",g_path);
#else
  wsprintf(dirmask,"%s\\*.ape",g_path);
#endif
  h = FindFirstFile(dirmask,&d);
	if (h != INVALID_HANDLE_VALUE)
	{
		do {
      char s[MAX_PATH];
      HINSTANCE hlib;
      wsprintf(s,"%s\\%s",g_path,d.cFileName);
      hlib=LoadLibrary(s);
      if (hlib)
      {
        int cre;
        char *inf;

        void (*sei)(HINSTANCE hDllInstance, APEinfo *ptr);
        *(void**)&sei = (void *) GetProcAddress(hlib,"_AVS_APE_SetExtInfo");
        if (sei)
          sei(hlib,&ext_info);

#ifdef LASER
        int (*retr)(HINSTANCE hDllInstance, char **info, int *create, C_LineListBase *linelist);
        retr = (int (*)(HINSTANCE, char ** ,int *, C_LineListBase*)) GetProcAddress(hlib,"_AVS_LPE_RetrFunc");
        if (retr && retr(hlib,&inf,&cre,g_laser_linelist))
        {
          _add_dll(hlib,(class C_RBASE *(__cdecl *)(char *))cre,inf,0, NULL);
        }
        else FreeLibrary(hlib);
#else
        int (*retr)(HINSTANCE hDllInstance, char **info, int *create);
        retr = FORCE_FUNCTION_CAST(int (*)(HINSTANCE, char ** ,int *)) GetProcAddress(hlib,"_AVS_APE_RetrFuncEXT2");
        if (retr && retr(hlib,&inf,&cre))
        {
          _add_dll(hlib,(class C_RBASE *(__cdecl *)(char *))cre,inf,1, NULL);
        }
        else
        {
          retr = FORCE_FUNCTION_CAST(int (*)(HINSTANCE, char ** ,int *)) GetProcAddress(hlib,"_AVS_APE_RetrFunc");
          if (retr && retr(hlib,&inf,&cre))
          {
            _add_dll(hlib,(class C_RBASE *(__cdecl *)(char *))cre,inf,0, NULL);
          }
          else FreeLibrary(hlib);
        }
#endif
      }	  	
		} while (FindNextFile(h,&d));
		FindClose(h);
  }	
}


int C_RLibrary::GetRendererDesc(int which, char *str)
{
  *str=0;
  if (which >= 0 && which < NumRetrFuncs)
  {    
    RetrFuncs[which].rf(str);
    return 1;
  }
  if (which >= DLLRENDERBASE)
  {
    which-=DLLRENDERBASE;
    if (which < NumDLLFuncs)
    {
      DLLFuncs[which].createfunc(str);
      return (int)DLLFuncs[which].idstring;
    }
  }
  return 0;
}

C_RBASE *C_RLibrary::CreateRenderer(int *which, int *has_r2)
{
  if (has_r2) *has_r2=0;

  if (*which >= 0 && *which < NumRetrFuncs)
  {
    if (has_r2) *has_r2 = RetrFuncs[*which].is_r2;
    return RetrFuncs[*which].rf(NULL);
  }

  if (*which == LIST_ID)
    return (C_RBASE *) new C_RenderListClass();

  if (*which >= DLLRENDERBASE)
  {
    int x;
    char *p=(char *)*which;
    for (x = 0; x < NumDLLFuncs; x ++)
    {
      if (!DLLFuncs[x].createfunc) break;
      if (DLLFuncs[x].idstring)
      {
        if (!strncmp(p,DLLFuncs[x].idstring,32))
        {
          *which=(int)DLLFuncs[x].idstring;
          if (DLLFuncs[x].set_info_func) {
            DLLFuncs[x].set_info_func(&ext_info);
          }
          return DLLFuncs[x].createfunc(NULL);
        }
      }
    }
    for (x = 0; x < ssizeof32(NamedApeToBuiltinTrans)/ssizeof32(NamedApeToBuiltinTrans[0]); x ++)
    {
      if (!strncmp(p,NamedApeToBuiltinTrans[x].id,32))
      {
        *which=NamedApeToBuiltinTrans[x].newidx;
        if (has_r2) *has_r2 = RetrFuncs[*which].is_r2;
        return RetrFuncs[*which].rf(NULL);
      }
    }
  }
  int r=*which;
  *which=UNKN_ID;
  C_UnknClass *p=new C_UnknClass();
  p->SetID(r,(r >= DLLRENDERBASE)?(char*)r:(char*)"");
  return (C_RBASE *)p;
}

C_RLibrary::C_RLibrary()
{
  DLLFuncs=NULL;
  NumDLLFuncs=0;
  RetrFuncs=0;
  NumRetrFuncs=0;
  ext_info.global_registers=NSEEL_getglobalregs();

  initfx();
  initdll();
  initbuiltinape();

}

C_RLibrary::~C_RLibrary()
{
  if (RetrFuncs) free(RetrFuncs);
  RetrFuncs=0;
  NumRetrFuncs=0;

  if (DLLFuncs)
  {
    int x;
    for (x = 0; x < NumDLLFuncs; x ++)
    {
      if (DLLFuncs[x].hDllInstance)
        FreeLibrary(DLLFuncs[x].hDllInstance);
    }
    free(DLLFuncs);
  }
  DLLFuncs=NULL;
  NumDLLFuncs=0;
}

HINSTANCE C_RLibrary::GetRendererInstance(int which, HINSTANCE hThisInstance)
{
  if (which < DLLRENDERBASE || which == UNKN_ID || which == LIST_ID) return hThisInstance;
  int x;
  char *p=(char *)which;
  for (x = 0; x < NumDLLFuncs; x ++)
  {
    if (DLLFuncs[x].idstring)
    {
      if (!strncmp(p,DLLFuncs[x].idstring,32))
      {
        if (DLLFuncs[x].hDllInstance)
          return DLLFuncs[x].hDllInstance;
        break;
      }
    }
  }
  return hThisInstance;
}


void *g_n_buffers[NBUF];
int g_n_buffers_w[NBUF],g_n_buffers_h[NBUF];


void *getGlobalBuffer(int w, int h, int n, int do_alloc)
{
  if (n < 0 || n >= NBUF) return 0;

  if (!g_n_buffers[n] || g_n_buffers_w[n] != w || g_n_buffers_h[n] != h)
  {
    if (g_n_buffers[n]) free(g_n_buffers[n]);
    if (do_alloc)
    {
      g_n_buffers_w[n]=w;
      g_n_buffers_h[n]=h;
      return g_n_buffers[n]=calloc(w*h, sizeof(int));
    }

    g_n_buffers[n]=NULL;
    g_n_buffers_w[n]=0;
    g_n_buffers_h[n]=0;

    return 0;
  }
  return g_n_buffers[n];
}

