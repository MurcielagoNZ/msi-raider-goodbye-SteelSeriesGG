#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hidapi.h"

int main()
{
	if (hid_init() != 0) return -1;

	// 1. 打开主键盘设备 (PID: 0x1122) keyboard
	hid_device *handle_main = hid_open(0x1038, 0x1122, NULL);
	// 2. 打开 Logo/灯条 设备 (PID: 0x1161) logo & lightbar
	hid_device *handle_special = hid_open(0x1038, 0x1161, NULL);

	if (!handle_main) printf("警告: 无法打开主键盘 0x1122！\n"); // error can't access keyboard
	if (!handle_special) printf("警告: 无法打开灯条设备 0x1161！\n"); // error can't access lightbar etc.
	if (!handle_main && !handle_special) return -1;

	printf("设备已连接！正在下发颜色...\n"); // successful connected

	// ==========================================================
	// 发送给 0x1122 (主键盘) to keyboard
	// ==========================================================

	if (handle_main)
	{
		unsigned char buf_main[525];
		memset(buf_main, 0, sizeof(buf_main));
		buf_main[0] = 0x00;
		buf_main[1] = 0x0c; buf_main[2] = 0x00; buf_main[3] = 0x66; buf_main[4] = 0x00;

		unsigned char key_ids[102] = {
			0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x29, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x49, 0x4b, 0x4c, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63, 0x66, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xf0, 0x28, 0x2a, 0x31, 0x64, 0xe5, 0xe6
		};

		int offset = 5;

		for (int i = 0; i < 102; i++)
		{
			buf_main[offset] = key_ids[i];
			offset++;

			switch (key_ids[i])
			{
				case 0x29: // Esc 键 -> 蓝色 ESC->blue
					buf_main[offset] = 0x00;
					offset++;
					buf_main[offset] = 0x00;
					offset++;
					buf_main[offset] = 0xff;
					offset++;
					break;

				case 0x66: // Power (电源) 键 -> 红色 POWER->red
					buf_main[offset] = 0xff;
					offset++;
					buf_main[offset] = 0x00;
					offset++;
					buf_main[offset] = 0x00;
					offset++;
					break;

				case 0xf0: // Fn 键 -> 绿色 Fn->green
					buf_main[offset] = 0x00;
					offset++;
					buf_main[offset] = 0xff;
					offset++;
					buf_main[offset] = 0x00;
					offset++;
					break;

				default: // 其余所有普通按键 -> 浅灰色 (#888888) others->#888888
					buf_main[offset] = 0x88;
					offset++;
					buf_main[offset] = 0x88;
					offset++;
					buf_main[offset] = 0x88;
					offset++;
					break;
			}
		}

		hid_send_feature_report(handle_main, buf_main, sizeof(buf_main));
	}

// ==========================================================
// 发送给 0x1161 (Logo/灯条) to logo and light bar
// ==========================================================
	if (handle_special)
	{
		unsigned char buf_special[525];
		unsigned char commit[525];

		memset(buf_special, 0x00, sizeof(buf_special));
		buf_special[0] = 0x00;  // Report ID
		buf_special[1] = 0x0C;  // 画图指令
		buf_special[2] = 0x00;
		buf_special[3] = 0x04;  // 分区数量 (4个) 4 zones
		buf_special[4] = 0x00;

		//灯条左段 lightbar left
		buf_special[5] = 0x00;  // ZoneID 0
		buf_special[6] = 0xff;  // R
		buf_special[7] = 0x00;  // G
		buf_special[8] = 0x00;  // B

		//灯条中段 lightbar middle
		buf_special[9] = 0x01;  // ZoneID 1
		buf_special[10] = 0x00; // R
		buf_special[11] = 0xff; // G
		buf_special[12] = 0x00; // B

		//灯条右段 lightbar right
		buf_special[13] = 0x02; // ZoneID 2
		buf_special[14] = 0x00; // R
		buf_special[15] = 0x00; // G
		buf_special[16] = 0xff; // B

		//Logo灯区 logo
		buf_special[17] = 0x03; // ZoneID 3
		buf_special[18] = 0xff; // R
		buf_special[19] = 0x00; // G
		buf_special[20] = 0xff; // B

		hid_send_feature_report(handle_special, buf_special, sizeof(buf_special));
	}

	printf("指令下发完毕！\n"); // done!

	if (handle_main) hid_close(handle_main);
	if (handle_special) hid_close(handle_special);
	hid_exit();
	return 0;
}