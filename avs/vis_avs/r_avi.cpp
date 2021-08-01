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
#include "c_avi.h"
#include <stdlib.h>
#include "r_defs.h"


#ifndef LASER

C_THISCLASS::C_THISCLASS() // set up default configuration
{
  AVIFileInit ( ) ; 
  hDrawDib = DrawDibOpen ( ) ; 
  lastWidth=0;
  lastHeight=0;
  lFrameIndex=0;
  loaded=0;
  rendering=0;
  length=0;
  blend=0;
  adapt=0;
  blendavg=1;
  persist=6;
  enabled=1;
  speed=0;
  lastspeed=0;
  old_image=NULL; old_image_h=0; old_image_w=0;
}

C_THISCLASS::~C_THISCLASS()
{
   closeAvi();
   SelectObject (hBitmapDC, hOldBitmap); 
   DeleteDC (hBitmapDC); 
   ReleaseDC (NULL, hDesktopDC); 
   AVIFileExit ( ); 
   DrawDibClose (hDrawDib); 
   if(old_image) {
	   free(old_image);
	   old_image=NULL;
	   old_image_h=old_image_w=0;
   }
}

void C_THISCLASS::loadAvi(char *name)
{
  char pathfile[MAX_PATH];

  if (loaded) closeAvi();

  wsprintf(pathfile,"%s\\%s",g_path, name);

  if (AVIStreamOpenFromFile ((PAVISTREAM FAR *) &PAVIVideo, pathfile, streamtypeVIDEO, 0, OF_READ | OF_SHARE_EXCLUSIVE, NULL) != 0)
  {
  //	MessageBox(NULL, "An error occured while trying to open a file. Effect is disabled", "Warning", 0);
	    return;
	}
  PgetFrame = AVIStreamGetFrameOpen (PAVIVideo, NULL); 
  length = AVIStreamLength(PAVIVideo);
  lFrameIndex = 0;

  lpFrame = (LPBITMAPINFOHEADER) AVIStreamGetFrame (PgetFrame, 0); 

  reinit(lastWidth, lastHeight);

  loaded=1;
}

void C_THISCLASS::closeAvi(void)
{
if (loaded)
	{
	while (rendering);
    loaded=0;
    AVIStreamGetFrameClose (PgetFrame); 
    AVIStreamRelease (PAVIVideo); 
	}
}

#define GET_INT() (data[pos]|(data[pos+1]<<8)|(data[pos+2]<<16)|(data[pos+3]<<24))
void C_THISCLASS::load_config(unsigned char *data, int len) // read configuration of max length "len" from data.
{
	int pos=0;
	char *p=ascName;
	if (len-pos >= 4) { enabled=GET_INT(); pos+=4; }
	if (len-pos >= 4) { blend=GET_INT(); pos+=4; }
	if (len-pos >= 4) { blendavg=GET_INT(); pos+=4; }
	while (data[pos] && len-pos > 0 && p-ascName < sizeof(ascName)-1) *p++=data[pos++];
	*p=0; pos++;
	if (len-pos >= 4) { adapt=GET_INT(); pos+=4; }
	if (len-pos >= 4) { persist=GET_INT(); pos+=4; }
	if (len-pos >= 4) { speed=GET_INT(); pos+=4; }

	if (*ascName)
		loadAvi(ascName);
}

#define PUT_INT(y) data[pos]=(y)&255; data[pos+1]=(y>>8)&255; data[pos+2]=(y>>16)&255; data[pos+3]=(y>>24)&255
int  C_THISCLASS::save_config(unsigned char *data) // write configuration to data, return length. config data should not exceed 64k.
{
  int pos=0;
  PUT_INT(enabled); pos+=4;
  PUT_INT(blend); pos+=4;
  PUT_INT(blendavg); pos+=4;
  strcpy((char *)data+pos, ascName);
  pos+=strlen(ascName)+1;
  PUT_INT(adapt); pos+=4;
  PUT_INT(persist); pos+=4;
  PUT_INT(speed); pos+=4;
  return pos;
}


void C_THISCLASS::reinit(int w, int h)
{
if (lastWidth || lastHeight)
   {
   SelectObject (hBitmapDC, hOldBitmap); 
   DeleteDC (hBitmapDC); 
   ReleaseDC (NULL, hDesktopDC); 
   }

   hDesktopDC = GetDC (NULL); 
   hRetBitmap = CreateCompatibleBitmap (hDesktopDC, w, h);
   hBitmapDC = CreateCompatibleDC (hDesktopDC); 
   hOldBitmap = (HBITMAP) SelectObject (hBitmapDC, hRetBitmap); 
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
}

// render function
// render should return 0 if it only used framebuffer, or 1 if the new output data is in fbout. this is
// used when you want to do something that you'd otherwise need to make a copy of the framebuffer.
// w and h are the width and height of the screen, in pixels.
// isBeat is 1 if a beat has been detected.
// visdata is in the format of [spectrum:0,wave:1][channel][band].

int C_THISCLASS::render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h)
{
  int *p,*d;
  int i,j;
  static int persistCount=0;

  if (!enabled || !loaded) return 0;

  if(h!=old_image_h||w!=old_image_w) {
	  if(old_image)
		  free(old_image);
	  old_image=(int *)malloc(sizeof(int)*w*h);
	  old_image_h=h; old_image_w=w;
  }

  if((lastspeed+speed)>GetTickCount()) {
	  memcpy(fbout,old_image,sizeof(int)*w*h);
  } else {
	  lastspeed=GetTickCount();

	  rendering=1;

	if (lastWidth != w || lastHeight != h)
		{
		lastWidth = w;
		lastHeight = h;
		reinit(w, h);
		}
  if (isBeat&0x80000000) return 0;

  if (!length) return 0;

	lFrameIndex %= length;
	lpFrame = (LPBITMAPINFOHEADER) AVIStreamGetFrame (PgetFrame, lFrameIndex++); 
	DrawDibDraw (hDrawDib, hBitmapDC, 0, 0, lastWidth, lastHeight, lpFrame,
	NULL, 0, 0, (int) lpFrame ->biWidth, (int) lpFrame ->biHeight, 0); 
	GetDIBits(hBitmapDC, hRetBitmap, 0, h, (void *)fbout, &bi, DIB_RGB_COLORS);
	rendering=0;
	memcpy(old_image,fbout,sizeof(int)*w*h);
  }

  if (isBeat)
	persistCount=persist;
  else
    if (persistCount>0) persistCount--;

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

C_RBASE *R_AVI(char *desc) // creates a new effect object if desc is NULL, otherwise fills in desc with description
{
	if (desc) { strcpy(desc,MOD_NAME); return NULL; }
	return (C_RBASE *) new C_THISCLASS();
}

#else//!LASER
C_RBASE *R_AVI(char *desc) { return NULL; }
#endif
