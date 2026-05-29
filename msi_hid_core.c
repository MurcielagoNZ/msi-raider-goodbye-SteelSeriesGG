#include "msi_hid_core.h"
#include "hidapi.h"
#include "shared_config.h"
#include <string.h>

#pragma comment(lib, "SetupAPI.lib")

int MsiHid_Init()
{
	return hid_init() == 0 ? 0 : -1;
}
void MsiHid_Exit()
{
	hid_exit();
}

int MsiHid_SetSingleKeyColor(unsigned char key_id, unsigned char r, unsigned char g, unsigned char b)
{
	hid_device *handle = hid_open(MSI_VID, MSI_PID_KEYBOARD, NULL);
	if (!handle) return -1;
	unsigned char buf[525] = { 0 }; buf[1] = 0x0C; buf[3] = 0x01;
	buf[5] = key_id; buf[6] = r; buf[7] = g; buf[8] = b;
	int res = hid_send_feature_report(handle, buf, sizeof(buf));
	hid_close(handle); return res >= 0 ? 0 : -1;
}

int MsiHid_SetSpecialZones(unsigned char *r_array, unsigned char *g_array, unsigned char *b_array)
{
	hid_device *handle = hid_open(MSI_VID, MSI_PID_SPECIAL, NULL);
	if (!handle) return -1;
	unsigned char buf[525] = { 0 }; buf[1] = 0x0C; buf[3] = 0x04;
	int offset = 5;
	for (int i = 0; i < 4; i++)
	{
		buf[offset++] = i; buf[offset++] = r_array[i]; buf[offset++] = g_array[i]; buf[offset++] = b_array[i];
	}
	int res = hid_send_feature_report(handle, buf, sizeof(buf));
	hid_close(handle); return res >= 0 ? 0 : -1;
}

int MsiHid_SetFullKeyboard(unsigned char base_r, unsigned char base_g, unsigned char base_b, int apply_matrix)
{
	hid_device *handle = hid_open(MSI_VID, MSI_PID_KEYBOARD, NULL);
	if (!handle) return -1;
	unsigned char buf[525] = { 0 }; buf[1] = 0x0C; buf[3] = 0x66;
	unsigned char keys[102] = { 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x29, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x49, 0x4b, 0x4c, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63, 0x66, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xf0, 0x28, 0x2a, 0x31, 0x64, 0xe5, 0xe6 };
	int offset = 5;
	for (int i = 0; i < 102; i++)
	{
		unsigned char id = keys[i]; buf[offset++] = id;
		unsigned char r = base_r, g = base_g, b = base_b;
		if (apply_matrix && g_key_matrix[id].name[0] != '\0')
		{
			r = g_key_matrix[id].c1.r; g = g_key_matrix[id].c1.g; b = g_key_matrix[id].c1.b;
		}
		buf[offset++] = r; buf[offset++] = g; buf[offset++] = b;
	}
	int res = hid_send_feature_report(handle, buf, sizeof(buf));
	hid_close(handle); return res >= 0 ? 0 : -1;
}
