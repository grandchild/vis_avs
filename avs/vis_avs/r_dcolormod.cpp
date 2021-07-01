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
#include "c_dcolormod.h"
#include <math.h>
#include "r_defs.h"
#include "avs_eelif.h"
#include "timing.h"


#ifndef LASER

#define PUT_INT(y) data[pos]=(y)&255; data[pos+1]=(y>>8)&255; data[pos+2]=(y>>16)&255; data[pos+3]=(y>>24)&255
#define GET_INT() (data[pos]|(data[pos+1]<<8)|(data[pos+2]<<16)|(data[pos+3]<<24))
void C_THISCLASS::load_config(unsigned char *data, int len)
{
	int pos=0;
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
	if (len-pos >= 4) { m_recompute=GET_INT(); pos+=4; }

}
int  C_THISCLASS::save_config(unsigned char *data)
{
	int pos=0;
  data[pos++]=1;
  save_string(data,pos,effect_exp[0]);
  save_string(data,pos,effect_exp[1]);
  save_string(data,pos,effect_exp[2]);
  save_string(data,pos,effect_exp[3]);
	PUT_INT(m_recompute); pos+=4;
	return pos;
}



C_THISCLASS::C_THISCLASS()
{
  AVS_EEL_INITINST();
  InitializeCriticalSection(&rcs);
  need_recompile=1;
  m_recompute=1;
  memset(codehandle,0,sizeof(codehandle));
  effect_exp[0].assign("");
  effect_exp[1].assign("");
  effect_exp[2].assign("");
  effect_exp[3].assign("");

  var_beat=0;
  m_tab_valid=0;
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


int C_THISCLASS::render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h)
{
  if (need_recompile)
  {
    EnterCriticalSection(&rcs);
    if (!var_beat || g_reset_vars_on_recompile)
    {
      clearVars();
      var_r = registerVar("red");
      var_g = registerVar("green");
      var_b = registerVar("blue");
      var_beat = registerVar("beat");
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

  *var_beat=isBeat?1.0:0.0;

  if (codehandle[3] && !inited) { executeCode(codehandle[3],visdata); inited=1; }
  executeCode(codehandle[1],visdata);

  if (isBeat) executeCode(codehandle[2],visdata);

  if (m_recompute || !m_tab_valid)
  {
    int x;
    unsigned char *t=m_tab;
    for (x = 0; x < 256; x ++)
    {
      *var_r=*var_b=*var_g=x/255.0;
      executeCode(codehandle[0],visdata);
      int r=(int) (*var_r*255.0 + 0.5);
      int g=(int) (*var_g*255.0 + 0.5);
      int b=(int) (*var_b*255.0 + 0.5);
      if (r < 0) r=0;
      else if (r > 255)r=255;
      if (g < 0) g=0;
      else if (g > 255)g=255;
      if (b < 0) b=0;
      else if (b > 255)b=255;
      t[512]=r;
      t[256]=g;
      t[0]=b;
      t++;
    }
    m_tab_valid=1;
  }

  unsigned char *fb=(unsigned char *)framebuffer;
  int l=w*h;
  while (l--)
  {
    fb[0]=m_tab[fb[0]];
    fb[1]=m_tab[(int)fb[1]+256];
    fb[2]=m_tab[(int)fb[2]+512];
    fb+=4;
  }


  return 0;
}

C_RBASE *R_DColorMod(char *desc)
{
	if (desc) { strcpy(desc,MOD_NAME); return NULL; }
	return (C_RBASE *) new C_THISCLASS();
}

#else
C_RBASE *R_DColorMod(char *desc) { return NULL; }
#endif
