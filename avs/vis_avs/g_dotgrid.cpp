#include "g__lib.h"
#include "g__defs.h"
#include "c_dotgrid.h"
#include "resource.h"
#include <windows.h>
#include <commctrl.h>


int win32_dlgproc_dotgrid(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
  C_THISCLASS* g_this = (C_THISCLASS*)g_current_render;

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
			SendDlgItemMessage(hwndDlg,IDC_SLIDER1,TBM_SETRANGEMIN,0,0);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER1,TBM_SETRANGEMAX,0,33);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER1,TBM_SETPOS,1,g_this->x_move/32+16);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER2,TBM_SETRANGEMIN,0,0);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER2,TBM_SETRANGEMAX,0,33);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER2,TBM_SETPOS,1,g_this->y_move/32+16);

      SetDlgItemInt(hwndDlg,IDC_NUMCOL,g_this->num_colors,FALSE);
      SetDlgItemInt(hwndDlg,IDC_EDIT1,g_this->spacing,FALSE);
      if (g_this->blend==1)
        CheckDlgButton(hwndDlg,IDC_RADIO2,BST_CHECKED);
      else if (g_this->blend==2)
        CheckDlgButton(hwndDlg,IDC_RADIO3,BST_CHECKED);
      else if (g_this->blend==3)
        CheckDlgButton(hwndDlg,IDC_RADIO4,BST_CHECKED);
      else
        CheckDlgButton(hwndDlg,IDC_RADIO1,BST_CHECKED);
    return 1;
		case WM_HSCROLL:
			{
				HWND swnd = (HWND) lParam;
				int t = (int) SendMessage(swnd,TBM_GETPOS,0,0);
				if (swnd == GetDlgItem(hwndDlg,IDC_SLIDER1))
				{
					g_this->x_move=(t-16)*32;
				}
        if (swnd == GetDlgItem(hwndDlg,IDC_SLIDER2))
        {
					g_this->y_move=(t-16)*32;
        }
			}
    return 0;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
        case IDC_BUTTON1:
          g_this->x_move=0;
  			  SendDlgItemMessage(hwndDlg,IDC_SLIDER1,TBM_SETPOS,1,16);
      return 0;
        case IDC_BUTTON3:
          g_this->y_move=0;
  			  SendDlgItemMessage(hwndDlg,IDC_SLIDER2,TBM_SETPOS,1,16);
        return 0;
        case IDC_RADIO1:
        case IDC_RADIO2:
        case IDC_RADIO3:
        case IDC_RADIO4:
          if (IsDlgButtonChecked(hwndDlg,IDC_RADIO1)) g_this->blend=0;
          else if (IsDlgButtonChecked(hwndDlg,IDC_RADIO2)) g_this->blend=1;
          else if (IsDlgButtonChecked(hwndDlg,IDC_RADIO3)) g_this->blend=2;
          else if (IsDlgButtonChecked(hwndDlg,IDC_RADIO4)) g_this->blend=3;
        break;
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
        case IDC_EDIT1:
          {
            int p;
            BOOL tr=FALSE;
            p=GetDlgItemInt(hwndDlg,IDC_EDIT1,&tr,FALSE);
            if (tr)
            {
              if (p < 2) p = 2;
              g_this->spacing=p;
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

