#pragma once

#include <string>

namespace util {


enum arch_info {
		arch_x86,
		arch_x64,
		arch_other
};

class sysinfo
{
public:
	static std::string get_username();
	static std::string get_computer_name();

	//static std::string get_os_version();
	
	static std::string get_cpu_name();
	static arch_info get_os_arch();
	static arch_info get_agent_arch();
	static uint32_t get_physical_core_count();
	static uint32_t get_logical_core_count();
	static uint32_t get_active_processor_count();
	static uint64_t get_active_processor_mask();
	
	static uint32_t get_gpu_count();
	static std::string get_gpu_name(uint32_t idx);

	static uint64_t get_total_memory();
	static uint64_t get_free_memory();
	
	static uint64_t get_uptime();
	static uint32_t get_idle_time();

	static bool is_process_running(std::string process_name);
};


}