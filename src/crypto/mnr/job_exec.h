#pragma once

#include "job.h"
#include "job_result.h"
#include "../../ext/randomx/randomx.h"
#include "../../util/endianness.h"

#include <vector>
#include <chrono>
#include <thread>
#include <mutex>

#define MNR_PAUSE_SLEEP_TIME 250
#define MNR_STOP_LAG_TIME 100

namespace mnr
{

enum MODE
{
	FAST_MODE,
	SLOW_MODE
};

class job_exec
{
private:

	static std::mutex static_member_mutex;
	static std::vector<job_exec*> instance_table;

	randomx_vm* m_vm = nullptr;
	static uint8_t cache_key[RANDOMX_HASH_SIZE];
	static randomx_cache* cache;
	static randomx_dataset* dataset;
	static bool is_cache_init;
	static bool is_dataset_init;

	static MODE mode;

	bool m_is_init;
	bool m_working_flag;
	bool m_shutdown_flag;
	std::mutex m_working_mutex;

	uint64_t m_hash_counter;
	uint64_t m_interhash_time;
	uint32_t m_sleep_every_x_hashes;

	job* m_job;

	std::thread m_worker;

	std::mutex m_result_list_mutex;
	std::vector<job_result> m_result_list;

	static void set_seed_hash(uint8_t* new_seed, uint32_t dataset_init_threads = 2);
	static bool is_cache_key_equal(uint8_t* new_key);
	
	void mine_func();
	bool check_hash(uint8_t* hash);

public:

	job_exec(const job& j, uint32_t dataset_init_threads = 2);
	~job_exec();

	void start();
	void stop();
	static void stop_all();

	void shutdown();

	static void set_mode(MODE mode);
	static MODE get_mode();

	void switch_job(const job& j);
	job get_job();

	void set_interhash_time(uint32_t microseconds, uint32_t sleep_every_x_hashes);
	uint64_t get_hash_count();

	bool check_new_results();
	std::vector<job_result> get_new_results();

	std::thread::native_handle_type get_worker_handle();
};


}