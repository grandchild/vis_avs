#include "g__lib.h"
#include "g__defs.h"
#include "c_videodelay.h"
#include "resource.h"
#include <windows.h>


int win32_dlgproc_videodelay(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
	C_DELAY* g_Delay = (C_DELAY*)g_current_render;
	char value[16];
	int val;
	unsigned int objectcode, objectmessage;
	HWND hwndEdit;
	switch (uMsg)
	{
		case WM_INITDIALOG: //init
			CheckDlgButton(hwndDlg,IDC_CHECK1,g_Delay->enabled);
			CheckDlgButton(hwndDlg,IDC_RADIO1,g_Delay->usebeats);
			CheckDlgButton(hwndDlg,IDC_RADIO2,!g_Delay->usebeats);
			hwndEdit = GetDlgItem(hwndDlg,IDC_EDIT1);
			_itoa(g_Delay->delay,value,10);
			SetWindowText(hwndEdit,value);
			return 1;
		case WM_COMMAND:
			objectcode = LOWORD(wParam);
			objectmessage = HIWORD(wParam);
			// see if enable checkbox is checked
			if (objectcode == IDC_CHECK1)
			{
				g_Delay->enabled = IsDlgButtonChecked(hwndDlg,IDC_CHECK1)==1;
			    return 0;
			}
			// see if beats radiobox is checked
			if (objectcode == IDC_RADIO1)
			{
				if(IsDlgButtonChecked(hwndDlg,IDC_RADIO1)==1)
				{
					g_Delay->usebeats = true;
					CheckDlgButton(hwndDlg,IDC_RADIO2,BST_UNCHECKED);
					g_Delay->framedelay = 0;
					g_Delay->framessincebeat = 0;
					hwndEdit = GetDlgItem(hwndDlg,IDC_EDIT1);	//new
					if (g_Delay->delay>16) {	//new
						g_Delay->delay = 16;	//new
						SetWindowText(hwndEdit,"16");	//new
					}	//new
				}
				else g_Delay->usebeats = false;
			    return 0;
			}
			// see if frames radiobox is checked
			if (objectcode == IDC_RADIO2)
			{
				if(IsDlgButtonChecked(hwndDlg,IDC_RADIO2)==1)
				{
					g_Delay->usebeats = false;
					CheckDlgButton(hwndDlg,IDC_RADIO1,BST_UNCHECKED);
					g_Delay->framedelay = g_Delay->delay;
				}
				else g_Delay->usebeats = true;
			    return 0;
			}
			//get and put data from the delay box
			if (objectcode == IDC_EDIT1)
			{
				hwndEdit = GetDlgItem(hwndDlg,IDC_EDIT1);
				if (objectmessage == EN_CHANGE)
				{
					GetWindowText(hwndEdit,value,16);
					val = atoi(value);
					if (g_Delay->usebeats) {if (val > 16) val = 16;}	//new
					else {if (val > 200) val = 200;}	//new
					g_Delay->delay = val;
					g_Delay->framedelay = g_Delay->usebeats?0:g_Delay->delay;
				}
				else if (objectmessage == EN_KILLFOCUS)
				{
					_itoa(g_Delay->delay,value,10);
					SetWindowText(hwndEdit,value);
				}
				return 0;
			}
	}
	return 0;
}

