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
#include "c_picture.h"
#include <stdlib.h>
#include "r_defs.h"


#ifndef LASER

C_THISCLASS::C_THISCLASS() // set up default configuration
{
  persistCount=0;
  enabled=1;
	blend=0;
  adapt=0;
  blendavg=1;
  persist=6;
	strcpy(ascName,"");
	hb=0; ratio=0; axis_ratio=0;
	hBitmapDC=0; hBitmapDC2=0;
}
C_THISCLASS::~C_THISCLASS()
{
	freePicture();
}

void C_THISCLASS::loadPicture(char *name)
{
	freePicture();

	char longName[MAX_PATH];
	wsprintf(longName,"%s\\%s",g_path,name);
	hb=(HBITMAP)LoadImage(0,longName,IMAGE_BITMAP,0,0,LR_LOADFROMFILE);
  
	BITMAP bm;
	GetObject(hb, sizeof(bm), (LPSTR)&bm);
  width=bm.bmWidth;
  height=bm.bmHeight;

	lastWidth=lastHeight=0;
}
void C_THISCLASS::freePicture()
{
	if(hb)
		{
		DeleteObject(hb);
		hb=0;
		}
}

#define GET_INT() (data[pos]|(data[pos+1]<<8)|(data[pos+2]<<16)|(data[pos+3]<<24))
void C_THISCLASS::load_config(unsigned char *data, int len) // read configuration of max length "len" from data.
{
	int pos=0;
	if (len-pos >= 4) { enabled=GET_INT(); pos+=4; }
	if (len-pos >= 4) { blend=GET_INT(); pos+=4; }
	if (len-pos >= 4) { blendavg=GET_INT(); pos+=4; }
	if (len-pos >= 4) { adapt=GET_INT(); pos+=4; }
	if (len-pos >= 4) { persist=GET_INT(); pos+=4; }

	char *p=ascName;
	while (data[pos] && len-pos > 0) *p++=data[pos++];
	*p=0; pos++;

	if (len-pos >= 4) { ratio=GET_INT(); pos+=4; }
	if (len-pos >= 4) { axis_ratio=GET_INT(); pos+=4; }

	if (*ascName)
		loadPicture(ascName);
}

#define PUT_INT(y) data[pos]=(y)&255; data[pos+1]=(y>>8)&255; data[pos+2]=(y>>16)&255; data[pos+3]=(y>>24)&255
int  C_THISCLASS::save_config(unsigned char *data) // write configuration to data, return length. config data should not exceed 64k.
{
	int pos=0;
	PUT_INT(enabled); pos+=4;
  PUT_INT(blend); pos+=4;
  PUT_INT(blendavg); pos+=4;
  PUT_INT(adapt); pos+=4;
  PUT_INT(persist); pos+=4;
  strcpy((char *)data+pos, ascName);
  pos+=strlen(ascName)+1;
  PUT_INT(ratio); pos+=4;
  PUT_INT(axis_ratio); pos+=4;
	return pos;
}

// render function
// render should return 0 if it only used framebuffer, or 1 if the new output data is in fbout. this is
// used when you want to do something that you'd otherwise need to make a copy of the framebuffer.
// w and h are the width and height of the screen, in pixels.
// isBeat is 1 if a beat has been detected.
// visdata is in the format of [spectrum:0,wave:1][channel][band].
int C_THISCLASS::render(char[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h)
{

  if (!enabled) return 0;

  if (!width || !height) return 0;


	if (lastWidth != w || lastHeight != h) 
  {
		lastWidth = w;
		lastHeight = h;

		if(hBitmapDC2) {
			DeleteDC(hBitmapDC2);
			DeleteObject(hb2);
			hBitmapDC2=0;
		}

		// Copy the bitmap from hBitmapDC to hBitmapDC2 and stretch it
	  hBitmapDC = CreateCompatibleDC (NULL); 
	  hOldBitmap = (HBITMAP) SelectObject (hBitmapDC, hb); 
		hBitmapDC2 = CreateCompatibleDC (NULL);
		hb2=CreateCompatibleBitmap (hBitmapDC, w, h);
		SelectObject(hBitmapDC2,hb2);
		{
			HBRUSH b=CreateSolidBrush(0);
			HPEN p=CreatePen(PS_SOLID,0,0);
			HBRUSH bold;
			HPEN pold;
			bold=(HBRUSH)SelectObject(hBitmapDC2,b);
			pold=(HPEN)SelectObject(hBitmapDC2,p);
			Rectangle(hBitmapDC2,0,0,w,h);
			SelectObject(hBitmapDC2,bold);
			SelectObject(hBitmapDC2,pold);
			DeleteObject(b);
			DeleteObject(p);
		}
		SetStretchBltMode(hBitmapDC2,COLORONCOLOR);
		int final_height=h,start_height=0;
		int final_width=w,start_width=0;
		if (ratio)
    {
			if(axis_ratio==0) {
				// ratio on X axis
				final_height=height*w/width;
				start_height=(h/2)-(final_height/2);
			} else {
				// ratio on Y axis
				final_width=width*h/height;
				start_width=(w/2)-(final_width/2);
			}
    }
		StretchBlt(hBitmapDC2,start_width,start_height,final_width,final_height,hBitmapDC,0,0,width,height,SRCCOPY);
		DeleteDC(hBitmapDC);
		hBitmapDC=0;
	}
  if (isBeat&0x80000000) return 0;

	// Copy the stretched bitmap to fbout
	BITMAPINFO bi;
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bi.bmiHeader.biWidth = w;
  bi.bmiHeader.biHeight = h;
  bi.bmiHeader.biPlanes = 1;
  bi.bmiHeader.biBitCount = 32;
  bi.bmiHeader.biCompression = BI_RGB;
  bi.bmiHeader.biSizeImage = 0;
  bi.bmiHeader.biXPelsPerMeter = 0;
  bi.bmiHeader.biYPelsPerMeter = 0;
  bi.bmiHeader.biClrUsed = 0;
  bi.bmiHeader.biClrImportant = 0;
	GetDIBits(hBitmapDC2, hb2, 0, h, (void *)fbout, &bi, DIB_RGB_COLORS);

	// Copy the bitmap from fbout to framebuffer applying replace/blend/etc...
	if (isBeat)
		persistCount=persist;
  else
    if (persistCount>0) persistCount--;

	int *p,*d;
	int i,j;

  p = fbout;
  d = framebuffer+w*(h-1);
  if (blend || (adapt && (isBeat || persistCount)))
   for (i=0;i<h;i++)
	{
	for (j=0;j<w;j++)
		{
		*d=BLEND(*p, *d);
		d++;
		p++;
		}
	d -= w*2;
	}
  else
  if (blendavg || adapt)
   for (i=0;i<h;i++)
	{
	for (j=0;j<w;j++)
		{
		*d=BLEND_AVG(*p, *d);
		d++;
		p++;
		}
	d -= w*2;
	}
  else
   for (i=0;i<h;i++)
	{
	memcpy(d, p, w*4);
	p+=w;
	d-=w;
	}

  return 0;
}


C_RBASE *R_Picture(char *desc) // creates a new effect object if desc is NULL, otherwise fills in desc with description
{
	if (desc) { strcpy(desc,MOD_NAME); return NULL; }
	return (C_RBASE *) new C_THISCLASS();
}

#else
C_RBASE *R_Picture(char *desc) // creates a new effect object if desc is NULL, otherwise fills in desc with description
{
return NULL;
}
#endif
