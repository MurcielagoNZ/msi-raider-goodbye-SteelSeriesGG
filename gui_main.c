#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <windows.h>
#include <stdio.h>
#include <commctrl.h> 
#include "shared_config.h"
#include "msi_hid_core.h"
#include "hidapi.h"
#include "sys_monitor.h"

extern void StartKeyConfigSubWindow(HWND hParent);
extern void StartStripConfigSubWindow(HWND hParent);

HWND hChkDynamic;
typedef struct
{
	BOOL isPressed;
	DWORD pressStartTime;
	DWORD releaseStartTime;
	RGBColor currentColor;
} FxState;

FxState g_fx[256] = { 0 };
HHOOK g_hPlaybackHook = NULL;

RGBColor CalcLinkageColor(int val, ItemConfig *cfg)
{
	if (val <= cfg->v1)
		return cfg->c1;

	if (val <= cfg->v2)
	{
		if (cfg->v1 == cfg->v2)
			return cfg->c2;
		float t = (float)(val - cfg->v1) / (cfg->v2 - cfg->v1);
		return LerpColor(cfg->c1, cfg->c2, t);
	}

	if (val <= cfg->v3)
	{
		if (cfg->v2 == cfg->v3)
			return cfg->c3;
		float t = (float)(val - cfg->v2) / (cfg->v3 - cfg->v2);
		return LerpColor(cfg->c2, cfg->c3, t);
	}

	return cfg->c3;
}

LRESULT CALLBACK PlaybackHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (HC_ACTION == nCode)
	{
		KBDLLHOOKSTRUCT *p = (KBDLLHOOKSTRUCT *)lParam;
		unsigned char hid = MapVKToHID(p->vkCode, p->flags, p->scanCode);
		if (0 != hid)
		{
			if (WM_KEYDOWN == wParam || WM_SYSKEYDOWN == wParam)
			{
				if (FALSE == g_fx[hid].isPressed)
				{
					g_fx[hid].isPressed = TRUE;
					g_fx[hid].pressStartTime = GetTickCount();
				}
			}
			else
				if (WM_KEYUP == wParam || WM_SYSKEYUP == wParam)
				{
					g_fx[hid].isPressed = FALSE;
					g_fx[hid].releaseStartTime = GetTickCount();
				}
		}
	}
	return CallNextHookEx(g_hPlaybackHook, nCode, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_CREATE:
		{
			LoadConfigWithFaultTolerance(hWnd);

			HWND hBtn1 = CreateWindow(L"BUTTON", L"配置键盘规则", WS_VISIBLE | WS_CHILD | 0x0000000E, 20, 20, 240, 60, hWnd, (HMENU)1001, NULL, NULL);
			SendMessage(hBtn1, 0x1609, 0, (LPARAM)L"设定单键的光效 (静态/按压/联动)");
			HWND hBtn2 = CreateWindow(L"BUTTON", L"配置灯光条规则", WS_VISIBLE | WS_CHILD | 0x0000000E, 280, 20, 240, 60, hWnd, (HMENU)1002, NULL, NULL);
			SendMessage(hBtn2, 0x1609, 0, (LPARAM)L"设定Logo与前置灯条规则");

			hChkDynamic = CreateWindow(L"BUTTON", L"启用后台动态渲染引擎", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 20, 95, 10, 20, hWnd, (HMENU)1003, NULL, NULL);
			SendMessage(hChkDynamic, BM_SETCHECK, g_global_dynamic ? BST_CHECKED : BST_UNCHECKED, 0);

			CreateWindow(L"STATIC", L"快速状态覆盖 (不保存配置):", WS_VISIBLE | WS_CHILD, 20, 130, 300, 20, hWnd, NULL, NULL, NULL);
			HWND hB4 = CreateWindow(L"BUTTON", L"全盘暗色", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 20, 155, 10, 35, hWnd, (HMENU)1004, NULL, NULL);
			HWND hB5 = CreateWindow(L"BUTTON", L"关闭键盘", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 20, 155, 10, 35, hWnd, (HMENU)1005, NULL, NULL);
			HWND hB6 = CreateWindow(L"BUTTON", L"关闭灯条", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 20, 155, 10, 35, hWnd, (HMENU)1006, NULL, NULL);
			HWND hB7 = CreateWindow(L"BUTTON", L"重启后台进程", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 20, 155, 10, 35, hWnd, (HMENU)1007, NULL, NULL);

			ApplyModernFont(hWnd);
			AutoSizeButton(hChkDynamic, 10, 0);
			AutoSizeButton(hB4, 25, 0);
			AutoSizeButton(hB5, 25, 0);
			AutoSizeButton(hB6, 25, 0);
			AutoSizeButton(hB7, 25, 0);

			RECT r4, r5, r6, r7, rcClient;
			GetWindowRect(hB4, &r4);
			GetWindowRect(hB5, &r5);
			GetWindowRect(hB6, &r6);
			GetWindowRect(hB7, &r7);
			GetClientRect(hWnd, &rcClient);
			int w4 = r4.right - r4.left, w5 = r5.right - r5.left, w6 = r6.right - r6.left, w7 = r7.right - r7.left;
			SetWindowPos(hB5, NULL, 20 + w4 + 10, 155, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			SetWindowPos(hB6, NULL, 20 + w4 + 10 + w5 + 10, 155, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			SetWindowPos(hB7, NULL, rcClient.right - w7 - 20, 155, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

			MsiHid_Init();
			SysMonitor_Init();
			g_hPlaybackHook = SetWindowsHookEx(WH_KEYBOARD_LL, PlaybackHookProc, GetModuleHandle(NULL), 0);
			SetTimer(hWnd, 1, 40, NULL);
			break;
		}
		case WM_CTLCOLORSTATIC:
			return (LRESULT)GetStockObject(WHITE_BRUSH);
		case WM_TIMER:
		{
			if (0 != g_global_dynamic && 0 == g_suspend_preview)
			{
				DWORD now = GetTickCount();
				int cur_cpu = 0, cur_gpu_power = 0, cur_vram = 0, cur_temp = 0;
				SysMonitor_Update(&cur_cpu, &cur_gpu_power, &cur_vram, &cur_temp);

				// 1. 渲染键盘按键区
				if (0 == g_pause_key_render)
				{
					for (int i = 1; i < 256; i++)
					{
						ItemConfig *cfg = &g_key_matrix[i];
						if (0 == cfg->id)
							continue;

						if (1 == cfg->mode)
						{
							if (TRUE == g_fx[i].isPressed)
							{
								if (cfg->v1 > 0)
								{
									float t = (float)(now - g_fx[i].pressStartTime) / cfg->v1;
									g_fx[i].currentColor = LerpColor(cfg->c1, cfg->c2, t);
								}
								else
									g_fx[i].currentColor = cfg->c2;
							}
							else
							{
								if (cfg->v2 > 0 && g_fx[i].releaseStartTime > 0)
								{
									float t = (float)(now - g_fx[i].releaseStartTime) / cfg->v2;
									g_fx[i].currentColor = LerpColor(cfg->c2, cfg->c1, t);
								}
								else
									g_fx[i].currentColor = cfg->c1;
							}
						}
						else
							if (cfg->mode >= 2)
							{
								int val = 0;
								if (2 == cfg->mode) val = cur_cpu;
								else
									if (3 == cfg->mode) val = cur_gpu_power;
									else
										if (4 == cfg->mode) val = cur_vram;
										else
											if (5 == cfg->mode) val = cur_temp;

								g_fx[i].currentColor = CalcLinkageColor(val, cfg);
							}
							else
								g_fx[i].currentColor = cfg->c1;
					}

					hid_device *handle = hid_open(MSI_VID, MSI_PID_KEYBOARD, NULL);
					if (NULL != handle)
					{
						unsigned char buf[525] = { 0 };
						buf[1] = 0x0C;
						buf[3] = 0x66;
						unsigned char keys[102] = { 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x29, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x49, 0x4b, 0x4c, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63, 0x66, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xf0, 0x28, 0x2a, 0x31, 0x64, 0xe5, 0xe6 };
						int offset = 5;
						for (int i = 0; i < 102; i++)
						{
							unsigned char id = keys[i];
							buf[offset++] = id;
							if (0 != g_key_matrix[id].id)
							{
								buf[offset++] = g_fx[id].currentColor.r;
								buf[offset++] = g_fx[id].currentColor.g;
								buf[offset++] = g_fx[id].currentColor.b;
							}
							else
							{
								buf[offset++] = 0;
								buf[offset++] = 0;
								buf[offset++] = 0;
							}
						}
						hid_send_feature_report(handle, buf, sizeof(buf));
						hid_close(handle);
					}
				}

				// 2. 渲染灯条/Logo区
				if (0 == g_pause_strip_render)
				{
					unsigned char sr[4] = { 0 }, sg[4] = { 0 }, sb[4] = { 0 };
					BOOL strip_needs_update = FALSE;

					for (int i = 0; i < 4; i++)
					{
						ItemConfig *cfg = &g_special_zones[i];
						if ('\0' == cfg->name[0])
							continue;

						strip_needs_update = TRUE;

						if (0 == cfg->mode)
						{
							int T = cfg->v1 + cfg->v2 + cfg->v3;
							if (T <= 0)
							{
								sr[i] = cfg->c1.r;
								sg[i] = cfg->c1.g;
								sb[i] = cfg->c1.b;
								continue;
							}

							DWORD t_cycle = now % (DWORD)T;
							if (t_cycle < (DWORD)cfg->v1)
							{
								sr[i] = cfg->c1.r;
								sg[i] = cfg->c1.g;
								sb[i] = cfg->c1.b;
							}
							else
								if (t_cycle < (DWORD)(cfg->v1 + cfg->v2))
								{
									sr[i] = cfg->c2.r;
									sg[i] = cfg->c2.g;
									sb[i] = cfg->c2.b;
								}
								else
								{
									sr[i] = cfg->c3.r;
									sg[i] = cfg->c3.g;
									sb[i] = cfg->c3.b;
								}
						}
						else
							if (1 == cfg->mode)
							{
								int T = cfg->v1 + cfg->v2 + cfg->v3;
								if (T <= 0)
								{
									sr[i] = cfg->c1.r;
									sg[i] = cfg->c1.g;
									sb[i] = cfg->c1.b;
									continue;
								}

								DWORD t_cycle = now % (DWORD)T;
								RGBColor cur = { 0 };

								if (t_cycle < (DWORD)cfg->v1)
									cur = LerpColor(cfg->c1, cfg->c2, (float)t_cycle / cfg->v1);
								else
									if (t_cycle < (DWORD)(cfg->v1 + cfg->v2))
										cur = LerpColor(cfg->c2, cfg->c3, (float)(t_cycle - cfg->v1) / cfg->v2);
									else
										cur = LerpColor(cfg->c3, cfg->c1, (float)(t_cycle - cfg->v1 - cfg->v2) / cfg->v3);

								sr[i] = cur.r;
								sg[i] = cur.g;
								sb[i] = cur.b;
							}
							else
							{
								int val = 0;
								if (2 == cfg->mode) val = cur_cpu;
								else
									if (3 == cfg->mode) val = cur_gpu_power;
									else
										if (4 == cfg->mode) val = cur_vram;
										else
											if (5 == cfg->mode) val = cur_temp;

								RGBColor mapped = CalcLinkageColor(val, cfg);
								sr[i] = mapped.r;
								sg[i] = mapped.g;
								sb[i] = mapped.b;
							}
					}

					if (TRUE == strip_needs_update)
						MsiHid_SetSpecialZones(sr, sg, sb);
				}
			}
			break;
		}
		case WM_COMMAND:
		{
			int wmId = LOWORD(wParam);
			switch (wmId)
			{
				case 1003:
					g_global_dynamic = (BST_CHECKED == SendMessage(hChkDynamic, BM_GETCHECK, 0, 0)) ? 1 : 0;
					SaveConfigToFile();
					g_pause_key_render = 0;
					g_pause_strip_render = 0;
					break;
				case 1001:
					g_pause_key_render = 0;
					g_pause_strip_render = 0;
					if (0 == g_global_dynamic)
						MsiHid_SetFullKeyboard(0, 0, 0, 1);
					ShowWindow(hWnd, SW_HIDE);
					StartKeyConfigSubWindow(hWnd);
					ShowWindow(hWnd, SW_SHOW);
					break;
				case 1002:
					g_pause_key_render = 0;
					g_pause_strip_render = 0;
					ShowWindow(hWnd, SW_HIDE);
					StartStripConfigSubWindow(hWnd);
					ShowWindow(hWnd, SW_SHOW);
					break;
				case 1004:
					g_pause_key_render = 1;
					MsiHid_SetFullKeyboard(0x88, 0x88, 0x88, 0);
					break;
				case 1005:
					g_pause_key_render = 1;
					MsiHid_SetFullKeyboard(0x00, 0x00, 0x00, 0);
					break;
				case 1006:
				{
					g_pause_strip_render = 1;
					unsigned char arr[4] = { 0 };
					MsiHid_SetSpecialZones(arr, arr, arr);
					break;
				}
				case 1007:
					MessageBoxW(hWnd, L"此功能将唤起 daemon.exe，模块独立化中...", L"预留", MB_OK);
					break;
			}
			break;
		}
		case WM_DESTROY:
			UnhookWindowsHookEx(g_hPlaybackHook);
			KillTimer(hWnd, 1);
			SysMonitor_Close();
			MsiHid_Exit();
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int show)
{
	INITCOMMONCONTROLSEX icce;
	icce.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icce.dwICC = ICC_BAR_CLASSES;
	InitCommonControlsEx(&icce);

	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, hInst, NULL, LoadCursor(NULL, IDC_ARROW), (HBRUSH)GetStockObject(WHITE_BRUSH), NULL, L"MsiConfig", NULL };
	if (0 == RegisterClassEx(&wc))
		return 0;

	int winW = 550, winH = 240;
	int screenW = GetSystemMetrics(SM_CXSCREEN);
	int screenH = GetSystemMetrics(SM_CYSCREEN);
	HWND hWnd = CreateWindowEx(0, L"MsiConfig", L"MSI 控制终端", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, (screenW - winW) / 2, (screenH - winH) / 2, winW, winH, NULL, NULL, hInst, NULL);

	ShowWindow(hWnd, show);
	UpdateWindow(hWnd);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}
