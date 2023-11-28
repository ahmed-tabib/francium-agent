#include "sysinfo.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <intrin.h>

namespace util {


std::string sysinfo::get_username()
{
	char name[257];
	DWORD size = 257;

	GetUserNameA(name, &size);

	return std::string(name);
}

std::string sysinfo::get_cpu_name()
{
	int info[4] = { -1 };
	char name[0x41] = {0};

	__cpuid(info, 0x80000002);
	memcpy(name, info, sizeof(info));

	__cpuid(info, 0x80000003);
	memcpy(name + 16, info, sizeof(info));

	__cpuid(info, 0x80000004);
	memcpy(name + 32, info, sizeof(info));

	return std::string(name);
}

uint32_t sysinfo::get_physical_core_count()
{
	DWORD size = 0;

	GetLogicalProcessorInformationEx(RelationProcessorCore, NULL, &size);

	uint8_t* buffer = new uint8_t[size];
	memset(buffer, 0, size);

	BOOL res = GetLogicalProcessorInformationEx(RelationProcessorCore, (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)buffer, &size);
	if (!res)
	{
		delete[] buffer;
		return 0;
	}

	uint32_t ctr = 0;
	for (size_t offset = 0; offset < size;)
	{
		SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX current = *((SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)(buffer + offset));
		if (current.Size > 0)
		{
			offset += current.Size;
			ctr++;
		}
		else
		{
			break;
		}
	}
	delete[] buffer;
	return ctr;
}

uint32_t sysinfo::get_logical_core_count()
{
	SYSTEM_INFO s;
	GetSystemInfo(&s);
	return s.dwNumberOfProcessors;
}

uint32_t sysinfo::get_active_processor_count()
{
	SYSTEM_INFO s;
	GetSystemInfo(&s);

	uint64_t mask = s.dwActiveProcessorMask;
	uint32_t count = 0;

	for (size_t i = 0; i < 64; i++)
	{
		if (mask & 1ULL)
			count++;
		mask >>= 1;
	}

	return count;
}

uint64_t sysinfo::get_active_processor_mask()
{
	SYSTEM_INFO s;
	GetSystemInfo(&s);
	return s.dwActiveProcessorMask;
}

arch_info sysinfo::get_os_arch()
{
	SYSTEM_INFO s;
	GetSystemInfo(&s);

	if (s.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
	{
		BOOL iswow64 = FALSE;
		BOOL(*fnIsWow64Process) (HANDLE, PBOOL) = nullptr;

		fnIsWow64Process = (BOOL (*) (HANDLE, PBOOL))GetProcAddress(GetModuleHandleA("Kernel32.dll"), "IsWow64Process");

		if (fnIsWow64Process != nullptr)
			fnIsWow64Process(GetCurrentProcess(), &iswow64);

		if (iswow64)
			return arch_x64;
		else
			return arch_x86;
	}
	else if (s.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
		return arch_x64;
	else
		return arch_other;
}

arch_info sysinfo::get_agent_arch()
{
#ifdef _WIN64
	return arch_x64;
#else
	return arch_x86;
#endif
}

std::string sysinfo::get_computer_name()
{
	char computer_name[MAX_COMPUTERNAME_LENGTH] = {0};
	DWORD size = MAX_COMPUTERNAME_LENGTH;
	GetComputerNameA(computer_name, &size);
	return std::string(computer_name);
}

uint64_t sysinfo::get_total_memory()
{
	MEMORYSTATUSEX mem_info = {0};
	mem_info.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&mem_info);

	return mem_info.ullTotalPhys;
}

uint64_t sysinfo::get_free_memory()
{
	MEMORYSTATUSEX mem_info = {0};
	mem_info.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&mem_info);

	return mem_info.ullAvailPhys;
}

uint64_t sysinfo::get_uptime()
{
	return GetTickCount64();
}

uint32_t sysinfo::get_gpu_count()
{
	uint32_t i;
	for (i = 0; ; i++)
	{
		DISPLAY_DEVICEA d = { 0 };
		d.cb = sizeof(d);
		BOOL res = EnumDisplayDevicesA(NULL, i, &d, NULL);
		if (!res)
			break;
	}

	return i;
}

std::string sysinfo::get_gpu_name(uint32_t idx)
{
	DISPLAY_DEVICEA d = { 0 };
	d.cb = sizeof(d);
	BOOL res = EnumDisplayDevicesA(NULL, idx, &d, EDD_GET_DEVICE_INTERFACE_NAME);
	if (!res)
		return std::string("");
	else
		return std::string(d.DeviceString);
}

uint32_t sysinfo::get_idle_time()
{
	LASTINPUTINFO info = {0};
	info.cbSize = sizeof(LASTINPUTINFO);
	GetLastInputInfo(&info);

	return (GetTickCount() - info.dwTime);
}

bool sysinfo::is_process_running(std::string process)
{
	return false;
}

}