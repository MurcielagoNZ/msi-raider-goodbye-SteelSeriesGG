#ifndef MSI_HID_CORE_H
#define MSI_HID_CORE_H

#define MSI_VID             0x1038
#define MSI_PID_KEYBOARD    0x1122
#define MSI_PID_SPECIAL     0x1161

#define ZONE_LIGHTBAR_L     0
#define ZONE_LIGHTBAR_M     1
#define ZONE_LIGHTBAR_R     2
#define ZONE_LOGO           3

#ifdef __cplusplus
extern "C" {
#endif

	int MsiHid_Init(void);
	void MsiHid_Exit(void);

	// 保持原有的单键/特殊灯区接口不变
	int MsiHid_SetSingleKeyColor(unsigned char key_id, unsigned char r, unsigned char g, unsigned char b);
	int MsiHid_SetSpecialZones(unsigned char *r_array, unsigned char *g_array, unsigned char *b_array);

	// ==========================================================
	// [新增] 批量发送全键盘矩阵 
	// ==========================================================
	// 参数 base_r, base_g, base_b: 全局的底色
	// 参数 apply_matrix: 传 1 则会在底色的基础上，覆盖 g_key_matrix 中已配置的颜色；传 0 则强刷全局底色。
	int MsiHid_SetFullKeyboard(unsigned char base_r, unsigned char base_g, unsigned char base_b, int apply_matrix);

#ifdef __cplusplus
}
#endif

#endif // MSI_HID_CORE_H
