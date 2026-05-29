#ifndef SHARED_CONFIG_H
#define SHARED_CONFIG_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct
	{
		unsigned char r, g, b;
	} RGBColor;

	typedef struct
	{
		char name[32];
		unsigned char id;
		int mode;
		RGBColor c1;
		int v1;
		RGBColor c2;
		int v2;
		RGBColor c3;
		int v3;
	} ItemConfig;

	extern ItemConfig g_key_matrix[256];
	extern ItemConfig g_special_zones[4];
	extern int g_global_dynamic;
	extern int g_pause_key_render;
	extern int g_pause_strip_render;
	extern int g_suspend_preview;

	void InitKeyNames(void);
	void CreateDefaultConfig(void);
	BOOL LoadConfigWithFaultTolerance(HWND hWnd);
	BOOL SaveConfigToFile(void);

	RGBColor LerpColor(RGBColor start, RGBColor end, float t);
	unsigned char MapVKToHID(DWORD vk, DWORD flags, DWORD scanCode);
	void ApplyModernFont(HWND hWnd);
	void AutoSizeButton(HWND hBtn, int extraPaddingX, int extraPaddingY);

#ifdef __cplusplus
}
#endif

#endif
