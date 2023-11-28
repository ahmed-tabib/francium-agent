#pragma once

#include <mutex>
#include <vector>
#include <string>

#include "job.h"
#include "job_exec.h"
#include "job_result.h"

namespace mnr
{


class controller
{
private:
	static uint32_t initialized_instance_count;

	std::vector<job_exec*> m_job_exec_list;
	job* m_job = nullptr;

	bool m_is_init = false;
	bool m_is_running = false;

	bool m_mode_control_flag = true;
	bool m_power_control_flag = true;
	bool m_exit_flag = false;

	double m_current_power;

	static std::mutex power_manipulation_mutex;
	static std::mutex mode_manipulation_mutex;

	std::vector<double> max_hashrates_fast;
	std::vector<double> max_hashrates_slow;
	std::vector<bool> active_thread_flag;
	
	static double normal_power;
	static double max_power;
	static double power_increment_rate;
	static uint32_t power_increment_time_ms;

	// formula is: max_power = maximum - (nominator / (max_hashrate - denominator))
	static double max_power_formula_maximum;
	static double max_power_formula_nominator;
	static double max_power_formula_denominator;

	static uint32_t afk_wait_time_ms;
	static uint64_t predataset_free_memory_bytes;
	static uint64_t postdataset_free_memory_bytes;

	static std::vector<std::string> bad_process_list;

	void calc_max_power();

	void power_incrementor(const bool* stop_flag);
	void control_func();
	std::thread m_control_thread;

	void set_mode(MODE mode);
	void set_power(double power);
	void increment_power(double increment);

public:
	controller();
	controller(const job& initial_job);
	controller(const controller& other) = delete;
	~controller();

	//mining control
	void initialize(const job& initial_job);
	void start();
	void stop();
	void shutdown();

	//metrics
	std::vector<double> get_hashrate_per_thread(uint32_t sample_time_ms);
	double get_current_hashrate(uint32_t sample_time_ms);
	double get_max_hashrate(MODE hashing_mode);
	uint64_t get_total_hash_count();

	//mining power/mode
	void set_auto_power();
	void set_fixed_power(double power);
	
	void set_auto_mode();
	void set_fixed_mode(MODE mode);

	//job management
	void set_job(const job& j);

	bool check_new_results();
	std::vector<job_result> get_new_results();

	double get_current_power() { return m_current_power; };
};


}