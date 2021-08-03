#include "g__lib.h"
#include "g__defs.h"
#include "c_multidelay.h"
#include "resource.h"
#include <windows.h>


int win32_dlgproc_multidelay(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM)
{
	C_DELAY* g_Delay = (C_DELAY*)g_current_render;
	char value[16];
	unsigned int objectcode, objectmessage;
	HWND hwndEdit;
	switch (uMsg)
	{
		case WM_INITDIALOG: //init
      {
        int i;
			  for (i=0;i<3;i++) CheckDlgButton(hwndDlg,1100+i,g_Delay->mode==i);
			  for (i=0;i<MULTIDELAY_NUM_BUFFERS;i++)
			  {
				  CheckDlgButton(hwndDlg,1000+i,g_Delay->activebuffer==i);
				  CheckDlgButton(hwndDlg,1020+i,g_Delay->usebeats(i));
				  CheckDlgButton(hwndDlg,1030+i,!g_Delay->usebeats(i));
				  hwndEdit = GetDlgItem(hwndDlg,1010+i);
				  _itoa(g_Delay->delay(i),value,10);
				  SetWindowText(hwndEdit,value);
			  }
      }
			return 1;
		case WM_COMMAND:
			objectcode = LOWORD(wParam);
			objectmessage = HIWORD(wParam);
			// mode stuff
			if (objectcode >= 1100)
			{
				if(IsDlgButtonChecked(hwndDlg,objectcode)==1)
				{
					g_Delay->mode = objectcode - 1100;
					for (int i=1100;i<1103;i++)	if (objectcode != i) CheckDlgButton(hwndDlg,i,BST_UNCHECKED);
				}
			    return 0;
			}
			// frames stuff
			if (objectcode >= 1030)
			{
				if(IsDlgButtonChecked(hwndDlg,objectcode)==1)
				{
					g_Delay->usebeats(objectcode-1030, false);
					CheckDlgButton(hwndDlg,objectcode-10,BST_UNCHECKED);
					g_Delay->framedelay(objectcode-1030, g_Delay->delay(objectcode-1030)+1);
				}
				else g_Delay->usebeats(objectcode-1030, true);
			    return 0;
			}
			// beats stuff
			if (objectcode >= 1020)
			{
				if(IsDlgButtonChecked(hwndDlg,objectcode)==1)
				{
					g_Delay->usebeats(objectcode-1020, true);
					CheckDlgButton(hwndDlg,objectcode+10,BST_UNCHECKED);
					g_Delay->framedelay(objectcode-1020, g_Delay->framesperbeat()+1);
				}
				else g_Delay->usebeats(objectcode-1020, false);
			    return 0;
			}
			// edit box stuff
			if (objectcode >= 1010)
			{
				hwndEdit = GetDlgItem(hwndDlg,objectcode);
				if (objectmessage == EN_CHANGE)
				{
					GetWindowText(hwndEdit,value,16);
					g_Delay->delay(objectcode-1010, max(atoi(value),0));
					g_Delay->framedelay(objectcode-1010, (g_Delay->usebeats(objectcode-1010)?g_Delay->framesperbeat():g_Delay->delay(objectcode-1010))+1);
				}
				else if (objectmessage == EN_KILLFOCUS)
				{
					_itoa(g_Delay->delay(objectcode-1010),value,10);
					SetWindowText(hwndEdit,value);
				}
				return 0;
			}
			// active buffer stuff
			if (objectcode >= 1000)
			{
				if(IsDlgButtonChecked(hwndDlg,objectcode)==1)
				{
					g_Delay->activebuffer = objectcode - 1000;
					for (int i=1000;i<1006;i++)	if (objectcode != i) CheckDlgButton(hwndDlg,i,BST_UNCHECKED);
				}
			    return 0;
			}
	}
	return 0;
}

