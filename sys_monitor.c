#include "sys_monitor.h"
#include <windows.h>

typedef enum nvmlReturn_enum
{
	NVML_SUCCESS = 0
} nvmlReturn_t;
typedef struct nvmlDevice_st *nvmlDevice_t;

typedef struct nvmlMemory_st
{
	unsigned long long total;
	unsigned long long free;
	unsigned long long used;
} nvmlMemory_t;

typedef nvmlReturn_t(*nvmlInit_t)(void);
typedef nvmlReturn_t(*nvmlShutdown_t)(void);
typedef nvmlReturn_t(*nvmlDeviceGetHandleByIndex_t)(unsigned int, nvmlDevice_t *);
typedef nvmlReturn_t(*nvmlDeviceGetMemoryInfo_t)(nvmlDevice_t, nvmlMemory_t *);
typedef nvmlReturn_t(*nvmlDeviceGetPowerManagementLimit_t)(nvmlDevice_t, unsigned int *);
typedef nvmlReturn_t(*nvmlDeviceGetPowerUsage_t)(nvmlDevice_t, unsigned int *);
typedef nvmlReturn_t(*nvmlDeviceGetTemperature_t)(nvmlDevice_t, int, unsigned int *);
typedef nvmlReturn_t(*nvmlDeviceGetTemperatureThreshold_t)(nvmlDevice_t, int, unsigned int *);

static HMODULE g_hNvml = NULL;
static nvmlInit_t p_nvmlInit = NULL;
static nvmlShutdown_t p_nvmlShutdown = NULL;
static nvmlDeviceGetHandleByIndex_t p_nvmlDeviceGetHandleByIndex = NULL;
static nvmlDeviceGetMemoryInfo_t p_nvmlDeviceGetMemoryInfo = NULL;
static nvmlDeviceGetPowerManagementLimit_t p_nvmlDeviceGetPowerManagementLimit = NULL;
static nvmlDeviceGetPowerUsage_t p_nvmlDeviceGetPowerUsage = NULL;
static nvmlDeviceGetTemperature_t p_nvmlDeviceGetTemperature = NULL;
static nvmlDeviceGetTemperatureThreshold_t p_nvmlDeviceGetTemperatureThreshold = NULL;

static nvmlDevice_t g_nvGpu = NULL;
static FILETIME g_prev_idle, g_prev_kernel, g_prev_user;
static unsigned int g_temp_wall = 85;

void SysMonitor_Init(void)
{
	GetSystemTimes(&g_prev_idle, &g_prev_kernel, &g_prev_user);

	g_hNvml = LoadLibraryW(L"nvml.dll");
	if (NULL != g_hNvml)
	{
		p_nvmlInit = (nvmlInit_t)GetProcAddress(g_hNvml, "nvmlInit_v2");
		if (NULL == p_nvmlInit)
			p_nvmlInit = (nvmlInit_t)GetProcAddress(g_hNvml, "nvmlInit");

		p_nvmlShutdown = (nvmlShutdown_t)GetProcAddress(g_hNvml, "nvmlShutdown");
		p_nvmlDeviceGetHandleByIndex = (nvmlDeviceGetHandleByIndex_t)GetProcAddress(g_hNvml, "nvmlDeviceGetHandleByIndex_v2");
		if (NULL == p_nvmlDeviceGetHandleByIndex)
			p_nvmlDeviceGetHandleByIndex = (nvmlDeviceGetHandleByIndex_t)GetProcAddress(g_hNvml, "nvmlDeviceGetHandleByIndex");

		p_nvmlDeviceGetMemoryInfo = (nvmlDeviceGetMemoryInfo_t)GetProcAddress(g_hNvml, "nvmlDeviceGetMemoryInfo");
		p_nvmlDeviceGetPowerManagementLimit = (nvmlDeviceGetPowerManagementLimit_t)GetProcAddress(g_hNvml, "nvmlDeviceGetPowerManagementLimit");
		p_nvmlDeviceGetPowerUsage = (nvmlDeviceGetPowerUsage_t)GetProcAddress(g_hNvml, "nvmlDeviceGetPowerUsage");
		p_nvmlDeviceGetTemperature = (nvmlDeviceGetTemperature_t)GetProcAddress(g_hNvml, "nvmlDeviceGetTemperature");
		p_nvmlDeviceGetTemperatureThreshold = (nvmlDeviceGetTemperatureThreshold_t)GetProcAddress(g_hNvml, "nvmlDeviceGetTemperatureThreshold");

		if (NULL != p_nvmlInit && NULL != p_nvmlDeviceGetHandleByIndex && NULL != p_nvmlDeviceGetMemoryInfo && NULL != p_nvmlDeviceGetPowerManagementLimit && NULL != p_nvmlDeviceGetPowerUsage && NULL != p_nvmlDeviceGetTemperature)
		{
			if (NVML_SUCCESS == p_nvmlInit())
			{
				p_nvmlDeviceGetHandleByIndex(0, &g_nvGpu);
				if (NULL != p_nvmlDeviceGetTemperatureThreshold && NULL != g_nvGpu)
				{
					unsigned int tw = 0;
					if (NVML_SUCCESS == p_nvmlDeviceGetTemperatureThreshold(g_nvGpu, 1, &tw))
						g_temp_wall = tw;
					else
						if (NVML_SUCCESS == p_nvmlDeviceGetTemperatureThreshold(g_nvGpu, 3, &tw))
							g_temp_wall = tw;
				}
			}
		}
	}
}

static unsigned long long FileTimeToULL(FILETIME ft)
{
	ULARGE_INTEGER uli;
	uli.LowPart = ft.dwLowDateTime;
	uli.HighPart = ft.dwHighDateTime;
	return uli.QuadPart;
}

void SysMonitor_Update(int *cpu_out, int *gpu_power_out, int *vram_out, int *temp_out)
{
	FILETIME idle, kernel, user;
	GetSystemTimes(&idle, &kernel, &user);

	unsigned long long sys_diff = (FileTimeToULL(kernel) + FileTimeToULL(user)) - (FileTimeToULL(g_prev_kernel) + FileTimeToULL(g_prev_user));
	unsigned long long idle_diff = FileTimeToULL(idle) - FileTimeToULL(g_prev_idle);

	g_prev_idle = idle;
	g_prev_kernel = kernel;
	g_prev_user = user;

	if (sys_diff > 0)
		*cpu_out = (int)((sys_diff - idle_diff) * 100 / sys_diff);
	else
		*cpu_out = 0;

	*gpu_power_out = 0;
	*vram_out = 0;
	*temp_out = 0;

	if (NULL != g_nvGpu)
	{
		unsigned int p_limit = 0, p_usage = 0;
		if (NVML_SUCCESS == p_nvmlDeviceGetPowerManagementLimit(g_nvGpu, &p_limit) && NVML_SUCCESS == p_nvmlDeviceGetPowerUsage(g_nvGpu, &p_usage))
			if (p_limit > 0)
				*gpu_power_out = (int)(p_usage * 100 / p_limit);

		nvmlMemory_t mem;
		if (NVML_SUCCESS == p_nvmlDeviceGetMemoryInfo(g_nvGpu, &mem))
			if (mem.total > 0)
				*vram_out = (int)(mem.used * 100 / mem.total);

		unsigned int temp = 0;
		if (NVML_SUCCESS == p_nvmlDeviceGetTemperature(g_nvGpu, 0, &temp))
			*temp_out = (int)temp;
	}
}

int SysMonitor_GetGpuTempWall(void)
{
	return (int)g_temp_wall;
}

void SysMonitor_Close(void)
{
	if (NULL != p_nvmlShutdown)
		p_nvmlShutdown();
	if (NULL != g_hNvml)
		FreeLibrary(g_hNvml);
}
