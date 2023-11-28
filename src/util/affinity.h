#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <thread>
#include <stdint.h>

namespace util
{
class affinity
{
public:
	
	static void set_thread_affinity(uint32_t cpuid, std::thread::native_handle_type thread);
	static void set_current_thread_affinity(uint32_t cpuid);

#ifdef _DEBUG
	static DWORD get_thread_affinity_mask(std::thread::native_handle_type thread);
	static DWORD get_current_thread_affinity_mask();
#endif
	
};
}