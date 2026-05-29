#ifndef SYS_MONITOR_H
#define SYS_MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

	void SysMonitor_Init(void);
	void SysMonitor_Update(int *cpu_out, int *gpu_power_out, int *vram_out, int *temp_out);
	int SysMonitor_GetGpuTempWall(void);
	void SysMonitor_Close(void);

#ifdef __cplusplus
}
#endif

#endif
