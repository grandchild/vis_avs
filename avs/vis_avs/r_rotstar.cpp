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
// alphachannel safe 11/21/99
#include "c_rotstar.h"
#include "r_defs.h"
#include <math.h>


#ifndef LASER

#define PUT_INT(y) data[pos]=(y)&255; data[pos+1]=(y>>8)&255; data[pos+2]=(y>>16)&255; data[pos+3]=(y>>24)&255
#define GET_INT() (data[pos]|(data[pos+1]<<8)|(data[pos+2]<<16)|(data[pos+3]<<24))

void C_THISCLASS::load_config(unsigned char *data, int len)
{
	int pos=0;
  int x=0;
	if (len-pos >= 4) { num_colors=GET_INT(); pos+=4; }
	if (num_colors <= 16) while (len-pos >= 4 && x < num_colors) { colors[x++]=GET_INT(); pos+=4; }
  else num_colors=0;
}

int  C_THISCLASS::save_config(unsigned char *data)
{
	int pos=0,x=0;
	PUT_INT(num_colors); pos+=4;
  while (x < num_colors) { PUT_INT(colors[x]); x++;  pos+=4; }
	return pos;
}


C_THISCLASS::C_THISCLASS()
{
  r1=0.0;
  num_colors=1;
  memset(colors,0,sizeof(colors));
  colors[0]=RGB(255,255,255);
  color_pos=0;
}

C_THISCLASS::~C_THISCLASS()
{
}
	
int C_THISCLASS::render(char visdata[2][2][576], int isBeat, int *framebuffer, int*, int w, int h)
{
	int x,y,c;
  int current_color;

  if (isBeat&0x80000000) return 0;
  if (!num_colors) return 0;
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

  x=(int) (cos(r1)*w/4.0);
  y=(int) (sin(r1)*h/4.0);
  for (c = 0; c < 2; c ++)
  {
    double r2=-r1;
    int s=0;
    int t;
    int a,b;
    int nx, ny;
    int lx,ly,l;
    a=x;
    b=y;

    for (l = 3; l < 14; l ++)
      if (visdata[0][c][l] > s &&
          visdata[0][c][l] > visdata[0][c][l+1]+4 &&
          visdata[0][c][l] > visdata[0][c][l-1]+4)
        s=visdata[0][c][l];

    if (c==1) { a=-a; b=-b; }

    double vw,vh;
    vw=w/8.0*(s+9)/88.0;
    vh=h/8.0*(s+9)/88.0;

    nx=(int) (cos(r2)*vw);
    ny=(int) (sin(r2)*vh);
    lx = w/2+a+nx;
    ly = h/2+b+ny;

    r2+=3.14159*4.0/5.0;

    for (t = 0; t < 5; t ++)
    {
      int nx, ny;
      nx=(int) (cos(r2)*vw+w/2+a);
      ny=(int) (sin(r2)*vh+h/2+b);
      r2+=3.14159*4.0/5.0;
      line(framebuffer,lx,ly,nx,ny,w,h,current_color,(g_line_blend_mode&0xff0000)>>16);
      lx=nx;
      ly=ny;
    }
  }
  r1+=0.1;
  return 0;
}

C_RBASE *R_RotStar(char *desc)
{
	if (desc) { strcpy(desc,MOD_NAME); return NULL; }
	return (C_RBASE *) new C_THISCLASS();
}

#else
C_RBASE *R_RotStar(char *desc)
{ return NULL; }
#endif
