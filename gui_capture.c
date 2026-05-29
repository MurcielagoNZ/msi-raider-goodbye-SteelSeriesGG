#include <windows.h>
#include <stdio.h>
#include <commctrl.h> 
#include <shellapi.h> 
#include "shared_config.h"
#include "msi_hid_core.h"
#include "sys_monitor.h"
#include <commdlg.h>

#define MAX_CAP 102
static unsigned char g_cap_keys[MAX_CAP];
static int g_cap_count = 0;
static HHOOK g_hCapHook = NULL;
static int g_pick_mode = 0;
static int g_strip_sel[4] = { 0 };
static RGBColor g_tmp_c1, g_tmp_c2, g_tmp_c3;
static const WCHAR *g_unit_str = L"%";

void KillDaemon()
{
	ShellExecute(NULL, L"open", L"taskkill.exe", L"/F /IM msi-light-control-background.exe", NULL, SW_HIDE);
}

void ToggleKeySelection(unsigned char hid)
{
	BOOL exists = FALSE;
	int idx = -1;
	for (int i = 0; i < g_cap_count; i++)
		if (hid == g_cap_keys[i])
		{
			exists = TRUE;
			idx = i;
			break;
		}

	if (TRUE == exists)
	{
		for (int i = idx; i < g_cap_count - 1; i++)
			g_cap_keys[i] = g_cap_keys[i + 1];
		g_cap_count--;
		MsiHid_SetSingleKeyColor(hid, 0x66, 0x66, 0x66);
	}
	else
		if (g_cap_count < MAX_CAP)
		{
			g_cap_keys[g_cap_count++] = hid;
			MsiHid_SetSingleKeyColor(hid, 0xFF, 0x00, 0x00);
		}
}

void UpdateStripPreview()
{
	unsigned char ar[4] = { 0 }, ag[4] = { 0 }, ab[4] = { 0 };
	for (int i = 0; i < 4; i++)
	{
		if (g_strip_sel[i])
		{
			ar[i] = 0xFF;
			ag[i] = 0;
			ab[i] = 0;
		}
		else
		{
			ar[i] = 0x66;
			ag[i] = 0x66;
			ab[i] = 0x66;
		}
	}
	MsiHid_SetSpecialZones(ar, ag, ab);
}

void RestoreStrips()
{
	unsigned char ar[4] = { 0 }, ag[4] = { 0 }, ab[4] = { 0 };
	for (int i = 0; i < 4; i++)
	{
		ar[i] = g_special_zones[i].c1.r;
		ag[i] = g_special_zones[i].c1.g;
		ab[i] = g_special_zones[i].c1.b;
	}
	MsiHid_SetSpecialZones(ar, ag, ab);
}

UINT_PTR CALLBACK CCRealTimeHook(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (WM_INITDIALOG == msg)
	{
		SetTimer(hdlg, 1, 50, NULL);
		return TRUE;
	}
	if (WM_TIMER == msg)
	{
		BOOL t1, t2, t3;
		UINT r = GetDlgItemInt(hdlg, 706, &t1, FALSE);
		UINT g = GetDlgItemInt(hdlg, 707, &t2, FALSE);
		UINT b = GetDlgItemInt(hdlg, 708, &t3, FALSE);
		if (t1 && t2 && t3)
		{
			if (0 == g_pick_mode)
				for (int i = 0; i < g_cap_count; i++)
					MsiHid_SetSingleKeyColor(g_cap_keys[i], (unsigned char)r, (unsigned char)g, (unsigned char)b);
			else
			{
				unsigned char ar[4] = { 0 }, ag[4] = { 0 }, ab[4] = { 0 };
				for (int i = 0; i < 4; i++)
				{
					if (g_strip_sel[i])
					{
						ar[i] = (unsigned char)r;
						ag[i] = (unsigned char)g;
						ab[i] = (unsigned char)b;
					}
					else
					{
						ar[i] = 0x66;
						ag[i] = 0x66;
						ab[i] = 0x66;
					}
				}
				MsiHid_SetSpecialZones(ar, ag, ab);
			}
		}
	}
	if (WM_DESTROY == msg)
		KillTimer(hdlg, 1);
	return 0;
}

BOOL PickColor(HWND hParent, RGBColor *outColor)
{
	CHOOSECOLOR cc = { 0 };
	static COLORREF cust[16] = { 0 };
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = hParent;
	cc.lpCustColors = (LPDWORD)cust;
	cc.rgbResult = RGB(outColor->r, outColor->g, outColor->b);
	cc.Flags = CC_FULLOPEN | CC_RGBINIT | CC_ENABLEHOOK;
	cc.lpfnHook = CCRealTimeHook;
	if (ChooseColor(&cc))
	{
		outColor->r = GetRValue(cc.rgbResult);
		outColor->g = GetGValue(cc.rgbResult);
		outColor->b = GetBValue(cc.rgbResult);
		return TRUE;
	}
	return FALSE;
}

void RunModalWindow(HWND hWnd, HWND hParent)
{
	EnableWindow(hParent, FALSE);
	ShowWindow(hWnd, SW_SHOW);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!IsWindow(hWnd))
			break;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	EnableWindow(hParent, TRUE);
	SetActiveWindow(hParent);
}

LRESULT CALLBACK RecDlgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
		case WM_CTLCOLORSTATIC:
			return (LRESULT)GetStockObject(WHITE_BRUSH);
		case WM_COMMAND:
		{
			int id = LOWORD(wp);
			if (101 == id)
				ToggleKeySelection(0xF0);
			else
				if (102 == id)
					ToggleKeySelection(0x66);
				else
					if (IDOK == id)
						DestroyWindow(hWnd);
					else
						if (IDCANCEL == id)
						{
							g_cap_count = 0;
							DestroyWindow(hWnd);
						}
			break;
		}
		default:
			return DefWindowProc(hWnd, msg, wp, lp);
	}
	return 0;
}

LRESULT CALLBACK CapHookProc(int code, WPARAM wp, LPARAM lp)
{
	if (HC_ACTION == code && (WM_KEYDOWN == wp || WM_SYSKEYDOWN == wp))
	{
		KBDLLHOOKSTRUCT *p = (KBDLLHOOKSTRUCT *)lp;
		unsigned char hid = MapVKToHID(p->vkCode, p->flags, p->scanCode);
		if (0 != hid)
		{
			ToggleKeySelection(hid);
			return 1;
		}
	}
	return CallNextHookEx(g_hCapHook, code, wp, lp);
}

HWND hKR_Static, hKR_Press, hKR_Link, hKGrpB[12], hKGrpL[16];

void ToggleKeyUI()
{
	BOOL isStatic = (BST_CHECKED == SendMessage(hKR_Static, BM_GETCHECK, 0, 0));
	BOOL isPress = (BST_CHECKED == SendMessage(hKR_Press, BM_GETCHECK, 0, 0));
	BOOL isLink = (BST_CHECKED == SendMessage(hKR_Link, BM_GETCHECK, 0, 0));

	ShowWindow(hKGrpB[0], (isStatic || isPress) ? SW_SHOW : SW_HIDE);
	for (int i = 1; i <= 7; i++)
		ShowWindow(hKGrpB[i], isPress ? SW_SHOW : SW_HIDE);
	for (int i = 0; i <= 12; i++)
		ShowWindow(hKGrpL[i], isLink ? SW_SHOW : SW_HIDE);
}

void UpdateSliderLabels()
{
	for (int i = 0; i < 3; i++)
	{
		int val = (int)SendMessage(hKGrpL[5 + i * 3], TBM_GETPOS, 0, 0);
		WCHAR buf[32];
		swprintf_s(buf, 32, L"阈值%d: %d%s", i + 1, val, g_unit_str);
		SetWindowText(hKGrpL[4 + i * 3], buf);
	}
}

LRESULT CALLBACK KeyDynDlgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
		case WM_CREATE:
		{
			CreateWindow(L"STATIC", L"选择光效模式:", WS_CHILD | WS_VISIBLE, 20, 20, 100, 20, hWnd, NULL, NULL, NULL);
			hKR_Static = CreateWindow(L"BUTTON", L"静态常亮", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP, 20, 50, 80, 20, hWnd, (HMENU)201, NULL, NULL);
			hKR_Press = CreateWindow(L"BUTTON", L"按下变色", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 120, 50, 80, 20, hWnd, (HMENU)202, NULL, NULL);
			hKR_Link = CreateWindow(L"BUTTON", L"系统联动", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 220, 50, 80, 20, hWnd, (HMENU)203, NULL, NULL);

			hKGrpB[0] = CreateWindow(L"BUTTON", L"设置静态底色", WS_CHILD | WS_BORDER, 20, 85, 120, 30, hWnd, (HMENU)210, NULL, NULL);
			hKGrpB[1] = CreateWindow(L"STATIC", L"恢复耗时", WS_CHILD, 160, 90, 60, 20, hWnd, NULL, NULL, NULL);
			hKGrpB[2] = CreateWindow(L"EDIT", L"1000", WS_CHILD | WS_BORDER | ES_NUMBER, 230, 88, 60, 22, hWnd, NULL, NULL, NULL);
			hKGrpB[3] = CreateWindow(L"STATIC", L"毫秒", WS_CHILD, 300, 90, 40, 20, hWnd, NULL, NULL, NULL);

			hKGrpB[4] = CreateWindow(L"BUTTON", L"设置按下颜色", WS_CHILD | WS_BORDER, 20, 135, 120, 30, hWnd, (HMENU)211, NULL, NULL);
			hKGrpB[5] = CreateWindow(L"STATIC", L"渐变耗时", WS_CHILD, 160, 140, 60, 20, hWnd, NULL, NULL, NULL);
			hKGrpB[6] = CreateWindow(L"EDIT", L"1000", WS_CHILD | WS_BORDER | ES_NUMBER, 230, 138, 60, 22, hWnd, NULL, NULL, NULL);
			hKGrpB[7] = CreateWindow(L"STATIC", L"毫秒", WS_CHILD, 300, 140, 40, 20, hWnd, NULL, NULL, NULL);

			hKGrpL[0] = CreateWindow(L"BUTTON", L"CPU使用率%", WS_CHILD | BS_AUTORADIOBUTTON | WS_GROUP, 20, 85, 95, 20, hWnd, (HMENU)220, NULL, NULL);
			hKGrpL[1] = CreateWindow(L"BUTTON", L"显卡功耗%", WS_CHILD | BS_AUTORADIOBUTTON, 120, 85, 95, 20, hWnd, (HMENU)221, NULL, NULL);
			hKGrpL[2] = CreateWindow(L"BUTTON", L"显存使用%", WS_CHILD | BS_AUTORADIOBUTTON, 220, 85, 95, 20, hWnd, (HMENU)222, NULL, NULL);
			hKGrpL[3] = CreateWindow(L"BUTTON", L"显卡温度℃", WS_CHILD | BS_AUTORADIOBUTTON, 320, 85, 100, 20, hWnd, (HMENU)229, NULL, NULL);

			hKGrpL[4] = CreateWindow(L"STATIC", L"阈值1: 20%", WS_CHILD, 20, 125, 110, 20, hWnd, NULL, NULL, NULL);
			hKGrpL[5] = CreateWindow(TRACKBAR_CLASS, L"", WS_CHILD | TBS_HORZ, 130, 123, 160, 25, hWnd, (HMENU)223, NULL, NULL);
			hKGrpL[6] = CreateWindow(L"BUTTON", L"设置颜色1", WS_CHILD | BS_PUSHBUTTON, 300, 122, 100, 25, hWnd, (HMENU)224, NULL, NULL);

			hKGrpL[7] = CreateWindow(L"STATIC", L"阈值2: 50%", WS_CHILD, 20, 165, 110, 20, hWnd, NULL, NULL, NULL);
			hKGrpL[8] = CreateWindow(TRACKBAR_CLASS, L"", WS_CHILD | TBS_HORZ, 130, 163, 160, 25, hWnd, (HMENU)225, NULL, NULL);
			hKGrpL[9] = CreateWindow(L"BUTTON", L"设置颜色2", WS_CHILD | BS_PUSHBUTTON, 300, 162, 100, 25, hWnd, (HMENU)226, NULL, NULL);

			hKGrpL[10] = CreateWindow(L"STATIC", L"阈值3: 80%", WS_CHILD, 20, 205, 110, 20, hWnd, NULL, NULL, NULL);
			hKGrpL[11] = CreateWindow(TRACKBAR_CLASS, L"", WS_CHILD | TBS_HORZ, 130, 203, 160, 25, hWnd, (HMENU)227, NULL, NULL);
			hKGrpL[12] = CreateWindow(L"BUTTON", L"设置颜色3", WS_CHILD | BS_PUSHBUTTON, 300, 202, 100, 25, hWnd, (HMENU)228, NULL, NULL);

			SendMessage(hKGrpL[5], TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
			SendMessage(hKGrpL[8], TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
			SendMessage(hKGrpL[11], TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
			SendMessage(hKGrpL[5], TBM_SETPOS, TRUE, 20);
			SendMessage(hKGrpL[8], TBM_SETPOS, TRUE, 50);
			SendMessage(hKGrpL[11], TBM_SETPOS, TRUE, 80);

			CreateWindow(L"BUTTON", L"确定", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 190, 270, 100, 35, hWnd, (HMENU)IDOK, NULL, NULL);

			SendMessage(hKR_Press, BM_SETCHECK, BST_CHECKED, 0);
			SendMessage(hKGrpL[0], BM_SETCHECK, BST_CHECKED, 0);
			g_unit_str = L"%";

			g_tmp_c1.r = 0; g_tmp_c1.g = 0; g_tmp_c1.b = 0;
			g_tmp_c2.r = 255; g_tmp_c2.g = 0; g_tmp_c2.b = 0;
			g_tmp_c3.r = 255; g_tmp_c3.g = 255; g_tmp_c3.b = 0;

			ApplyModernFont(hWnd);
			ToggleKeyUI();
			UpdateSliderLabels();
			break;
		}
		case WM_HSCROLL:
		{
			HWND hScroll = (HWND)lp;
			int id = GetDlgCtrlID(hScroll);
			if (223 == id || 225 == id || 227 == id)
			{
				int v1 = (int)SendMessage(hKGrpL[5], TBM_GETPOS, 0, 0);
				int v2 = (int)SendMessage(hKGrpL[8], TBM_GETPOS, 0, 0);
				int v3 = (int)SendMessage(hKGrpL[11], TBM_GETPOS, 0, 0);

				if (223 == id)
				{
					if (v1 > v2)
					{
						v2 = v1;
						SendMessage(hKGrpL[8], TBM_SETPOS, TRUE, v2);
					}
					if (v2 > v3)
					{
						v3 = v2;
						SendMessage(hKGrpL[11], TBM_SETPOS, TRUE, v3);
					}
				}
				else
					if (225 == id)
					{
						if (v1 > v2)
						{
							v1 = v2;
							SendMessage(hKGrpL[5], TBM_SETPOS, TRUE, v1);
						}
						if (v2 > v3)
						{
							v3 = v2;
							SendMessage(hKGrpL[11], TBM_SETPOS, TRUE, v3);
						}
					}
					else
						if (227 == id)
						{
							if (v2 > v3)
							{
								v2 = v3;
								SendMessage(hKGrpL[8], TBM_SETPOS, TRUE, v2);
							}
							if (v1 > v2)
							{
								v1 = v2;
								SendMessage(hKGrpL[5], TBM_SETPOS, TRUE, v1);
							}
						}
				UpdateSliderLabels();
			}
			break;
		}
		case WM_CTLCOLORSTATIC:
			return (LRESULT)GetStockObject(WHITE_BRUSH);
		case WM_COMMAND:
		{
			int id = LOWORD(wp);
			if (201 == id || 202 == id || 203 == id)
				ToggleKeyUI();
			else
				if (220 == id || 221 == id || 222 == id)
				{
					g_unit_str = L"%";
					for (int i = 0; i < 3; i++)
						SendMessage(hKGrpL[5 + i * 3], TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
					SendMessage(hKGrpL[5], TBM_SETPOS, TRUE, 20);
					SendMessage(hKGrpL[8], TBM_SETPOS, TRUE, 50);
					SendMessage(hKGrpL[11], TBM_SETPOS, TRUE, 80);
					UpdateSliderLabels();
				}
				else
					if (229 == id)
					{
						g_unit_str = L"°C";
						int wall = SysMonitor_GetGpuTempWall();
						for (int i = 0; i < 3; i++)
							SendMessage(hKGrpL[5 + i * 3], TBM_SETRANGE, TRUE, MAKELPARAM(0, wall));
						SendMessage(hKGrpL[5], TBM_SETPOS, TRUE, 45 > wall ? wall : 45);
						SendMessage(hKGrpL[8], TBM_SETPOS, TRUE, 65 > wall ? wall : 65);
						SendMessage(hKGrpL[11], TBM_SETPOS, TRUE, 80 > wall ? wall : 80);
						UpdateSliderLabels();
					}
					else
						if (210 == id || 224 == id || 211 == id || 226 == id || 228 == id)
						{
							g_suspend_preview = 1;
							if (210 == id || 224 == id)
							{
								if (PickColor(hWnd, &g_tmp_c1))
									for (int i = 0; i < g_cap_count; i++)
										g_key_matrix[g_cap_keys[i]].c1 = g_tmp_c1;
							}
							else
								if (211 == id || 226 == id)
									PickColor(hWnd, &g_tmp_c2);
								else
									if (228 == id)
										PickColor(hWnd, &g_tmp_c3);
							g_suspend_preview = 0;
							MsiHid_SetFullKeyboard(0, 0, 0, 1);
						}
						else
							if (IDOK == id)
							{
								WCHAR b[16];
								int tMode = 0;
								if (BST_CHECKED == SendMessage(hKR_Press, BM_GETCHECK, 0, 0))
									tMode = 1;
								else
									if (BST_CHECKED == SendMessage(hKR_Link, BM_GETCHECK, 0, 0))
									{
										if (BST_CHECKED == SendMessage(hKGrpL[0], BM_GETCHECK, 0, 0)) tMode = 2;
										else if (BST_CHECKED == SendMessage(hKGrpL[1], BM_GETCHECK, 0, 0)) tMode = 3;
										else if (BST_CHECKED == SendMessage(hKGrpL[2], BM_GETCHECK, 0, 0)) tMode = 4;
										else tMode = 5;
									}

								int val1 = 0, val2 = 0, val3 = 0;
								if (tMode >= 2)
								{
									val1 = (int)SendMessage(hKGrpL[5], TBM_GETPOS, 0, 0);
									val2 = (int)SendMessage(hKGrpL[8], TBM_GETPOS, 0, 0);
									val3 = (int)SendMessage(hKGrpL[11], TBM_GETPOS, 0, 0);
								}
								else
								{
									GetWindowText(hKGrpB[6], b, 16); val1 = _wtoi(b);
									GetWindowText(hKGrpB[2], b, 16); val2 = _wtoi(b);
								}

								for (int i = 0; i < g_cap_count; i++)
								{
									unsigned char hid = g_cap_keys[i];
									g_key_matrix[hid].mode = tMode;
									g_key_matrix[hid].c1 = g_tmp_c1;
									g_key_matrix[hid].v1 = val1;
									g_key_matrix[hid].c2 = g_tmp_c2;
									g_key_matrix[hid].v2 = val2;
									g_key_matrix[hid].c3 = g_tmp_c3;
									g_key_matrix[hid].v3 = val3;
								}
								DestroyWindow(hWnd);
							}
			break;
		}
		default:
			return DefWindowProc(hWnd, msg, wp, lp);
	}
	return 0;
}

HWND hSR_Inst, hSR_Grad, hSR_Link, hSGrpB[12];
void ToggleStripUI()
{
	BOOL isLink = (BST_CHECKED == SendMessage(hSR_Link, BM_GETCHECK, 0, 0));
	for (int i = 0; i < 12; i++)
		ShowWindow(hSGrpB[i], !isLink ? SW_SHOW : SW_HIDE);
	for (int i = 0; i <= 12; i++)
		ShowWindow(hKGrpL[i], isLink ? SW_SHOW : SW_HIDE);
}

LRESULT CALLBACK StripDynDlgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
		case WM_CREATE:
		{
			CreateWindow(L"STATIC", L"选择光效模式:", WS_CHILD | WS_VISIBLE, 20, 20, 100, 20, hWnd, NULL, NULL, NULL);
			hSR_Inst = CreateWindow(L"BUTTON", L"瞬间突变", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP, 20, 50, 90, 20, hWnd, (HMENU)201, NULL, NULL);
			hSR_Grad = CreateWindow(L"BUTTON", L"平滑渐变", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 120, 50, 90, 20, hWnd, (HMENU)202, NULL, NULL);
			hSR_Link = CreateWindow(L"BUTTON", L"系统联动", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 220, 50, 90, 20, hWnd, (HMENU)203, NULL, NULL);

			hSGrpB[0] = CreateWindow(L"BUTTON", L"设置颜色 1", WS_CHILD | WS_BORDER, 20, 85, 120, 30, hWnd, (HMENU)210, NULL, NULL);
			hSGrpB[1] = CreateWindow(L"STATIC", L"停留耗时", WS_CHILD, 160, 90, 60, 20, hWnd, NULL, NULL, NULL);
			hSGrpB[2] = CreateWindow(L"EDIT", L"1000", WS_CHILD | WS_BORDER | ES_NUMBER, 230, 86, 60, 22, hWnd, NULL, NULL, NULL);
			hSGrpB[3] = CreateWindow(L"STATIC", L"毫秒", WS_CHILD, 300, 90, 40, 20, hWnd, NULL, NULL, NULL);

			hSGrpB[4] = CreateWindow(L"BUTTON", L"设置颜色 2", WS_CHILD | WS_BORDER, 20, 135, 120, 30, hWnd, (HMENU)211, NULL, NULL);
			hSGrpB[5] = CreateWindow(L"STATIC", L"停留耗时", WS_CHILD, 160, 140, 60, 20, hWnd, NULL, NULL, NULL);
			hSGrpB[6] = CreateWindow(L"EDIT", L"1000", WS_CHILD | WS_BORDER | ES_NUMBER, 230, 136, 60, 22, hWnd, NULL, NULL, NULL);
			hSGrpB[7] = CreateWindow(L"STATIC", L"毫秒", WS_CHILD, 300, 140, 40, 20, hWnd, NULL, NULL, NULL);

			hSGrpB[8] = CreateWindow(L"BUTTON", L"设置颜色 3", WS_CHILD | WS_BORDER, 20, 185, 120, 30, hWnd, (HMENU)212, NULL, NULL);
			hSGrpB[9] = CreateWindow(L"STATIC", L"停留耗时", WS_CHILD, 160, 190, 60, 20, hWnd, NULL, NULL, NULL);
			hSGrpB[10] = CreateWindow(L"EDIT", L"1000", WS_CHILD | WS_BORDER | ES_NUMBER, 230, 186, 60, 22, hWnd, NULL, NULL, NULL);
			hSGrpB[11] = CreateWindow(L"STATIC", L"毫秒", WS_CHILD, 300, 190, 40, 20, hWnd, NULL, NULL, NULL);

			hKGrpL[0] = CreateWindow(L"BUTTON", L"CPU使用率%", WS_CHILD | BS_AUTORADIOBUTTON | WS_GROUP, 20, 85, 95, 20, hWnd, (HMENU)220, NULL, NULL);
			hKGrpL[1] = CreateWindow(L"BUTTON", L"显卡功耗%", WS_CHILD | BS_AUTORADIOBUTTON, 120, 85, 95, 20, hWnd, (HMENU)221, NULL, NULL);
			hKGrpL[2] = CreateWindow(L"BUTTON", L"显存使用%", WS_CHILD | BS_AUTORADIOBUTTON, 220, 85, 95, 20, hWnd, (HMENU)222, NULL, NULL);
			hKGrpL[3] = CreateWindow(L"BUTTON", L"显卡温度℃", WS_CHILD | BS_AUTORADIOBUTTON, 320, 85, 100, 20, hWnd, (HMENU)229, NULL, NULL);

			hKGrpL[4] = CreateWindow(L"STATIC", L"阈值1: 20%", WS_CHILD, 20, 125, 110, 20, hWnd, NULL, NULL, NULL);
			hKGrpL[5] = CreateWindow(TRACKBAR_CLASS, L"", WS_CHILD | TBS_HORZ, 130, 123, 160, 25, hWnd, (HMENU)223, NULL, NULL);
			hKGrpL[6] = CreateWindow(L"BUTTON", L"设置颜色1", WS_CHILD | BS_PUSHBUTTON, 300, 122, 100, 25, hWnd, (HMENU)224, NULL, NULL);

			hKGrpL[7] = CreateWindow(L"STATIC", L"阈值2: 50%", WS_CHILD, 20, 165, 110, 20, hWnd, NULL, NULL, NULL);
			hKGrpL[8] = CreateWindow(TRACKBAR_CLASS, L"", WS_CHILD | TBS_HORZ, 130, 163, 160, 25, hWnd, (HMENU)225, NULL, NULL);
			hKGrpL[9] = CreateWindow(L"BUTTON", L"设置颜色2", WS_CHILD | BS_PUSHBUTTON, 300, 162, 100, 25, hWnd, (HMENU)226, NULL, NULL);

			hKGrpL[10] = CreateWindow(L"STATIC", L"阈值3: 80%", WS_CHILD, 20, 205, 110, 20, hWnd, NULL, NULL, NULL);
			hKGrpL[11] = CreateWindow(TRACKBAR_CLASS, L"", WS_CHILD | TBS_HORZ, 130, 203, 160, 25, hWnd, (HMENU)227, NULL, NULL);
			hKGrpL[12] = CreateWindow(L"BUTTON", L"设置颜色3", WS_CHILD | BS_PUSHBUTTON, 300, 202, 100, 25, hWnd, (HMENU)228, NULL, NULL);

			SendMessage(hKGrpL[5], TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
			SendMessage(hKGrpL[8], TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
			SendMessage(hKGrpL[11], TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
			SendMessage(hKGrpL[5], TBM_SETPOS, TRUE, 20);
			SendMessage(hKGrpL[8], TBM_SETPOS, TRUE, 50);
			SendMessage(hKGrpL[11], TBM_SETPOS, TRUE, 80);

			CreateWindow(L"BUTTON", L"确定", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 190, 270, 100, 35, hWnd, (HMENU)IDOK, NULL, NULL);

			SendMessage(hSR_Inst, BM_SETCHECK, BST_CHECKED, 0);
			SendMessage(hKGrpL[0], BM_SETCHECK, BST_CHECKED, 0);
			g_unit_str = L"%";

			g_tmp_c1.r = 255; g_tmp_c1.g = 0; g_tmp_c1.b = 0;
			g_tmp_c2.r = 0; g_tmp_c2.g = 255; g_tmp_c2.b = 0;
			g_tmp_c3.r = 0; g_tmp_c3.g = 0; g_tmp_c3.b = 255;

			ApplyModernFont(hWnd);
			ToggleStripUI();
			UpdateSliderLabels();
			break;
		}
		case WM_HSCROLL:
		{
			HWND hScroll = (HWND)lp;
			int id = GetDlgCtrlID(hScroll);
			if (223 == id || 225 == id || 227 == id)
			{
				int v1 = (int)SendMessage(hKGrpL[5], TBM_GETPOS, 0, 0);
				int v2 = (int)SendMessage(hKGrpL[8], TBM_GETPOS, 0, 0);
				int v3 = (int)SendMessage(hKGrpL[11], TBM_GETPOS, 0, 0);

				if (223 == id)
				{
					if (v1 > v2)
					{
						v2 = v1;
						SendMessage(hKGrpL[8], TBM_SETPOS, TRUE, v2);
					}
					if (v2 > v3)
					{
						v3 = v2;
						SendMessage(hKGrpL[11], TBM_SETPOS, TRUE, v3);
					}
				}
				else
					if (225 == id)
					{
						if (v1 > v2)
						{
							v1 = v2;
							SendMessage(hKGrpL[5], TBM_SETPOS, TRUE, v1);
						}
						if (v2 > v3)
						{
							v3 = v2;
							SendMessage(hKGrpL[11], TBM_SETPOS, TRUE, v3);
						}
					}
					else
						if (227 == id)
						{
							if (v2 > v3)
							{
								v2 = v3;
								SendMessage(hKGrpL[8], TBM_SETPOS, TRUE, v2);
							}
							if (v1 > v2)
							{
								v1 = v2;
								SendMessage(hKGrpL[5], TBM_SETPOS, TRUE, v1);
							}
						}
				UpdateSliderLabels();
			}
			break;
		}
		case WM_CTLCOLORSTATIC:
			return (LRESULT)GetStockObject(WHITE_BRUSH);
		case WM_COMMAND:
		{
			int id = LOWORD(wp);
			if (201 == id || 202 == id || 203 == id)
				ToggleStripUI();
			else
				if (220 == id || 221 == id || 222 == id)
				{
					g_unit_str = L"%";
					for (int i = 0; i < 3; i++)
						SendMessage(hKGrpL[5 + i * 3], TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
					SendMessage(hKGrpL[5], TBM_SETPOS, TRUE, 20);
					SendMessage(hKGrpL[8], TBM_SETPOS, TRUE, 50);
					SendMessage(hKGrpL[11], TBM_SETPOS, TRUE, 80);
					UpdateSliderLabels();
				}
				else
					if (229 == id)
					{
						g_unit_str = L"°C";
						int wall = SysMonitor_GetGpuTempWall();
						for (int i = 0; i < 3; i++)
							SendMessage(hKGrpL[5 + i * 3], TBM_SETRANGE, TRUE, MAKELPARAM(0, wall));
						SendMessage(hKGrpL[5], TBM_SETPOS, TRUE, 45 > wall ? wall : 45);
						SendMessage(hKGrpL[8], TBM_SETPOS, TRUE, 65 > wall ? wall : 65);
						SendMessage(hKGrpL[11], TBM_SETPOS, TRUE, 80 > wall ? wall : 80);
						UpdateSliderLabels();
					}
					else
						if (210 == id || 224 == id || 211 == id || 226 == id || 212 == id || 228 == id)
						{
							g_suspend_preview = 1;
							if (210 == id || 224 == id)
							{
								if (PickColor(hWnd, &g_tmp_c1))
									for (int i = 0; i < 4; i++)
										if (g_strip_sel[i])
											g_special_zones[i].c1 = g_tmp_c1;
							}
							else
								if (211 == id || 226 == id)
									PickColor(hWnd, &g_tmp_c2);
								else
									if (212 == id || 228 == id)
										PickColor(hWnd, &g_tmp_c3);
							g_suspend_preview = 0;
							MsiHid_SetFullKeyboard(0, 0, 0, 1);
						}
						else
							if (IDOK == id)
							{
								WCHAR b[16];
								int tMode = 0;
								if (BST_CHECKED == SendMessage(hSR_Grad, BM_GETCHECK, 0, 0))
									tMode = 1;
								else
									if (BST_CHECKED == SendMessage(hSR_Link, BM_GETCHECK, 0, 0))
									{
										if (BST_CHECKED == SendMessage(hKGrpL[0], BM_GETCHECK, 0, 0)) tMode = 2;
										else if (BST_CHECKED == SendMessage(hKGrpL[1], BM_GETCHECK, 0, 0)) tMode = 3;
										else if (BST_CHECKED == SendMessage(hKGrpL[2], BM_GETCHECK, 0, 0)) tMode = 4;
										else tMode = 5;
									}

								int val1 = 0, val2 = 0, val3 = 0;
								if (tMode >= 2)
								{
									val1 = (int)SendMessage(hKGrpL[5], TBM_GETPOS, 0, 0);
									val2 = (int)SendMessage(hKGrpL[8], TBM_GETPOS, 0, 0);
									val3 = (int)SendMessage(hKGrpL[11], TBM_GETPOS, 0, 0);
								}
								else
								{
									GetWindowText(hSGrpB[2], b, 16); val1 = _wtoi(b);
									GetWindowText(hSGrpB[6], b, 16); val2 = _wtoi(b);
									GetWindowText(hSGrpB[10], b, 16); val3 = _wtoi(b);
								}

								for (int i = 0; i < 4; i++)
								{
									if (g_strip_sel[i])
									{
										g_special_zones[i].mode = tMode;
										g_special_zones[i].c1 = g_tmp_c1;
										g_special_zones[i].v1 = val1;
										g_special_zones[i].c2 = g_tmp_c2;
										g_special_zones[i].v2 = val2;
										g_special_zones[i].c3 = g_tmp_c3;
										g_special_zones[i].v3 = val3;
									}
								}
								DestroyWindow(hWnd);
							}
			break;
		}
		default:
			return DefWindowProc(hWnd, msg, wp, lp);
	}
	return 0;
}

LRESULT CALLBACK StripSelDlgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
		case WM_CTLCOLORSTATIC:
			return (LRESULT)GetStockObject(WHITE_BRUSH);
		case WM_COMMAND:
		{
			int id = LOWORD(wp);
			if (301 == id)
			{
				g_strip_sel[0] = !g_strip_sel[0];
				UpdateStripPreview();
			}
			else
				if (302 == id)
				{
					g_strip_sel[1] = !g_strip_sel[1];
					UpdateStripPreview();
				}
				else
					if (303 == id)
					{
						g_strip_sel[2] = !g_strip_sel[2];
						UpdateStripPreview();
					}
					else
						if (304 == id)
						{
							g_strip_sel[3] = !g_strip_sel[3];
							UpdateStripPreview();
						}
						else
							if (IDOK == id)
								DestroyWindow(hWnd);
							else
								if (IDCANCEL == id)
								{
									g_strip_sel[0] = 0;
									g_strip_sel[1] = 0;
									g_strip_sel[2] = 0;
									g_strip_sel[3] = 0;
									DestroyWindow(hWnd);
								}
			break;
		}
		default:
			return DefWindowProc(hWnd, msg, wp, lp);
	}
	return 0;
}

LRESULT CALLBACK SubKeyProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
		case WM_CREATE:
		{
			KillDaemon();
			if (!g_global_dynamic)
				MsiHid_SetFullKeyboard(0, 0, 0, 1);
			HWND hBtnSet = CreateWindow(L"BUTTON", L"进入灯光设置", WS_CHILD | WS_VISIBLE | 0x0000000E, 20, 20, 200, 60, hWnd, (HMENU)101, NULL, NULL);
			SendMessage(hBtnSet, 0x1609, 0, (LPARAM)L"录制按键并为其上色");
			HWND b2 = CreateWindow(L"BUTTON", L"保存并返回", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 100, 10, 35, hWnd, (HMENU)102, NULL, NULL);
			HWND b3 = CreateWindow(L"BUTTON", L"取消", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 100, 10, 35, hWnd, (HMENU)103, NULL, NULL);
			ApplyModernFont(hWnd);
			AutoSizeButton(b2, 25, 0);
			AutoSizeButton(b3, 40, 0);
			RECT r;
			GetWindowRect(b2, &r);
			SetWindowPos(b3, NULL, 20 + (r.right - r.left) + 10, 100, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			break;
		}
		case WM_CTLCOLORSTATIC:
			return (LRESULT)GetStockObject(WHITE_BRUSH);
		case WM_COMMAND:
		{
			int id = LOWORD(wp);
			if (101 == id)
			{
				g_suspend_preview = 1;
				g_pick_mode = 0;
				g_cap_count = 0;
				MsiHid_SetFullKeyboard(0x66, 0x66, 0x66, 0);
				RECT rp;
				GetWindowRect(hWnd, &rp);
				WNDCLASSEX wc = { sizeof(WNDCLASSEX), 0, RecDlgProc, 0, 0, GetModuleHandle(NULL), NULL, LoadCursor(NULL, IDC_ARROW), (HBRUSH)GetStockObject(WHITE_BRUSH), NULL, L"RecKey", NULL };
				RegisterClassEx(&wc);
				HWND hRec = CreateWindowEx(WS_EX_TOPMOST | WS_EX_DLGMODALFRAME, L"RecKey", L"第 1 步: 物理按键录制", WS_POPUP | WS_CAPTION | WS_SYSMENU, rp.left, rp.top, 360, 200, hWnd, NULL, GetModuleHandle(NULL), NULL);
				CreateWindow(L"STATIC", L"请直接敲击键盘录制 (再次敲击可取消)\n不支持的按键请点下方按钮:", WS_CHILD | WS_VISIBLE, 20, 20, 300, 40, hRec, NULL, NULL, NULL);
				HWND hb1 = CreateWindow(L"BUTTON", L"Fn", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 75, 10, 30, hRec, (HMENU)101, NULL, NULL);
				HWND hb2 = CreateWindow(L"BUTTON", L"电源", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 70, 75, 10, 30, hRec, (HMENU)102, NULL, NULL);
				HWND hb3 = CreateWindow(L"BUTTON", L"下一步", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 80, 120, 10, 30, hRec, (HMENU)IDOK, NULL, NULL);
				HWND hb4 = CreateWindow(L"BUTTON", L"取消", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 180, 120, 10, 30, hRec, (HMENU)IDCANCEL, NULL, NULL);
				ApplyModernFont(hRec);
				AutoSizeButton(hb1, 20, 0);
				AutoSizeButton(hb2, 20, 0);
				AutoSizeButton(hb3, 20, 0);
				AutoSizeButton(hb4, 20, 0);
				RECT r;
				GetWindowRect(hb1, &r);
				SetWindowPos(hb2, NULL, 20 + (r.right - r.left) + 10, 75, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

				g_hCapHook = SetWindowsHookEx(WH_KEYBOARD_LL, CapHookProc, GetModuleHandle(NULL), 0);
				RunModalWindow(hRec, hWnd);
				UnhookWindowsHookEx(g_hCapHook);
				UnregisterClass(L"RecKey", GetModuleHandle(NULL));

				if (0 == g_cap_count)
				{
					g_suspend_preview = 0;
					MsiHid_SetFullKeyboard(0, 0, 0, 1);
					break;
				}

				g_suspend_preview = 0;
				MsiHid_SetFullKeyboard(0, 0, 0, 1);

				if (g_global_dynamic)
				{
					WNDCLASSEX wc2 = { sizeof(WNDCLASSEX), 0, KeyDynDlgProc, 0, 0, GetModuleHandle(NULL), NULL, LoadCursor(NULL, IDC_ARROW), (HBRUSH)GetStockObject(WHITE_BRUSH), NULL, L"KeyDyn", NULL };
					RegisterClassEx(&wc2);
					HWND hDyn = CreateWindowEx(WS_EX_DLGMODALFRAME, L"KeyDyn", L"第 2 步: 动态光效参数", WS_POPUP | WS_CAPTION | WS_SYSMENU, rp.left, rp.top, 480, 360, hWnd, NULL, GetModuleHandle(NULL), NULL);
					RunModalWindow(hDyn, hWnd);
					UnregisterClass(L"KeyDyn", GetModuleHandle(NULL));
				}
				else
				{
					g_suspend_preview = 1;
					if (PickColor(hWnd, &g_tmp_c1))
						for (int i = 0; i < g_cap_count; i++)
						{
							g_key_matrix[g_cap_keys[i]].mode = 0;
							g_key_matrix[g_cap_keys[i]].c1 = g_tmp_c1;
						}
					g_suspend_preview = 0;
				}
				MsiHid_SetFullKeyboard(0, 0, 0, 1);
			}
			else
				if (102 == id)
				{
					SaveConfigToFile();
					DestroyWindow(hWnd);
				}
				else
					if (103 == id) DestroyWindow(hWnd);
			break;
		}
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, msg, wp, lp);
	}
	return 0;
}

LRESULT CALLBACK SubStripProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
		case WM_CREATE:
		{
			KillDaemon();
			HWND hBtnSet = CreateWindow(L"BUTTON", L"进入灯区设置", WS_CHILD | WS_VISIBLE | 0x0000000E, 20, 20, 200, 60, hWnd, (HMENU)101, NULL, NULL);
			SendMessage(hBtnSet, 0x1609, 0, (LPARAM)L"选中灯条/Logo并上色");
			HWND b2 = CreateWindow(L"BUTTON", L"保存并返回", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 100, 10, 35, hWnd, (HMENU)102, NULL, NULL);
			HWND b3 = CreateWindow(L"BUTTON", L"取消", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 100, 10, 35, hWnd, (HMENU)103, NULL, NULL);
			ApplyModernFont(hWnd);
			AutoSizeButton(b2, 25, 0);
			AutoSizeButton(b3, 40, 0);
			RECT r;
			GetWindowRect(b2, &r);
			SetWindowPos(b3, NULL, 20 + (r.right - r.left) + 10, 100, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			break;
		}
		case WM_CTLCOLORSTATIC:
			return (LRESULT)GetStockObject(WHITE_BRUSH);
		case WM_COMMAND:
		{
			int id = LOWORD(wp);
			if (101 == id)
			{
				g_suspend_preview = 1;
				g_pick_mode = 1;
				g_strip_sel[0] = 0;
				g_strip_sel[1] = 0;
				g_strip_sel[2] = 0;
				g_strip_sel[3] = 0;
				UpdateStripPreview();
				RECT rp;
				GetWindowRect(hWnd, &rp);
				WNDCLASSEX wc = { sizeof(WNDCLASSEX), 0, StripSelDlgProc, 0, 0, GetModuleHandle(NULL), NULL, LoadCursor(NULL, IDC_ARROW), (HBRUSH)GetStockObject(WHITE_BRUSH), NULL, L"StripSel", NULL };
				RegisterClassEx(&wc);
				HWND hSel = CreateWindowEx(WS_EX_TOPMOST | WS_EX_DLGMODALFRAME, L"StripSel", L"第 1 步: 选择灯区", WS_POPUP | WS_CAPTION | WS_SYSMENU, rp.left, rp.top, 380, 190, hWnd, NULL, GetModuleHandle(NULL), NULL);
				CreateWindow(L"STATIC", L"请选择要配置的灯区 (支持多选):", WS_CHILD | WS_VISIBLE, 20, 20, 300, 20, hSel, NULL, NULL, NULL);
				HWND b1 = CreateWindow(L"BUTTON", L"左灯条", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 50, 10, 30, hSel, (HMENU)301, NULL, NULL);
				HWND b2 = CreateWindow(L"BUTTON", L"中灯条", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 50, 10, 30, hSel, (HMENU)302, NULL, NULL);
				HWND b3 = CreateWindow(L"BUTTON", L"右灯条", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 50, 10, 30, hSel, (HMENU)303, NULL, NULL);
				HWND b4 = CreateWindow(L"BUTTON", L"Logo", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 50, 10, 30, hSel, (HMENU)304, NULL, NULL);
				HWND b5 = CreateWindow(L"BUTTON", L"下一步", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 80, 100, 10, 30, hSel, (HMENU)IDOK, NULL, NULL);
				HWND b6 = CreateWindow(L"BUTTON", L"取消", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 180, 100, 10, 30, hSel, (HMENU)IDCANCEL, NULL, NULL);
				ApplyModernFont(hSel);
				AutoSizeButton(b1, 10, 0);
				AutoSizeButton(b2, 10, 0);
				AutoSizeButton(b3, 10, 0);
				AutoSizeButton(b4, 10, 0);
				AutoSizeButton(b5, 20, 0);
				AutoSizeButton(b6, 20, 0);
				RECT r;
				GetWindowRect(b1, &r);
				int w = r.right - r.left;
				SetWindowPos(b2, NULL, 20 + w + 5, 50, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
				SetWindowPos(b3, NULL, 20 + (w + 5) * 2, 50, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
				SetWindowPos(b4, NULL, 20 + (w + 5) * 3, 50, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
				RunModalWindow(hSel, hWnd);
				UnregisterClass(L"StripSel", GetModuleHandle(NULL));

				if (!g_strip_sel[0] && !g_strip_sel[1] && !g_strip_sel[2] && !g_strip_sel[3])
				{
					g_suspend_preview = 0;
					RestoreStrips();
					break;
				}

				g_suspend_preview = 0;
				RestoreStrips();

				if (g_global_dynamic)
				{
					WNDCLASSEX wc2 = { sizeof(WNDCLASSEX), 0, StripDynDlgProc, 0, 0, GetModuleHandle(NULL), NULL, LoadCursor(NULL, IDC_ARROW), (HBRUSH)GetStockObject(WHITE_BRUSH), NULL, L"StripDyn", NULL };
					RegisterClassEx(&wc2);
					HWND hDyn = CreateWindowEx(WS_EX_DLGMODALFRAME, L"StripDyn", L"第 2 步: 灯条动态参数", WS_POPUP | WS_CAPTION | WS_SYSMENU, rp.left, rp.top, 480, 360, hWnd, NULL, GetModuleHandle(NULL), NULL);
					RunModalWindow(hDyn, hWnd);
					UnregisterClass(L"StripDyn", GetModuleHandle(NULL));
				}
				else
				{
					g_suspend_preview = 1;
					if (PickColor(hWnd, &g_tmp_c1))
						for (int i = 0; i < 4; i++)
							if (g_strip_sel[i])
							{
								g_special_zones[i].mode = 0;
								g_special_zones[i].c1 = g_tmp_c1;
							}
					g_suspend_preview = 0;
				}
				RestoreStrips();
			}
			else
				if (102 == id)
				{
					SaveConfigToFile();
					DestroyWindow(hWnd);
				}
				else
					if (103 == id) DestroyWindow(hWnd);
			break;
		}
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, msg, wp, lp);
	}
	return 0;
}

void StartKeyConfigSubWindow(HWND hParent)
{
	RECT rp;
	GetWindowRect(hParent, &rp);
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), 0, SubKeyProc, 0, 0, GetModuleHandle(NULL), NULL, LoadCursor(NULL, IDC_ARROW), (HBRUSH)GetStockObject(WHITE_BRUSH), NULL, L"SubKey", NULL };
	RegisterClassEx(&wc);
	HWND hSub = CreateWindowEx(WS_EX_DLGMODALFRAME, L"SubKey", L"键盘配置", WS_POPUP | WS_CAPTION | WS_SYSMENU, rp.left, rp.top, 400, 200, hParent, NULL, GetModuleHandle(NULL), NULL);
	RunModalWindow(hSub, hParent);
	UnregisterClass(L"SubKey", GetModuleHandle(NULL));
}

void StartStripConfigSubWindow(HWND hParent)
{
	RECT rp;
	GetWindowRect(hParent, &rp);
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), 0, SubStripProc, 0, 0, GetModuleHandle(NULL), NULL, LoadCursor(NULL, IDC_ARROW), (HBRUSH)GetStockObject(WHITE_BRUSH), NULL, L"SubStrip", NULL };
	RegisterClassEx(&wc);
	HWND hSub = CreateWindowEx(WS_EX_DLGMODALFRAME, L"SubStrip", L"灯条配置", WS_POPUP | WS_CAPTION | WS_SYSMENU, rp.left, rp.top, 400, 200, hParent, NULL, GetModuleHandle(NULL), NULL);
	RunModalWindow(hSub, hParent);
	UnregisterClass(L"SubStrip", GetModuleHandle(NULL));
}
