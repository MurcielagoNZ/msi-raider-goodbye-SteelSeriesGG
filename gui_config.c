#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#include <windows.h>
#include <commctrl.h> 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "shared_config.h"
#include "sys_monitor.h"

#pragma comment(lib, "Comctl32.lib")

ItemConfig g_key_matrix[256] = { 0 };
ItemConfig g_special_zones[4] = { 0 };
int g_global_dynamic = 0, g_pause_key_render = 0, g_pause_strip_render = 0, g_suspend_preview = 0;

const char *CONFIG_FILE = "config.csv";
HFONT g_hModernFont = NULL;

char g_file_lines[2000][256] = { 0 };
int g_file_line_count = 0;

char g_global_extra[256] = { 0 };
char g_key_extra[256][256] = { 0 };
char g_zone_extra[4][256] = { 0 };

const unsigned char VALID_KEYS[102] = { 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x29, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x49, 0x4b, 0x4c, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63, 0x66, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xf0, 0x28, 0x2a, 0x31, 0x64, 0xe5, 0xe6 };
static const char *key_names_dict[256] = { 0 };

BOOL CALLBACK SetFontEnumProc(HWND hwndChild, LPARAM lParam)
{
	SendMessage(hwndChild, WM_SETFONT, (WPARAM)lParam, TRUE);
	return TRUE;
}

void ApplyModernFont(HWND hWnd)
{
	if (NULL == g_hModernFont)
	{
		LOGFONT lf = { 0 };
		lf.lfHeight = -16;
		lf.lfWeight = FW_NORMAL;
		lf.lfQuality = CLEARTYPE_QUALITY;
		wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Microsoft YaHei UI");
		g_hModernFont = CreateFontIndirect(&lf);
	}
	EnumChildWindows(hWnd, SetFontEnumProc, (LPARAM)g_hModernFont);
}

void AutoSizeButton(HWND hBtn, int extraPaddingX, int extraPaddingY)
{
	SIZE size = { 0 };
	if (SendMessage(hBtn, BCM_GETIDEALSIZE, 0, (LPARAM)&size))
		SetWindowPos(hBtn, NULL, 0, 0, size.cx + extraPaddingX, size.cy + extraPaddingY, SWP_NOMOVE | SWP_NOZORDER);
}

void InitKeyNames(void)
{
	key_names_dict[0x04] = "A"; key_names_dict[0x05] = "B"; key_names_dict[0x06] = "C"; key_names_dict[0x07] = "D"; key_names_dict[0x08] = "E"; key_names_dict[0x09] = "F"; key_names_dict[0x0A] = "G"; key_names_dict[0x0B] = "H"; key_names_dict[0x0C] = "I"; key_names_dict[0x0D] = "J"; key_names_dict[0x0E] = "K"; key_names_dict[0x0F] = "L"; key_names_dict[0x10] = "M"; key_names_dict[0x11] = "N"; key_names_dict[0x12] = "O"; key_names_dict[0x13] = "P"; key_names_dict[0x14] = "Q"; key_names_dict[0x15] = "R"; key_names_dict[0x16] = "S"; key_names_dict[0x17] = "T"; key_names_dict[0x18] = "U"; key_names_dict[0x19] = "V"; key_names_dict[0x1A] = "W"; key_names_dict[0x1B] = "X"; key_names_dict[0x1C] = "Y"; key_names_dict[0x1D] = "Z";
	key_names_dict[0x1E] = "1"; key_names_dict[0x1F] = "2"; key_names_dict[0x20] = "3"; key_names_dict[0x21] = "4"; key_names_dict[0x22] = "5"; key_names_dict[0x23] = "6"; key_names_dict[0x24] = "7"; key_names_dict[0x25] = "8"; key_names_dict[0x26] = "9"; key_names_dict[0x27] = "0";
	key_names_dict[0x28] = "Enter"; key_names_dict[0x29] = "Esc"; key_names_dict[0x2A] = "Backspace"; key_names_dict[0x2B] = "Tab"; key_names_dict[0x2C] = "Space"; key_names_dict[0x2D] = "Minus"; key_names_dict[0x2E] = "Equal"; key_names_dict[0x2F] = "LBracket"; key_names_dict[0x30] = "RBracket"; key_names_dict[0x31] = "Backslash"; key_names_dict[0x33] = "Semicolon"; key_names_dict[0x34] = "Quote"; key_names_dict[0x35] = "Tilde"; key_names_dict[0x36] = "Comma"; key_names_dict[0x37] = "Period"; key_names_dict[0x38] = "Slash"; key_names_dict[0x39] = "CapsLock";
	key_names_dict[0x3A] = "F1"; key_names_dict[0x3B] = "F2"; key_names_dict[0x3C] = "F3"; key_names_dict[0x3D] = "F4"; key_names_dict[0x3E] = "F5"; key_names_dict[0x3F] = "F6"; key_names_dict[0x40] = "F7"; key_names_dict[0x41] = "F8"; key_names_dict[0x42] = "F9"; key_names_dict[0x43] = "F10"; key_names_dict[0x44] = "F11"; key_names_dict[0x45] = "F12";
	key_names_dict[0x46] = "PrintScreen"; key_names_dict[0x47] = "ScrollLock"; key_names_dict[0x48] = "Pause"; key_names_dict[0x49] = "Insert"; key_names_dict[0x4A] = "Home"; key_names_dict[0x4B] = "PageUp"; key_names_dict[0x4C] = "Delete"; key_names_dict[0x4D] = "End"; key_names_dict[0x4E] = "PageDown";
	key_names_dict[0x4F] = "Right"; key_names_dict[0x50] = "Left"; key_names_dict[0x51] = "Down"; key_names_dict[0x52] = "Up";
	key_names_dict[0x53] = "NumLock"; key_names_dict[0x54] = "Num_Div"; key_names_dict[0x55] = "Num_Mul"; key_names_dict[0x56] = "Num_Sub"; key_names_dict[0x57] = "Num_Add"; key_names_dict[0x58] = "Num_Enter"; key_names_dict[0x59] = "Num_1"; key_names_dict[0x5A] = "Num_2"; key_names_dict[0x5B] = "Num_3"; key_names_dict[0x5C] = "Num_4"; key_names_dict[0x5D] = "Num_5"; key_names_dict[0x5E] = "Num_6"; key_names_dict[0x5F] = "Num_7"; key_names_dict[0x60] = "Num_8"; key_names_dict[0x61] = "Num_9"; key_names_dict[0x62] = "Num_0"; key_names_dict[0x63] = "Num_Dot"; key_names_dict[0x64] = "NonUS_Backslash"; key_names_dict[0x66] = "Power";
	key_names_dict[0xE0] = "LCtrl"; key_names_dict[0xE1] = "LShift"; key_names_dict[0xE2] = "LAlt"; key_names_dict[0xE3] = "LWin"; key_names_dict[0xE4] = "RCtrl"; key_names_dict[0xE5] = "RShift"; key_names_dict[0xE6] = "RAlt"; key_names_dict[0xF0] = "Fn";
}

unsigned char MapVKToHID(DWORD vk, DWORD flags, DWORD scanCode)
{
	if (vk >= 'A' && vk <= 'Z') return (unsigned char)(vk - 'A' + 0x04);
	if (vk >= '1' && vk <= '9') return (unsigned char)(vk - '1' + 0x1E);
	if (0 == vk - '0') return 0x27;
	if (VK_LCONTROL == vk) return 0xE0;
	if (VK_RCONTROL == vk) return 0xE4;
	if (VK_LMENU == vk) return 0xE2;
	if (VK_RMENU == vk) return 0xE6;
	if (VK_LWIN == vk) return 0xE3;
	if (VK_RWIN == vk) return 0xE7;
	if (VK_LSHIFT == vk) return 0xE1;
	if (VK_RSHIFT == vk) return 0xE5;
	if (VK_CONTROL == vk) return (flags & LLKHF_EXTENDED) ? 0xE4 : 0xE0;
	if (VK_MENU == vk) return (flags & LLKHF_EXTENDED) ? 0xE6 : 0xE2;
	if (VK_SHIFT == vk) return (0x36 == scanCode) ? 0xE5 : 0xE1;
	if (vk >= VK_F1 && vk <= VK_F12) return (unsigned char)(vk - VK_F1 + 0x3A);
	if (VK_ESCAPE == vk) return 0x29;
	if (VK_RETURN == vk) return 0x28;
	if (VK_BACK == vk) return 0x2A;
	if (VK_TAB == vk) return 0x2B;
	if (VK_SPACE == vk) return 0x2C;
	if (VK_OEM_MINUS == vk) return 0x2D;
	if (VK_OEM_PLUS == vk) return 0x2E;
	if (VK_OEM_4 == vk) return 0x2F;
	if (VK_OEM_6 == vk) return 0x30;
	if (VK_OEM_5 == vk) return 0x31;
	if (VK_OEM_1 == vk) return 0x33;
	if (VK_OEM_7 == vk) return 0x34;
	if (VK_OEM_3 == vk) return 0x35;
	if (VK_OEM_COMMA == vk) return 0x36;
	if (VK_OEM_PERIOD == vk) return 0x37;
	if (VK_OEM_2 == vk) return 0x38;
	if (VK_CAPITAL == vk) return 0x39;
	if (VK_RIGHT == vk) return 0x4F;
	if (VK_LEFT == vk) return 0x50;
	if (VK_DOWN == vk) return 0x51;
	if (VK_UP == vk) return 0x52;
	if (VK_SNAPSHOT == vk) return 0x46;
	if (VK_SCROLL == vk) return 0x47;
	if (VK_PAUSE == vk) return 0x48;
	if (VK_INSERT == vk) return 0x49;
	if (VK_HOME == vk) return 0x4A;
	if (VK_PRIOR == vk) return 0x4B;
	if (VK_DELETE == vk) return 0x4C;
	if (VK_END == vk) return 0x4D;
	if (VK_NEXT == vk) return 0x4E;
	if (vk >= VK_NUMPAD1 && vk <= VK_NUMPAD9) return (unsigned char)(vk - VK_NUMPAD1 + 0x59);
	if (VK_NUMPAD0 == vk) return 0x62;
	if (VK_DECIMAL == vk) return 0x63;
	if (VK_MULTIPLY == vk) return 0x55;
	if (VK_ADD == vk) return 0x57;
	if (VK_SUBTRACT == vk) return 0x56;
	if (VK_DIVIDE == vk) return 0x54;
	if (VK_NUMLOCK == vk) return 0x53;
	return 0;
}

RGBColor LerpColor(RGBColor start, RGBColor end, float t)
{
	if (t < 0.0f)
		t = 0.0f;
	if (t > 1.0f)
		t = 1.0f;
	RGBColor res;
	res.r = (unsigned char)(start.r + (end.r - start.r) * t);
	res.g = (unsigned char)(start.g + (end.g - start.g) * t);
	res.b = (unsigned char)(start.b + (end.b - start.b) * t);
	return res;
}

static char *GetNextCsvField(char **cursor)
{
	if (NULL == *cursor)
		return "";
	char *start = *cursor;
	char *end = strchr(start, ',');
	if (NULL != end)
	{
		*end = '\0';
		*cursor = end + 1;
	}
	else
		*cursor = NULL;
	return start;
}

static int ParseIntStrict(const char *str, BOOL *is_err)
{
	if (NULL == str || '\0' == str[0])
	{
		*is_err = TRUE;
		return 0;
	}
	char *endptr;
	long val = strtol(str, &endptr, 10);
	if (endptr == str || '\0' != *endptr)
		*is_err = TRUE;
	if (val < 0)
		*is_err = TRUE;
	return (int)val;
}

static RGBColor ParseHexColorStrict(const char *str, BOOL *is_err)
{
	RGBColor c = { 0 };
	if (NULL == str || '\0' == str[0])
	{
		*is_err = TRUE;
		return c;
	}
	const char *p = str;
	if (NULL != strstr(str, "0x"))
		p += 2;
	else
		if (NULL != strstr(str, "0X"))
			p += 2;
		else
			if ('#' == str[0])
				p += 1;

	if (strlen(p) != 6)
	{
		*is_err = TRUE;
		return c;
	}
	for (int i = 0; i < 6; i++)
		if (0 == isxdigit(p[i]))
			*is_err = TRUE;

	unsigned int r, g, b;
	if (3 != sscanf_s(p, "%2x%2x%2x", &r, &g, &b))
		*is_err = TRUE;

	c.r = (unsigned char)r;
	c.g = (unsigned char)g;
	c.b = (unsigned char)b;
	return c;
}

static void FormatHexColor(char *buf, size_t bufSize, RGBColor color)
{
	sprintf_s(buf, bufSize, "0x%02X%02X%02X", color.r, color.g, color.b);
}

int GetDefaultValueForMode(int mode, int step)
{
	if (mode <= 1)
		return 1024;

	if (mode >= 2 && mode <= 4)
	{
		if (1 == step) return 30;
		if (2 == step) return 50;
		if (3 == step) return 80;
	}

	if (5 == mode)
	{
		int max_t = SysMonitor_GetGpuTempWall();
		if (1 == step) return max_t * 30 / 100;
		if (2 == step) return max_t * 50 / 100;
		if (3 == step) return max_t * 80 / 100;
	}

	return 0;
}

void InitializeDefaultData(void)
{
	InitKeyNames();
	memset(g_key_matrix, 0, sizeof(ItemConfig) * 256);
	memset(g_special_zones, 0, sizeof(ItemConfig) * 4);
	g_global_dynamic = 0;
	memset(g_global_extra, 0, sizeof(g_global_extra));
	memset(g_key_extra, 0, sizeof(g_key_extra));
	memset(g_zone_extra, 0, sizeof(g_zone_extra));

	RGBColor defColor = { 0x66, 0x66, 0x66 };

	for (int i = 0; i < 102; i++)
	{
		unsigned char id = VALID_KEYS[i];
		g_key_matrix[id].id = id;
		const char *name = key_names_dict[id];

		if (NULL != name)
			strcpy_s(g_key_matrix[id].name, 32, name);
		else
			sprintf_s(g_key_matrix[id].name, 32, "KEY_%02X", id);

		g_key_matrix[id].mode = 0;
		g_key_matrix[id].c1 = defColor;
		g_key_matrix[id].c2 = defColor;
		g_key_matrix[id].c3 = defColor;
		g_key_matrix[id].v1 = GetDefaultValueForMode(0, 1);
		g_key_matrix[id].v2 = GetDefaultValueForMode(0, 2);
		g_key_matrix[id].v3 = GetDefaultValueForMode(0, 3);
	}

	const char *spNames[] = { "lightBar_L", "lightBar_M", "lightBar_R", "logo" };
	for (int i = 0; i < 4; i++)
	{
		strcpy_s(g_special_zones[i].name, 32, spNames[i]);
		g_special_zones[i].id = (unsigned char)i;
		g_special_zones[i].mode = 0;
		g_special_zones[i].c1 = defColor;
		g_special_zones[i].c2 = defColor;
		g_special_zones[i].c3 = defColor;
		g_special_zones[i].v1 = GetDefaultValueForMode(0, 1);
		g_special_zones[i].v2 = GetDefaultValueForMode(0, 2);
		g_special_zones[i].v3 = GetDefaultValueForMode(0, 3);
	}
}

void ReloadFileToMemory(void)
{
	g_file_line_count = 0;
	FILE *fp = NULL;
	if (0 == fopen_s(&fp, CONFIG_FILE, "r") && NULL != fp)
	{
		char raw_line[512];
		while (fgets(raw_line, sizeof(raw_line), fp))
		{
			char *nl = strchr(raw_line, '\r');
			if (NULL != nl)
				*nl = '\0';
			nl = strchr(raw_line, '\n');
			if (NULL != nl)
				*nl = '\0';

			strcpy_s(g_file_lines[g_file_line_count], 256, raw_line);
			g_file_line_count++;
			if (g_file_line_count >= 2000)
				break;
		}
		fclose(fp);
	}
}

BOOL LoadConfigWithFaultTolerance(HWND hWnd)
{
	while (TRUE)
	{
		InitializeDefaultData();
		ReloadFileToMemory();

		if (0 == g_file_line_count)
		{
			SaveConfigToFile();
			return TRUE;
		}

		BOOL has_error = FALSE;

		for (int i = 0; i < g_file_line_count; i++)
		{
			char line_copy[256];
			strcpy_s(line_copy, 256, g_file_lines[i]);
			char *trim = line_copy;
			while (' ' == *trim || '\t' == *trim)
				trim++;

			if ('\0' == *trim || '#' == *trim)
				continue;

			char *cursor = trim;
			char *f_name = GetNextCsvField(&cursor);

			if (0 == strcmp("GLOBAL_DYNAMIC", f_name))
			{
				char *f_val = GetNextCsvField(&cursor);
				if ('\0' != f_val[0])
				{
					BOOL is_err = FALSE;
					int val = ParseIntStrict(f_val, &is_err);
					if (FALSE == is_err)
						g_global_dynamic = val;
					else
						has_error = TRUE;
				}
				else
					has_error = TRUE;

				if (NULL != cursor)
					strcpy_s(g_global_extra, 256, cursor);

				continue;
			}

			int is_zone = -1;
			unsigned char key_id = 0;

			if (0 == strcmp("lightBar_L", f_name)) is_zone = 0;
			else if (0 == strcmp("lightBar_M", f_name)) is_zone = 1;
			else if (0 == strcmp("lightBar_R", f_name)) is_zone = 2;
			else if (0 == strcmp("logo", f_name)) is_zone = 3;

			BOOL is_known = FALSE;
			if (is_zone >= 0)
				is_known = TRUE;
			else
			{
				for (int k = 0; k < 102; k++)
				{
					unsigned char id = VALID_KEYS[k];
					if (NULL != key_names_dict[id])
					{
						if (0 == strcmp(f_name, key_names_dict[id]))
						{
							is_known = TRUE;
							key_id = id;
							break;
						}
					}
				}
			}

			if (FALSE == is_known)
				continue;

			ItemConfig *cfg = (is_zone >= 0) ? &g_special_zones[is_zone] : &g_key_matrix[key_id];

			char *f_id = GetNextCsvField(&cursor);

			char *f_mode = GetNextCsvField(&cursor);
			if ('\0' != f_mode[0])
			{
				BOOL is_err = FALSE;
				int m = ParseIntStrict(f_mode, &is_err);
				if (FALSE == is_err)
					cfg->mode = m;
				else
					has_error = TRUE;
			}
			else
				has_error = TRUE;

			char *f_c1 = GetNextCsvField(&cursor);
			if ('\0' != f_c1[0])
			{
				BOOL is_err = FALSE;
				RGBColor c = ParseHexColorStrict(f_c1, &is_err);
				if (FALSE == is_err)
					cfg->c1 = c;
				else
					has_error = TRUE;
			}
			else
				has_error = TRUE;

			if (is_zone >= 0 || cfg->mode > 0)
			{
				char *f_v1 = GetNextCsvField(&cursor);
				if ('\0' != f_v1[0])
				{
					BOOL err = FALSE;
					int v = ParseIntStrict(f_v1, &err);
					if (FALSE == err)
						cfg->v1 = v;
					else
						has_error = TRUE;
				}
				else
					has_error = TRUE;

				char *f_c2 = GetNextCsvField(&cursor);
				if ('\0' != f_c2[0])
				{
					BOOL err = FALSE;
					RGBColor c = ParseHexColorStrict(f_c2, &err);
					if (FALSE == err)
						cfg->c2 = c;
					else
						has_error = TRUE;
				}
				else
					has_error = TRUE;

				char *f_v2 = GetNextCsvField(&cursor);
				if ('\0' != f_v2[0])
				{
					BOOL err = FALSE;
					int v = ParseIntStrict(f_v2, &err);
					if (FALSE == err)
						cfg->v2 = v;
					else
						has_error = TRUE;
				}
				else
					has_error = TRUE;
			}

			if (is_zone >= 0 || cfg->mode > 1)
			{
				char *f_c3 = GetNextCsvField(&cursor);
				if ('\0' != f_c3[0])
				{
					BOOL err = FALSE;
					RGBColor c = ParseHexColorStrict(f_c3, &err);
					if (FALSE == err)
						cfg->c3 = c;
					else
						has_error = TRUE;
				}
				else
					has_error = TRUE;

				char *f_v3 = GetNextCsvField(&cursor);
				if ('\0' != f_v3[0])
				{
					BOOL err = FALSE;
					int v = ParseIntStrict(f_v3, &err);
					if (FALSE == err)
						cfg->v3 = v;
					else
						has_error = TRUE;
				}
				else
					has_error = TRUE;
			}

			if (NULL != cursor)
			{
				if (is_zone >= 0)
					strcpy_s(g_zone_extra[is_zone], 256, cursor);
				else
					strcpy_s(g_key_extra[key_id], 256, cursor);
			}
		}

		if (TRUE == has_error)
		{
			TASKDIALOG_BUTTON buttons[] = { { 101, L"好的，我知道了" } };
			TASKDIALOGCONFIG tc = { 0 };
			tc.cbSize = sizeof(tc);
			tc.hwndParent = hWnd;
			tc.dwFlags = TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION;
			tc.pszWindowTitle = L"配置文件纠错机制";
			tc.pszMainIcon = TD_WARNING_ICON;
			tc.pszMainInstruction = L"配置文件中包含格式错误或缺失数据";
			tc.pszContent = L"程序已自动将无效数据安全重置为对应模式的默认参数（#666666/1024等），稍后将重新整洁地保存到原文件。";
			tc.pButtons = buttons;
			tc.cButtons = 1;
			int btn = 0;
			TaskDialogIndirect(&tc, &btn, NULL, NULL);

			if (IDCANCEL == btn)
				ExitProcess(1);
			if (101 == btn)
			{
				SaveConfigToFile();
				continue;
			}
		}

		break;
	}
	return TRUE;
}

static void FormatAndPrintZone(FILE *fp, int i)
{
	char c1[16], c2[16], c3[16];
	FormatHexColor(c1, 16, g_special_zones[i].c1);
	FormatHexColor(c2, 16, g_special_zones[i].c2);
	FormatHexColor(c3, 16, g_special_zones[i].c3);

	fprintf(fp, "%s,%d,%d,%s,%d,%s,%d,%s,%d",
		g_special_zones[i].name, g_special_zones[i].id, g_special_zones[i].mode,
		c1, g_special_zones[i].v1, c2, g_special_zones[i].v2, c3, g_special_zones[i].v3);

	if ('\0' != g_zone_extra[i][0])
		fprintf(fp, ",%s", g_zone_extra[i]);

	fprintf(fp, "\n");
}

static void FormatAndPrintKey(FILE *fp, unsigned char id)
{
	char c1[16], c2[16], c3[16];
	FormatHexColor(c1, 16, g_key_matrix[id].c1);
	FormatHexColor(c2, 16, g_key_matrix[id].c2);
	FormatHexColor(c3, 16, g_key_matrix[id].c3);

	fprintf(fp, "%s,0x%02X,%d,%s,%d,%s,%d,%s,%d",
		g_key_matrix[id].name, g_key_matrix[id].id, g_key_matrix[id].mode,
		c1, g_key_matrix[id].v1, c2, g_key_matrix[id].v2, c3, g_key_matrix[id].v3);

	if ('\0' != g_key_extra[id][0])
		fprintf(fp, ",%s", g_key_extra[id]);

	fprintf(fp, "\n");
}

BOOL SaveConfigToFile(void)
{
	FILE *fp = NULL;
	if (0 != fopen_s(&fp, CONFIG_FILE, "w") || NULL == fp)
	{
		MessageBoxW(NULL, L"无法写入 config.csv！\n文件可能正被其他程序占用，请关闭后重试。", L"严重错误", MB_ICONERROR);
		ExitProcess(1);
		return FALSE;
	}

	if (0 == g_file_line_count)
	{
		fprintf(fp, "# ==========================================================\n");
		fprintf(fp, "# MSI Light Control - 键盘与灯条光效配置文件\n");
		fprintf(fp, "# 模式说明 (键盘): 0=静态, 1=按下变色, 2=CPU, 3=显卡功耗, 4=显存, 5=温度\n");
		fprintf(fp, "# 模式说明 (灯条): 0=三色突变, 1=平滑渐变, 2/3/4/5=系统联动\n");
		fprintf(fp, "# ==========================================================\n\n");

		fprintf(fp, "# [ 全局渲染开关 ]\n");
		fprintf(fp, "GLOBAL_DYNAMIC,%d\n", g_global_dynamic);

		fprintf(fp, "\n# [ 特殊灯区/Logo配置 ]\n");
		fprintf(fp, "# Name,ID,Mode,C1,V1,C2,V2,C3,V3\n");
		for (int i = 0; i < 4; i++)
			FormatAndPrintZone(fp, i);

		fprintf(fp, "\n# [ 键盘按键矩阵配置 ]\n");
		fprintf(fp, "# Name,ID,Mode,C1,V1,C2,V2,C3,V3\n");
		for (int i = 0; i < 102; i++)
			FormatAndPrintKey(fp, VALID_KEYS[i]);
	}
	else
	{
		int global_flag = 0;
		int zone_flags[4] = { 0 };
		int key_flags[256] = { 0 };

		for (int i = 0; i < g_file_line_count; i++)
		{
			char line_copy[256];
			strcpy_s(line_copy, 256, g_file_lines[i]);
			char *trim = line_copy;
			while (' ' == *trim || '\t' == *trim)
				trim++;

			if ('\0' == *trim || '#' == *trim)
			{
				fprintf(fp, "%s\n", g_file_lines[i]);
				continue;
			}

			char *cursor = trim;
			char *f_name = GetNextCsvField(&cursor);

			if (0 == strcmp("GLOBAL_DYNAMIC", f_name))
			{
				fprintf(fp, "GLOBAL_DYNAMIC,%d", g_global_dynamic);
				if ('\0' != g_global_extra[0])
					fprintf(fp, ",%s", g_global_extra);
				fprintf(fp, "\n");
				global_flag = 1;
				continue;
			}

			int is_zone = -1;
			unsigned char key_id = 0;

			if (0 == strcmp("lightBar_L", f_name)) is_zone = 0;
			else if (0 == strcmp("lightBar_M", f_name)) is_zone = 1;
			else if (0 == strcmp("lightBar_R", f_name)) is_zone = 2;
			else if (0 == strcmp("logo", f_name)) is_zone = 3;

			if (is_zone >= 0)
			{
				FormatAndPrintZone(fp, is_zone);
				zone_flags[is_zone] = 1;
				continue;
			}

			for (int k = 0; k < 102; k++)
			{
				unsigned char id = VALID_KEYS[k];
				if (NULL != key_names_dict[id])
				{
					if (0 == strcmp(f_name, key_names_dict[id]))
					{
						key_id = id;
						break;
					}
				}
			}

			if (key_id > 0)
			{
				FormatAndPrintKey(fp, key_id);
				key_flags[key_id] = 1;
				continue;
			}

			fprintf(fp, "%s\n", g_file_lines[i]);
		}

		if (0 == global_flag)
			fprintf(fp, "GLOBAL_DYNAMIC,%d\n", g_global_dynamic);

		int missing_zone = 0;
		for (int i = 0; i < 4; i++)
			if (0 == zone_flags[i])
				missing_zone = 1;

		if (1 == missing_zone)
		{
			fprintf(fp, "\n# [ Zone Configuration Title ]\n");
			fprintf(fp, "# Name,ID,Mode,C1,V1,C2,V2,C3,V3\n");
			for (int i = 0; i < 4; i++)
				if (0 == zone_flags[i])
					FormatAndPrintZone(fp, i);
		}

		int missing_key = 0;
		for (int i = 0; i < 102; i++)
			if (0 == key_flags[VALID_KEYS[i]])
				missing_key = 1;

		if (1 == missing_key)
		{
			fprintf(fp, "\n# [ Key Configuration Title ]\n");
			fprintf(fp, "# Name,ID,Mode,C1,V1,C2,V2,C3,V3\n");
			for (int i = 0; i < 102; i++)
			{
				unsigned char id = VALID_KEYS[i];
				if (0 == key_flags[id])
					FormatAndPrintKey(fp, id);
			}
		}
	}

	fclose(fp);
	ReloadFileToMemory();
	return TRUE;
}
