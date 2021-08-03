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
#include "c_sscope.h"
#include <math.h>
#include "r_defs.h"
#include "avs_eelif.h"
#if 0//syntax highlighting
#include "richedit.h"
#endif
#include "timing.h"


#define PUT_INT(y) data[pos]=(y)&255; data[pos+1]=(y>>8)&255; data[pos+2]=(y>>16)&255; data[pos+3]=(y>>24)&255
#define GET_INT() (data[pos]|(data[pos+1]<<8)|(data[pos+2]<<16)|(data[pos+3]<<24))
void C_THISCLASS::load_config(unsigned char *data, int len)
{
	int pos=0;
  int x=0;
  if (data[pos] == 1)
  {
    pos++;
    load_string(effect_exp[0],data,pos,len);
    load_string(effect_exp[1],data,pos,len);
    load_string(effect_exp[2],data,pos,len);
    load_string(effect_exp[3],data,pos,len);
  }
  else
  {
    char buf[1025];
    if (len-pos >= 1024)
    {
      memcpy(buf,data+pos,1024);
      pos+=1024;
      buf[1024]=0;
      effect_exp[3].assign(buf+768);
      buf[768]=0;
      effect_exp[2].assign(buf+512);
      buf[512]=0;
      effect_exp[1].assign(buf+256);
      buf[256]=0;
      effect_exp[0].assign(buf);
    }
  }
	if (len-pos >= 4) { which_ch=GET_INT(); pos+=4; }
	if (len-pos >= 4) { num_colors=GET_INT(); pos+=4; }
	if (num_colors <= 16) while (len-pos >= 4 && x < num_colors) { colors[x++]=GET_INT(); pos+=4; }
  else num_colors=0;
  if (len-pos >= 4) { mode=GET_INT(); pos+=4; }
  need_recompile=1;
}

int  C_THISCLASS::save_config(unsigned char *data)
{
  int x=0;
	int pos=0;
  data[pos++]=1;
  save_string(data,pos,effect_exp[0]);
  save_string(data,pos,effect_exp[1]);
  save_string(data,pos,effect_exp[2]);
  save_string(data,pos,effect_exp[3]);
	PUT_INT(which_ch); pos+=4;
	PUT_INT(num_colors); pos+=4;
  while (x < num_colors) { PUT_INT(colors[x]); x++;  pos+=4; }
  PUT_INT(mode); pos+=4;
	return pos;
}


C_THISCLASS::C_THISCLASS()
{
  InitializeCriticalSection(&rcs);
  AVS_EEL_INITINST();
#ifdef LASER
  mode=1;
#else
  mode=0;
#endif


  need_recompile=1;
  which_ch=2;
  num_colors=1;
  memset(colors,0,sizeof(colors));
  colors[0]=RGB(255,255,255);
  color_pos=0;
  memset(codehandle,0,sizeof(codehandle));

#ifdef LASER
  effect_exp[0].assign("d=i+v*0.2; r=t+i*$PI*4; x=cos(r)*d; y=sin(r)*d");
  effect_exp[1].assign("t=t-0.05");
  effect_exp[2].assign("");
  effect_exp[3].assign("n=100");
#else
  effect_exp[0].assign("d=i+v*0.2; r=t+i*$PI*4; x=cos(r)*d; y=sin(r)*d");
  effect_exp[1].assign("t=t-0.05");
  effect_exp[2].assign("");
  effect_exp[3].assign("n=800");
#endif

  var_n=0;
}

C_THISCLASS::~C_THISCLASS()
{
  int x;
  for (x = 0; x < 4; x ++) 
  {
    freeCode(codehandle[x]);
    codehandle[x]=0;
  }
  AVS_EEL_QUITINST();
  DeleteCriticalSection(&rcs);
}

static __inline int makeint(double t)
{
  if (t <= 0.0) return 0;
  if (t >= 1.0) return 255;
  return (int)(t*255.0);
}
	
int C_THISCLASS::render(char visdata[2][2][576], int isBeat, int *framebuffer, int*, int w, int h)
{
  if (need_recompile)
  {
    EnterCriticalSection(&rcs);

    if (!var_n || g_reset_vars_on_recompile) 
    {
      clearVars();
      var_n = registerVar("n");
      var_b = registerVar("b");
      var_x = registerVar("x");
      var_y = registerVar("y");
      var_i = registerVar("i");
      var_v = registerVar("v");
      var_w = registerVar("w");
      var_h = registerVar("h");
      var_red = registerVar("red");
      var_green = registerVar("green");
      var_blue = registerVar("blue");
      var_linesize = registerVar("linesize");
      var_skip = registerVar("skip");
      var_drawmode = registerVar("drawmode");
      *var_n=100.0;
      inited=0;
    }
  
    need_recompile=0;
    int x;
    for (x = 0; x < 4; x ++) 
    {
      freeCode(codehandle[x]);
      codehandle[x]=compileCode((char*)effect_exp[x].c_str());
    }

    LeaveCriticalSection(&rcs);
  }
  if (isBeat&0x80000000) return 0;

  if (!num_colors) return 0;

  int x;
  int current_color;
  unsigned char *fa_data;
  char center_channel[576];
  int ws=(which_ch&4)?1:0;
  int xorv=(ws*128)^128;

  if ((which_ch&3) >=2)
  {
    for (x = 0; x < 576; x ++) center_channel[x]=visdata[ws^1][0][x]/2+visdata[ws^1][1][x]/2;
    fa_data=(unsigned char *)center_channel;
  }
  else fa_data=(unsigned char *)&visdata[ws^1][which_ch&3][0];

  color_pos++;
  if (color_pos >= num_colors * 64) color_pos=0;

  {
    int p=color_pos/64;
    int r=color_pos&63;
    int c1,c2;
    int r1,r2,r3;
    c1=colors[p];
    if (p+1 < num_colors)
      c2=colors[p+1];
    else c2=colors[0];

    r1=(((c1&255)*(63-r))+((c2&255)*r))/64;
    r2=((((c1>>8)&255)*(63-r))+(((c2>>8)&255)*r))/64;
    r3=((((c1>>16)&255)*(63-r))+(((c2>>16)&255)*r))/64;
    
    current_color=r1|(r2<<8)|(r3<<16);
  }

  *var_h=h;
  *var_w=w;
  *var_b=isBeat?1.0:0.0;
  *var_blue=(current_color&0xff)/255.0;
  *var_green=((current_color>>8)&0xff)/255.0;
  *var_red=((current_color>>16)&0xff)/255.0;
  *var_skip=0.0;
  *var_linesize = (double) ((g_line_blend_mode&0xff0000)>>16);
  *var_drawmode = mode ? 1.0 : 0.0;
  if (codehandle[3] && !inited) { executeCode(codehandle[3],visdata); inited=1; }
  executeCode(codehandle[1],visdata);
  if (isBeat) executeCode(codehandle[2],visdata);
  if (codehandle[0]) 
  {
    int candraw=0,lx=0,ly=0;
#ifdef LASER
    double dlx=0.0,dly=0.0;
#endif
    int a;
    int l=(int)*var_n;
    if (l > 128*1024) l = 128*1024;
    for (a = 0; a < l; a ++)
    {
      int x,y;
      double r=(a*576.0)/l;
      double s1=r-(int)r;
      double yr=(fa_data[(int)r]^xorv)*(1.0f-s1)+(fa_data[(int)r+1]^xorv)*(s1);
      *var_v = yr/128.0 - 1.0;
      *var_i = (double)a/(double)(l-1);
      *var_skip=0.0;
      executeCode(codehandle[0],visdata);
      x=(int)((*var_x+1.0)*w*0.5);
      y=(int)((*var_y+1.0)*h*0.5);
      if (*var_skip < 0.00001)
      {
        int thiscolor=makeint(*var_blue)|(makeint(*var_green)<<8)|(makeint(*var_red)<<16);
        if (*var_drawmode < 0.00001)
        {
          if (y >= 0 && y < h && x >= 0 && x < w) 
          {
  #ifdef LASER
            laser_drawpoint((float)*var_x,(float)*var_y,thiscolor);
  #else
            BLEND_LINE(framebuffer+x+y*w,thiscolor);
  #endif
          }
        }
        else
        {
          if (candraw)
          {
  #ifdef LASER
            LineType l;
            l.color=thiscolor;
            l.mode=0;
            l.x1=(float)*var_x;
            l.y1=(float)*var_y;
            l.x2=(float)dlx;
            l.y2=(float)dly;
            g_laser_linelist->AddLine(&l);
  #else
            if ((thiscolor&0xffffff) || (g_line_blend_mode&0xff)!=1)
            {
              line(framebuffer,lx,ly,x,y,w,h,thiscolor,(int) (*var_linesize+0.5));
            }
  #endif
          } // candraw
        } // line
      } // skip
      candraw=1;
      lx=x;
      ly=y;
  #ifdef LASER
      dlx=*var_x;
      dly=*var_y;
  #endif
    }
  }
 
  return 0;
}

C_RBASE *R_SScope(char *desc)
{
	if (desc) { strcpy(desc,MOD_NAME); return NULL; }
	return (C_RBASE *) new C_THISCLASS();
}
