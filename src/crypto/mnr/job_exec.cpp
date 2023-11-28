#include "job_exec.h"

namespace mnr
{

uint8_t job_exec::cache_key[RANDOMX_HASH_SIZE] = { 0 };

std::vector<job_exec*> job_exec::instance_table;

randomx_cache* job_exec::cache = nullptr;
randomx_dataset* job_exec::dataset = nullptr;

bool job_exec::is_cache_init = false;
bool job_exec::is_dataset_init = false;

std::mutex job_exec::static_member_mutex;

MODE job_exec::mode = SLOW_MODE;

job_exec::job_exec(const job& j, uint32_t dataset_init_threads): m_job(new job(j))
{
	static_member_mutex.lock();
	instance_table.emplace_back(this);
	static_member_mutex.unlock();

	this->m_is_init = false;
	this->m_working_flag = false;
	this->m_shutdown_flag = false;

	this->m_hash_counter = 0;
	this->m_interhash_time = 0;
	this->m_sleep_every_x_hashes = 1;

	set_seed_hash(this->m_job->get_seed_hash(), dataset_init_threads);

	this->m_worker = std::thread(&job_exec::mine_func, this);
}

job_exec::~job_exec()
{
	this->shutdown();
}

void job_exec::start()
{
	this->m_working_flag = true;
}

void job_exec::stop()
{
	this->m_working_flag = false;
	std::lock_guard<std::mutex> working_guard(m_working_mutex);
}

void job_exec::stop_all()
{
	for (size_t i = 0; i < instance_table.size(); i++)
		instance_table[i]->m_working_flag = false;

	for (size_t i = 0; i < instance_table.size(); i++)
		std::lock_guard<std::mutex> working_guard(instance_table[i]->m_working_mutex);
}

void job_exec::shutdown()
{
	if (m_shutdown_flag)
		return;

	this->stop();

	delete this->m_job;

	this->m_shutdown_flag = true;
	this->m_worker.join();

	if (m_vm != nullptr)
		randomx_destroy_vm(m_vm);

	this->m_is_init = false;

	std::lock_guard<std::mutex> mutex_guard(static_member_mutex);

	for (size_t i = 0; i < instance_table.size(); i++)
	{
		if (instance_table[i] == this)
			instance_table.erase(instance_table.begin() + i);
	}

	if (instance_table.size() == 0)
	{
		for (size_t i = 0; i < RANDOMX_HASH_SIZE; i++)
			cache_key[i] = 0x00;

		if (dataset != nullptr)
		{
			randomx_release_dataset(dataset);
			is_dataset_init = false;
		}
		if (cache != nullptr)
		{
			randomx_release_cache(cache);
			is_cache_init = false;
		}
	}
}

bool job_exec::check_hash(uint8_t* hash)
{
	// something counterintuitive, monero hashes are always little endian, and so are targets
	// so we need to swap endianness if the system is big endian
	uint64_t t = (util::endianness::is_little_endian() ? this->m_job->get_target() : util::endianness::swap_endianness<uint64_t>(this->m_job->get_target()));
	uint64_t h = (util::endianness::is_little_endian() ? ((uint64_t*)hash)[3] : util::endianness::swap_endianness<uint64_t>(((uint64_t*)hash)[3]));

	return (h < t);
}

bool job_exec::is_cache_key_equal(uint8_t* new_key)
{
	for (size_t i = 0; i < RANDOMX_HASH_SIZE / sizeof(uint64_t); i++)
	{
		if (((uint64_t*)cache_key)[i] != ((uint64_t*)new_key)[i])
			return false;
	}

	return true;
}

void job_exec::set_seed_hash(uint8_t* new_seed, uint32_t dataset_init_threads)
{
	static_member_mutex.lock();
	std::vector<size_t> working_instances;
	working_instances.reserve(instance_table.size());

	//send stop signal to all working instances
	for (size_t i = 0; i < instance_table.size(); i++)
	{
		if (instance_table[i]->m_working_flag)
		{
			instance_table[i]->m_working_flag = false;
			working_instances.emplace_back(i);
		}
	}

	//then wait for them to actually stop, this two stage stop makes
	//the whole process take the time of the worst thread stopping
	//instead of the sum of every thread's time
	for (size_t i = 0; i < working_instances.size(); i++)
	{
		std::lock_guard<std::mutex> mutex_guard(instance_table[working_instances[i]]->m_working_mutex);
	}

	if (mode == SLOW_MODE)
	{
		randomx_flags flags = RANDOMX_FLAG_DEFAULT;
		flags |= RANDOMX_FLAG_HARD_AES;
#ifdef _WIN64
		flags |= RANDOMX_FLAG_JIT;
		flags |= RANDOMX_FLAG_SECURE;
#endif

		if (!is_cache_init)
		{
			cache = randomx_alloc_cache(flags);
			if (cache != nullptr)
				is_cache_init = true;
		}

		if (!is_cache_key_equal(new_seed))
		{
			for (int i = 0; i < RANDOMX_HASH_SIZE; i++)
				cache_key[i] = new_seed[i];

			randomx_init_cache(cache, cache_key, RANDOMX_HASH_SIZE);
		}

		for (size_t i = 0; i < instance_table.size(); i++)
		{
			if (instance_table[i]->m_vm == nullptr)
				instance_table[i]->m_vm = randomx_create_vm(flags, cache, nullptr);
			else
				randomx_vm_set_cache(instance_table[i]->m_vm, cache);
		}
	}
	else
	{
		randomx_flags flags = RANDOMX_FLAG_DEFAULT;
		flags |= RANDOMX_FLAG_HARD_AES;
		flags |= RANDOMX_FLAG_FULL_MEM;
#ifdef _WIN64
		flags |= RANDOMX_FLAG_JIT;
		flags |= RANDOMX_FLAG_SECURE;
#endif

		if (!is_dataset_init)
		{
			dataset = randomx_alloc_dataset(flags);
			if (dataset != nullptr)
				is_dataset_init = true;
		}

		if (!is_cache_key_equal(new_seed))
		{
			for (int i = 0; i < RANDOMX_HASH_SIZE; i++)
				cache_key[i] = new_seed[i];

			if (cache != nullptr)
			{
				randomx_release_cache(cache);
				cache = nullptr;
			}

			cache = randomx_alloc_cache(flags);
			randomx_init_cache(cache, cache_key, RANDOMX_HASH_SIZE);

			std::vector<std::thread> init_threads;
			uint32_t dataset_item_count = randomx_dataset_item_count();
			for (uint32_t i = 0; i < dataset_init_threads; i++)
			{
				init_threads.emplace_back(
					std::thread(randomx_init_dataset,
						dataset,
						cache,
						(i * (dataset_item_count / dataset_init_threads)),
						((i == dataset_init_threads - 1) ? ((dataset_item_count / dataset_init_threads) + (dataset_item_count % dataset_init_threads)) : (dataset_item_count / dataset_init_threads))));
			}

			for (uint32_t i = 0; i < dataset_init_threads; i++)
				init_threads[i].join();

			randomx_release_cache(cache);
			cache = nullptr;
		}

		for (size_t i = 0; i < instance_table.size(); i++)
		{
			if (instance_table[i]->m_vm == nullptr)
				instance_table[i]->m_vm = randomx_create_vm(flags, nullptr, dataset);
			else
				randomx_vm_set_dataset(instance_table[i]->m_vm, dataset);
		}
	}

	for (size_t i = 0; i < working_instances.size(); i++)
	{
		instance_table[working_instances[i]]->start();
	}
	
	static_member_mutex.unlock();
}

void job_exec::mine_func()
{
	bool mutex_unlocked = true;

	while (true)
	{
		if (!m_working_flag)
			std::this_thread::sleep_for(std::chrono::milliseconds(MNR_PAUSE_SLEEP_TIME));
		else
		{
			m_working_mutex.lock();
			mutex_unlocked = false;
			break;
		}
	}

	uint32_t nonce = this->m_job->get_nonce();
	uint32_t iter_count = this->m_job->get_iteration_count();

	uint8_t hash[RANDOMX_HASH_SIZE] = {0};

	randomx_calculate_hash_first(this->m_vm, this->m_job->get_blob(), this->m_job->get_size());
		
	uint64_t sleep_at_hash_count = m_hash_counter + m_sleep_every_x_hashes;
	
	while (!m_shutdown_flag)
	{
		if (m_working_flag)
		{
			if (m_hash_counter >= sleep_at_hash_count && this->m_interhash_time > 0)
			{
				sleep_at_hash_count = m_hash_counter + m_sleep_every_x_hashes;
				std::this_thread::sleep_for(std::chrono::microseconds(this->m_interhash_time * m_sleep_every_x_hashes));
			}

			this->m_job->incr_nonce();
			randomx_calculate_hash_next(this->m_vm, this->m_job->get_blob(), this->m_job->get_size(), hash);

			if (check_hash(hash))
			{
				m_result_list_mutex.lock();
				m_result_list.push_back(job_result(hash, nonce, *this->m_job));
				m_result_list_mutex.unlock();
			}
			
			nonce = this->m_job->get_nonce();

			m_hash_counter++;
		}
		else
		{
			while (true)
			{
				if (!m_working_flag)
				{
					if (!mutex_unlocked)
					{
						m_working_mutex.unlock();
						mutex_unlocked = true;
					}

					if (m_shutdown_flag)
						return;

					std::this_thread::sleep_for(std::chrono::milliseconds(MNR_PAUSE_SLEEP_TIME));
				}
				else
				{
					m_working_mutex.lock();
					mutex_unlocked = false;
					break;
				}
			}
		}
	}

	if (!mutex_unlocked)
		m_working_mutex.unlock();
}

void job_exec::set_mode(MODE m)
{
	if (m != mode)
	{
		static_member_mutex.lock();
		std::vector<size_t> working_instances;
		working_instances.reserve(instance_table.size());

		for (size_t i = 0; i < instance_table.size(); i++)
		{
			if (instance_table[i]->m_working_flag)
			{
				instance_table[i]->stop();
				working_instances.emplace_back(i);
			}
		}

		mode = m;

		if (dataset != nullptr)
		{
			randomx_release_dataset(dataset);
			dataset = nullptr;
			is_dataset_init = false;
		}
		if (cache != nullptr)
		{
			randomx_release_cache(cache);
			cache = nullptr;
			is_cache_init = false;
		}

		memset(cache_key, 0, RANDOMX_HASH_SIZE);

		static_member_mutex.unlock();

		for (size_t i = 0; i < instance_table.size(); i++)
		{
			if (instance_table[i]->m_vm != nullptr)
			{
				randomx_destroy_vm(instance_table[i]->m_vm);
				instance_table[i]->m_vm = nullptr;
			}
		}

		for (size_t i = 0; i < instance_table.size(); i++)
			set_seed_hash(instance_table[i]->m_job->get_seed_hash());

		for (size_t i = 0; i < working_instances.size(); i++)
		{
			instance_table[working_instances[i]]->start();
		}
	}
}

MODE job_exec::get_mode()
{
	return mode;
}

void job_exec::switch_job(const job& j)
{
	bool working = this->m_working_flag;

	if (working)
		this->stop();

	delete this->m_job;
	this->m_job = new job(j);
	if (!is_cache_key_equal(this->m_job->get_seed_hash()))
		set_seed_hash(this->m_job->get_seed_hash());

	if (working)
		this->start();
}

job job_exec::get_job()
{
	return job(*this->m_job);
}

void job_exec::set_interhash_time(uint32_t microseconds, uint32_t sleep_every_x_hashes)
{
	this->m_sleep_every_x_hashes = sleep_every_x_hashes;
	this->m_interhash_time = microseconds;
}

uint64_t job_exec::get_hash_count()
{
	return m_hash_counter;
}

std::thread::native_handle_type job_exec::get_worker_handle()
{
	return this->m_worker.native_handle();
}

bool job_exec::check_new_results()
{
	if (m_result_list.size() > 0)
		return true;
	else
		return false;
}

std::vector<job_result> job_exec::get_new_results()
{
	m_result_list_mutex.lock();

	std::vector<job_result> copy = std::vector<job_result>(m_result_list);

	m_result_list.clear();
	m_result_list.shrink_to_fit();

	m_result_list_mutex.unlock();

	return copy;
}

}