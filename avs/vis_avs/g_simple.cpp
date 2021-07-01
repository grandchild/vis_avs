#include "g__lib.h"
#include "g__defs.h"
#include "c_simple.h"
#include "resource.h"
#include <windows.h>


int win32_dlgproc_simple(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
  C_THISCLASS* g_this = (C_THISCLASS*)g_current_render;

        const static int chex[8]=
        {
          IDC_SA,IDC_SOLID,
          IDC_SA,IDC_LINES,
          IDC_OSC,IDC_LINES,
          IDC_OSC,IDC_SOLID,
        };
	int *a=NULL;
	switch (uMsg)
	{
		case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *di=(DRAWITEMSTRUCT *)lParam;
			if (di->CtlID == IDC_DEFCOL && g_this->num_colors>0)
			{
      	int x;
        int w=di->rcItem.right-di->rcItem.left;
        int l=0,nl;
        for (x = 0; x < g_this->num_colors; x ++)
        {
          unsigned int color=g_this->colors[x];
          nl = (w*(x+1))/g_this->num_colors;
          color = ((color>>16)&0xff)|(color&0xff00)|((color<<16)&0xff0000);

	        HPEN hPen,hOldPen;
	        HBRUSH hBrush,hOldBrush;
	        LOGBRUSH lb={BS_SOLID,color,0};
	        hPen = (HPEN)CreatePen(PS_SOLID,0,color);
	        hBrush = CreateBrushIndirect(&lb);
	        hOldPen=(HPEN)SelectObject(di->hDC,hPen);
	        hOldBrush=(HBRUSH)SelectObject(di->hDC,hBrush);
	        Rectangle(di->hDC,di->rcItem.left+l,di->rcItem.top,di->rcItem.left+nl,di->rcItem.bottom);
	        SelectObject(di->hDC,hOldPen);
	        SelectObject(di->hDC,hOldBrush);
	        DeleteObject(hBrush);
	        DeleteObject(hPen);
          l=nl;
			  }
      }
	  }
		return 0;
		case WM_INITDIALOG:
      if ((g_this->effect>>6)&1)
      {
        CheckDlgButton(hwndDlg,IDC_DOT,BST_CHECKED);
        if (g_this->effect&2)
          CheckDlgButton(hwndDlg,IDC_OSC,BST_CHECKED);
        else
          CheckDlgButton(hwndDlg,IDC_SA,BST_CHECKED);
      }
      else
      {
        CheckDlgButton(hwndDlg,chex[(g_this->effect&3)*2],BST_CHECKED);
        CheckDlgButton(hwndDlg,chex[(g_this->effect&3)*2+1],BST_CHECKED);
      }
      
      switch ((g_this->effect>>2)&3)
      {
        case 0: CheckDlgButton(hwndDlg,IDC_LEFTCH,BST_CHECKED); break;
        case 1: CheckDlgButton(hwndDlg,IDC_RIGHTCH,BST_CHECKED); break;
        case 2: CheckDlgButton(hwndDlg,IDC_MIDCH,BST_CHECKED); break;
      }
      switch ((g_this->effect>>4)&3)
      {
        case 0: CheckDlgButton(hwndDlg,IDC_TOP,BST_CHECKED); break;
        case 1: CheckDlgButton(hwndDlg,IDC_BOTTOM,BST_CHECKED); break;
        case 2: CheckDlgButton(hwndDlg,IDC_CENTER,BST_CHECKED); break;
      }
      SetDlgItemInt(hwndDlg,IDC_NUMCOL,g_this->num_colors,FALSE);
    return 1;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
        case IDC_DOT: case IDC_SA: case IDC_SOLID: case IDC_OSC: case IDC_LINES: 
          if (!IsDlgButtonChecked(hwndDlg,IDC_DOT))
          {
            int c;
            
            g_this->effect &= ~(1<<6);
            for (c = 0; c < 4; c ++)
            {
                if (IsDlgButtonChecked(hwndDlg,chex[c*2]) && IsDlgButtonChecked(hwndDlg,chex[c*2+1])) break;
            }
            if (c!=4) { g_this->effect&=~3; g_this->effect|=c;}
          }
          else
          {
            g_this->effect&=~3;
            if (!IsDlgButtonChecked(hwndDlg,IDC_SA)) g_this->effect|=2;
            g_this->effect |= 1<<6;
          }
        break;
				case IDC_LEFTCH: g_this->effect&=~12; break;
				case IDC_RIGHTCH: g_this->effect&=~12; g_this->effect|=4; break;
				case IDC_MIDCH: g_this->effect&=~12; g_this->effect|=8; break;
				case IDC_TOP: g_this->effect&=~48; break;
				case IDC_BOTTOM: g_this->effect&=~48; g_this->effect|=16; break;
				case IDC_CENTER: g_this->effect&=~48; g_this->effect|=32; break;
        case IDC_NUMCOL:
          {
            int p;
            BOOL tr=FALSE;
            p=GetDlgItemInt(hwndDlg,IDC_NUMCOL,&tr,FALSE);
            if (tr)
            {
              if (p > 16) p = 16;
              g_this->num_colors=p;
              InvalidateRect(GetDlgItem(hwndDlg,IDC_DEFCOL),NULL,TRUE);
            }
          }
        break;
        case IDC_DEFCOL:
          {
            int wc=-1,w,h;
            POINT p;
            RECT r;
            GetCursorPos(&p);
            GetWindowRect(GetDlgItem(hwndDlg,IDC_DEFCOL),&r);
            p.x -= r.left;
            p.y -= r.top;
            w=r.right-r.left;
            h=r.bottom-r.top;
            if (p.x >= 0 && p.x < w  && p.y >= 0 && p.y < h)
            {
              wc = (p.x*g_this->num_colors)/w;
            }
            if (wc>=0) 
            {
              GR_SelectColor(hwndDlg,g_this->colors+wc);
              InvalidateRect(GetDlgItem(hwndDlg,IDC_DEFCOL),NULL,TRUE);            
            }
          }
			}

	}
	return 0;
}

